#include "ClingLoadModule.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FClingLoadModuleModule"


FString GetClingBinariesPath()
{
	FString BaseDir = IPluginManager::Get().FindPlugin("UECling")->GetBaseDir();
#if PLATFORM_WINDOWS
#if PLATFORM_CPU_ARM_FAMILY
	const TCHAR* BinariesArch = TEXT("arm64");
#else
	const TCHAR* BinariesArch = TEXT("x64");
#endif
	return FPaths::Combine(BaseDir, TEXT("Source"), TEXT("ThirdParty"), TEXT("ClingLibrary"),TEXT("LLVM"), TEXT("bin"));
#elif PLATFORM_MAC
	return FPaths::Combine(FPaths::EngineDir(), TEXT("Binaries"), TEXT("Mac"), TEXT("UnrealBuildAccelerator"));
#elif PLATFORM_LINUX
	return FPaths::Combine(FPaths::EngineDir(), TEXT("Binaries"), TEXT("Linux"), TEXT("UnrealBuildAccelerator"));
#else
#error Unsupported platform to compile UbaController plugin. Only Win64, Mac, and Linux are supported!
#endif
}

void FClingLoadModuleModule::StartupModule()
{
	const FString BinariesPath = GetClingBinariesPath();
	FPlatformProcess::AddDllDirectory(*BinariesPath);
	// FPlatformProcess::GetDllHandle(*(FPaths::Combine(UbaBinariesPath, "UbaHost.dll")));    
}

void FClingLoadModuleModule::ShutdownModule()
{
    
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FClingLoadModuleModule, ClingLoadModule)