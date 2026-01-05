// Fill out your copyright notice in the Description page of Project Settings.


#include "ClingSetting.h"

#include <CppInterOp/CppInterOp.h>

#include "JsonObjectConverter.h"
#include "ClingLog/ClingLog.h"
#include "HAL/FileManagerGeneric.h"
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

FString UClingSetting::GetPCHSourceFilePath()
{
	return GetPluginDir()/TEXT("ClingPCH.h");
}

FString UClingSetting::GetRspSavePath()
{
	return GetPluginDir()/TEXT("ClingIncludePaths.rsp");
}

FString UClingSetting::GetGlobalBuildDefinsPath()
{
	return GetPluginDir()/TEXT("Source")/TEXT("BuildGlobalDefines.h");
}

void UClingSetting::RefreshIncludePaths()
{
	SCOPED_NAMED_EVENT(RefreshIncludePaths, FColor::Red)
	UE_LOG(LogTemp,Log,TEXT("Begin Refresh IncludePaths"))
	RefreshModuleIncludePaths();
	RefreshGeneratedHeaderIncludePaths();
}

template<typename T>
void AppendCompileArgs(T& InOutCompileArgs)
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
		// "-O1",				
		// "-Xclang",
		// "-detailed-preprocessing-record",
		// "-fno-delayed-template-parsing",
		// "-fno-inline",
		"-gcodeview",
		"-g2",
		"-resource-dir"
	};
	static FString ResourceDir = GetPluginDir()/TEXT("Source/ThirdParty/ClingLibrary/LLVM/lib/clang/20");
	if constexpr (TIsTArray<typename TDecay<T>::Type>::Value)
	{
		// For TArray<FString>
		for (const char* Arg : CommonArgs)
		{
			InOutCompileArgs.Add(Arg);
		}
		InOutCompileArgs.Add(ResourceDir);
	}
	else
	{
		for (const char* Arg : CommonArgs)
		{
			// For std::vector<const char*> or similar container
			InOutCompileArgs.emplace_back(Arg);
		}
		auto CastString = StringCast<ANSICHAR>(*ResourceDir);
		static char StaticStored[256]{};
		FMemory::Memcpy(StaticStored,CastString.Get(),CastString.Length());
		InOutCompileArgs.emplace_back(StaticStored);
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

void UClingSetting::AppendRuntimeArgs(TArray<FString>& InOutRuntimeArgs)
{
	InOutRuntimeArgs.Append(RuntimeArgs);
}

void UClingSetting::AppendRuntimeArgs(std::vector<const char*>& Argv)
{
	RuntimeArgsForConvert.SetNum(RuntimeArgs.Num());
	for (int32 i = 0; i < RuntimeArgs.Num(); i++)
	{
		RuntimeArgsForConvert[i] = std::string(StringCast<ANSICHAR>(*RuntimeArgs[i]).Get());
		Argv.emplace_back(RuntimeArgsForConvert[i].c_str());
	}	
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
	IterThroughIncludePaths(AddInclude);
	
	AppendCompileArgs(RspLines);
	// align with clang-repl, this settings are add automatically by clang-repl
	RspLines.Add(TEXT("-Xclang"));
	RspLines.Add(TEXT("-fincremental-extensions"));
	
	// other debug settings
	// RspLines.Add(TEXT("-Xclang"));
	// RspLines.Add(TEXT("-femit-all-decls"));
	// RspLines.Add(TEXT("-Xclang"));
	// RspLines.Add(TEXT("-fkeep-static-consts"));
	// RspLines.Add(TEXT("-Xclang"));
	// RspLines.Add(TEXT("-detailed-preprocessing-record"));
	// RspLines.Add(TEXT("-H"));
	RspLines.Add(TEXT("-v"));

	// output pch
	RspLines.Add(TEXT("-o ") + GetPCHSourceFilePath() + TEXT(".pch"));
	RspLines.Add(GetPCHSourceFilePath());	
	
	FFileHelper::SaveStringArrayToFile(RspLines, *GetRspSavePath());
}

void UClingSetting::GeneratePCHHeaderFile(bool bForce)
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
		PCHLines.Add(MacroDefine + TEXT(" __declspec(dllimport)"));
		// PCHLines.Add(MacroDefine);
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
	for (auto& GeneratedIncludePath : PCHAdditionalIncludeFiles)
	{
		PCHLines.Add(TEXT("#include \""+GeneratedIncludePath+TEXT("\"")));
	}

		
	using FHashBuffer = TTuple<uint32,uint32,uint32,uint32>;
	auto HashFString = [](const FString& InString) -> FHashBuffer
	{
		FMD5 NewFileHash;
		FHashBuffer HashBuffer = {0,0,0,0};
		NewFileHash.Update(reinterpret_cast<const uint8*>(GetData(InString)), InString.Len() * sizeof(TCHAR));
		NewFileHash.Final(reinterpret_cast<uint8*>(&HashBuffer));
		return HashBuffer;
	};
	FString NewPCHContent = FString::Join(PCHLines, TEXT("\n")) + TEXT("\n");
	// Check current file status
	FString ExistingContent;
	FString PCHPath = GetPCHSourceFilePath();
	if (FFileHelper::LoadFileToString(ExistingContent, *PCHPath))
	{
		auto NewMD5 = HashFString(NewPCHContent);
		auto ExistingMD5 = HashFString(ExistingContent);
		
		// compare md5
		if (NewMD5 != ExistingMD5)
		{
			FFileHelper::SaveStringToFile(NewPCHContent, *PCHPath);
			UE_LOG(LogCling, Log, TEXT("PCH header file content needs to be updated (MD5 changed form: %i-%i-%i-%i to %i-%i-%i-%i )"),
				ExistingMD5.Get<0>(),ExistingMD5.Get<1>(),ExistingMD5.Get<2>(),ExistingMD5.Get<3>(),
				NewMD5.Get<0>(),NewMD5.Get<1>(),NewMD5.Get<2>(),NewMD5.Get<3>())
			;
		}
		else
		{
			UE_LOG(LogCling, Log, TEXT("PCH header file unchanged, skipping save (MD5: %i-%i-%i-%i)"),
				ExistingMD5.Get<0>(),ExistingMD5.Get<1>(),ExistingMD5.Get<2>(),ExistingMD5.Get<3>());
			// Todo should we ask user whether force to generate pch header file?
		}
	}
	else
	{
		FFileHelper::SaveStringToFile(NewPCHContent, *PCHPath);
		UE_LOG(LogCling, Log, TEXT("PCH header file created"));
	}
}

