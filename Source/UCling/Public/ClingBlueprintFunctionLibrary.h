// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ClingBlueprintFunctionLibrary.generated.h"


/**
 * 
 */
UCLASS()
class UCLING_API UClingBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable,CustomThunk,Category = "Cling|Execution",meta=(Variadic, BlueprintInternalUseOnly="true"))
	static bool RunCppScript(
		UPARAM(meta=(MultiLine=True)) const FString& Includes,
		UPARAM(meta=(MultiLine=True)) const FString& CppScript,
		const TArray<FString>& CppInputs,
		const TArray<FString>& CppOutputs);
	DECLARE_FUNCTION(execRunCppScript);
};
