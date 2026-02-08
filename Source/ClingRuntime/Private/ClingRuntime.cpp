// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClingRuntime.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
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
	
	// Decalre(BaseInterp,"#define WITH_CLING 1");
	// if(Setting->bAllowRedefine)
	// 	Execute(BaseInterp,"gClingOpts->AllowRedefinition = true;");
}

void FClingRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FScopeLock Lock(&CppInterOpLock);
	SemanticInfoProviders.Empty();
	Cpp::DeleteInterpreter(BaseInterp);
}

void* FClingRuntimeModule::GetInterp(int Version)
{
	const int id = Interps.Num()-Version-1;
	if(Interps.IsValidIndex(id))
		return Interps[id];
	UE_LOG(LogCling,Log,TEXT("Invalid version of Interpreter!"));
	return Interps[Interps.Num()-1];
}

Cpp::TInterp_t FClingRuntimeModule::StartNewInterp(FName PCHProfile)
{
	return Interps.Add_GetRef(StartInterpreterInternal(PCHProfile));
}

void FClingRuntimeModule::DeleteInterp(void* CurrentInterp)
{
	FScopeLock Lock(&CppInterOpLock);
	Cpp::DeleteInterpreter(CurrentInterp);
	Interps.Remove(CurrentInterp);
	SemanticInfoProviders.Remove(CurrentInterp);
}

FClingRuntimeModule& FClingRuntimeModule::Get()
{
	return FModuleManager::LoadModuleChecked<FClingRuntimeModule>(TEXT("ClingRuntime"));
}

struct FClingSemanticInfoProvider* FClingRuntimeModule::GetDefaultSemanticInfoProvider()
{
	return GetSemanticInfoProvider(BaseInterp);
}

struct FClingSemanticInfoProvider* FClingRuntimeModule::GetSemanticInfoProvider(void* InInterp)
{
	if (!InInterp) return nullptr;

	if (FClingSemanticInfoProvider* Found = SemanticInfoProviders.Find(InInterp))
	{
		return Found;
	}

	FClingSemanticInfoProvider& NewProvider = SemanticInfoProviders.Add(InInterp);
	NewProvider.Refresh(InInterp);
	return &NewProvider;
}

Cpp::TInterp_t FClingRuntimeModule::StartInterpreterInternal(FName PCHProfile)
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
	Cpp::TInterp_t Interp;
	{
		SCOPED_NAMED_EVENT(TrueStart, FColor::Red);
		FScopeLock Lock(&CppInterOpLock);
		Interp = Cpp::CreateInterpreter(&Argv[0], Argv.Num(),nullptr,0);
		UE_LOG(LogCling,Log,TEXT("CreateInterpreter %p"),Interp);
	}
	{
		SCOPED_NAMED_EVENT(DeclareOverride, FColor::Red);
		// Cling-safe UE_LOG wrapper (CLING_LOG macro and ClingLog:: namespace)
		Cpp::Declare("#include \"ClingScript/Private/UEClingCoreScript/ClingLogWrapper.h\"");
	}
// #else
	// auto Interp = Cpp::CreateInterpreter(Argv, {});
// #endif
	return Interp;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FClingRuntimeModule, ClingRuntime);
