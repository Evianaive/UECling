// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

namespace cling
{
	class Interpreter;
}

class FUClingModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	cling::Interpreter* Interp{nullptr};
private:
	/** Handle to the test dll we will load */
	TArray<void*> ExampleLibraryHandles;
};
