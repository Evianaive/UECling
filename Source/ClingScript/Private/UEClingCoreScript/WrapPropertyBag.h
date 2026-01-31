// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
#include "WrapPropertyBag.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FWrapPropertyBag
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category="Cling")
	FInstancedPropertyBag PropertyBag;
};
