// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClingRuntime.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Async/TaskGraphInterfaces.h"
#include <algorithm>
#include "Engine/Engine.h"
#include "CppInterOp/CppInterOp.h"
#include "ClingSetting.h"
#include "ClingLog/ClingLog.h"
#include "ClingLog/LogRedirector.h"

#if PLATFORM_WINDOWS
#include <windows.h>
#endif
#define LOCTEXT_NAMESPACE "FClingRuntimeModule"


FString GetLLVMDir()
{
	FString BaseDir = IPluginManager::Get().FindPlugin("UECling")->GetBaseDir();
	return FPaths::ConvertRelativePathToFull(BaseDir/TEXT("Source/ThirdParty/ClingLibrary/LLVM"));
}
FString GetLLVMInclude()
{
	return GetLLVMDir()/TEXT("include");
}

void FClingRuntimeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString RelativePath = FPaths::ConvertRelativePathToFull(TEXT(".."));
	
	FLogRedirector::RedirectToUELog();	
	UClingSetting* Setting = GetMutableDefault<UClingSetting>();	
	// Load file contains all module build infos
	Setting->RefreshIncludePaths();
	Setting->GenerateAllPCHProfiles();	
	// Cpp::EnableDebugOutput();
	BaseInterp = StartInterpreterInternal();
	// Initial pool fill
	RefillPool();
	// Pool is filled lazily when notebooks become idle, see ProcessNextInQueue.
}

void FClingRuntimeModule::ShutdownModule()
{
	// Signal pool workers to stop
	bPoolShuttingDown.store(true);

	// Wait for any in-flight pool creation tasks to finish
	while (PoolInFlight.load() > 0)
	{
		FPlatformProcess::Sleep(0.01f);
	}

	FScopeLock CppLock(&CppInterOpLock);

	// Delete pooled (pre-created, not yet acquired) interpreters
	for (CppImpl::CppInterpWrapper& Interp : PooledInterps)
	{
		if (Interp.IsValid())
		{
			Interp.DeleteInterpreter();
		}
	}
	PooledInterps.Empty();

	SemanticInfoProviders.Empty();
	// CppInterpWrapper destructor will handle cleanup
}

CppImpl::CppInterpWrapper& FClingRuntimeModule::GetInterp(int Version)
{
	const int id = Interps.Num()-Version-1;
	if(Interps.IsValidIndex(id))
		return Interps[id];
	UE_LOG(LogCling,Log,TEXT("Invalid version of Interpreter!"));
	return Interps[Interps.Num()-1];
}

CppImpl::CppInterpWrapper FClingRuntimeModule::StartNewInterp(FName PCHProfile)
{
	return StartInterpreterInternal(PCHProfile);
}

CppImpl::CppInterpWrapper& FClingRuntimeModule::StartNewRecordInterp(FName PCHProfile)
{
	return Interps.Add_GetRef(StartInterpreterInternal(PCHProfile));
}

void FClingRuntimeModule::DeleteInterp(int32 Index)
{
	FScopeLock Lock(&CppInterOpLock);
	if (Interps.IsValidIndex(Index))
	{
		void* InterpreterPtr = Interps[Index].GetInterpreter();
		SemanticInfoProviders.Remove(InterpreterPtr);
		Interps.RemoveAt(Index);
	}
}

int32 FClingRuntimeModule::FindInterpIndex(CppImpl::CppInterpWrapper& InInterp)
{
	void* InterpreterPtr = InInterp.GetInterpreter();
	for (int32 i = 0; i < Interps.Num(); ++i)
	{
		if (Interps[i].GetInterpreter() == InterpreterPtr)
			return i;
	}
	return -1;
}

FClingRuntimeModule& FClingRuntimeModule::Get()
{
	return FModuleManager::LoadModuleChecked<FClingRuntimeModule>(TEXT("ClingRuntime"));
}

struct FClingSemanticInfoProvider* FClingRuntimeModule::GetDefaultSemanticInfoProvider()
{
	return GetSemanticInfoProvider(BaseInterp);
}

struct FClingSemanticInfoProvider* FClingRuntimeModule::GetSemanticInfoProvider(CppImpl::CppInterpWrapper& InInterp)
{
	void* InterpreterPtr = InInterp.GetInterpreter();
	if (!InInterp.IsValid()) return nullptr;

	if (FClingSemanticInfoProvider* Found = SemanticInfoProviders.Find(InterpreterPtr))
	{
		return Found;
	}

	FClingSemanticInfoProvider& NewProvider = SemanticInfoProviders.Add(InterpreterPtr);
	NewProvider.Refresh(InInterp);
	return &NewProvider;
}

