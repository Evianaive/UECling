// Fill out your copyright notice in the Description page of Project Settings.


#include "K2Node_ExecuteCppScript.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "CallFunctionHandler.h"
#include "cling-demo.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_MakeArray.h"
#include "KismetCompiler.h"
#include "ClingRuntime.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ClingRuntime/Public/ClingBlueprintFunctionLibrary.h"
#include "RecoverCodeGeneration.h"

//We have to recompile these files because they are not exported!
#include "ClingScriptMarks.h"
#include "ClingSetting.h"
#include "ISourceCodeAccessModule.h"
#include "ISourceCodeAccessor.h"
#include "Editor/BlueprintGraph/Private/CallFunctionHandler.cpp"
#include "Editor/BlueprintGraph/Private/PushModelHelpers.cpp"
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>2
#else
#include "Misc/FileHelper.h"
#endif
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>2
#else
// #include "GraphEditAction.h"
// FEdGraphEditAction MakeAction(UEdGraph* Graph, UEdGraphNode* Node)
// {
// 	FEdGraphEditAction Action;
// 	Action.Graph = Graph;
// 	Action.Action = EEdGraphActionType::GRAPHACTION_Default;
// 	Action.Nodes.Add(Node);
// 	return Action;
// }
#endif
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

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		TGuardValue GuardCurNode{CurNode,CastChecked<UK2Node_ExecuteCppScript>(Node)};
		FKCHandler_CallFunction::Compile(Context, Node);
		
		FString FunctionDeclare;
		// Todo check recover code is valid or not 
		// Code only exist in Node Property
		if(!CurNode->HasTempFile())
			// CurNode is copied so it is temp
			FunctionDeclare = CurNode->GenerateCode(true);
		// Node have a temp local file
		else
			FunctionDeclare = FString::Printf(TEXT("#include \"%s\""),*CurNode->GetTempFileName());
		
		int64 FunctionPtr = 0;
		
		auto PtrPin = CurNode->GetFunctionPtrPin();
		FString LogInformation = NSLOCTEXT("KismetCompiler", "CompileCppScriptFaild_Error", "faild to compile cpp script of @@, check output log and code preview to get more information!").ToString();
		ON_SCOPE_EXIT
		{
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
				CompilerContext.MessageLog.Error(*LogInformation, Node);
			}
		};
		// File will not be included, left it un-compiled
		if(CurNode->bFileOpenedInIDE)
		{
			LogInformation = NSLOCTEXT("KismetCompiler", "CompileCppScriptFaild_Error", "faild to compile cpp script of @@, node must be compiled after read back all code from IDE!").ToString();
			return;
		}
		auto& Module = FModuleManager::Get().GetModuleChecked<FClingRuntimeModule>(TEXT("ClingRuntime"));
		::Decalre(Module.GetInterp(),StringCast<ANSICHAR>(*FunctionDeclare).Get(),nullptr);
		
		FString StubLambda;
		StubLambda += FString::Printf(TEXT("{\n\tsigned long long& FunctionPtr = *(signed long long*)%I64d;\n"),size_t(&FunctionPtr));
		StubLambda += FString::Printf(TEXT("\tFunctionPtr = reinterpret_cast<int64>((void(*)(signed long long*))(%s));\n}"),*CurNode->GetLambdaName());
		
		auto* CompileResult = reinterpret_cast<FCppScriptCompiledResult*>(CurNode->ResultPtr);
		::Process(Module.GetInterp(),StringCast<ANSICHAR>(*StubLambda).Get(),nullptr);
		
		if(CompileResult)
		{
			CompileResult->CodePreview = FunctionDeclare;
			CompileResult->FunctionPtrAddress = FunctionPtr;
			CompileResult->bGuidCompiled = true;
			CompileResult->FunctionGuid = CurNode->FunctionGuid;
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

FString UK2Node_ExecuteCppScript::GenerateCode(bool UpdateGuid)
{
	struct FWrapToLambdaFunction
	{
		FWrapToLambdaFunction(FString& InCode, UK2Node_ExecuteCppScript* InSelf, bool InUpdateGuid)
		:Code(InCode)
		,Self(InSelf)
		{
			if(InUpdateGuid)
				Self->FunctionGuid = FGuid::NewGuid();
			InCode += TEXT(MARK_INC "\n");
			InCode += Self->Includes + TEXT("\n");
			InCode += TEXT(MARK_INC "\n");
			InCode += FString::Printf(TEXT("auto %s = [](signed long long* Args){\n"),*Self->GetLambdaName());
			InCode += FString::Printf(TEXT("\tSCOPED_NAMED_EVENT(%s, FColor::Red);\n" MARK_INC ":Begin recover Args\n"),*Self->FunctionName.ToString());
		}
		~FWrapToLambdaFunction()
		{
			Code += TEXT(MARK_INC ":End recover Args\n");
			Code += TEXT(MARK_SCRPT ":You CppScript start from here\n");
			Code += Self->Snippet + TEXT("\n");
			Code += TEXT(MARK_SCRPT ":You CppScript end at here\n\n};\n");
		}
		FString& Code;
		UK2Node_ExecuteCppScript* Self;
	};
	FString LambdaString;
	FWrapToLambdaFunction WrapLambdaRAII{LambdaString,this,UpdateGuid};
	
	FName FirstArg;
	if(Inputs.Num())
		FirstArg = Inputs[0];
	else if(Outputs.Num())
		FirstArg = Outputs[0];
	
	int32 PinIndex = 0;
	for(;PinIndex<Pins.Num();PinIndex++)
	{
		if(Pins[PinIndex]->GetFName() == FirstArg)
			break;
	}
	if(PinIndex==Pins.Num())
		return LambdaString;

	auto RecoverPin = [&LambdaString](UEdGraphPin* Pin, int32 ParamIndex)
	{
		// LambdaString += TEXT(MARK_STRCT ":Begin Depends Blueprint Struct\n");
		if(Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
		{
			FRecoverCodeGeneration::GenerateStructCode(Cast<UScriptStruct>(Pin->PinType.PinSubCategoryObject), LambdaString);		
		}	
		if(Pin->PinType.IsMap())
		{
			FRecoverCodeGeneration::GenerateStructCode(Cast<UScriptStruct>(Pin->PinType.PinValueType.TerminalSubCategoryObject), LambdaString);
		}
		// LambdaString += TEXT(MARK_STRCT ":End Depends Blueprint Struct\n");
		FString CppType;
		FRecoverCodeGeneration::GetCppTypeFromPin(CppType,Pin->PinType,Pin);
		LambdaString += FString::Printf(TEXT("\t%s& %s = *(%s*)Args[%i];\n"),*CppType,*Pin->GetName(),*CppType,ParamIndex);
	};
	int32 ParamIndex = 0;
	for (auto& Input : Inputs)
	{
		RecoverPin(Pins[PinIndex++],ParamIndex++);
	}
	for (auto& Output : Outputs)
	{
		RecoverPin(Pins[PinIndex++],ParamIndex++);
	}
	return LambdaString;
}

FString UK2Node_ExecuteCppScript::GetLambdaName() const
{
	return FString::Printf(TEXT("%s_%s"),*FunctionName.ToString(),*FunctionGuid.ToString());
}

FString UK2Node_ExecuteCppScript::GetTempFileName() const
{
	return GetLambdaName() + TEXT(".h");
}

bool UK2Node_ExecuteCppScript::HasTempFile(bool bCheckFile) const
{
	if(!bCheckFile)
		return FunctionGuid.IsValid();	
	return FunctionGuid.IsValid() && FPaths::FileExists(GetFileFolderPath()/GetTempFileName());
}

FString UK2Node_ExecuteCppScript::GetFileFolderPath() const
{
	// Todo use PathForFunctionLibrarySync when function come from a library
	return FPaths::ConvertRelativePathToFull(GetDefault<UClingSetting>()->PathForLambdaScriptCompile.Path);
}

FString UK2Node_ExecuteCppScript::GetTempFilePath() const
{
	return GetFileFolderPath()/GetTempFileName();
}

bool UK2Node_ExecuteCppScript::IsOpenedInIDE() const
{
	return bFileOpenedInIDE;
}

bool UK2Node_ExecuteCppScript::IsCurrentGuidCompiled() const
{
	if(reinterpret_cast<FCppScriptCompiledResult*>(ResultPtr)->FunctionGuid==FunctionGuid)
		return reinterpret_cast<FCppScriptCompiledResult*>(ResultPtr)->bGuidCompiled;
	return false;
}

void UK2Node_ExecuteCppScript::OpenInIDE()
{
	struct FEnsureOpenInIDE
	{
		FEnsureOpenInIDE(UK2Node_ExecuteCppScript* InSelf)
			:Self(InSelf){}
		~FEnsureOpenInIDE()
		{
			auto CountLinesInFString = [](const FString& InString)
			{
				int32 LineCount = 1; 
				for (TCHAR Ch : InString)
				{
					if (Ch == TEXT('\n'))
					{
						LineCount++;
					}
				}
				return LineCount;
			};
			FString File;
			if(bRegenerated)
			{
				FString NewCode = Self->GenerateCode(true);
				File = Self->GetTempFilePath();
				FFileHelper::SaveStringToFile(NewCode,*File);
			}
			else
			{
				File = Self->GetTempFilePath();
			}			
			int32 Line = CountLinesInFString(Self->Includes)+1;
			ISourceCodeAccessModule& SourceCodeAccessModule = FModuleManager::LoadModuleChecked<ISourceCodeAccessModule>("SourceCodeAccess");
			ISourceCodeAccessor& SourceCodeAccessor = SourceCodeAccessModule.GetAccessor();
			if (FPaths::FileExists(File))
			{
				// Todo IDE like rider will not include this file in solution automatically
				SourceCodeAccessor.OpenFileAtLine(File, Line);
				Self->bFileOpenedInIDE = true;
				// Todo check if this is right
				Self->Modify();
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>2
				Self->GetGraph()->NotifyNodeChanged(Self);
#else
				Self->GetGraph()->NotifyGraphChanged();
#endif
			}
			else
			{
				if(bRegenerated)
				{
					// Todo tell user that file is not generated rightly!
				}
				// Todo change to display a model window
				SourceCodeAccessModule.OnOpenFileFailed().Broadcast(File);
			}
		}
		UK2Node_ExecuteCppScript* Self;
		bool bRegenerated{false};
		TSharedPtr<SWidget> WidgetToShow;
	};
	FEnsureOpenInIDE EnsureOpenInIDE{this};
	// We always need to create a new file if file is compiled(included)!
	bool bFunctionCompiled = IsCurrentGuidCompiled();
	bool bOpenedInIDE = IsOpenedInIDE();
	
	ensureMsgf(!(bFunctionCompiled&&bOpenedInIDE),TEXT("Function should be compiled when code is write back from IDE"));

	FString NewCode;
	if(bFunctionCompiled)
	{
		EnsureOpenInIDE.bRegenerated = true;
		return;
	}
	// We dont need to check whether file exist
	FString File = GetTempFilePath();
	if(!FPaths::FileExists(File))
	{
		if(bOpenedInIDE)
		{
			// Show UI to tell user the temp file should not be deleted
			// EnsureOpenInIDE.WidgetToShow;
			return;
		}
		EnsureOpenInIDE.bRegenerated = true;
		return;
	}
}

void UK2Node_ExecuteCppScript::BackFromIDE()
{
	if(!IsOpenedInIDE())
	{
		//Todo this means we will get old code from file. It may be not compatible with node
	}
	if(!FPaths::FileExists(GetTempFilePath()))
	{
		bFileOpenedInIDE = false;
		FunctionGuid.Invalidate();
		//Todo allow user edit in node
		return;
	}
	
	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines,*GetTempFilePath());
	// Todo Currently it should only have one element
	TArray<int32> ScopeStartIds;
	TArray<FName> ScopeStartNames;

	TArray<FName> CodeBlockNames;
	TArray<FString> CodeBlocks;
	for (int i = 0; i<Lines.Num(); i++)
	{
		auto& Line = Lines[i];
		if(Line.StartsWith(CLING_MARK))
		{
			int32 LineMarkEnd = INDEX_NONE;
			Line.FindChar(':',LineMarkEnd);
			if(LineMarkEnd != INDEX_NONE)
			{
				Line.LeftInline(LineMarkEnd);				
			}
			else
			{
				Line.RemoveSpacesInline();
			}
			FName Mark{Line};
			if(ScopeStartNames.Num() && ScopeStartNames.Last()==Mark)
			{
				ScopeStartNames.Pop();
				ScopeStartIds.Pop();
				continue;
			}
			ScopeStartNames.Add(Mark);
			ScopeStartIds.Add(i);
			
			CodeBlockNames.Add(Mark);
			CodeBlocks.Emplace();
			continue;
		}
		if(ScopeStartNames.Num())
			CodeBlocks.Last().Append(Line+TEXT("\n"));
	}
	// Todo check if recover code match inputs and outputs!
	auto Id = CodeBlockNames.Find(FName(MARK_INC));
	if(Id!=INDEX_NONE)
	{
		CodeBlocks[Id].RemoveAt(CodeBlocks[Id].Len()-1);
		Includes = CodeBlocks[Id];
	}
	Id = CodeBlockNames.Find(FName(MARK_RCVR));
	if(Id!=INDEX_NONE)
	{
		//CodeBlocks[Id];
		// Todo Check recover code!
	}
	Id = CodeBlockNames.Find(FName(MARK_SCRPT));
    if(Id!=INDEX_NONE)
    {
    	CodeBlocks[Id].RemoveAt(CodeBlocks[Id].Len()-1);
    	Snippet = CodeBlocks[Id];
    }
	bFileOpenedInIDE = false;
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>2
	GetGraph()->NotifyNodeChanged(this);
#else
	GetGraph()->NotifyGraphChanged();
#endif
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
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>2
		GetGraph()->NotifyNodeChanged(this);
#else
		GetGraph()->NotifyGraphChanged();
#endif
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
	if( PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_ExecuteCppScript, bEditInIDE))
	{
		if(bEditInIDE)
			OpenInIDE();
		else
			BackFromIDE();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>2
	GetGraph()->NotifyNodeChanged(this);
#else
	GetGraph()->NotifyGraphChanged();
#endif
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

