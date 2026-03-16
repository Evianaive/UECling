// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClingInterpreterPool.h"
#include "ClingRuntime.h"
#include "ClingSetting.h"
#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "ClingLog/ClingLog.h"

FClingInterpreterPool::FClingInterpreterPool()
	: TotalInFlight(0)
	, bShuttingDown(false)
{
}

FClingInterpreterPool::~FClingInterpreterPool()
{
	Shutdown();
}

ClingCoro::TClingTask<CppImpl::CppInterpWrapper> FClingInterpreterPool::AcquireAsync(FName PCHProfile)
{
	if (PCHProfile.IsNone()) PCHProfile = TEXT("Default");

	// 1. Try immediate
	CppImpl::CppInterpWrapper Found = TryAcquireImmediate(PCHProfile);
	if (Found.IsValid())
	{
		co_return Found;
	}

	// 2. Pool miss, need to create. Ensure we are in a background thread for creation.
	co_await ClingCoro::MoveToBackgroundTask();

	double StartTime = FPlatformTime::Seconds();
	UE_LOG(LogCling, Log, TEXT("[InterpPool] Pool miss for [%s], creating new interpreter..."), *PCHProfile.ToString());

	// Use the central creation logic in RuntimeModule
	CppImpl::CppInterpWrapper NewInterp = CreateNew(PCHProfile);
	
	double Duration = FPlatformTime::Seconds() - StartTime;
	if (NewInterp.IsValid())
	{
		UE_LOG(LogCling, Log, TEXT("[InterpPool] Created new interpreter for [%s] in %.2f seconds."), *PCHProfile.ToString(), Duration);
	}
	else
	{
		UE_LOG(LogCling, Error, TEXT("[InterpPool] Failed to create interpreter for [%s]."), *PCHProfile.ToString());
	}

	co_return NewInterp;
}

CppImpl::CppInterpWrapper FClingInterpreterPool::TryAcquireImmediate(FName PCHProfile)
{
	if (PCHProfile.IsNone()) PCHProfile = TEXT("Default");

	FScopeLock Lock(&PoolLock);
	if (TArray<CppImpl::CppInterpWrapper>* Pool = PooledInterps.Find(PCHProfile))
	{
		if (Pool->Num() > 0)
		{
			CppImpl::CppInterpWrapper Interp = Pool->Pop();
			// Refill asynchronously if we dropped below target
			AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this]() { Refill(); });
			return Interp;
		}
	}
	return CppImpl::CppInterpWrapper();
}

void FClingInterpreterPool::Refill()
{
	if (bShuttingDown.load()) return;

	UClingSetting* Settings = GetMutableDefault<UClingSetting>();
	TArray<const FClingPCHProfile*> AllProfiles;
	AllProfiles.Add(&Settings->DefaultPCHProfile);
	for (const auto& Profile : Settings->PCHProfiles)
	{
		AllProfiles.Add(&Profile);
	}

	for (const FClingPCHProfile* Profile : AllProfiles)
	{
		if (!Profile->bEnabled || Profile->PoolSize <= 0) continue;

		FName ProfileName = Profile->ProfileName;
		int32 TargetSize = Profile->PoolSize;
		
		int32 ToCreate = 0;
		{
			FScopeLock Lock(&PoolLock);
			int32 Current = 0;
			if (TArray<CppImpl::CppInterpWrapper>* Pool = PooledInterps.Find(ProfileName))
			{
				Current += Pool->Num();
			}
			if (int32* InFlight = PoolInFlightPerProfile.Find(ProfileName))
			{
				Current += *InFlight;
			}
			
			ToCreate = FMath::Max(0, TargetSize - Current);
			if (ToCreate > 0)
			{
				PoolInFlightPerProfile.FindOrAdd(ProfileName) += ToCreate;
				TotalInFlight.fetch_add(ToCreate);
			}
		}

		for (int32 i = 0; i < ToCreate; ++i)
		{
			AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, ProfileName]()
			{
				CreatePoolEntry(ProfileName);
			});
		}
	}
}

