// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "UObject/Object.h"

namespace cling
{
	class Interpreter;
}

extern "C" CLINGRUNTIME_API void* ClingRuntime_GetClingInstance(UObject* Obj);
extern "C" CLINGRUNTIME_API int32 ClingRuntime_GetStepInt(void* StackPtr);
extern "C" CLINGRUNTIME_API float ClingRuntime_GetStepFloat(void* StackPtr);
extern "C" CLINGRUNTIME_API double ClingRuntime_GetStepDouble(void* StackPtr);
extern "C" CLINGRUNTIME_API int64 ClingRuntime_GetStepInt64(void* StackPtr);
extern "C" CLINGRUNTIME_API uint8 ClingRuntime_GetStepByte(void* StackPtr);
extern "C" CLINGRUNTIME_API bool ClingRuntime_GetStepBool(void* StackPtr);
extern "C" CLINGRUNTIME_API void ClingRuntime_FinishStep(void* StackPtr);

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
