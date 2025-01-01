// Fill out your copyright notice in the Description page of Project Settings.


#include "K2Node_ExecuteCppScript.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "CallFunctionHandler.h"
#include "cling-demo.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_MakeArray.h"
#include "KismetCompiler.h"
#include "UCling.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UCling/Public/ClingBlueprintFunctionLibrary.h"
//We have to recompile these files because they are not exported!
#include "Editor/BlueprintGraph/Private/CallFunctionHandler.cpp"
#include "Editor/BlueprintGraph/Private/PushModelHelpers.cpp"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_ExecuteCppScript)

#define LOCTEXT_NAMESPACE "K2Node_ExecuteCppScript"

namespace ExecuteCppScriptUtil
{
	// const FName CppInputsPinName = "CppInputs";
	// const FName CppOutputsPinName = "CppOutputs";
	const FName ArgCountPinName = "ArgCount";

	FString CppizePinName(const FName InPinName)
	{
		return InPinName.ToString();
	}

	FString CppizePinName(const UEdGraphPin* InPin)
	{
		return CppizePinName(InPin->PinName);
	}
}


struct FBlueprintCompiledStatement;

class FKCHandler_CompileAndStorePtr : public FKCHandler_CallFunction
{
public:
	FKCHandler_CompileAndStorePtr(FKismetCompilerContext& InCompilerContext)
		: FKCHandler_CallFunction(InCompilerContext)
	{
	}
	UK2Node_ExecuteCppScript* CurNode{nullptr};
	
	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		/* auto register other pin(literal) by default method*/
		TGuardValue GuardCurNode{CurNode,CastChecked<UK2Node_ExecuteCppScript>(Node)};
		FKCHandler_CallFunction::RegisterNets(Context, Node);
		