void FClingInterpreterPool::Invalidate(FName PCHProfile, bool bRefill /*= false*/)
{
	if (PCHProfile.IsNone()) PCHProfile = TEXT("Default");
	{
		FScopeLock Lock(&PoolLock);
		if (TArray<CppImpl::CppInterpWrapper>* Pool = PooledInterps.Find(PCHProfile))
		{
			for (CppImpl::CppInterpWrapper& Interp : *Pool)
			{
				if (Interp.IsValid())
				{
					Interp.DeleteInterpreter();
				}
			}
			Pool->Empty();
		}
	}
	if (bRefill)
	{
		Refill();
	}
}

void FClingInterpreterPool::Shutdown()
{
	bShuttingDown.store(true);

	// Wait for in-flight creations
	while (TotalInFlight.load() > 0)
	{
		FPlatformProcess::Sleep(0.01f);
	}

	FScopeLock Lock(&PoolLock);
	for (auto& Pair : PooledInterps)
	{
		for (CppImpl::CppInterpWrapper& Interp : Pair.Value)
		{
			if (Interp.IsValid())
			{
				Interp.DeleteInterpreter();
			}
		}
		Pair.Value.Empty();
	}
	PooledInterps.Empty();
}

void FClingInterpreterPool::CleanOrphanedPools()
{
	UClingSetting* Settings = GetMutableDefault<UClingSetting>();
	TSet<FName> ValidProfiles;
	if (Settings->DefaultPCHProfile.bEnabled && Settings->DefaultPCHProfile.PoolSize > 0)
	{
		ValidProfiles.Add(TEXT("Default"));
	}
	for (const auto& Profile : Settings->PCHProfiles)
	{
		if (Profile.bEnabled && Profile.PoolSize > 0)
		{
			ValidProfiles.Add(Profile.ProfileName);
		}
	}

	TArray<FName> ToRemove;
	{
		FScopeLock Lock(&PoolLock);
		for (auto& Pair : PooledInterps)
		{
			if (!ValidProfiles.Contains(Pair.Key))
			{
				for (CppImpl::CppInterpWrapper& Interp : Pair.Value)
				{
					if (Interp.IsValid())
					{
						Interp.DeleteInterpreter();
					}
				}
				Pair.Value.Empty();
				ToRemove.Add(Pair.Key);
			}
		}
		for (const FName& Key : ToRemove)
		{
			PooledInterps.Remove(Key);
			PoolInFlightPerProfile.Remove(Key);
		}
	}
}

CppImpl::CppInterpWrapper FClingInterpreterPool::CreateNew(FName PCHProfile)
{
	// Forward to module as it has access to the lock and StartInternal
	return FClingRuntimeModule::Get().StartNewInterp(PCHProfile);
}

void FClingInterpreterPool::CreatePoolEntry(FName PCHProfile)
{
	if (bShuttingDown.load())
	{
		FScopeLock Lock(&PoolLock);
		if (int32* InFlight = PoolInFlightPerProfile.Find(PCHProfile)) (*InFlight)--;
		TotalInFlight.fetch_sub(1);
		return;
	}

	CppImpl::CppInterpWrapper NewInterp = CreateNew(PCHProfile);

	// Validate that profile is still enabled/desired
	bool bProfileStillValid = false;
	{
		UClingSetting* Settings = GetMutableDefault<UClingSetting>();
		if (PCHProfile == TEXT("Default"))
		{
			bProfileStillValid = Settings->DefaultPCHProfile.bEnabled && Settings->DefaultPCHProfile.PoolSize > 0;
		}
		else
		{
			for (const auto& Profile : Settings->PCHProfiles)
			{
				if (Profile.ProfileName == PCHProfile)
				{
					bProfileStillValid = Profile.bEnabled && Profile.PoolSize > 0;
					break;
				}
			}
		}
	}

	{
		FScopeLock Lock(&PoolLock);
		if (!bShuttingDown.load() && bProfileStillValid && NewInterp.IsValid())
		{
			PooledInterps.FindOrAdd(PCHProfile).Add(NewInterp);
		}
		else if (NewInterp.IsValid())
		{
			NewInterp.DeleteInterpreter();
		}

		if (int32* InFlight = PoolInFlightPerProfile.Find(PCHProfile)) (*InFlight)--;
		TotalInFlight.fetch_sub(1);
	}
}
