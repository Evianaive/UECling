#include "ClingLoadModule.h"

#include "Interfaces/IPluginManager.h"
#include "ClingCompileSetting.h"

#define LOCTEXT_NAMESPACE "FClingLoadModuleModule"

// nop function export
extern "C" void my_nop_impl() {
#if defined(_MSC_VER)
	__nop();
#elif defined(__GNUC__)
	__asm__ __volatile__("nop");
#endif
}
#pragma comment(linker, "/EXPORT:__nop=my_nop_impl")
#pragma comment(linker, "/EXPORT:??2@YAPEAX_K@Z")
#pragma comment(linker, "/EXPORT:??3@YAXPEAX@Z")
#pragma comment(linker, "/EXPORT:??_U@YAPEAX_K@Z")
#pragma comment(linker, "/EXPORT:??_V@YAXPEAX@Z")
#pragma comment(linker, "/EXPORT:??3@YAXPEAX_K@Z")

#if defined(_MSC_VER)
#if USE_THREADSAFE_STATICS
//
// #pragma comment(lib, "vcruntime.lib")
// #pragma comment(lib, "msvcrt.lib")
//
#pragma comment(linker, "/EXPORT:_Init_thread_abort")
#pragma comment(linker, "/EXPORT:_Init_thread_epoch")
#pragma comment(linker, "/EXPORT:_Init_thread_footer")
#pragma comment(linker, "/EXPORT:_Init_thread_header")
#endif
#pragma comment(linker, "/EXPORT:_tls_index")
#pragma comment(linker, "/EXPORT:__tlregdtor")
#endif

FString GetClingBinariesPath()
{
	FString BaseDir = IPluginManager::Get().FindPlugin("UECling")->GetBaseDir();
#if PLATFORM_WINDOWS
#if PLATFORM_CPU_ARM_FAMILY
	const TCHAR* BinariesArch = TEXT("arm64");
#else
	const TCHAR* BinariesArch = TEXT("x64");
#endif
	return FPaths::Combine(BaseDir, TEXT("Source"), TEXT("ThirdParty"), TEXT("ClingLibrary"),TEXT("LLVM"), TEXT("bin"),
#if USING_CPPINTEROP_DEBUG
		TEXT("Debug"));
#else
		TEXT("RelWithDebInfo"));
#endif 
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