		CurNode->CreateTempFunctionPtrPin();
		auto PtrPin = CurNode->GetFunctionPtrPin();
		// int64 to store Function ptr, get PtrTerm
		// FKCHandler_CallFunction::RegisterNet(Context,PtrPin);
		RegisterLiteral(Context,PtrPin);
		FBPTerminal** PtrTermResult = Context.NetMap.Find(PtrPin);
		if(PtrTermResult==nullptr)
		{
			Context.MessageLog.Error(*LOCTEXT("Error_PtrPinNotFounded", "ICE: Ptr Pin is not created @@").ToString(), CurNode);
		}
	}

	// Generate cpp type name of property
	static FString GetCppTypeOfProperty(const FProperty* Property)
	{
		FString CppType = Property->GetCPPType();
		if(const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
		{
			CppType += TEXT("<")+GetCppTypeOfProperty(ArrayProp->Inner)+TEXT(">");
		}
		else if(const FSetProperty* SetProp = CastField<FSetProperty>(Property))
		{
			CppType += TEXT("<")+GetCppTypeOfProperty(SetProp->ElementProp)+TEXT(">");
		}
		else if(const FMapProperty* MapProp = CastField<FMapProperty>(Property))
		{
			CppType += TEXT("<")+GetCppTypeOfProperty(MapProp->KeyProp)+TEXT(",")+GetCppTypeOfProperty(MapProp->ValueProp)+TEXT(">");
		}
		return CppType;
	}

	// Generate define of this struct if it is a blueprint struct!
	static void GenerateStructCode(const UScriptStruct* Struct, FString& InOutDeclare)
	{
		if(IsValid(Struct) && !Struct->IsNative())
		{
			auto SuperStruct = Cast<UScriptStruct>(Struct->GetSuperStruct());
			GenerateStructCode(SuperStruct,InOutDeclare);
			for (const FProperty* ChildProperty : TFieldRange<FProperty>(Struct))
			{
				RecursivelyGenerateDeclareOfNonNativeProp(ChildProperty,InOutDeclare);
			}
			FString StructName = Struct->GetStructCPPName();
			if(SuperStruct)
				StructName +=TEXT(" : public ") + SuperStruct->GetStructCPPName();
			
			InOutDeclare += FString::Printf(TEXT("struct %s{\n"),*StructName);	
			
			for (const FProperty* ChildProperty : TFieldRange<FProperty>(Struct))
			{
				InOutDeclare += FString::Printf(TEXT("%s %s;\n"),*GetCppTypeOfProperty(ChildProperty),*ChildProperty->GetAuthoredName());
			}
			InOutDeclare += TEXT("};\n");
		}
	}

	// Find Struct in the property
	static void RecursivelyGenerateDeclareOfNonNativeProp(const FProperty* Property, FString& InOutDeclare)
	{
		if(auto StructProperty = CastField<FStructProperty>(Property))
		{
			GenerateStructCode(StructProperty->Struct,InOutDeclare);
		}
		else if(const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
		{
			RecursivelyGenerateDeclareOfNonNativeProp(ArrayProp->Inner,InOutDeclare);
		}
		else if(const FSetProperty* SetProp = CastField<FSetProperty>(Property))
		{
			RecursivelyGenerateDeclareOfNonNativeProp(SetProp->ElementProp,InOutDeclare);
		}
		else if(const FMapProperty* MapProp = CastField<FMapProperty>(Property))
		{
			RecursivelyGenerateDeclareOfNonNativeProp(MapProp->KeyProp,InOutDeclare);
			RecursivelyGenerateDeclareOfNonNativeProp(MapProp->ValueProp,InOutDeclare);			
		}
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		TGuardValue GuardCurNode{CurNode,CastChecked<UK2Node_ExecuteCppScript>(Node)};
		FKCHandler_CallFunction::Compile(Context, Node);
		auto PtrPin = CurNode->GetFunctionPtrPin();
		
		// Todo use function name property?
		FString LambdaName = TEXT("Lambda") + FGuid::NewGuid().ToString();
		FString NameSpaceAvoidReDefine = TEXT("auto ")+LambdaName+TEXT(" = [](signed long long* Args){\n");
		NameSpaceAvoidReDefine += FString::Printf(TEXT("\tSCOPED_NAMED_EVENT(%s, FColor::Red);\n\t//Begin recover Args\n"),*CurNode->FunctionName.ToString());
		
		FString AddressInjectCode;
		int DynamicParamCount = 0;
		auto ExportParam = [&AddressInjectCode](const FProperty* Property,const FString& ParamName, int32 ParamIndex){
			FString NecessaryPreDeclare;			
			RecursivelyGenerateDeclareOfNonNativeProp(Property,NecessaryPreDeclare);
			FString CppType = GetCppTypeOfProperty(Property);
			AddressInjectCode += NecessaryPreDeclare;
			AddressInjectCode += FString::Printf(TEXT("\t%s& %s = *(%s*)Args[%i];\n"),*CppType,*ParamName,*CppType,ParamIndex);
		};
		auto GetBPTerminal=[this, &Context](const FName& PinName)->FBPTerminal*
		{
			auto Pin = CurNode->FindPin(PinName);
			if(!Pin)
				return nullptr;
			UEdGraphPin* PinToTry = FEdGraphUtilities::GetNetFromPin(Pin);
			FBPTerminal** Term = Context.NetMap.Find(PinToTry);
			if(!Term)
				return nullptr;
			return (*Term);			
		};
		for (const FName& Input : CurNode->Inputs)
		{
			if(auto* Term = GetBPTerminal(Input))
			{
				ExportParam(Term->AssociatedVarProperty, Input.ToString(), DynamicParamCount++);	
			}
		}
		for (const FName& Output : CurNode->Outputs)
		{
			if(auto* Term = GetBPTerminal(Output))
			{
				ExportParam(Term->AssociatedVarProperty, Output.ToString(), DynamicParamCount++);	
			}
		}
		int64 FunctionPtr = 0;
		NameSpaceAvoidReDefine += AddressInjectCode + TEXT("	//You CppScript start from here\n") + CurNode->Snippet + TEXT("\n};\n{\n");
		NameSpaceAvoidReDefine += FString::Printf(TEXT("	signed long long& FunctionPtr = *(signed long long*)%I64d;\n"),size_t(&FunctionPtr));
		NameSpaceAvoidReDefine += FString::Printf(TEXT("	FunctionPtr = reinterpret_cast<int64>((void(*)(signed long long*))(%s));\n}"),*LambdaName);
		
		auto* CompileResult = reinterpret_cast<FCppScriptCompiledResult*>(CurNode->ResultPtr);
		if(CompileResult==nullptr || !CompileResult->DebugCodeGen)
		{
			auto& Module = FModuleManager::Get().GetModuleChecked<FUClingModule>(TEXT("UCling"));
			::Decalre(Module.Interp,StringCast<ANSICHAR>(*CurNode->Includes).Get(),nullptr);
			::Process(Module.Interp,StringCast<ANSICHAR>(*NameSpaceAvoidReDefine).Get(),nullptr);
		}
		if(CompileResult)
		{
			CompileResult->CodePreview = NameSpaceAvoidReDefine;
			CompileResult->FunctionPtrAddress = FunctionPtr;
		}		 
		UEdGraphPin* PtrLiteral = FEdGraphUtilities::GetNetFromPin(PtrPin);
		if (FBPTerminal** Term = Context.NetMap.Find(PtrLiteral))
		{
			//Name is the literal value of int64! See FScriptBuilderBase::EmitTermExpr
			(*Term)->Name = FString::Printf(TEXT("%I64d"),FunctionPtr);		
			if(auto StatementsResult = Context.StatementsPerNode.Find(Node))
			{
				const int32 CurrentStatementCount = StatementsResult->Num();			
				for (int i = 0; i<CurrentStatementCount;i++)
				{
					auto* Statement = (*StatementsResult)[i];
					if(Statement->Type == KCST_CallFunction && Statement->FunctionToCall == CurNode->GetTargetFunction())
					{
						Statement->RHS.Add(*Term);
					}
				}
			}
		}
		if (FunctionPtr == 0)
		{
			CompilerContext.MessageLog.Error(*NSLOCTEXT("KismetCompiler", "CompileCppScriptFaild_Error", "faild to compile cpp script of @@, check output log and code preview to get more information!").ToString(), Node);
		}
	}
};

UK2Node_ExecuteCppScript::UK2Node_ExecuteCppScript()
{
	FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UClingBlueprintFunctionLibrary, RunCppScript), UClingBlueprintFunctionLibrary::StaticClass());
	ResultPtr = reinterpret_cast<int64>(&Result);
	FunctionName = TEXT("MyFunction");
}

