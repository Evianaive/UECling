#pragma once

#include <coroutine>
#include "CoreMinimal.h"
#include "Async/TaskGraphInterfaces.h"

namespace ClingCoro
{

// ---------------------------------------------------------------------------
// TTaskState<T> - shared result state between a coroutine and its awaiters
// ---------------------------------------------------------------------------

template<typename T>
struct TTaskState
{
	FCriticalSection Mutex;
	bool bDone = false;
	T Value{};
	TArray<std::coroutine_handle<>> Waiters;

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

	// Returns false if already done (caller must resume itself)
	bool TryAddWaiter(std::coroutine_handle<> H)
	{
		FScopeLock Lock(&Mutex);
		if (bDone) return false;
		Waiters.Add(H);
		return true;
	}
};

template<>
struct TTaskState<void>
{
	FCriticalSection Mutex;
	bool bDone = false;
	TArray<std::coroutine_handle<>> Waiters;

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

	bool TryAddWaiter(std::coroutine_handle<> H)
	{
		FScopeLock Lock(&Mutex);
		if (bDone) return false;
		Waiters.Add(H);
		return true;
	}
};

// ---------------------------------------------------------------------------
// TClingTask<T> - lazy C++20 coroutine task
// ---------------------------------------------------------------------------

template<typename T>
class TClingTask
{
public:
	struct promise_type
	{
		TSharedPtr<TTaskState<T>> State = MakeShared<TTaskState<T>>();

		TClingTask get_return_object()
		{
			return TClingTask{std::coroutine_handle<promise_type>::from_promise(*this), State};
		}

		std::suspend_always initial_suspend() noexcept { return {}; }

		struct FFinalSuspend
		{
			bool await_ready() noexcept { return false; }
			void await_suspend(std::coroutine_handle<promise_type> H) noexcept
			{
				H.destroy(); // self-destruct after completion
			}
			void await_resume() noexcept {}
		};

		FFinalSuspend final_suspend() noexcept { return {}; }

		void return_value(T Val) { State->Complete(MoveTemp(Val)); }
		void unhandled_exception() { State->Complete(T{}); }
	};

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

	T await_resume() const
	{
		return State->Value;
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
		// Only valid if task was never started (Launch/await_suspend sets Handle={})
		if (Handle) Handle.destroy();
	}

private:
	TClingTask(std::coroutine_handle<promise_type> InHandle, TSharedPtr<TTaskState<T>> InState)
		: Handle(InHandle), State(MoveTemp(InState))
	{}

	std::coroutine_handle<promise_type> Handle;
	TSharedPtr<TTaskState<T>> State;
};

// ---------------------------------------------------------------------------
// TClingTask<void> specialization
// ---------------------------------------------------------------------------

template<>
class TClingTask<void>
{
public:
	struct promise_type
	{
		TSharedPtr<TTaskState<void>> State = MakeShared<TTaskState<void>>();

		TClingTask get_return_object()
		{
			return TClingTask{std::coroutine_handle<promise_type>::from_promise(*this), State};
		}

		std::suspend_always initial_suspend() noexcept { return {}; }

		struct FFinalSuspend
		{
			bool await_ready() noexcept { return false; }
			void await_suspend(std::coroutine_handle<promise_type> H) noexcept
			{
				H.destroy();
			}
			void await_resume() noexcept {}
		};

		FFinalSuspend final_suspend() noexcept { return {}; }

		void return_void() { State->Complete(); }
		void unhandled_exception() { State->Complete(); }
	};

	bool await_ready() const noexcept
	{
		return State && State->bDone;
	}

	std::coroutine_handle<> await_suspend(std::coroutine_handle<> Caller) noexcept
	{
		if (State && !State->TryAddWaiter(Caller))
		{
			return Caller;
		}
		auto H = static_cast<std::coroutine_handle<>>(Handle);
		Handle = {};
		return H ? H : std::noop_coroutine();
	}

	void await_resume() const noexcept {}

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
		if (Handle) Handle.destroy();
	}

private:
	TClingTask(std::coroutine_handle<promise_type> InHandle, TSharedPtr<TTaskState<void>> InState)
		: Handle(InHandle), State(MoveTemp(InState))
	{}

	std::coroutine_handle<promise_type> Handle;
	TSharedPtr<TTaskState<void>> State;
};

// ---------------------------------------------------------------------------
// Thread-switching awaitables
// ---------------------------------------------------------------------------

class FMoveToTask
{
public:
	bool await_ready() const noexcept { return false; }

	template<typename PromiseType>
	void await_suspend(std::coroutine_handle<PromiseType> Handle)
	{
		auto H = static_cast<std::coroutine_handle<>>(Handle);
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[H]() mutable { H.resume(); },
			TStatId{}, nullptr, ENamedThreads::AnyThread
		);
	}

	void await_resume() const noexcept {}
};

class FMoveToGameThread
{
public:
	bool await_ready() const noexcept { return IsInGameThread(); }

	template<typename PromiseType>
	void await_suspend(std::coroutine_handle<PromiseType> Handle)
	{
		auto H = static_cast<std::coroutine_handle<>>(Handle);
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[H]() mutable { H.resume(); },
			TStatId{}, nullptr, ENamedThreads::GameThread
		);
	}

	void await_resume() const noexcept {}
};

inline FMoveToTask MoveToTask() { return {}; }
inline FMoveToGameThread MoveToGameThread() { return {}; }

} // namespace ClingCoro
