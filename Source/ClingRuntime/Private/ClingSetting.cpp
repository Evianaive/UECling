// Fill out your copyright notice in the Description page of Project Settings.


#include "ClingSetting.h"

#include <CppInterOp/CppInterOp.h>

#include "JsonObjectConverter.h"
#include "ClingLog/ClingLog.h"
#include "HAL/FileManagerGeneric.h"
#include "Interfaces/IPluginManager.h"
#include "ClingCompileSetting.h"

FString GetPluginDir()
{
	static FString PluginDir = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin(UE_PLUGIN_NAME)->GetBaseDir());
	return PluginDir;
}

FString FClingPCHProfile::GetPCHHeaderPath() const
{
	return GetPluginDir() / FString::Printf(TEXT("ClingPCH_%s.h"), *ProfileName.ToString());
}

FString FClingPCHProfile::GetPCHBinaryPath() const
{
	return GetPCHHeaderPath() + TEXT(".pch");
}

FString FClingPCHProfile::GetRspFilePath() const
{
	return GetPluginDir() / FString::Printf(TEXT("ClingIncludePaths_%s.rsp"), *ProfileName.ToString());
}

UClingSetting::UClingSetting()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("Cling");
	
	FString Path = GetPluginDir()/TEXT("Source/ClingScript");
	PathForLambdaScriptCompile.Path = Path/TEXT("Private/LambdaScript");
	PathForFunctionLibrarySync.Path = Path/TEXT("Private/FunctionLibrary");

	// Initialize default PCH profile
	DefaultPCHProfile.ProfileName = TEXT("Default");
	DefaultPCHProfile.bEnabled = true;
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

FString UClingSetting::GetGlobalBuildDefinsPath()
{
	return GetPluginDir()/TEXT("Source")/TEXT("BuildGlobalDefines.h");
}

const FClingPCHProfile& UClingSetting::GetProfile(FName ProfileName) const
{
	// Search in additional profiles
	for (const FClingPCHProfile& Profile : PCHProfiles)
	{
		if (Profile.ProfileName == ProfileName)
		{
			return Profile;
		}
	}
	
	// Fallback to default profile if not found
	return DefaultPCHProfile;
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
	// Common compile arguments that should be applied to both TArray<FString> and TArray<const char*>
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
		"-Wno-unused-value",
		
		"-fms-compatibility", "-fms-extensions",
#if !USE_THREADSAFE_STATICS
		"-fno-threadsafe-statics",
#endif
		// "-O1",				
		// "-Xclang",
		// "-detailed-preprocessing-record",
		// "-fno-delayed-template-parsing",
		// "-fno-inline",
		"-gcodeview",
		"-g2"
#if USE_RESOURCE_DIR
		,"-resource-dir"
#endif
	};
#if USE_RESOURCE_DIR
	constexpr bool IsFString = std::is_same_v<typename TDecay<T>::Type::ElementType, FString>;
	constexpr bool IsCharStar = std::is_same_v<typename TDecay<T>::Type::ElementType, const char*>;
	
	static FString ResourceDir = GetPluginDir()/TEXT("Source/ThirdParty/ClingLibrary/LLVM/lib/clang/20");
	static FAnsiString ResourceDirAnsi = StringCast<ANSICHAR>(*ResourceDir).Get();
#endif
	if constexpr (TIsTArray<typename TDecay<T>::Type>::Value)
	{
		for (const char* Arg : CommonArgs)
		{
			if constexpr (IsFString)
				InOutCompileArgs.Add(Arg);
			else if constexpr (IsCharStar)
				InOutCompileArgs.Add(Arg);
		}
#if USE_RESOURCE_DIR
		if constexpr (IsFString)
			InOutCompileArgs.Add(ResourceDir);
		else if constexpr (IsCharStar)
			InOutCompileArgs.Add(*ResourceDirAnsi);
#endif
	}	
	static_assert(TIsTArray<typename TDecay<T>::Type>::Value, "T must be TArray<FString> or TArray<const char*>");
}

