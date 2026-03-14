// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "HAL/CriticalSection.h"
#include "ClingSemanticInfoProvider.h"

#include "CppInterOp/CppInterOp.h"

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

	CppImpl::CppInterpWrapper BaseInterp;
	TArray<CppImpl::CppInterpWrapper> Interps;

	CppImpl::CppInterpWrapper& GetInterp(int Version=0);
	CppImpl::CppInterpWrapper StartNewInterp(FName PCHProfile = TEXT("Default"));
	CppImpl::CppInterpWrapper& StartNewRecordInterp(FName PCHProfile = TEXT("Default"));
	void DeleteInterp(int32 Index);
	int32 FindInterpIndex(CppImpl::CppInterpWrapper& InInterp);
	static FClingRuntimeModule& Get();

	FClingSemanticInfoProvider* GetSemanticInfoProvider(CppImpl::CppInterpWrapper& InInterp);
	FClingSemanticInfoProvider* GetDefaultSemanticInfoProvider();

	/** Get the global lock for CppInterOp calls */
	FCriticalSection& GetCppInterOpLock() { return CppInterOpLock; }

private:
	static CppImpl::CppInterpWrapper StartInterpreterInternal(FName PCHProfile = TEXT("Default"));

	FCriticalSection CppInterOpLock;

	TMap<void*, FClingSemanticInfoProvider> SemanticInfoProviders;
};
