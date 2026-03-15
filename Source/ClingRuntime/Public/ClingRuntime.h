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
	 * Returns an invalid wrapper if pool is empty for the given profile.
	 */
	CppImpl::CppInterpWrapper TryAcquireFromPool(FName PCHProfile = TEXT("Default"));

	/**
	 * Ensure the pool is being filled up to PoolTargetSize in the background.
	 * Safe to call from any thread.
	 */
	void RefillPool();

	/** Clear pool for a specific profile (e.g. when configuration changes) */
	void InvalidatePool(FName PCHProfile);

	/** Clear all pools */
	void InvalidateAllPools();

private:
	static CppImpl::CppInterpWrapper StartInterpreterInternal(FName PCHProfile = TEXT("Default"));

	/** Background worker: create one interpreter for the profile and add to pool */
	void CreatePoolEntry(FName PCHProfile);

	FCriticalSection CppInterOpLock;
	TMap<void*, FClingSemanticInfoProvider> SemanticInfoProviders;

	// --- Interpreter Pool ---
	TMap<FName, TArray<CppImpl::CppInterpWrapper>> PooledInterps;       // Ready-to-use interpreters per profile (not yet acquired)
	TMap<FName, int32> PoolInFlightPerProfile; // How many are being created right now per profile
	FCriticalSection PoolLock;         // Guards PooledInterps and PoolInFlightPerProfile
	std::atomic<int32> PoolInFlight{0}; // Total number being created right now
	std::atomic<bool> bPoolShuttingDown{false};
};