void UClingSetting::AppendCompileArgs(TArray<FString>& InOutCompileArgs)
{
	::AppendCompileArgs(InOutCompileArgs);
}

void UClingSetting::AppendCompileArgs(TArray<const char*>& InOutCompileArgs)
{
	::AppendCompileArgs(InOutCompileArgs);
}

const char* RuntimeArgs[] = {"-v",
"-Winvalid-offsetof" ,
"-Winconsistent-missing-override", 
"-Wdeprecated-enum-enum-conversion", 
"-Wswitch",
"-Wtautological-undefined-compare", 
"-Wgnu-string-literal-operator-template", 
"-Wunused-value"};

#define CLING_ADD_RUNTIME_ARGS \
if(bVerbose)\
	InOutRuntimeArgs.Add(RuntimeArgs[0]);\
if(bEnableInvalidOffsetOf)\
	InOutRuntimeArgs.Add(RuntimeArgs[1]);\
if(bEnableMissingOverride)\
	InOutRuntimeArgs.Add(RuntimeArgs[2]);\
if(bEnableDeprecatedEnumEnumConversion)\
	InOutRuntimeArgs.Add(RuntimeArgs[3]);\
if(bEnableInCompleteSwitch)\
	InOutRuntimeArgs.Add(RuntimeArgs[4]);\
if(bEnableAutoLogicalUndefinedCompare)\
	InOutRuntimeArgs.Add(RuntimeArgs[5]);\
if(bEnableStringLiteralOperatorTemplate)\
	InOutRuntimeArgs.Add(RuntimeArgs[6]);\
if(bEnableUnusedValue)\
	InOutRuntimeArgs.Add(RuntimeArgs[7]);\

void UClingSetting::AppendRuntimeArgs(FName ProfileName, TArray<FString>& InOutRuntimeArgs)
{
	CLING_ADD_RUNTIME_ARGS;
	const FClingPCHProfile& Profile = GetProfile(ProfileName);
	InOutRuntimeArgs.Append(Profile.RuntimeArgs);
}

void UClingSetting::AppendRuntimeArgs(FName ProfileName, TArray<const char*>& InOutRuntimeArgs)
{
	CLING_ADD_RUNTIME_ARGS;
	FClingPCHProfile& Profile = const_cast<FClingPCHProfile&>(GetProfile(ProfileName));
	const TArray<FString>& TargetArgs = Profile.RuntimeArgs;
	Profile.RuntimeArgsForConvert.SetNum(TargetArgs.Num());
	for (int32 i = 0; i < TargetArgs.Num(); i++)
	{
		Profile.RuntimeArgsForConvert[i] = StringCast<ANSICHAR>(*TargetArgs[i]).Get();
		InOutRuntimeArgs.Add(*Profile.RuntimeArgsForConvert[i]);
	}
}
#undef CLING_ADD_RUNTIME_ARGS

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


// Common PCH content generation (shared across all profiles)
void GenerateCommonPCHContent(TArray<FString>& OutPCHLines, const TMap<FName, FModuleCompileInfo>& ModuleBuildInfos)
{
	OutPCHLines.Add(TEXT("#pragma once"));
	OutPCHLines.Add(TEXT("#include \"") + UClingSetting::GetGlobalBuildDefinsPath() + TEXT("\""));

	for (const auto& ModuleBuildInfo : ModuleBuildInfos)
	{
		// Begin Definitions
		for (FString Define : ModuleBuildInfo.Value.PublicDefinitions)
		{
			Define.ReplaceCharInline('=',' ');
			OutPCHLines.Add(TEXT("#define ") + Define);
		}
		FString MacroDefine = TEXT("#define ") + ModuleBuildInfo.Value.Name.ToString().ToUpper() + TEXT("_API ");
		OutPCHLines.Add(MacroDefine);
	}

	// fix msvc
#ifdef __clang__
#else
	OutPCHLines.Add(TEXT("#include \"Microsoft/MinimalWindowsApi.h\""));
	OutPCHLines.Add(TEXT("#include \"Runtime/Core/Private/Microsoft/MinimalWindowsApi.cpp\""));
#endif

	// default pch includes
	OutPCHLines.Add(TEXT("#include \"CoreMinimal.h\""));
	OutPCHLines.Add(TEXT("#include \"UObject/Object.h\""));
	OutPCHLines.Add(TEXT("#include \"Logging/LogMacros.h\""));
}

