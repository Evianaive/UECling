#pragma once

#include "KismetCompiler.h"

class UClingScriptBlueprint;
class UClingScriptBlueprintGeneratedClass;

class FClingScriptBlueprintCompiler : public FKismetCompilerContext
{
	typedef FKismetCompilerContext Super;
public:
	FClingScriptBlueprintCompiler(UClingScriptBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions);
	virtual ~FClingScriptBlueprintCompiler();

protected:
	// Implementation of FKismetCompilerContext interface
	virtual void CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& InOldCDO) override;
	virtual void CreateClassVariablesFromBlueprint() override;
	virtual void CreateFunctionList() override;
	virtual void FinishCompilingClass(UClass* Class) override;
	virtual void EnsureProperGeneratedClass(UClass*& TargetUClass) override;
	virtual void SpawnNewClass(const FString& NewClassName) override;
	virtual void OnNewClassSet(UBlueprintGeneratedClass* ClassToUse) override;
	virtual void MergeUbergraphPagesIn(UEdGraph* Ubergraph) override;
	virtual void ProcessOneFunctionGraph(UEdGraph* SourceGraph, bool bInternalFunction = false) override;
	virtual void OnPostCDOCompiled(const UObject::FPostCDOCompiledContext& Context) override;
	virtual void CopyTermDefaultsToDefaultObject(UObject* DefaultObject) override;
	virtual void PostCompile() override;
	virtual void PostCompileDiagnostics() override;
	virtual void PrecompileFunction(FKismetFunctionContext& Context, EInternalCompilerFlags InternalFlags) override;
	virtual void SetCalculatedMetaDataAndFlags(UFunction* Function, UK2Node_FunctionEntry* EntryNode, const UEdGraphSchema_K2* InSchema ) override;
	virtual bool ShouldForceKeepNode(const UEdGraphNode* Node) const override;
	virtual void PostExpansionStep(const UEdGraph* Graph) override;
	virtual void PreCompileUpdateBlueprintOnLoad(UBlueprint* BP) override;
	// End of FKismetCompilerContext interface

	UClingScriptBlueprint* ClingScriptBlueprint() const;

	UClingScriptBlueprintGeneratedClass* NewClingScriptBlueprintClass;
};
