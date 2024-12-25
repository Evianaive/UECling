#include "ClingEditor.h"

#include "UCling.h"
#include "ClingEditor/Public/ClingCommandExecutor.h"

#define LOCTEXT_NAMESPACE "FClingEditorModule"

void FClingEditorModule::StartupModule()
{
	StartupCommandExecutor();
	FModuleManager::LoadModuleChecked<FUClingModule>(TEXT("UCling"));
}

void FClingEditorModule::ShutdownModule()
{
	ShutdownCommandExecutor();
}

void FClingEditorModule::StartupCommandExecutor()
{
	Executor = new FClingCommandExecutor(FModuleManager::LoadModuleChecked<FUClingModule>(TEXT("UCling")).Interp);
	IModularFeatures::Get().RegisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), Executor);
}

void FClingEditorModule::ShutdownCommandExecutor()
{
	IModularFeatures::Get().UnregisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), Executor);
	delete Executor;
}
#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FClingEditorModule, ClingEditor)