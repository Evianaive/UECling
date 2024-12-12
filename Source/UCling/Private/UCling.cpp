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

	// for (const auto& DynamicLinkName : DynamicLinkNames)
	// {
	// 	FString DynamicLink = LibraryPath+DynamicLinkName;
	// 	ExampleLibraryHandles.Add(!DynamicLink.IsEmpty() ? FPlatformProcess::GetDllHandle(*DynamicLink) : nullptr);
	// }
	
	FString LLVMDir = FPaths::ConvertRelativePathToFull(BaseDir/TEXT("Source/ThirdParty/UClingLibrary/LLVM"));
	FString LLVMInclude = LLVMDir/TEXT("include");
	
	FString RelativePath = FPaths::ConvertRelativePathToFull(TEXT(".."));
	FString UE_Exec = FPlatformProcess::ExecutablePath();
	
	UClingSetting* Setting = GetMutableDefault<UClingSetting>();
	
	TArray<const char*> Argv;
	Argv.Add(StringCast<ANSICHAR>(*UE_Exec).Get());
	Argv.Add("-I");
	Argv.Add(StringCast<ANSICHAR>(*LLVMInclude).Get());
	if(Setting->bVerbose)
		Argv.Add("-v");
	Interp = CreateInterp(Argv.Num(), Argv.GetData(), StringCast<ANSICHAR>(*LLVMDir).Get());
	
	IPluginManager::Get().GetEnabledPlugins();
#if WITH_EDITOR
	Decalre(Interp,"#define WITH_EDITOR 1");
	Decalre(Interp,"#define WITH_EDITORONLY_DATA_1");
	Decalre(Interp,"#define UE_EDITOR 1");
	//Todo Read from buildinfo
	Decalre(Interp,"#define UE_BUILD_DEVELOPMENT 1");
	Decalre(Interp,"#define WITH_ENGINE 1");
	Decalre(Interp,"#define WITH_UNREAL_DEVELOPER_TOOLS 1");
	Decalre(Interp,"#define WITH_PLUGIN_SUPPORT 1");
	Decalre(Interp,"#define IS_MONOLITHIC 0");
	Decalre(Interp,"#define IS_PROGRAM 0");
	Decalre(Interp,"#define UBT_COMPILED_PLATFORM Windows");
	Decalre(Interp,"#define PLATFORM_WINDOWS 1");
	Decalre(Interp,"#define WINVER 0x601");
	Decalre(Interp,"#define __TCHAR_DEFINED 1");
	Decalre(Interp,"#define _TCHAR_DEFINED 1");
	Decalre(Interp,"#define _UNICODE 1");
	Decalre(Interp,"#define WITH_SERVER_CODE 1");
	Decalre(Interp,"#define WINDOWS_MAX_NUM_TLS_SLOTS 2048");
#endif
	
	Setting->RefreshIncludePaths();
	 
	for (auto& ModuleBuildInfo : Setting->ModuleBuildInfos)
	{
		// Begin IncludePaths
		for (const auto& PublicIncludePath : ModuleBuildInfo.Value.PublicIncludePaths)
		{
			AddIncludePath(Interp,StringCast<ANSICHAR>(*PublicIncludePath).Get());
		}
		for (const auto& PrivateIncludePath : ModuleBuildInfo.Value.PrivateIncludePaths)
		{
			AddIncludePath(Interp,StringCast<ANSICHAR>(*PrivateIncludePath).Get());
		}
		// Begin Definitions
		FString MacroDefine = TEXT("#define ") + ModuleBuildInfo.Value.Name.ToString().ToUpper() + TEXT("_API ");
		Decalre(Interp,StringCast<ANSICHAR>(*MacroDefine).Get());
		for (FString& Define : ModuleBuildInfo.Value.PublicDefinitions)
		{
			Define.ReplaceCharInline('=',' ');
			Decalre(Interp,StringCast<ANSICHAR>(*(TEXT("#define ")+Define)).Get());
		}
	}
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
