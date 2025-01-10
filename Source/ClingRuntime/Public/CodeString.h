// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CodeString.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct CLINGRUNTIME_API FCodeString
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere)
	FString Code;
};
