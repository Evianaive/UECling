// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <atomic>
#include "Modules/ModuleManager.h"
#include "HAL/CriticalSection.h"
#include "ClingSemanticInfoProvider.h"

#include "CppInterOp/CppInterOp.h"

namespace cling
{
	class Interpreter;
}

class CLINGRUNTIME_API FClingRuntimeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	CppImpl::CppInterpWrapper BaseInterp;
	TArray<CppImpl::CppInterpWrapper> Interps;

	CppImpl::CppInterpWrapper& GetInterp(int Version=0);
	CppImpl::CppInterpWrapper StartNewInterp(FName PCHProfile = TEXT("Default"));
	CppImpl::CppInterpWrapper& StartNewRecordInterp(FName PCHProfile = TEXT("Default"));
	void DeleteInterp(int32 Index);
	int32 FindInterpIndex(CppImpl::CppInterpWrapper& InInterp);
	static FClingRuntimeModule& Get();

	FClingSemanticInfoProvider* GetSemanticInfoProvider(CppImpl::CppInterpWrapper& InInterp);
	FClingSemanticInfoProvider* GetDefaultSemanticInfoProvider();

	/** Get the global lock for CppInterOp calls */
	FCriticalSection& GetCppInterOpLock() { return CppInterOpLock; }

	/**
	 * Try to acquire a pre-warmed interpreter from the pool (non-blocking).
	 * Only returns interpreters created with the "Default" PCH profile.
	 * Returns an invalid wrapper if pool is empty.
	 */
	CppImpl::CppInterpWrapper TryAcquireFromPool();

	/**
	 * Ensure the pool is being filled up to PoolTargetSize in the background.
	 * Safe to call from any thread.
	 */
	void RefillPool();

private:
	static CppImpl::CppInterpWrapper StartInterpreterInternal(FName PCHProfile = TEXT("Default"));

	/** Background worker: create one "Default" interpreter and add to pool */
	void CreatePoolEntry();

	FCriticalSection CppInterOpLock;
	TMap<void*, FClingSemanticInfoProvider> SemanticInfoProviders;

	// --- Interpreter Pool ---
	static constexpr int32 PoolTargetSize = 2;
	TArray<CppImpl::CppInterpWrapper> PooledInterps;       // Ready-to-use interpreters (not yet acquired)
	FCriticalSection PoolLock;         // Guards PooledInterps
	std::atomic<int32> PoolInFlight{0}; // How many are being created right now
	std::atomic<bool> bPoolShuttingDown{false};
};
