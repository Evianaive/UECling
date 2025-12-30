// Fill out your copyright notice in the Description page of Project Settings.


#include "ClingSetting.h"

#include <CppInterOp/CppInterOp.h>

#include "JsonObjectConverter.h"
#include "ClingRuntime.h"
#include "Interfaces/IPluginManager.h"

FString GetPluginDir()
{
	static FString PluginDir = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin(UE_PLUGIN_NAME)->GetBaseDir());
	return PluginDir;
}

UClingSetting::UClingSetting()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("Cling");
	
	FString Path = GetPluginDir()/TEXT("Source/ClingScript");
	PathForLambdaScriptCompile.Path = Path/TEXT("Private/LambdaScript");
	PathForFunctionLibrarySync.Path = Path/TEXT("Private/FunctionLibrary");
}

FName UClingSetting::GetCategoryName() const
{
	return Super::GetCategoryName();
}

FName UClingSetting::GetContainerName() const
{
	return Super::GetContainerName();
}

FName UClingSetting::GetSectionName() const
{
	return Super::GetSectionName();
}

FString UClingSetting::GetPCHPath()
{
	return GetPluginDir()/TEXT("ClingPCH.h");
}

FString UClingSetting::GetRspSavePath()
{
	return GetPluginDir()/TEXT("ClingIncludePaths.rsp");
}

void UClingSetting::RefreshIncludePaths()
{
	UE_LOG(LogTemp,Log,TEXT("Begin Refresh IncludePaths"))
	RefreshModuleIncludePaths();
	RefreshGeneratedHeaderIncludePaths();
}

template<typename T>
void AppendCompileArgs(T& InOUtCompileArgs)
{
	// Common compile arguments that should be applied to both TArray<FString> and std::vector<const char*>
	static const char* CommonArgs[] = {
		"-std=c++20",
		"-march=native", 
		"-Wno-invalid-constexpr",
		"-Wno-inconsistent-missing-override",
		"-Wno-invalid-offsetof",
		"-Wno-deprecated-enum-enum-conversion",
		"-Wno-switch",
		"-Wno-tautological-undefined-compare",
		"-Wno-gnu-string-literal-operator-template",
		"-gcodeview",
		"-g2"
	};
	
	for (const char* Arg : CommonArgs)
	{
		if constexpr (TIsTArray<typename TDecay<T>::Type>::Value)
		{
			// For TArray<FString>
			InOUtCompileArgs.Add(Arg);
		}
		else
		{
			// For std::vector<const char*> or similar container
			InOUtCompileArgs.emplace_back(Arg);
		}
	}
}

void UClingSetting::AppendCompileArgs(TArray<FString>& InOutCompileArgs)
{
	::AppendCompileArgs(InOutCompileArgs);
}

void UClingSetting::AppendCompileArgs(std::vector<const char*>& InOutCompileArgs)
{
	::AppendCompileArgs(InOutCompileArgs);
}

template<typename T>
void GetFileContent(const FString& FileName, T& Result)
{
	FString ModuleBuildInfoJson = IPluginManager::Get().FindPlugin("UECling")->GetBaseDir()/FileName;
	if constexpr (TIsTArray<typename TDecay<T>::Type>::Value)
	{
		FFileHelper::LoadFileToStringArray(Result,*ModuleBuildInfoJson);
	}
	else
	{
		FFileHelper::LoadFileToString(Result,*ModuleBuildInfoJson);	
	}
}

