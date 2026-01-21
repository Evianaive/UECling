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
	// FKismetCompilerContext interface
	virtual void CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& InOldCDO) override;
	virtual void CreateClassVariablesFromBlueprint() override;
	virtual void CreateFunctionList() override;
	virtual void FinishCompilingClass(UClass* Class) override;
	virtual void EnsureProperGeneratedClass(UClass*& TargetUClass) override;
	virtual void SpawnNewClass(const FString& NewClassName) override;
	virtual void OnNewClassSet(UBlueprintGeneratedClass* ClassToUse) override;
	// End of FKismetCompilerContext interface

	UClingScriptBlueprint* ClingScriptBlueprint() const;

	UClingScriptBlueprintGeneratedClass* NewClingScriptBlueprintClass;
};
