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
	TArray<FString> DynamicLinkNames{"libclang.dll", "libcling", "libclingJupyter.dll",	"LLVM-C.dll", "LTO.dll", "Remarks.dll", "RelWithDebInfo/cling-demo.dll"};
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

	//if (ExampleLibraryHandle)
	//{
	//	// Call the test function in the third party library that opens a message box
	//	ExampleLibraryFunction();
	//}
	//else
	//{
	//	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "Failed to load example third party library"));
	//}

	FString LLVMDir = FPaths::ConvertRelativePathToFull(BaseDir/TEXT("Source/ThirdParty/UClingLibrary/LLVM"));
	FString LLVMInclude = LLVMDir/TEXT("include");
	
	
	// GEngine->
	FString RelativePath = FPaths::ConvertRelativePathToFull(TEXT(".."));
	FString UE_Exec = FPlatformProcess::ExecutablePath();
	// UE_Exec = UE_Exec+TEXT(" -v");
	TArray<const char*> Argv;
	Argv.Add(StringCast<ANSICHAR>(*UE_Exec).Get());
	Argv.Add("-I");
	Argv.Add(StringCast<ANSICHAR>(*LLVMInclude).Get());
	Argv.Add("-v");
	// cling::Interpreter interp2(1, &Path
	// 	, StringCast<ANSICHAR>(*LLVMDir).Get()
	// 	);
	// FString PathToStaticFuncHeader = BaseDir/TEXT("Source/UCling/Public/TestStatic.h");
	
	Interp = CreateInterp(Argv.Num(), Argv.GetData(), StringCast<ANSICHAR>(*LLVMDir).Get());
	// Decalre(Interp,"#define UCLING_API ");
	// Decalre(Interp,"#define TESTCALLANOTHERMODULE_API ");
	
	IPluginManager::Get().GetEnabledPlugins();
	TArray<FModuleStatus> ModuleStatuses;
	FModuleManager::Get().QueryModules(ModuleStatuses);
	const UClingSetting* Setting = GetDefault<UClingSetting>();
	// TJsonReaderFactory<>
	for (auto& Module : ModuleStatuses)
	{
		// Todo here we assume the module name is the folder name!
		FString Source = Module.FilePath/TEXT("../../../Source");
		FString IncludePath = Source/Module.Name;
		FString IncludePathPublic = Source/Module.Name/TEXT("Public");
		
		// UE_LOG(LogTemp, Log,TEXT("name:%s"),*IncludePath);
		UE_LOG(LogTemp, Log,TEXT("path:%s"),*Module.FilePath);
		
		FString MacroDefine = TEXT("#define ") + Module.Name.ToUpper() + TEXT("_API ");
		Decalre(Interp,StringCast<ANSICHAR>(*MacroDefine).Get());
		AddIncludePath(Interp,StringCast<ANSICHAR>(*IncludePath).Get());
		AddIncludePath(Interp,StringCast<ANSICHAR>(*IncludePathPublic).Get());
		
		if(const auto* Paths = Setting->ModuleIncludePaths.Find(FName(Module.Name)))
		{
			for (const auto& AdditionalPath : Paths->AdditionIncludePaths)
			{
				AddIncludePath(Interp,StringCast<ANSICHAR>(*(Source/AdditionalPath)).Get());
			}
		}
	}
	
	
	// LoadHeader(Interp,StringCast<ANSICHAR>(*PathToStaticFuncHeader).Get());
	// Interp->declare()
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
