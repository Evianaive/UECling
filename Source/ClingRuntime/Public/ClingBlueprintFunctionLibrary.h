// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ClingBlueprintFunctionLibrary.generated.h"


/**
 * 
 */
UCLASS()
class CLINGRUNTIME_API UClingBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable,CustomThunk,Category = "Cling|Execution",meta=(Variadic, BlueprintInternalUseOnly="true"))
	static bool RunCppScript(
		int32 ArgCount);
	DECLARE_FUNCTION(execRunCppScript);

	UFUNCTION(BlueprintCallable, Category = "Cling|Test")
	static int32 CallClingTestFunction(UObject* Object, FName FunctionName);
};
