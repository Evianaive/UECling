// Copyright Epic Games, Inc. All Rights Reserved.

#include "UCling.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
//#include "UClingLibrary/ExampleLibrary.h"
#include <algorithm>
#include "cling/Interpreter/Interpreter.h"
#include "Engine/Engine.h"
#include "cling-demo.h"
#include "ClingSetting.h"
#include "ClingLog/LogRedirector.h"

#define LOCTEXT_NAMESPACE "FUClingModule"

void FUClingModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Get the base directory of this plugin
	FString BaseDir = IPluginManager::Get().FindPlugin("UCling")->GetBaseDir();

	// Add on the relative location of the third party dll and load it
	FString LibraryPath;
#if PLATFORM_WINDOWS
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/UClingLibrary/LLVM/bin/"));
	// TArray<FString> DynamicLinkNames{"libclang.dll", "libcling", "libclingJupyter.dll",	"LLVM-C.dll", "LTO.dll", "Remarks.dll", "RelWithDebInfo/cling-demo.dll"};
#elif PLATFORM_MAC
    LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/UClingLibrary/Mac/Release/libExampleLibrary.dylib"));
#elif PLATFORM_LINUX
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/UClingLibrary/Linux/x86_64-unknown-linux-gnu/libExampleLibrary.so"));
#endif // PLATFORM_WINDOWS
	
	FString LLVMDir = FPaths::ConvertRelativePathToFull(BaseDir/TEXT("Source/ThirdParty/UClingLibrary/LLVM"));
	FString LLVMInclude = LLVMDir/TEXT("include");
	
	FString RelativePath = FPaths::ConvertRelativePathToFull(TEXT(".."));
	FString UE_Exec = FPlatformProcess::ExecutablePath();
	
	UClingSetting* Setting = GetMutableDefault<UClingSetting>();

	// Start ClingInterpreter
	TArray<const char*> Argv;
	Argv.Add(StringCast<ANSICHAR>(*UE_Exec).Get());
	Argv.Add("-I");
	Argv.Add(StringCast<ANSICHAR>(*LLVMInclude).Get());
	FLogRedirector::RedirectToUELog();
	// Forbid RTTI	
	// Argv.Add("-fno-rtti");
	if(Setting->bVerbose)
		Argv.Add("-v");
	if(Setting->bIgnoreMissingOverride)
		Argv.Add("-Wno-inconsistent-missing-override");
	if(Setting->bIgnoreInvalidOffsetOf)
		Argv.Add("-Wno-invalid-offsetof");	
	if(Setting->bIgnoreDeprecatedEnumEnumConversion)
		Argv.Add("-Wno-deprecated-enum-enum-conversion");
	if(Setting->bIgnoreInCompleteSwitch)
		Argv.Add("-Wno-switch");
	if(Setting->bIgnoreAutoLogicalUndefinedCompare)
		Argv.Add("-Wno-tautological-undefined-compare");
	if(Setting->bIgnoreStringLiteralOperatorTemplate)
		Argv.Add("-Wno-gnu-string-literal-operator-template");
	Argv.Add("-march=native");
	// Todo #include "HAL/Platform.h"	
	Interp = CreateInterp(Argv.Num(), Argv.GetData(), StringCast<ANSICHAR>(*LLVMDir).Get());

	// Include file of all global definitions of build context export by UnrealBuildTool 
	UE_Exec.ReplaceCharInline('\\','/');
	FString GlobalDefinesFilePath = UE_Exec/TEXT("../UECling/BuildGlobalDefines.h");
	Decalre(Interp,StringCast<ANSICHAR>(*(TEXT("#include \"")+GlobalDefinesFilePath+TEXT("\""))).Get());

	// Load file contains all module build infos
	Setting->RefreshIncludePaths();
	AddIncludePath(Interp,StringCast<ANSICHAR>(*FPaths::EngineSourceDir()).Get());
	for (auto& ModuleBuildInfo : Setting->ModuleBuildInfos)
	{
		// Begin IncludePaths
		for (const auto& PublicIncludePath : ModuleBuildInfo.Value.PublicIncludePaths)
		{
			AddIncludePath(Interp,StringCast<ANSICHAR>(*PublicIncludePath).Get());
		}
		// We don't need to add Private Include Path! A module should not add path to headers
		// that can be include by other module in PrivateIncludePaths		
		// for (const auto& PrivateIncludePath : ModuleBuildInfo.Value.PrivateIncludePaths)
		// {
		// 	AddIncludePath(Interp,StringCast<ANSICHAR>(*PrivateIncludePath).Get());
		// }

		for (const auto& PrivateIncludePath : ModuleBuildInfo.Value.PublicSystemIncludePaths)
		{
			AddIncludePath(Interp,StringCast<ANSICHAR>(*PrivateIncludePath).Get());
		}
		// Begin Definitions
		// Todo Ignore LAUNCH_API since it is defined in BuildGlobalDefines.h generated by us
		FString MacroDefine = TEXT("#define ") + ModuleBuildInfo.Value.Name.ToString().ToUpper() + TEXT("_API ");
		Decalre(Interp,StringCast<ANSICHAR>(*MacroDefine).Get());
		for (FString& Define : ModuleBuildInfo.Value.PublicDefinitions)
		{
			Define.ReplaceCharInline('=',' ');
			Decalre(Interp,StringCast<ANSICHAR>(*(TEXT("#define ")+Define)).Get());
		}
		if(!ModuleBuildInfo.Value.Directory.Contains(TEXT("Engine/Source")))
			AddIncludePath(Interp,StringCast<ANSICHAR>(*ModuleBuildInfo.Value.Directory).Get());
	}
	for (const auto& GeneratedHeaderIncludePath : Setting->GeneratedHeaderIncludePaths)
	{
		AddIncludePath(Interp,StringCast<ANSICHAR>(*GeneratedHeaderIncludePath).Get());
	}
// #ifdef _MSC_VER
// #define EVA_MACRO(_M) #_M
// #define STR_MACRO(_M) "#define " #_M " " EVA_MACRO(_M)	
// 	Decalre(Interp,StringCast<ANSICHAR>(TEXT(STR_MACRO(_MSC_VER))).Get());
// 	Decalre(Interp,StringCast<ANSICHAR>(TEXT(STR_MACRO(_WIN64))).Get());
// #endif
	// Declare some files which are general in Unreal
#ifdef __clang__
#else 
	Decalre(Interp,"#include \"Microsoft/MinimalWindowsApi.h\"");
	Decalre(Interp,"#include \"Runtime/Core/Private/Microsoft/MinimalWindowsApi.cpp\"");
#endif
	Decalre(Interp,"#include \"CoreMinimal.h\"");
	Decalre(Interp,"#include \"UObject/Object.h\"");
	Decalre(Interp,"#include \"Logging/LogMacros.h\"");
}

void FUClingModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// Free the dll handle
	for (auto& Handle: ExampleLibraryHandles)
	{
		FPlatformProcess::FreeDllHandle(Handle);
		Handle = nullptr;
	}
	ExampleLibraryHandles.Reset();

	delete Interp;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUClingModule, UCling)
