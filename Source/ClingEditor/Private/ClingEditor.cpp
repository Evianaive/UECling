#include "ClingEditor.h"

#include "ClingRuntime.h"
#include "CppHighLight/CodeEditorStyle.h"
#include "ClingEditor/Public/ClingCommandExecutor.h"
#include "Customization/CodeStringCustomization.h"

#define LOCTEXT_NAMESPACE "FClingEditorModule"

void FClingEditorModule::StartupModule()
{
	StartupCommandExecutor();
	FModuleManager::LoadModuleChecked<FClingRuntimeModule>(TEXT("ClingRuntime"));
	FClingCodeEditorStyle::Initialize();
	
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout("CodeString", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCodeStringCustomization::MakeInstance));	
}

void FClingEditorModule::ShutdownModule()
{
	FPropertyEditorModule* PropertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");
	if (PropertyModule)
		PropertyModule->UnregisterCustomPropertyTypeLayout("CodeString");
	FClingCodeEditorStyle::Shutdown();
	ShutdownCommandExecutor();
}

void FClingEditorModule::StartupCommandExecutor()
{
	FClingRuntimeModule& Module = FModuleManager::LoadModuleChecked<FClingRuntimeModule>(TEXT("ClingRuntime"));
	Executor = new FClingCommandExecutor(Module.BaseInterp);
	Executor->RestartInterpreter.BindLambda([&Module](){return Module.StartNewInterp();});
	IModularFeatures::Get().RegisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), Executor);
}

void FClingEditorModule::ShutdownCommandExecutor()
{
	IModularFeatures::Get().UnregisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), Executor);
	delete Executor;
}
#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FClingEditorModule, ClingEditor)