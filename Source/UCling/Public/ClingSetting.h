// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ClingSetting.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FModuleIncludePaths
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere)
	TArray<FString> AdditionIncludePaths;
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
	TMap<FName,FModuleIncludePaths> ModuleIncludePaths;

	UFUNCTION(BlueprintCallable,CallInEditor)
	void RefreshIncludePaths();	
};
