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
	void* BaseInterp{nullptr};
public:
	// Todo use unique ptr
	TArray<void*> Interps;

	void* GetInterp(int Version=0);
	void* StartNewInterp();
	void DeleteInterp(void* CurrentInterp);
private:
	void* StartInterpreterInternal();
};
