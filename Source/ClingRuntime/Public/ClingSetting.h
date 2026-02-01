// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ClingSetting.generated.h"

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

	static FString GetPCHSourceFilePath();
	static FString GetRspSavePath();
	static FString GetGlobalBuildDefinsPath();
	void RefreshIncludePaths();
	
	void AppendCompileArgs(TArray<FString>& InOutCompileArgs);
	void AppendCompileArgs(TArray<const char*>& InOutCompileArgs);
	
	UPROPERTY(EditAnywhere,Config)
	TArray<FString> PCHAdditionalIncludeFiles;
	UPROPERTY(EditAnywhere,Config)
	TArray<FString> RuntimeArgs;
	// temporary for holding char* for args
	TArray<FAnsiString> RuntimeArgsForConvert; 
	void AppendRuntimeArgs(TArray<FString>& InOutRuntimeArgs);
	void AppendRuntimeArgs(TArray<const char*>& Argv);
	
	UFUNCTION(BlueprintCallable,CallInEditor)
	void RefreshModuleIncludePaths();
	UFUNCTION(BlueprintCallable,CallInEditor)
	void RefreshGeneratedHeaderIncludePaths();
	UFUNCTION(BlueprintCallable,CallInEditor)
	void GenerateRspFile();
	UFUNCTION(BlueprintCallable,CallInEditor)
	void GeneratePCHHeaderFile(bool bForce = false);
	/** Updates the generated BuildGlobalDefines.h file from engine binaries to the plugin source directory
	 * if not exist or bForce = true*/
	UFUNCTION(BlueprintCallable,CallInEditor)
	void UpdateBuildGlobalDefinesFile(bool bForce = false);
	UFUNCTION(BlueprintCallable,CallInEditor)
	void GeneratePCH(bool bForce = false);
	
	void IterThroughIncludePaths(TFunction<void(const FString&)> InFunc);
};
