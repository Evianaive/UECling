// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "HAL/CriticalSection.h"
#include "ClingSemanticInfoProvider.h"

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
	void* StartNewInterp(FName PCHProfile = TEXT("Default"));
	void DeleteInterp(void* CurrentInterp);
	static FClingRuntimeModule& Get();

	struct FClingSemanticInfoProvider* GetSemanticInfoProvider(void* InInterp);
	struct FClingSemanticInfoProvider* GetDefaultSemanticInfoProvider();

	/** Get the global lock for CppInterOp calls */
	FCriticalSection& GetCppInterOpLock() { return CppInterOpLock; }

private:
	void* StartInterpreterInternal(FName PCHProfile = TEXT("Default"));

	FCriticalSection CppInterOpLock;

	TMap<void*, FClingSemanticInfoProvider> SemanticInfoProviders;
};
