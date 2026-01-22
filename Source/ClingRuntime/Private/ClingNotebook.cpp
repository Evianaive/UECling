#include "ClingNotebook.h"
#include "ClingRuntime.h"
#include "CppInterOp/CppInterOp.h"
#include "Modules/ModuleManager.h"

void UClingNotebook::FinishDestroy()
{
	if (Interpreter)
	{
		FClingRuntimeModule::Get().DeleteInterp(Interpreter);
		Interpreter = nullptr;
	}
	Super::FinishDestroy();
}

void* UClingNotebook::GetInterpreter()
{
	if (!Interpreter)
	{
		Interpreter = FClingRuntimeModule::Get().StartNewInterp();
	}
	return Interpreter;
}

void UClingNotebook::RestartInterpreter()
{
	if (Interpreter)
	{
		FClingRuntimeModule::Get().DeleteInterp(Interpreter);
	}
	Interpreter = FClingRuntimeModule::Get().StartNewInterp();
}
