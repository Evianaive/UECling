#pragma once

/*
 * This header provides a small set of C++20 coroutine utilities used by the Cling runtime.
 *
 * Lifetime / flow overview:
 *
 * - TClingTask<T> is a lazy coroutine task type.
 *   The coroutine is created suspended (initial_suspend returns suspend_always).
 *
 * - promise_type owns a shared TTaskState<T> (TSharedPtr) which stores completion state and
 *   the waiters list (awaiting coroutine handles).
 *
 * - Launch():
 *   Transfers ownership of the coroutine handle out of the task object (Handle is cleared)
 *   and schedules resume on UE TaskGraph (AnyThread).
 *
 * - co_await task:
 *   await_suspend() registers the awaiting coroutine as a waiter. If the task is not yet
 *   started, it performs symmetric transfer by returning the task's coroutine handle and
 *   clears Handle in the task object.
 *
 * - Completion:
 *   promise returns via return_value/return_void, which completes the shared state and resumes
 *   all waiters. After completion, final_suspend destroys the coroutine frame.
 *
 * - Task object destruction:
 *   If the task was never started (Handle not cleared), the destructor destroys the coroutine
 *   frame. Otherwise, the coroutine self-destructs at final_suspend and only the shared state
 *   remains for awaiters.
 */

//   Game Thread                                              |  Background Thread
//
//   User coroutine entry (returns ClingCoro::TClingTask<T>)   |
//   e.g. ClingCoro::TClingTask<int> FooAsync();               |
//         |                                                  |
//         | compiler calls promise_type::get_return_object()  |
//         |   -> Detail::TClingPromiseBase<Promise,T>::get_return_object()
//         v                                                  |
//   ClingCoro::TClingTask<T>{ Handle, State }                 |
//         |                                                  |
//   (A) co_await Task;                                       |
//         |  TClingTask<T>::await_suspend(Caller)             |
//         |   - State->TryAddWaiter(Caller)                   |
//         |   - returns coroutine Handle                      |
//         +-------------------------------------------------> |  coroutine frame resumes
//                                                            |
//   (B) Task.Launch();                                       |
//         |  TClingTask<T>::Launch()                          |
//         |   - clears Handle                                 |
//         |   - FFunctionGraphTask::CreateAndDispatch...       |
//         +-------------------------------------------------> |  [AnyThread] H.resume()
//                                                            |
//                                                            |  promise.return_value(...) / return_void()
//                                                            |    -> TTaskState<T>::Complete(...)
//                                                            |       - marks done, resumes all Waiters
//         +<------------------------------------------------- |  waiters resume (H.resume())
//         |  TClingTask<T>::await_resume()                     |
//         |   - (non-void) returns State->Value                |
//         |                                                  |
//                                                            | promise.final_suspend(): destroy coroutine frame

#include <coroutine>
#include <type_traits>
#include "CoreMinimal.h"
#include "Async/TaskGraphInterfaces.h"

namespace ClingCoro
{

template<typename T>
class TClingTask;

// ---------------------------------------------------------------------------
// TTaskState<T> - shared result state between a coroutine and its awaiters
// ---------------------------------------------------------------------------
	
struct TTaskStateBase
{
	FCriticalSection Mutex;
	bool bDone = false;
	TArray<std::coroutine_handle<>> Waiters;
	// Returns false if already done (caller must resume itself)
	bool TryAddWaiter(std::coroutine_handle<> H)
	{
		FScopeLock Lock(&Mutex);
		if (bDone) return false;
		Waiters.Add(H);
		return true;
	}
};

template<typename T>
struct TTaskState : public TTaskStateBase
{
	T Value{};

	void Complete(T InValue)
	{
		TArray<std::coroutine_handle<>> ToResume;
		{
			FScopeLock Lock(&Mutex);
			Value = MoveTemp(InValue);
			bDone = true;
			ToResume = MoveTemp(Waiters);
		}
		for (auto H : ToResume)
		{
			H.resume();
		}
	}
};

template<>
struct TTaskState<void> : public TTaskStateBase
{
	void Complete()
	{
		TArray<std::coroutine_handle<>> ToResume;
		{
			FScopeLock Lock(&Mutex);
			bDone = true;
			ToResume = MoveTemp(Waiters);
		}
		for (auto H : ToResume)
		{
			H.resume();
		}
	}
};

namespace Detail
{
	template<typename DerivedPromise, typename T>
	struct TClingPromiseBase
	{
		TSharedPtr<TTaskState<T>> State = MakeShared<TTaskState<T>>();

		TClingTask<T> get_return_object();

		std::suspend_always initial_suspend() noexcept { return {}; }

		struct FFinalSuspend
		{
			bool await_ready() noexcept { return false; }
			void await_suspend(std::coroutine_handle<DerivedPromise> H) noexcept
			{
				H.destroy(); // self-destruct after completion
			}
			void await_resume() noexcept {}
		};