void UK2Node_ExecuteCppScript::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Hide the argument count pin
	UEdGraphPin* ArgCountPin = FindPinChecked(ExecuteCppScriptUtil::ArgCountPinName, EGPD_Input);
	ArgCountPin->bHidden = true;

	// Rename the result pin
	UEdGraphPin* ResultPin = FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue, EGPD_Output);
	ResultPin->PinFriendlyName = LOCTEXT("ResultPinFriendlyName", "Success?");

	// Add user-defined pins
	auto AddUserDefinedPins = [this](const TArray<FName>& InPinNames, EEdGraphPinDirection InPinDirection)
	{
		for (const FName& PinName : InPinNames)
		{
			UEdGraphPin* ArgPin = CreatePin(InPinDirection, UEdGraphSchema_K2::PC_Wildcard, PinName);
			ArgPin->PinFriendlyName = FText::AsCultureInvariant(ExecuteCppScriptUtil::CppizePinName(ArgPin));
		}
	};
	AddUserDefinedPins(Inputs, EGPD_Input);
	AddUserDefinedPins(Outputs, EGPD_Output);
}

void UK2Node_ExecuteCppScript::PostReconstructNode()
{
	Super::PostReconstructNode();

	auto SynchronizeUserDefinedPins = [this](const TArray<FName>& InPinNames, EEdGraphPinDirection InPinDirection)
	{
		for (const FName& PinName : InPinNames)
		{
			UEdGraphPin* ArgPin = FindArgumentPinChecked(PinName, InPinDirection);
			SynchronizeArgumentPinTypeImpl(ArgPin);
		}
	};
	SynchronizeUserDefinedPins(Inputs, EGPD_Input);
	SynchronizeUserDefinedPins(Outputs, EGPD_Output);
}

FNodeHandlingFunctor* UK2Node_ExecuteCppScript::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_CompileAndStorePtr(CompilerContext);
}

