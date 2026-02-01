// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ClingSetting.generated.h"

/**
 * PCH Configuration Profile
 * Defines a precompiled header configuration with its own header file and include list
 */
USTRUCT(BlueprintType)
struct FClingPCHProfile
{
	GENERATED_BODY()
	
	/** Profile name */
	UPROPERTY(EditAnywhere, Config, Category = "PCH")
	FName ProfileName;
	
	// Todo create a menu from module info to select file
	/** Additional include files to be added to this PCH */
	UPROPERTY(EditAnywhere, Config, Category = "PCH")
	TArray<FString> AdditionalIncludeFiles;

	/** Custom runtime arguments for interpreters using this PCH */
	UPROPERTY(EditAnywhere, Config, Category = "PCH")
	TArray<FString> RuntimeArgs;
	
	// temporary for holding char* for args
	TArray<FAnsiString> RuntimeArgsForConvert; 

	/** Whether this profile is enabled */
	UPROPERTY(EditAnywhere, Config, Category = "PCH")
	bool bEnabled = true;

	/** Get the PCH header file path for this profile */
	FString GetPCHHeaderPath() const;

	/** Get the PCH binary file path for this profile */
	FString GetPCHBinaryPath() const;

	/** Get the RSP file path for this profile */
	FString GetRspFilePath() const;
};

/**
 * 
 */
USTRUCT()
struct FRuntimeDependencies
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere)
	FString Path;
	UPROPERTY(VisibleAnywhere)
	FString Type;
};

USTRUCT(BlueprintType)
struct FModuleCompileInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FName Name;
	UPROPERTY(VisibleAnywhere)
	FString Type;
	UPROPERTY(VisibleAnywhere)
	FString Directory;
	UPROPERTY(VisibleAnywhere)
	FString Rules;
	UPROPERTY(VisibleAnywhere)
	FName PCHUsage;
	UPROPERTY(VisibleAnywhere)
	bool ChainSharedPCH{0};
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PublicDependencyModules;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PublicIncludePathModules;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PrivateDependencyModules;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PrivateIncludePathModules;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> DynamicallyLoadedModules;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PublicSystemIncludePaths;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PublicIncludePaths;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> InternalIncludePaths;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PrivateIncludePaths;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PublicLibraries;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PublicSystemLibraries;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PublicSystemLibraryPaths;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PublicFrameworks;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PublicWeakFrameworks;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PublicDelayLoadDLLs;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> PublicDefinitions;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> CircularlyReferencedModules;
	UPROPERTY(VisibleAnywhere)
	TArray<FRuntimeDependencies> RuntimeDependencies;
};

UCLASS(Config=Cling, DefaultConfig)
class CLINGRUNTIME_API UClingSetting : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UClingSetting();
	virtual FName GetCategoryName() const override;
	virtual FName GetContainerName() const override;
	virtual FName GetSectionName() const override;

	UPROPERTY(EditAnywhere,Config)
	FDirectoryPath PathForLambdaScriptCompile;
	UPROPERTY(EditAnywhere,Config)
	FDirectoryPath PathForFunctionLibrarySync;

	UPROPERTY(EditAnywhere,Config)
	bool bVerbose{true};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Wno-inconsistent-missing-override")
	bool bIgnoreMissingOverride{true};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Wno-invalid-offsetof")
	bool bIgnoreInvalidOffsetOf{true};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Wno-deprecated-enum-enum-conversion")
	bool bIgnoreDeprecatedEnumEnumConversion{true};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Wno-switch")
	bool bIgnoreInCompleteSwitch{true};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Wno-tautological-undefined-compare")
	bool bIgnoreAutoLogicalUndefinedCompare{true};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Wno-gnu-string-literal-operator-template")
	bool bIgnoreStringLiteralOperatorTemplate{true};
	
	UPROPERTY(EditAnywhere,Config,DisplayName="Allow Redefine")
	bool bAllowRedefine{true};
	
	UPROPERTY(VisibleAnywhere)
	TMap<FName,FModuleCompileInfo> ModuleBuildInfos;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> GeneratedHeaderIncludePaths;

	/** Default PCH profile (not in array) */
	UPROPERTY(EditAnywhere, Config, Category = "PCH")
	FClingPCHProfile DefaultPCHProfile;

	/** Additional PCH Profiles */
	UPROPERTY(EditAnywhere, Config, Category = "PCH")
	TArray<FClingPCHProfile> PCHProfiles;

	/** Find profile by name (searches both default and array), always returns valid profile */
	const FClingPCHProfile& GetProfile(FName ProfileName) const;

	static FString GetGlobalBuildDefinsPath();
	void RefreshIncludePaths();
	
	void AppendCompileArgs(TArray<FString>& InOutCompileArgs);
	void AppendCompileArgs(TArray<const char*>& InOutCompileArgs);
	
	void AppendRuntimeArgs(FName ProfileName, TArray<FString>& InOutRuntimeArgs);
	void AppendRuntimeArgs(FName ProfileName, TArray<const char*>& Argv);
	
	UFUNCTION(BlueprintCallable,CallInEditor)
	void RefreshModuleIncludePaths();
	UFUNCTION(BlueprintCallable,CallInEditor)
	void RefreshGeneratedHeaderIncludePaths();

	/** Generate RSP file for a specific PCH profile */
	UFUNCTION(BlueprintCallable,CallInEditor)
	void GenerateRspFile();
	UFUNCTION(BlueprintCallable,CallInEditor, meta=(DisplayName="Generate RSP File (Profile)"))
	void GenerateRspFileForProfile(FName ProfileName);

	/** Generate PCH header file for a specific profile */
	UFUNCTION(BlueprintCallable, CallInEditor)
	void GeneratePCHHeaderFile(bool bForce = false);
	UFUNCTION(BlueprintCallable,CallInEditor, meta=(DisplayName="Generate PCH Header File (Profile)"))
	void GeneratePCHHeaderFileForProfile(FName ProfileName, bool bForce = false);

	/** Updates the generated BuildGlobalDefines.h file from engine binaries to the plugin source directory
	 * if not exist or bForce = true*/
	UFUNCTION(BlueprintCallable, CallInEditor)
	void UpdateBuildGlobalDefinesFile(bool bForce = false);

	/** Generate PCH binary for a specific profile */
	UFUNCTION(BlueprintCallable, CallInEditor)
	void GeneratePCH(bool bForce = false);
	UFUNCTION(BlueprintCallable, CallInEditor, meta=(DisplayName="Generate PCH (Profile)"))
	void GeneratePCHForProfile(FName ProfileName, bool bForce = false);

	/** Generate all enabled PCH profiles */
	UFUNCTION(BlueprintCallable)
	void GenerateAllPCHProfiles(bool bForce = false);
	UFUNCTION(CallInEditor)
	void ReGenerateAllPCHs(){GenerateAllPCHProfiles(true);}
	
	void IterThroughIncludePaths(TFunction<void(const FString&)> InFunc);
};