// Common RSP content generation (shared include paths)
void GenerateCommonRspContent(TArray<FString>& OutRspLines, const UClingSetting* Settings)
{
	OutRspLines.Add(TEXT("D:/clang.exe"));
	OutRspLines.Add(TEXT("-x c++-header"));

	// Add include paths
	auto AddInclude = [&OutRspLines](const FString& Include)
	{
		OutRspLines.Add(TEXT("-I \"") + Include.Replace(TEXT("\\"),TEXT("/")) + TEXT("\""));
	};
	const_cast<UClingSetting*>(Settings)->IterThroughIncludePaths(AddInclude);

	// Add compile arguments
	TArray<FString> CompileArgs;
	const_cast<UClingSetting*>(Settings)->AppendCompileArgs(CompileArgs);
	OutRspLines.Append(CompileArgs);

	// align with clang-repl
	OutRspLines.Add(TEXT("-Xclang"));
	OutRspLines.Add(TEXT("-fincremental-extensions"));
	OutRspLines.Add(TEXT("-v"));
}

void UClingSetting::GenerateRspFile()
{
	GenerateRspFileForProfile(TEXT("Default"));
}

void UClingSetting::GenerateRspFileForProfile(FName ProfileName)
{
	TArray<FString> RspLines;
	GenerateCommonRspContent(RspLines, this);

	// Get profile and generate paths
	const FClingPCHProfile& Profile = GetProfile(ProfileName);
	FString PCHPath = Profile.GetPCHHeaderPath();
	FString RspPath = Profile.GetRspFilePath();

	RspLines.Add(TEXT("-o ") + PCHPath + TEXT(".pch"));
	RspLines.Add(PCHPath);

	FFileHelper::SaveStringArrayToFile(RspLines, *RspPath);
}

void UClingSetting::GeneratePCHHeaderFile(bool bForce)
{
	GeneratePCHHeaderFileForProfile(TEXT("Default"), bForce);
}

void UClingSetting::GeneratePCHHeaderFileForProfile(FName ProfileName, bool bForce)
{
	TArray<FString> PCHLines;
	GenerateCommonPCHContent(PCHLines, ModuleBuildInfos);

	// Add profile-specific includes
	const FClingPCHProfile& Profile = GetProfile(ProfileName);
	for (const FString& IncludeFile : Profile.AdditionalIncludeFiles)
	{
		PCHLines.Add(TEXT("#include \"") + IncludeFile + TEXT("\""));
	}

	using FHashBuffer = TTuple<uint32,uint32,uint32,uint32>;
	auto HashFString = [](const FString& InString) -> FHashBuffer
	{
		FMD5 NewFileHash;
		FHashBuffer HashBuffer = FHashBuffer(0,0,0,0);
		NewFileHash.Update(reinterpret_cast<const uint8*>(GetData(InString)), InString.Len() * sizeof(TCHAR));
		NewFileHash.Final(reinterpret_cast<uint8*>(&HashBuffer));
		return HashBuffer;
	};

	FString NewPCHContent = FString::Join(PCHLines, TEXT("\n")) + TEXT("\n");
	FString PCHPath = Profile.GetPCHHeaderPath();

	// Check the current file status
	FString ExistingContent;
	if (FFileHelper::LoadFileToString(ExistingContent, *PCHPath))
	{
		auto NewMD5 = HashFString(NewPCHContent);
		auto ExistingMD5 = HashFString(ExistingContent);

		if (NewMD5 != ExistingMD5)
		{
			FFileHelper::SaveStringToFile(NewPCHContent, *PCHPath);
			UE_LOG(LogCling, Log, TEXT("PCH header file [%s] content updated (MD5 changed)"), *ProfileName.ToString());
		}
		else
		{
			UE_LOG(LogCling, Log, TEXT("PCH header file [%s] unchanged, skipping save"), *ProfileName.ToString());
		}
	}
	else
	{
		FFileHelper::SaveStringToFile(NewPCHContent, *PCHPath);
		UE_LOG(LogCling, Log, TEXT("PCH header file [%s] created"), *ProfileName.ToString());
	}
}