		FFinalSuspend final_suspend() noexcept { return {}; }
	};

	template<typename T>
	struct TClingTaskPromise : TClingPromiseBase<TClingTaskPromise<T>, T>
	{
		void return_value(T Val) { this->State->Complete(MoveTemp(Val)); }
		void unhandled_exception() { this->State->Complete(T{}); }
	};

	template<>
	struct TClingTaskPromise<void> : TClingPromiseBase<TClingTaskPromise<void>, void>
	{
		void return_void() { this->State->Complete(); }
		void unhandled_exception() { this->State->Complete(); }
	};
}

// ---------------------------------------------------------------------------
// TClingTask<T> - lazy C++20 coroutine task
// ---------------------------------------------------------------------------

template<typename T>
class TClingTask
{
public:
	using promise_type = Detail::TClingTaskPromise<T>;

	// --- Awaitable interface (supports co_await task) ---

	bool await_ready() const noexcept
	{
		return State && State->bDone;
	}

	// Symmetric transfer: starts the task inline (or noop if already launched)
	std::coroutine_handle<> await_suspend(std::coroutine_handle<> Caller) noexcept
	{
		if (State && !State->TryAddWaiter(Caller))
		{
			// Already done - resume caller immediately via symmetric transfer
			return Caller;
		}
		auto H = static_cast<std::coroutine_handle<>>(Handle);
		Handle = {};
		return H ? H : std::noop_coroutine();
	}

	decltype(auto) await_resume() const
	{
		if constexpr (std::is_void_v<T>)
		{
			return;
		}
		else
		{
			return State->Value;
		}
	}

	// --- Launch on background thread ---
	// Dispatches to any task thread. Returns *this for co_await chaining.
	TClingTask& Launch(const TCHAR* Name = TEXT("ClingTask"))
	{
		auto H = static_cast<std::coroutine_handle<>>(Handle);
		Handle = {};
		if (H)
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[H]() mutable { H.resume(); },
				TStatId{}, nullptr, ENamedThreads::AnyThread
			);
		}
		return *this;
	}

	// --- Lifecycle ---

	TClingTask() = default;

	TClingTask(TClingTask&& Other) noexcept
		: Handle(Other.Handle), State(MoveTemp(Other.State))
	{
		Other.Handle = {};
	}

	TClingTask& operator=(TClingTask&& Other) noexcept
	{
		if (this != &Other)
		{
			if (Handle) Handle.destroy();
			Handle = Other.Handle;
			State = MoveTemp(Other.State);
			Other.Handle = {};
		}
		return *this;
	}

	TClingTask(const TClingTask&) = delete;
	TClingTask& operator=(const TClingTask&) = delete;

	~TClingTask()
	{
		// Only valid if the task was never started (Launch/await_suspend sets Handle={})
		if (Handle) Handle.destroy();
	}

private:
	template<typename U>
	friend struct Detail::TClingTaskPromise;

	template<typename DerivedPromise, typename U>
	friend struct Detail::TClingPromiseBase;

	TClingTask(std::coroutine_handle<promise_type> InHandle, TSharedPtr<TTaskState<T>> InState)
		: Handle(InHandle), State(MoveTemp(InState))
	{}

	std::coroutine_handle<promise_type> Handle;
	TSharedPtr<TTaskState<T>> State;
};

namespace Detail
{
	template<typename DerivedPromise, typename T>
	TClingTask<T> TClingPromiseBase<DerivedPromise, T>::get_return_object()
	{
		auto& Self = *static_cast<DerivedPromise*>(this);
		return TClingTask<T>{std::coroutine_handle<DerivedPromise>::from_promise(Self), State};
	}
}

// ---------------------------------------------------------------------------
// Thread-switching awaitables
// ---------------------------------------------------------------------------

template<ENamedThreads::Type DstThread>
class FMoveToThread
{
public:
	bool await_ready() const noexcept
	{
		// Todo: this does not support priority bits 
		if constexpr (DstThread == ENamedThreads::GameThread)
		{
			return IsInGameThread();
		}
		else
		{
			return false;			
		}
	}

	template<typename PromiseType>
	void await_suspend(std::coroutine_handle<PromiseType> Handle)
	{
		auto H = static_cast<std::coroutine_handle<>>(Handle);
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[H]() mutable { H.resume(); },
			TStatId{}, nullptr, DstThread
		);
	}

	void await_resume() const noexcept {}
};

// Todo priority is not considered!
using FMoveToTask = FMoveToThread<ENamedThreads::AnyThread>;
using FMoveToGameThread = FMoveToThread<ENamedThreads::GameThread>;

inline FMoveToTask MoveToTask() { return {}; }
inline FMoveToGameThread MoveToGameThread() { return {}; }

} // namespace ClingCoro
