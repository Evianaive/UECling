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
	Setting->GeneratePCH();	
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

Cpp::TInterp_t FClingRuntimeModule::StartNewInterp()
{
	return Interps.Add_GetRef(StartInterpreterInternal());
}

void FClingRuntimeModule::DeleteInterp(void* CurrentInterp)
{
	Cpp::DeleteInterpreter(CurrentInterp);
	Interps.Remove(CurrentInterp);
}

Cpp::TInterp_t FClingRuntimeModule::StartInterpreterInternal()
{
	SCOPED_NAMED_EVENT(StartInterpreterInternal, FColor::Red)
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
	FAnsiString PCHHeaderFilePath{StringCast<ANSICHAR>(*UClingSetting::GetPCHSourceFilePath()).Get()};
	Argv.Add("-include");
	Argv.Add(*PCHHeaderFilePath);

	// Todo use if is debug build
// #if USING_CPPINTEROP_DEBUG
	// the abi of debug build of std::vector is different between unreal and clang! use raw input
	auto Interp = Cpp::CreateInterpreter(&Argv[0], Argv.Num(),nullptr,0);
// #else
	// auto Interp = Cpp::CreateInterpreter(Argv, {});
// #endif
	return Interp;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FClingRuntimeModule, ClingRuntime)