void UClingSetting::UpdateBuildGlobalDefinesFile(bool bForce)
{
	// global defines
	FString EngineDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
	FString FileSource = EngineDir/TEXT("Binaries/Win64/UECling/BuildGlobalDefines.h");

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
	GeneratePCHForProfile(TEXT("Default"), bForce);
}

void UClingSetting::GeneratePCHForProfile(FName ProfileName, bool bForce)
{
	SCOPED_NAMED_EVENT(GeneratePCH, FColor::Red)
	UpdateBuildGlobalDefinesFile(false);
	GeneratePCHHeaderFileForProfile(ProfileName, false);
	GenerateRspFileForProfile(ProfileName);

	const FClingPCHProfile& Profile = GetProfile(ProfileName);
	auto& FM = FFileManagerGeneric::Get();
	FString PCHSourcePath = Profile.GetPCHHeaderPath();
	FString PCHBinaryPath = Profile.GetPCHBinaryPath();
	FString RspPath = Profile.GetRspFilePath();

	auto PCHSourceFileTime = FM.GetTimeStamp(*PCHSourcePath);
	auto BuildGlobalDefinesTime = FM.GetTimeStamp(*GetGlobalBuildDefinsPath());
	auto PCHTime = FM.GetTimeStamp(*PCHBinaryPath);

	if (bForce)
	{
		UE_LOG(LogCling, Log, TEXT("Force generate PCH [%s], regenerating"), *ProfileName.ToString());
		Cpp::CreatePCH(StringCast<ANSICHAR>(*(TEXT("@") + RspPath)).Get());
	}
	else if (BuildGlobalDefinesTime > PCHTime || PCHSourceFileTime > PCHTime)
	{
		UE_LOG(LogCling, Log, TEXT("PCH [%s] source files newer than binary, regenerating"), *ProfileName.ToString());
		Cpp::CreatePCH(StringCast<ANSICHAR>(*(TEXT("@") + RspPath)).Get());
	}
	else
	{
		UE_LOG(LogCling, Log, TEXT("PCH [%s] is up-to-date, skip generation"), *ProfileName.ToString());
	}
}

void UClingSetting::GenerateAllPCHProfiles(bool bForce)
{
	SCOPED_NAMED_EVENT(GenerateAllPCHProfiles, FColor::Orange)
	// Generate default profile
	if (DefaultPCHProfile.bEnabled)
	{
		UE_LOG(LogCling, Log, TEXT("Generating default PCH profile"));
		GeneratePCHForProfile(TEXT("Default"), bForce);
	}

	// Generate additional profiles
	for (const FClingPCHProfile& Profile : PCHProfiles)
	{
		if (Profile.bEnabled)
		{
			UE_LOG(LogCling, Log, TEXT("Generating PCH profile: %s"), *Profile.ProfileName.ToString());
			GeneratePCHForProfile(Profile.ProfileName, bForce);
		}
		else
		{
			UE_LOG(LogCling, Log, TEXT("Skipping disabled PCH profile: %s"), *Profile.ProfileName.ToString());
		}
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
		// We don't need to add Private Include Path! A module should not add the path to headers
		// that can be included by other modules in PrivateIncludePaths
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
