#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "AssetTypeCategories.h"
#include "IAssetTypeActions.h"

class FClingCommandExecutor;

class FClingEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
private:
    void StartupCommandExecutor();
    void ShutdownCommandExecutor();

    void RegisterAssetActions();
    void UnregisterAssetActions();
    
    FClingCommandExecutor* Executor{nullptr};
    TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetActions;
};
