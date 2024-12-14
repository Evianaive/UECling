// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ClingSetting.generated.h"

/**
 * 
 */

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
	bool ChainSharedPCH;
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
	TArray<FString> RuntimeDependencies;
};

UCLASS(Config=Cling)
class UCLING_API UClingSetting : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UClingSetting();
	virtual FName GetCategoryName() const override;
	virtual FName GetContainerName() const override;
	virtual FName GetSectionName() const override;

	UPROPERTY(EditAnywhere,Config)
	bool bVerbose{true};
	UPROPERTY(EditAnywhere)
	TMap<FName,FModuleCompileInfo> ModuleBuildInfos;
	UPROPERTY(VisibleAnywhere)
	TArray<FString> GeneratedHeaderIncludePaths;

	UFUNCTION(BlueprintCallable,CallInEditor)
	void RefreshIncludePaths();
	UFUNCTION(BlueprintCallable,CallInEditor)
	void RefreshModuleIncludePaths();
	UFUNCTION(BlueprintCallable,CallInEditor)
	void RefreshGeneratedHeaderIncludePaths();
};
