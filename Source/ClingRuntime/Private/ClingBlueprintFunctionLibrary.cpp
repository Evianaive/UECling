// Fill out your copyright notice in the Description page of Project Settings.


#include "ClingBlueprintFunctionLibrary.h"
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>3
#include "Blueprint/BlueprintExceptionInfo.h"
#endif

bool UClingBlueprintFunctionLibrary::RunCppScript(
	int32 ArgCount)
{
	check(0)
	return false;
}

DEFINE_FUNCTION(UClingBlueprintFunctionLibrary::execRunCppScript)
{
	P_GET_PROPERTY(FIntProperty,ArgCount);
	TArray<int64,TInlineAllocator<16>> Args;
	Args.SetNum(ArgCount);
	for (int i = 0;i<ArgCount;i++)
	{
		Stack.StepCompiledIn<FProperty>(nullptr);
		Args[i] = reinterpret_cast<int64>(Stack.MostRecentPropertyAddress);
	}		
	P_GET_PROPERTY_REF(FInt64Property, FunctionPtr);		
	if(UNLIKELY(FunctionPtr==0))
	{
		FBlueprintExceptionInfo ExceptionInfo(
		EBlueprintExceptionType::AbortExecution,
		NSLOCTEXT("RunCppScript", "RunCppScript_Function_Error", "Function Faild to Compile")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		*(bool*)RESULT_PARAM = false;
		return;
	}
	void(*Function)(int64*) = reinterpret_cast<void(*)(int64*)>(FunctionPtr);
	if(Function)
	{
		Function(Args.GetData());
	}		
	P_FINISH;
	*(bool*)RESULT_PARAM = true;
}
