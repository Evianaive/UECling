// Fill out your copyright notice in the Description page of Project Settings.


#include "ClingBlueprintFunctionLibrary.h"

#include "cling-demo.h"
#include "UCling.h"

bool UClingBlueprintFunctionLibrary::RunCppScript(
	const FString& Includes,
	const FString& CppScript,
	UPARAM(ref)int64& FunctionPtr,
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
		P_GET_PROPERTY_REF(FStrProperty, Includes);
		P_GET_PROPERTY_REF(FStrProperty, CppScript);
		P_GET_PROPERTY_REF(FInt64Property, FunctionPtr);
		P_GET_TARRAY_REF(FString, BPInputs);
		P_GET_TARRAY_REF(FString, BPOutputs);
		
		struct FClingOutputParam
		{
			FProperty* Property = nullptr;
			uint8* PropAddr = nullptr;
			const TCHAR* OutputName = nullptr;			
		};

		auto& Module = FModuleManager::Get().GetModuleChecked<FUClingModule>(TEXT("UCling"));
		TArray<int64> Args;
		if(FunctionPtr==0)
		{
			::Decalre(Module.Interp,StringCast<ANSICHAR>(*Includes).Get(),nullptr);
			FString LambdaName = TEXT("Lambda") + FGuid::NewGuid().ToString();
			FString NameSpaceAvoidReDefine = TEXT("auto ")+LambdaName+TEXT(" = [](signed long long* Args){\n");
			NameSpaceAvoidReDefine+= TEXT("SCOPED_NAMED_EVENT(CppPart, FColor::Red);\n");
			FString AddressInjectCode;
			int DynamicParamCount = 0;
			auto ExportParam = [&AddressInjectCode, &Stack](const FString& ParamName, int32 ParamIndex){
				FString CppType = Stack.MostRecentProperty->GetCPPType(&CppType);
				if(auto StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty))
				{
					if(!StructProperty->Struct->IsNative())
					{
						// Todo Generate define of this struct
					}
				}
				AddressInjectCode += FString::Printf(TEXT("%s& %s = *(%s*)Args[%i];\n"),*CppType,*ParamName,*CppType,ParamIndex);
			};
			// Read the input values and write them to the Python context
			bool bHasValidInputValues = true;
			
			for (const FString& Input : BPInputs)
			{
				// Note: Medium term the Blueprint interpreter will change to provide us with a list of properties and 
				// instance pointers, rather than forcing us to jump in and out of its interpreter loop (via StepCompiledIn)
				Stack.StepCompiledIn<FProperty>(nullptr);
				check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);
				Args.Add(reinterpret_cast<int64>(Stack.MostRecentPropertyAddress));
				ExportParam(Input,DynamicParamCount++);
			}
			for (const FString& Output : BPOutputs)
			{
				// Note: Medium term the Blueprint interpreter will change to provide us with a list of properties and 
				// instance pointers, rather than forcing us to jump in and out of its interpreter loop (via StepCompiledIn)
				Stack.StepCompiledIn<FProperty>(nullptr);
				check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);
				Args.Add(reinterpret_cast<int64>(Stack.MostRecentPropertyAddress));
				ExportParam(Output,DynamicParamCount++);
			}
			NameSpaceAvoidReDefine += AddressInjectCode + CppScript + TEXT("};\n{\n");
			NameSpaceAvoidReDefine += FString::Printf(TEXT("signed long long& StorePtr = *(signed long long*)%I64d;\n"),size_t(&FunctionPtr));
			NameSpaceAvoidReDefine += FString::Printf(TEXT("StorePtr = reinterpret_cast<int64>((void(*)(signed long long*))(%s));\n}"),*LambdaName);
			::Process(Module.Interp,StringCast<ANSICHAR>(*NameSpaceAvoidReDefine).Get(),nullptr);
		}
		else
		{
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
		}

		void(*Function)(int64*) = reinterpret_cast<void(*)(int64*)>(FunctionPtr);
		if(Function)
		{
			Function(Args.GetData());
		}
		
		P_FINISH;
		return true;
	};

	// We still need to call this function to step the bytecode correctly...
	ExecuteCustomPythonScriptImpl();
	*(bool*)RESULT_PARAM = true;
}
