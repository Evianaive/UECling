// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

namespace cling
{
	class Interpreter;
}

class CLINGRUNTIME_API FClingRuntimeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
// private:
	cling::Interpreter* BaseInterp{nullptr};
public:
	// Todo use unique ptr
	TArray<cling::Interpreter*> Interps;

	cling::Interpreter* GetInterp(int Version=0);
	cling::Interpreter* StartNewInterp();
private:
	cling::Interpreter* StartInterpreterInternal(bool bBaseInterp);
	/** Handle to the test dll we will load */
	TArray<void*> ExampleLibraryHandles;
};
