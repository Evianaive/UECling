// Fill out your copyright notice in the Description page of Project Settings.


#include "ClingBlueprintFunctionLibrary.h"
#include "Blueprint/BlueprintExceptionInfo.h"

bool UClingBlueprintFunctionLibrary::RunCppScript(
	const TArray<FString>& BPInputs,
	const TArray<FString>& BPOutputs)
{
	check(0)
	return false;
}

DEFINE_FUNCTION(UClingBlueprintFunctionLibrary::execRunCppScript)
{
	auto ExecuteCustomPythonScriptImpl = [&]() -> bool
	{
		const FString FunctionErrorName = Stack.Node->GetName();
		const FString FunctionErrorCtxt = Stack.Node->GetOutermost()->GetName();

		// Read the standard function arguments
		P_GET_TARRAY_REF(FString, BPInputs);
		P_GET_TARRAY_REF(FString, BPOutputs);
		
		struct FClingOutputParam
		{
			FProperty* Property = nullptr;
			uint8* PropAddr = nullptr;
			const TCHAR* OutputName = nullptr;			
		};

		TArray<int64> Args;
		for (const FString& Input : BPInputs)
		{
			Stack.StepCompiledIn<FProperty>(nullptr);
			Args.Add(reinterpret_cast<int64>(Stack.MostRecentPropertyAddress));
		}
		for (const FString& Output : BPOutputs)
		{
			Stack.StepCompiledIn<FProperty>(nullptr);
			Args.Add(reinterpret_cast<int64>(Stack.MostRecentPropertyAddress));
		}
		
		P_GET_PROPERTY_REF(FInt64Property, FunctionPtr);
		
		if(FunctionPtr!=0)
		{
			void(*Function)(int64*) = reinterpret_cast<void(*)(int64*)>(FunctionPtr);
			if(Function)
			{
				Function(Args.GetData());
			}
		}
		else
		{
			FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			NSLOCTEXT("RunCppScript", "RunCppScript_Function_Error", "Function Faild to Compile")
			);
			FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
			return false;
		}
		
		P_FINISH;
		return true;
	};

	// We still need to call this function to step the bytecode correctly...
	*(bool*)RESULT_PARAM = ExecuteCustomPythonScriptImpl();
}