void UClingSetting::RefreshModuleIncludePaths()
{
	// auto& Module = FModuleManager::Get().GetModuleChecked<FClingRuntimeModule>(TEXT("ClingRuntime"));
	FString JsonString;
	::GetFileContent(TEXT("ModuleBuildInfos.json"),JsonString);
	// Parse the JSON string
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
	TSharedPtr<FJsonObject> JsonObject;    

	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		// Access the "Modules" object
		TSharedPtr<FJsonObject> ModulesObject = JsonObject->GetObjectField(TEXT("Modules"));
		if (ModulesObject && ModulesObject.IsValid())
		{
			// Access the "UnrealClingTestPack" object within "Modules"
			for (auto Value : ModulesObject->Values)
			{
				auto& Struct = ModuleBuildInfos.Add(FName(Value.Key));
				FJsonObjectConverter::JsonObjectToUStruct(Value.Value->AsObject().ToSharedRef(),&Struct);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find 'Modules' object in JSON!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON!"));
	}
}

void UClingSetting::RefreshGeneratedHeaderIncludePaths()
{	
	GeneratedHeaderIncludePaths.Reset(); 
	::GetFileContent(TEXT("GeneratedHeaderPaths.txt"),GeneratedHeaderIncludePaths);
}

void UClingSetting::GenerateRspFile()
{
	TArray<FString> RspLines={
		"D:/clang.exe",
		"-x c++-header",
	};
	
	auto AddInclude = [&](const FString& Include)
	{
		RspLines.Add(TEXT("-I \"") + Include.Replace(TEXT("\\"),TEXT("/")) + TEXT("\""));
	};
	AddInclude(FPaths::ConvertRelativePathToFull(FPaths::EngineSourceDir()));
	for (auto& ModuleBuildInfo : ModuleBuildInfos)
	{
		// Begin IncludePaths
		for (const auto& PublicIncludePath : ModuleBuildInfo.Value.PublicIncludePaths)
		{
			AddInclude(PublicIncludePath);
		}
		// We don't need to add Private Include Path! A module should not add path to headers
		// that can be included by other module in PrivateIncludePaths		
		// for (const auto& PrivateIncludePath : ModuleBuildInfo.Value.PrivateIncludePaths)
		// {
		// 	AddInclude(PrivateIncludePath);
		// }
		// Begin Public lSystem Include Paths
		for (const auto& PublicSystemIncludePath : ModuleBuildInfo.Value.PublicSystemIncludePaths)
		{
			AddInclude(PublicSystemIncludePath);
		}
		if(!ModuleBuildInfo.Value.Directory.Contains(TEXT("Engine/Source")))
			AddInclude(ModuleBuildInfo.Value.Directory);
	}
	for (const auto& GeneratedHeaderIncludePath : GeneratedHeaderIncludePaths)
	{
		AddInclude(GeneratedHeaderIncludePath);
	}
	
	const auto& PluginDir = GetPluginDir();
	RspLines.Add(TEXT("-resource-dir ")+PluginDir/TEXT("Source/ThirdParty/ClingLibrary/LLVM/lib/clang/20"));
	AppendCompileArgs(RspLines);
	RspLines.Add(TEXT("-Xclang"));
	RspLines.Add(TEXT("-fincremental-extensions"));
	RspLines.Add(TEXT("-v"));	
	RspLines.Add(TEXT("-o ") + GetPCHPath() + TEXT(".pch"));
	RspLines.Add(GetPCHPath());	
	
	FFileHelper::SaveStringArrayToFile(RspLines, *GetRspSavePath());
}

void UClingSetting::GeneratePCHHeaderFile()
{
	TArray<FString> PCHLines={"#pragma once"};
	for (auto& ModuleBuildInfo : ModuleBuildInfos)
	{
		// Begin Definitions			
		for (FString& Define : ModuleBuildInfo.Value.PublicDefinitions)
		{
			if (UNLIKELY(Define==TEXT("LAUNCH_API")))
				continue;
			Define.ReplaceCharInline('=',' ');
			PCHLines.Add(TEXT("#define ")+Define);
		}		
		// Ignore LAUNCH_API since it is defined in BuildGlobalDefines.h generated by us
		FString MacroDefine = TEXT("#define ") + ModuleBuildInfo.Value.Name.ToString().ToUpper() + TEXT("_API ");
		if (UNLIKELY(MacroDefine == "#define LAUNCH_API"))
			continue;
		PCHLines.Add(MacroDefine);
	}
// #ifdef _MSC_VER
// #define EVA_MACRO(_M) #_M
// #define STR_MACRO(_M) "#define " #_M " " EVA_MACRO(_M)	
// 	PCHLines.Add(TEXT(STR_MACRO(_MSC_VER)));
// 	PCHLines.Add(TEXT(STR_MACRO(_WIN64)));
// #undef EVA_MACRO
// #undef STR_MACRO
// #endif

#if 0
	// Include file of all global definitions of build context export by UnrealBuildTool
	FString UE_Exec = FPlatformProcess::ExecutablePath();
	UE_Exec.ReplaceCharInline('\\','/');
	FString GlobalDefinesFilePath = UE_Exec/TEXT("../UECling/BuildGlobalDefines.h");
	PCHLines.Add(TEXT("#include \"")+GlobalDefinesFilePath+TEXT("\""));
#else
	// global defines
	FString EngineDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
	PCHLines.Add(TEXT("#include \"")+EngineDir+TEXT("/Binaries/Win64/UECling/BuildGlobalDefines.h\""));
#endif
	
	// fix msvc
	PCHLines.Add(TEXT("#ifdef __clang__"));
	PCHLines.Add(TEXT("#else"));
	PCHLines.Add(TEXT("#include \"Microsoft/MinimalWindowsApi.h\""));
	PCHLines.Add(TEXT("#include \"Runtime/Core/Private/Microsoft/MinimalWindowsApi.cpp\""));
	PCHLines.Add(TEXT("#endif"));

	// default pch includes
	PCHLines.Add(TEXT("#include \"CoreMinimal.h\""));
	PCHLines.Add(TEXT("#include \"UObject/Object.h\""));
	PCHLines.Add(TEXT("#include \"Logging/LogMacros.h\""));
	// user defined pch includes
	for (auto& GeneratedIncludePath : GeneratedIncludePaths)
	{
		PCHLines.Add(TEXT("#include \""+GeneratedIncludePath.FilePath+TEXT("\"")));
	}
	// UE_PLUGIN_NAME
	FFileHelper::SaveStringArrayToFile(PCHLines, *GetPCHPath());
}

void UClingSetting::GeneratePCH()
{
	SCOPED_NAMED_EVENT(GenPCH, FColor::Red)
	GenerateRspFile();
	GeneratePCHHeaderFile();
	// Call the test function in the third party library that opens a message box
	Cpp::CreatePCH(StringCast<ANSICHAR>(*(TEXT("@")+GetRspSavePath())).Get());
}
