#include "ClingKismet.h"
#include "ClingKismetLog.h"
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
		UE_LOG(LogKismetCling, Log, TEXT("Starting compilation of ClingScriptBlueprint: %s"), *Blueprint->GetName());
		FClingScriptBlueprintCompiler Compiler(CastChecked<UClingScriptBlueprint>(Blueprint), MessageLog, CompileOptions);
		Compiler.Compile();
		UE_LOG(LogKismetCling, Log, TEXT("Finished compilation of ClingScriptBlueprint: %s"), *Blueprint->GetName());
	}
	static TSharedPtr<FKismetCompilerContext> GetControlRigCompiler(UBlueprint* BP, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompileOptions)
	{
		return TSharedPtr<FKismetCompilerContext>(new FClingScriptBlueprintCompiler(CastChecked<UClingScriptBlueprint>(BP), InMessageLog, InCompileOptions));
	}
};

static FClingScriptBlueprintCompilerFactory ClingScriptBlueprintCompilerFactory;

void FClingKismetModule::StartupModule()
{
	IKismetCompilerInterface& KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(TEXT("KismetCompiler"));
	FKismetCompilerContext::RegisterCompilerForBP(UClingScriptBlueprint::StaticClass(), &FClingScriptBlueprintCompilerFactory::GetControlRigCompiler);
	KismetCompilerModule.GetCompilers().Add(&ClingScriptBlueprintCompilerFactory);
}

void FClingKismetModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FClingKismetModule, ClingKismet);