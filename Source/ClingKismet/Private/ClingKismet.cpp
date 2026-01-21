#include "ClingKismet.h"
#include "KismetCompilerModule.h"
#include "ClingScriptBlueprintCompiler.h"
#include "ClingScriptBlueprint.h"

#define LOCTEXT_NAMESPACE "FClingKismetModule"

class FClingScriptBlueprintCompilerFactory : public IBlueprintCompiler
{
public:
	virtual bool CanCompile(const UBlueprint* Blueprint) override
	{
		return Cast<UClingScriptBlueprint>(Blueprint) != nullptr;
	}

	virtual void Compile(UBlueprint* Blueprint, const FKismetCompilerOptions& CompileOptions, FCompilerResultsLog& MessageLog) override
	{
		FClingScriptBlueprintCompiler Compiler(CastChecked<UClingScriptBlueprint>(Blueprint), MessageLog, CompileOptions);
		Compiler.Compile();
	}
};

static FClingScriptBlueprintCompilerFactory ClingScriptBlueprintCompilerFactory;

void FClingKismetModule::StartupModule()
{
	IKismetCompilerInterface& KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(TEXT("KismetCompiler"));
	KismetCompilerModule.GetCompilers().Add(&ClingScriptBlueprintCompilerFactory);
}

void FClingKismetModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FClingKismetModule, ClingKismet);