void UK2Node_ExecuteCppScript::SynchronizeArgumentPinType(UEdGraphPin* Pin)
{
	auto SynchronizeUserDefinedPin = [this, Pin](const TArray<FName>& InPinNames, EEdGraphPinDirection InPinDirection)
	{
		if (Pin->Direction == InPinDirection && InPinNames.Contains(Pin->PinName))
		{
			// Try and find the argument pin and make sure we get the same result as the pin we were asked to update
			// If not we may have a duplicate pin name with another non-argument pin
			UEdGraphPin* ArgPin = FindArgumentPinChecked(Pin->PinName, InPinDirection);
			if (ArgPin == Pin)
			{
				SynchronizeArgumentPinTypeImpl(Pin);
			}
		}
	};
	SynchronizeUserDefinedPin(Inputs, EGPD_Input);
	SynchronizeUserDefinedPin(Outputs, EGPD_Output);
}

void UK2Node_ExecuteCppScript::SynchronizeArgumentPinTypeImpl(UEdGraphPin* Pin)
{
	FEdGraphPinType NewPinType;
	if (Pin->LinkedTo.Num() > 0)
	{
		NewPinType = Pin->LinkedTo[0]->PinType;
	}
	else
	{
		NewPinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
	}

	if (Pin->PinType != NewPinType)
	{
		Pin->PinType = NewPinType;

		GetGraph()->NotifyNodeChanged(this);

		UBlueprint* Blueprint = GetBlueprint();
		if (!Blueprint->bBeingCompiled)
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		}
	}
}

UEdGraphPin* UK2Node_ExecuteCppScript::FindArgumentPin(const FName PinName, EEdGraphPinDirection PinDirection)
{
	for (int32 PinIndex = Pins.Num() - 1; PinIndex >= 0; --PinIndex)
	{
		UEdGraphPin* Pin = Pins[PinIndex];
		if ((PinDirection == EGPD_MAX || PinDirection == Pin->Direction) && Pin->PinName == PinName)
		{
			return Pin;
		}
	}
	return nullptr;
}

UEdGraphPin* UK2Node_ExecuteCppScript::FindArgumentPinChecked(const FName PinName, EEdGraphPinDirection PinDirection)
{
	UEdGraphPin* Pin = FindArgumentPin(PinName, PinDirection);
	check(Pin);
	return Pin;
}

void UK2Node_ExecuteCppScript::CreateTempFunctionPtrPin()
{
	FunctionPtrAddressPin = CreatePin(EGPD_Input,
		UEdGraphSchema_K2::PC_Int64,FName(TEXT("FunctionPtr")));
	Pins.RemoveAt(Pins.Num()-1);
}

UEdGraphPin* UK2Node_ExecuteCppScript::GetFunctionPtrPin() const
{
	return FunctionPtrAddressPin;
}

void UK2Node_ExecuteCppScript::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property  ? PropertyChangedEvent.Property->GetFName() : NAME_None);
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_ExecuteCppScript, Inputs) || PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_ExecuteCppScript, Outputs))
	{
		ReconstructNode();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
	GetGraph()->NotifyNodeChanged(this);
}

void UK2Node_ExecuteCppScript::PostLoad()
{
	Super::PostLoad();
	// if(!HasAnyFlags(RF_Transient))
	// {
	// 	ResultPtr = reinterpret_cast<int64>(&Result);		
	// }
}

void UK2Node_ExecuteCppScript::PreDuplicate(FObjectDuplicationParameters& DupParams)
{
	Super::PreDuplicate(DupParams);
}

void UK2Node_ExecuteCppScript::PreloadRequiredAssets()
{
	Super::PreloadRequiredAssets();
	
	if(!HasAnyFlags(RF_Transient))
	{
		ResultPtr = reinterpret_cast<int64>(&Result);
	}
}

void UK2Node_ExecuteCppScript::PinConnectionListChanged(UEdGraphPin* Pin)
{
	// Potentially update an argument pin type
	SynchronizeArgumentPinType(Pin);
}

void UK2Node_ExecuteCppScript::PinTypeChanged(UEdGraphPin* Pin)
{
	// Potentially update an argument pin type
	SynchronizeArgumentPinType(Pin);

	Super::PinTypeChanged(Pin);
}

FText UK2Node_ExecuteCppScript::GetPinDisplayName(const UEdGraphPin* Pin) const
{
	if ((Inputs.Contains(Pin->PinName) || Outputs.Contains(Pin->PinName)) && !Pin->PinFriendlyName.IsEmpty())
	{
		return Pin->PinFriendlyName;
	}
	return Super::GetPinDisplayName(Pin);
}