void UClingSetting::UpdateBuildGlobalDefinesFile(bool bForce)
{
#if 0
	// Include file of all global definitions of build context export by UnrealBuildTool
	FString UE_Exec = FPlatformProcess::ExecutablePath();
	UE_Exec.ReplaceCharInline('\\','/');
	FString GlobalDefinesFilePath = UE_Exec/TEXT("../UECling/BuildGlobalDefines.h");
	PCHLines.Add(TEXT("#include \"")+GlobalDefinesFilePath+TEXT("\""));
#else
	// global defines
	FString EngineDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
	FString FileSource = EngineDir/TEXT("Binaries/Win64/UECling/BuildGlobalDefines.h");
#endif
	FString CopyDest = GetGlobalBuildDefinsPath();
	auto& FM = FFileManagerGeneric::Get();
	if (!FM.FileExists(*FileSource))
	{
		UE_LOG(LogCling, Warning, TEXT("BuildGlobalDefines.h source file does not exist at: %s"), *FileSource);
		return;
	}
	if (FM.FileExists(*CopyDest) && !bForce)
	{
		UE_LOG(LogCling, Verbose, TEXT("BuildGlobalDefines.h already exists at destination: %s, skipping copy"), *CopyDest);
		return;
	}
	FM.Copy(*CopyDest,*FileSource,true,true);
	UE_LOG(LogCling, Log, TEXT("BuildGlobalDefines.h copy to destination: %s"), *CopyDest);
}

void UClingSetting::GeneratePCH(bool bForce)
{
	SCOPED_NAMED_EVENT(GeneratePCH, FColor::Red)
	UpdateBuildGlobalDefinesFile(false);
	GeneratePCHHeaderFile(false);
	GenerateRspFile();
	auto& FM = FFileManagerGeneric::Get();
	auto PCHSourceFileTime = FM.GetTimeStamp(*GetPCHSourceFilePath());
	auto BuildGlobalDefinesTime = FM.GetTimeStamp(*GetGlobalBuildDefinsPath());
	auto PCHTime = FM.GetTimeStamp(*(GetPCHSourceFilePath()+TEXT(".pch")));
	if (bForce)
	{
		UE_LOG(LogCling, Log, TEXT("Force generate ClingPCH.h.pch file, regenerating PCH"));
		Cpp::CreatePCH(StringCast<ANSICHAR>(*(TEXT("@")+GetRspSavePath())).Get());
	}
	else if (BuildGlobalDefinesTime > PCHTime || PCHSourceFileTime > PCHTime)
	{
		UE_LOG(LogCling, Log, TEXT("BuildGlobalDefines.h or ClingPCH.h is newer than ClingPCH.h.pch, regenerating PCH"));
		Cpp::CreatePCH(StringCast<ANSICHAR>(*(TEXT("@")+GetRspSavePath())).Get());
	}
	else
	{
		UE_LOG(LogCling, Log, TEXT("BuildGlobalDefines.h or ClingPCH.h is older than ClingPCH.h.pch, skip generate PCH"));
	}
}

void UClingSetting::IterThroughIncludePaths(TFunction<void(const FString&)> InFunc)
{	
	InFunc(FPaths::ConvertRelativePathToFull(FPaths::EngineSourceDir()));
	for (auto& ModuleBuildInfo : ModuleBuildInfos)
	{
		// Begin IncludePaths
		for (const auto& PublicIncludePath : ModuleBuildInfo.Value.PublicIncludePaths)
		{
			InFunc(PublicIncludePath);
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
			InFunc(PublicSystemIncludePath);
		}
		if(!ModuleBuildInfo.Value.Directory.Contains(TEXT("Engine/Source")))
			InFunc(ModuleBuildInfo.Value.Directory);
	}
	for (const auto& GeneratedHeaderIncludePath : GeneratedHeaderIncludePaths)
	{
		InFunc(GeneratedHeaderIncludePath);
	}
}
