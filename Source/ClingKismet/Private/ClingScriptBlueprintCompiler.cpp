#include "ClingScriptBlueprintCompiler.h"
#include "ClingScriptBlueprint.h"
#include "ClingScriptBlueprintGeneratedClass.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "Kismet2/KismetReinstanceUtilities.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_VariableGet.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_Self.h"
#include "EdGraphSchema_K2.h"
#include "ClingBlueprintFunctionLibrary.h"
#include "Kismet2/KismetEditorUtilities.h"

FClingScriptBlueprintCompiler::FClingScriptBlueprintCompiler(UClingScriptBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions)
	: Super(SourceSketch, InMessageLog, InCompilerOptions)
	, NewClingScriptBlueprintClass(nullptr)
{
}

FClingScriptBlueprintCompiler::~FClingScriptBlueprintCompiler()
{
}

void FClingScriptBlueprintCompiler::CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& InOldCDO)
{
	Super::CleanAndSanitizeClass(ClassToClean, InOldCDO);
}

void FClingScriptBlueprintCompiler::CreateClassVariablesFromBlueprint()
{
	Super::CreateClassVariablesFromBlueprint();
}

void FClingScriptBlueprintCompiler::CreateFunctionList()
{
	Super::CreateFunctionList();

	// Test case: Add a function "foo()" returning int32
	UClingScriptBlueprint* ScriptBP = ClingScriptBlueprint();
}

void FClingScriptBlueprintCompiler::FinishCompilingClass(UClass* Class)
{
	UClingScriptBlueprint* ScriptBP = ClingScriptBlueprint();
	UClingScriptBlueprintGeneratedClass* ScriptClass = CastChecked<UClingScriptBlueprintGeneratedClass>(Class);
	ScriptClass->SourceCode = ScriptBP->SourceCode;

	Super::FinishCompilingClass(Class);
}

UClingScriptBlueprint* FClingScriptBlueprintCompiler::ClingScriptBlueprint() const
{
	return CastChecked<UClingScriptBlueprint>(Blueprint);
}

void FClingScriptBlueprintCompiler::EnsureProperGeneratedClass(UClass*& TargetUClass)
{
	if (TargetUClass && !TargetUClass->UObjectBaseUtility::IsA<UClingScriptBlueprintGeneratedClass>())
	{
		FKismetCompilerUtilities::ConsignToOblivion(TargetUClass, Blueprint->bIsRegeneratingOnLoad);
		TargetUClass = nullptr;
	}
}

void FClingScriptBlueprintCompiler::SpawnNewClass(const FString& NewClassName)
{
	NewClingScriptBlueprintClass = FindObject<UClingScriptBlueprintGeneratedClass>(Blueprint->GetOutermost(), *NewClassName);

	if (NewClingScriptBlueprintClass == nullptr)
	{
		NewClingScriptBlueprintClass = NewObject<UClingScriptBlueprintGeneratedClass>(Blueprint->GetOutermost(), FName(*NewClassName), RF_Public | RF_Transactional);
	}
	else
	{
		FBlueprintCompileReinstancer::Create(NewClingScriptBlueprintClass);
	}
	NewClass = NewClingScriptBlueprintClass;
}

void FClingScriptBlueprintCompiler::OnNewClassSet(UBlueprintGeneratedClass* ClassToUse)
{
	NewClingScriptBlueprintClass = CastChecked<UClingScriptBlueprintGeneratedClass>(ClassToUse);
}
