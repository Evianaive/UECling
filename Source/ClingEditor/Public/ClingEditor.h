#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FClingCommandExecutor;

class FClingEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
private:
    void StartupCommandExecutor();
    void ShutdownCommandExecutor();
    
    FClingCommandExecutor* Executor{nullptr};
};
