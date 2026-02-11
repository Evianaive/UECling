#include "ClingEditor.h"

#include "ClingRuntime.h"
#include "CppHighLight/CodeEditorStyle.h"
#include "ClingEditor/Public/ClingCommandExecutor.h"
#include "Customization/CodeStringCustomization.h"
#include "Customization/ClingFunctionSignatureCustomization.h"

#include "Asset/ClingNotebookAssetTypeActions.h"
#include "Asset/ClingScriptBlueprintAssetTypeActions.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FClingEditorModule"

void FClingEditorModule::StartupModule()
{
	StartupCommandExecutor();
	FModuleManager::LoadModuleChecked<FClingRuntimeModule>(TEXT("ClingRuntime"));
	FClingCodeEditorStyle::Initialize();
	
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout("CodeString", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCodeStringCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout("ClingFunctionSignature", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FClingFunctionSignatureCustomization::MakeInstance));

	RegisterAssetActions();
}

void FClingEditorModule::ShutdownModule()
{
	UnregisterAssetActions();
	FPropertyEditorModule* PropertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");
	if (PropertyModule)
	{
		PropertyModule->UnregisterCustomPropertyTypeLayout("CodeString");
		PropertyModule->UnregisterCustomPropertyTypeLayout("ClingFunctionSignature");
	}
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

void FClingEditorModule::RegisterAssetActions()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	TSharedPtr<IAssetTypeActions> Action = MakeShareable(new FClingNotebookAssetTypeActions);
	AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());
	RegisteredAssetActions.Add(Action);

	TSharedPtr<IAssetTypeActions> ScriptBlueprintAction = MakeShareable(new FClingScriptBlueprintAssetTypeActions);
	AssetTools.RegisterAssetTypeActions(ScriptBlueprintAction.ToSharedRef());
	RegisteredAssetActions.Add(ScriptBlueprintAction);
}

void FClingEditorModule::UnregisterAssetActions()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (auto& Action : RegisteredAssetActions)
		{
			AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
		}
	}
	RegisteredAssetActions.Empty();
}
#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FClingEditorModule, ClingEditor)