#include "ClingEditor.h"

#include "ClingRuntime.h"
#include "ClingEditor/Public/ClingCommandExecutor.h"

#define LOCTEXT_NAMESPACE "FClingEditorModule"

void FClingEditorModule::StartupModule()
{
	StartupCommandExecutor();
	FModuleManager::LoadModuleChecked<FClingRuntimeModule>(TEXT("ClingRuntime"));
}

void FClingEditorModule::ShutdownModule()
{
	ShutdownCommandExecutor();
}

void FClingEditorModule::StartupCommandExecutor()
{
	Executor = new FClingCommandExecutor(FModuleManager::LoadModuleChecked<FClingRuntimeModule>(TEXT("ClingRuntime")).Interp);
	IModularFeatures::Get().RegisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), Executor);
}

void FClingEditorModule::ShutdownCommandExecutor()
{
	IModularFeatures::Get().UnregisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), Executor);
	delete Executor;
}
#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FClingEditorModule, ClingEditor)