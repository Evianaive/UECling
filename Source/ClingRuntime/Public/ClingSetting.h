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

	UPROPERTY(EditAnywhere,Config,DisplayName="Verbose (Enable verbose output)")
	bool bVerbose{false};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Winvalid-offsetof (Enable warning when using offsetof macro on non-standard-layout (non-POD) types, which is undefined behavior per C++ standard)")
	bool bEnableInvalidOffsetOf{false};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Winconsistent-missing-override (Enable warning when some overrides of a virtual function use override keyword while others omit it in the same inheritance hierarchy)")
	bool bEnableMissingOverride{false};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Wdeprecated-enum-enum-conversion (Enable warning for deprecated arithmetic operations/comparisons between different enumeration types in C++20)")
	bool bEnableDeprecatedEnumEnumConversion{false};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Wswitch (Enable warning for incomplete switch statements)")
	bool bEnableInCompleteSwitch{false};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Wtautological-undefined-compare (Enable warning for comparisons that are always true/false due to undefined behavior)")
	bool bEnableAutoLogicalUndefinedCompare{false};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Wgnu-string-literal-operator-template (Enable warning for GNU extension: string literal operator templates)")
	bool bEnableStringLiteralOperatorTemplate{false};
	UPROPERTY(EditAnywhere,Config,DisplayName="-Wunused-value (Enable warning when a value is not used)")
	bool bEnableUnusedValue{false};
	
	// Todo Can't be use in clang-repl based
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
