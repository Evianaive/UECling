// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "CppInterOp/CppInterOp.h"
#include "ClingCoroUtils.h"
#include <atomic>

/**
 * Manages a pool of pre-warmed Cling interpreters for different PCH profiles.
 */
class CLINGRUNTIME_API FClingInterpreterPool
{
public:
	FClingInterpreterPool();
	~FClingInterpreterPool();

	/** 
	 * Acquire an interpreter. 
	 * This will first try the pool, and if empty, it will start a new creation task.
	 * Always returns a valid interpreter (or hangs/errors if creation fails).
	 */
	ClingCoro::TClingTask<CppImpl::CppInterpWrapper> AcquireAsync(FName PCHProfile);

	/** Non-blocking attempt to get from pool. Returns invalid wrapper if empty. */
	CppImpl::CppInterpWrapper TryAcquireImmediate(FName PCHProfile);

	/** Ensure pools are filled according to settings */
	void Refill();

	/** Invalidate pool for a specific profile */
	void Invalidate(FName PCHProfile);

	/** Shutdown the pool, canceling in-flight tasks and deleting pooled interpreters */
	void Shutdown();

private:
	/** Internal creation function that also respects shutdown signals */
	CppImpl::CppInterpWrapper CreateNew(FName PCHProfile);

	/** Background task implementation for Refill */
	void CreatePoolEntry(FName PCHProfile);

	TMap<FName, TArray<CppImpl::CppInterpWrapper>> PooledInterps;
	TMap<FName, int32> PoolInFlightPerProfile;
	
	FCriticalSection PoolLock;
	std::atomic<int32> TotalInFlight{0};
	std::atomic<bool> bShuttingDown{false};
};
