// Fill out your copyright notice in the Description page of Project Settings.


#include "ClingBlueprintFunctionLibrary.h"

#include "cling-demo.h"
#include "UCling.h"

bool UClingBlueprintFunctionLibrary::RunCppScript(
	const FString& Includes,
	const FString& CppScript,
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
		P_GET_TARRAY_REF(FString, BPInputs);
		P_GET_TARRAY_REF(FString, BPOutputs);

		// TEXT("namespace Local")+FGuid::NewGuid().ToString()+
		FString NameSpaceAvoidReDefine = TEXT("{\n");
		FString AddressInjectCode;
		auto ExportParam = [&AddressInjectCode, &Stack](const FString& ParamName){
			FString CppType = Stack.MostRecentProperty->GetCPPType(&CppType);
			if(auto StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty))
			{
				if(!StructProperty->Struct->IsNative())
				{
					// Todo Generate define of this struct
				}
			}
			AddressInjectCode += FString::Printf(TEXT("%s& %s = *(%s*)%I64d;\n"),*CppType,*ParamName,*CppType,size_t(Stack.MostRecentPropertyAddress));
		};
		// Read the input values and write them to the Python context
		bool bHasValidInputValues = true;
		{
			for (const FString& Input : BPInputs)
			{
				// Note: Medium term the Blueprint interpreter will change to provide us with a list of properties and 
				// instance pointers, rather than forcing us to jump in and out of its interpreter loop (via StepCompiledIn)
				Stack.StepCompiledIn<FProperty>(nullptr);
				check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);
				ExportParam(Input);
			}
		}

		struct FClingOutputParam
		{
			FProperty* Property = nullptr;
			uint8* PropAddr = nullptr;
			const TCHAR* OutputName = nullptr;			
		};

		// Read the output values and store them to write to later from the Python context
		TArray<FClingOutputParam, TInlineAllocator<4>> OutParms;
		for (const FString& Output : BPOutputs)
		{
			// Note: Medium term the Blueprint interpreter will change to provide us with a list of properties and 
			// instance pointers, rather than forcing us to jump in and out of its interpreter loop (via StepCompiledIn)
			Stack.StepCompiledIn<FProperty>(nullptr);
			check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

			FClingOutputParam& OutParam = OutParms.AddDefaulted_GetRef();
			OutParam.Property = Stack.MostRecentProperty;
			OutParam.PropAddr = Stack.MostRecentPropertyAddress;
			OutParam.OutputName = *Output;
			ExportParam(Output);
		}
		NameSpaceAvoidReDefine += AddressInjectCode + CppScript + TEXT("};");
		
		auto& Module = FModuleManager::Get().GetModuleChecked<FUClingModule>(TEXT("UCling"));
		::Decalre(Module.Interp,StringCast<ANSICHAR>(*Includes).Get(),nullptr);
		::Process(Module.Interp,StringCast<ANSICHAR>(*NameSpaceAvoidReDefine).Get(),nullptr);
		P_FINISH;
		return true;
	};

	// We still need to call this function to step the bytecode correctly...
	ExecuteCustomPythonScriptImpl();
	*(bool*)RESULT_PARAM = true;
}