CppImpl::CppInterpWrapper FClingRuntimeModule::StartInterpreterInternal(FName PCHProfile)
{
	SCOPED_NAMED_EVENT(StartInterpreterInternal, FColor::Red);
	// FString LLVMDir = GetLLVMDir();
	// FString LLVMInclude = GetLLVMInclude();
	// FString UE_Exec = FPlatformProcess::ExecutablePath();
	UClingSetting* Setting = GetMutableDefault<UClingSetting>();
	// Start ClingInterpreter

	TArray<const char*> Argv;

	// Argv.PRIVATE_ADD("-I");
	// Argv.PRIVATE_ADD(StringCast<ANSICHAR>(*LLVMInclude).Get());
	Setting->AppendCompileArgs(Argv);
	// add all include paths even if we use PCH
	TArray<FAnsiString> Paths;
	auto AddIncludePath = [&Paths,&Argv](const FString& Path)
	{
		Paths.Add(StringCast<ANSICHAR>(*Path.Replace(TEXT("\\"),TEXT("/"))).Get());
		// Cpp::AddIncludePath(StringCast<ANSICHAR>(*Path.Replace(TEXT("\\"),TEXT("/"))).Get());
		Argv.Add("-I");
		Argv.Add(*Paths.Last());
	}; 
	Setting->IterThroughIncludePaths(AddIncludePath);
	
	// include PCH source file to use PCH automatically
	const FClingPCHProfile& Profile = Setting->GetProfile(PCHProfile);
	FAnsiString PCHHeaderFilePath{StringCast<ANSICHAR>(*Profile.GetPCHHeaderPath()).Get()};
	Argv.Add("-include");
	Argv.Add(*PCHHeaderFilePath);

	// Todo use if is debug build
// #if USING_CPPINTEROP_DEBUG
	// the abi of debug build of std::vector is different between unreal and clang! use raw input
	CppImpl::CppInterpWrapper Interp;
	{
		SCOPED_NAMED_EVENT(TrueStart, FColor::Red);
		Interp.CreateInterpreter(&Argv[0], Argv.Num(),nullptr,0);
		UE_LOG(LogCling,Log,TEXT("CreateInterpreter %p"), Interp.GetInterpreter());
	}
	{
		SCOPED_NAMED_EVENT(DeclareOverride, FColor::Red);
		// Cling-safe UE_LOG wrapper (CLING_LOG macro and ClingLog:: namespace)
		Interp.Declare("#include \"ClingScript/Private/UEClingCoreScript/ClingLogWrapper.h\"");
	}
// #else
	// auto Interp = Cpp::CreateInterpreter(Argv, {});
// #endif
	return Interp;
}

#undef LOCTEXT_NAMESPACE

// ---------------------------------------------------------------------------
// Interpreter Pool
// ---------------------------------------------------------------------------

CppImpl::CppInterpWrapper FClingRuntimeModule::TryAcquireFromPool()
{
	FScopeLock Lock(&PoolLock);
	if (PooledInterps.Num() > 0)
	{
		CppImpl::CppInterpWrapper Interp = PooledInterps.Pop();
		UE_LOG(LogCling, Log, TEXT("[InterpPool] Acquired from pool. Remaining: %d"), PooledInterps.Num());
		return Interp;
	}
	UE_LOG(LogCling, Warning, TEXT("[InterpPool] Pool empty! Notebook will create a new interpreter synchronously (slow)."));
	return CppImpl::CppInterpWrapper();
}

void FClingRuntimeModule::RefillPool()
{
	if (bPoolShuttingDown.load()) return;

	int32 ToCreate;
	{
		FScopeLock Lock(&PoolLock);
		const int32 Current = PooledInterps.Num() + PoolInFlight.load();
		ToCreate = FMath::Max(0, PoolTargetSize - Current);
		PoolInFlight.fetch_add(ToCreate);
	}

	for (int32 i = 0; i < ToCreate; ++i)
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[this]()
			{
				CreatePoolEntry();
			},
			TStatId{}, nullptr, ENamedThreads::AnyThread
		);
	}

	if (ToCreate > 0)
	{
		UE_LOG(LogCling, Log, TEXT("[InterpPool] Dispatched %d background create task(s)."), ToCreate);
	}
}

void FClingRuntimeModule::CreatePoolEntry()
{
	if (bPoolShuttingDown.load())
	{
		PoolInFlight.fetch_sub(1);
		return;
	}

	UE_LOG(LogCling, Log, TEXT("[InterpPool] Creating pool interpreter..."));
	
	double StartTime = FPlatformTime::Seconds();
	CppImpl::CppInterpWrapper NewInterp = StartInterpreterInternal(TEXT("Default"));
	double Duration = FPlatformTime::Seconds() - StartTime;

	UE_LOG(LogCling, Log, TEXT("[InterpPool] Created pool interpreter in %.2f seconds."), Duration);

	{
		FScopeLock Lock(&PoolLock);
		if (!bPoolShuttingDown.load() && NewInterp.IsValid())
		{
			PooledInterps.Add(NewInterp);
			UE_LOG(LogCling, Log, TEXT("[InterpPool] Pool size: %d"), PooledInterps.Num());
		}
		else if (NewInterp.IsValid())
		{
			// Shutting down - discard
			NewInterp.DeleteInterpreter();
		}
	}

	PoolInFlight.fetch_sub(1);
}
	
IMPLEMENT_MODULE(FClingRuntimeModule, ClingRuntime);