void UK2Node_ExecuteCppScript::EarlyValidation(FCompilerResultsLog& MessageLog) const
{
	Super::EarlyValidation(MessageLog);

	TSet<FString> AllPinNames;
	auto ValidatePinArray = [this, &AllPinNames, &MessageLog](const TArray<FName>& InPinNames)
	{
		for (const FName& PinName : InPinNames)
		{
			const FString CppizedPinName = ExecuteCppScriptUtil::CppizePinName(PinName);

			if (PinName == UEdGraphSchema_K2::PN_Execute ||
				PinName == UEdGraphSchema_K2::PN_Then ||
				PinName == UEdGraphSchema_K2::PN_ReturnValue ||
				PinName == ExecuteCppScriptUtil::ArgCountPinName
				)
			{
				MessageLog.Error(*FText::Format(LOCTEXT("InvalidPinName_RestrictedName", "Pin '{0}' ({1}) on @@ is using a restricted name."), FText::AsCultureInvariant(PinName.ToString()), FText::AsCultureInvariant(CppizedPinName)).ToString(), this);
			}

			if (CppizedPinName.Len() == 0)
			{
				MessageLog.Error(*LOCTEXT("InvalidPinName_EmptyName", "Empty pin name found on @@").ToString(), this);
			}
			else
			{
				bool bAlreadyUsed = false;
				AllPinNames.Add(CppizedPinName, &bAlreadyUsed);

				if (bAlreadyUsed)
				{
					MessageLog.Error(*FText::Format(LOCTEXT("InvalidPinName_DuplicateName", "Pin '{0}' ({1}) on @@ has the same name as another pin when exposed to Cpp."), FText::AsCultureInvariant(PinName.ToString()), FText::AsCultureInvariant(CppizedPinName)).ToString(), this);
				}
			}
		}
	};

	ValidatePinArray(Inputs);
	ValidatePinArray(Outputs);
}

void UK2Node_ExecuteCppScript::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	static const FName MakeLiteralIntFunctionName = GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralInt);
	// Create the "Make Literal Int" node
	UK2Node_CallFunction* MakeLiteralIntNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	MakeLiteralIntNode->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(MakeLiteralIntFunctionName));
	MakeLiteralIntNode->AllocateDefaultPins();
	
	UEdGraphPin* MakeLiteralIntValuePin = MakeLiteralIntNode->FindPinChecked(TEXT("Value"), EGPD_Input);
	MakeLiteralIntValuePin->DefaultValue = FString::FromInt(Inputs.Num()+Outputs.Num());
	
	MakeLiteralIntNode->GetReturnValuePin()->MakeLinkTo(FindPinChecked(ExecuteCppScriptUtil::ArgCountPinName, EGPD_Input));	
	
	Super::ExpandNode(CompilerContext, SourceGraph);
}

bool UK2Node_ExecuteCppScript::CanPasteHere(const UEdGraph* TargetGraph) const
{
	bool bCanPaste = Super::CanPasteHere(TargetGraph);
	if (bCanPaste)
	{
		bCanPaste &= FBlueprintEditorUtils::IsEditorUtilityBlueprint(FBlueprintEditorUtils::FindBlueprintForGraphChecked(TargetGraph));
	}
	return bCanPaste;
}

bool UK2Node_ExecuteCppScript::IsActionFilteredOut(const FBlueprintActionFilter& Filter)
{
	bool bIsFilteredOut = Super::IsActionFilteredOut(Filter);
	if (!bIsFilteredOut)
	{
		for (UEdGraph* TargetGraph : Filter.Context.Graphs)
		{
			bIsFilteredOut |= !CanPasteHere(TargetGraph);
		}
	}
	return bIsFilteredOut;
}

void UK2Node_ExecuteCppScript::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// actions get registered under specific object-keys; the idea is that 
	// actions might have to be updated (or deleted) if their object-key is  
	// mutated (or removed)... here we use the node's class (so if the node 
	// type disappears, then the action should go with it)
	UClass* ActionKey = GetClass();
	// to keep from needlessly instantiating a UBlueprintNodeSpawner, first   
	// check to make sure that the registrar is looking for actions of this type
	// (could be regenerating actions for a specific asset, and therefore the 
	// registrar would only accept actions corresponding to that asset)
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

#undef LOCTEXT_NAMESPACE

