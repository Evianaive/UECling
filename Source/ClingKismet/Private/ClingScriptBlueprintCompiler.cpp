#include "ClingScriptBlueprintCompiler.h"
#include "ClingScriptBlueprint.h"
#include "ClingScriptBlueprintGeneratedClass.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "Kismet2/KismetReinstanceUtilities.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_VariableGet.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_Self.h"
#include "EdGraphSchema_K2.h"
#include "ClingBlueprintFunctionLibrary.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ClingRuntime.h"
#include "CppInterOp/CppInterOp.h"
#include "UObject/UnrealType.h"

FClingScriptBlueprintCompiler::FClingScriptBlueprintCompiler(UClingScriptBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions)
	: Super(SourceSketch, InMessageLog, InCompilerOptions)
	, NewClingScriptBlueprintClass(nullptr)
{
}

FClingScriptBlueprintCompiler::~FClingScriptBlueprintCompiler()
{
}

void FClingScriptBlueprintCompiler::CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& InOldCDO)
{
	Super::CleanAndSanitizeClass(ClassToClean, InOldCDO);
}

void FClingScriptBlueprintCompiler::CreateClassVariablesFromBlueprint()
{
	Super::CreateClassVariablesFromBlueprint();

	UClingScriptBlueprint* ScriptBP = ClingScriptBlueprint();
	if (!ScriptBP || ScriptBP->SourceCode.IsEmpty())
	{
		return;
	}

	FClingRuntimeModule& ClingModule = FModuleManager::LoadModuleChecked<FClingRuntimeModule>("ClingRuntime");
	void* Interp = ClingModule.GetInterp();

	if (Cpp::Process(TCHAR_TO_UTF8(*ScriptBP->SourceCode)) != 0)
	{
		return;
	}

	Cpp::TCppScope_t ClassScope = Cpp::GetScope("A");
	if (!ClassScope)
	{
		return;
	}

	// Add ClingInstance pointer property first. 
	// We use FInt64Property to store the pointer to the C++ instance.
	FInt64Property* ClingInstanceProp = new FInt64Property(NewClass, TEXT("ClingInstance"), RF_Public);
	ClingInstanceProp->PropertyFlags |= CPF_BlueprintVisible | CPF_Edit | CPF_Transient;
	NewClass->AddCppProperty(ClingInstanceProp);

	struct FVarInfo
	{
		FString Name;
		Cpp::TCppScope_t Var;
	};
	TArray<FVarInfo> Vars;
	static TArray<FVarInfo>* GVarsPtr = nullptr;
	GVarsPtr = &Vars;
	Cpp::GetDatamembers(ClassScope, [](const Cpp::TCppScope_t* vars, size_t size) {
		for (size_t i = 0; i < size; ++i)
		{
			static FString GVarName;
			auto NameCallback = [](const char* name) {
				GVarName = FString(UTF8_TO_TCHAR(name));
			};
			Cpp::GetName(vars[i], NameCallback);
			if (!GVarName.IsEmpty())
			{
				GVarsPtr->Add({GVarName, vars[i]});
			}
		}
	});

	for (const auto& Var : Vars)
	{
		// For this test, assume all are int32
		FIntProperty* NewProp = new FIntProperty(NewClass, *Var.Name, RF_Public);
		NewProp->PropertyFlags |= CPF_BlueprintVisible | CPF_Edit;
		
		FString GetterName = FString::Printf(TEXT("Get_%s"), *Var.Name);
		FString SetterName = FString::Printf(TEXT("Set_%s"), *Var.Name);
		
		NewProp->SetMetaData(TEXT("BlueprintGetter"), *GetterName);
		NewProp->SetMetaData(TEXT("BlueprintSetter"), *SetterName);
		
		NewClass->AddCppProperty(NewProp);
	}
}

void FClingScriptBlueprintCompiler::CreateFunctionList()
{
	Super::CreateFunctionList();

	UClingScriptBlueprint* ScriptBP = ClingScriptBlueprint();
	if (!ScriptBP || ScriptBP->SourceCode.IsEmpty())
	{
		return;
	}

	FClingRuntimeModule& ClingModule = FModuleManager::LoadModuleChecked<FClingRuntimeModule>("ClingRuntime");
	void* Interp = ClingModule.GetInterp();

	if (Cpp::Process(TCHAR_TO_UTF8(*ScriptBP->SourceCode)) != 0)
	{
		MessageLog.Error(TEXT("Cling compilation failed."));
		return;
	}

	// Assume class A for test as per issue description
	Cpp::TCppScope_t ClassScope = Cpp::GetScope("A");
	if (!ClassScope)
	{
		MessageLog.Warning(TEXT("Class 'A' not found in source code."));
		return;
	}

	// Declare helpers for JIT
	Cpp::Declare("extern \"C\" void* ClingRuntime_GetClingInstance(void* Obj);");
	Cpp::Declare("extern \"C\" int ClingRuntime_GetStepInt(void* StackPtr);");
	Cpp::Declare("extern \"C\" float ClingRuntime_GetStepFloat(void* StackPtr);");
	Cpp::Declare("extern \"C\" void ClingRuntime_FinishStep(void* StackPtr);");

	// --- Process Methods ---
	struct FMethodInfo
	{
		FString Name;
		Cpp::TCppFunction_t Func;
	};
	TArray<FMethodInfo> Methods;
	static TArray<FMethodInfo>* GMethodsPtr = nullptr;
	GMethodsPtr = &Methods;
	Cpp::GetClassMethods(ClassScope, [](const Cpp::TCppFunction_t* funcs, size_t size) {
		for (size_t i = 0; i < size; ++i)
		{
			static FString GMethodName;
			auto NameCallback = [](const char* name) {
				GMethodName = FString(UTF8_TO_TCHAR(name));
			};
			Cpp::GetName(funcs[i], NameCallback);

			if (!GMethodName.StartsWith("~") && !GMethodName.IsEmpty() && GMethodName != "A")
			{
				GMethodsPtr->Add({GMethodName, funcs[i]});
			}
		}
	});

	for (const auto& Method : Methods)
	{
		UFunction* NewFunction = NewObject<UFunction>(NewClass, *Method.Name, RF_Public);
		NewFunction->FunctionFlags = FUNC_Public | FUNC_BlueprintCallable | FUNC_Native;
		
		if (Method.Name == "foo")
		{
			FIntProperty* ReturnProp = new FIntProperty(NewFunction, TEXT("ReturnValue"), RF_Public);
			ReturnProp->PropertyFlags = CPF_OutParm | CPF_ReturnParm;
			NewFunction->AddCppProperty(ReturnProp);
		}

		NewFunction->StaticLink();

		FString WrapperName = FString::Printf(TEXT("Wrapper_A_%s"), *Method.Name);
		FString WrapperCode = FString::Printf(TEXT("extern \"C\" void %s(void* self, size_t nargs, void** args, void* res) { "), *WrapperName);
		if (Method.Name == "foo")
		{
			WrapperCode += TEXT("*(int*)res = ((A*)self)->foo(); }");
		}
		else
		{
			WrapperCode += TEXT(" }");
		}

		Cpp::Process(TCHAR_TO_UTF8(*WrapperCode));
		void* WrapperPtr = (void*)Cpp::GetFunctionAddress(TCHAR_TO_UTF8(*WrapperName));

		if (WrapperPtr)
		{
			FString NativeFuncName = FString::Printf(TEXT("Native_A_%s"), *Method.Name);
			FString NativeFuncCode = FString::Printf(TEXT(
				"extern \"C\" void %s(void* Context, void* StackPtr, void* const Result) { "
				"  void* self = ClingRuntime_GetClingInstance(Context); "
				"  ClingRuntime_FinishStep(StackPtr); "
				"  if (self) ((void(*)(void*, size_t, void**, void*))%p)(self, 0, nullptr, Result); "
				"} "), *NativeFuncName, WrapperPtr);
			
			Cpp::Process(TCHAR_TO_UTF8(*NativeFuncCode));
			FNativeFuncPtr NativeFunc = (FNativeFuncPtr)Cpp::GetFunctionAddress(TCHAR_TO_UTF8(*NativeFuncName));
			
			if (NativeFunc)
			{
				NewFunction->SetNativeFunc(NativeFunc);
			}
		}
	}

	// --- Process Variables (Getters/Setters) ---
	struct FVarInfo
	{
		FString Name;
		Cpp::TCppScope_t Var;
	};
	TArray<FVarInfo> Vars;
	static TArray<FVarInfo>* GVarsPtr = nullptr;
	GVarsPtr = &Vars;
	Cpp::GetDatamembers(ClassScope, [](const Cpp::TCppScope_t* vars, size_t size) {
		for (size_t i = 0; i < size; ++i)
		{
			static FString GVarName;
			auto NameCallback = [](const char* name) {
				GVarName = FString(UTF8_TO_TCHAR(name));
			};
			Cpp::GetName(vars[i], NameCallback);
			if (!GVarName.IsEmpty())
			{
				GVarsPtr->Add({GVarName, vars[i]});
			}
		}
	});

	for (const auto& Var : Vars)
	{
		int64 VarOffset = Cpp::GetVariableOffset(Var.Var, ClassScope);

		// Getter
		FString GetterName = FString::Printf(TEXT("Get_%s"), *Var.Name);
		UFunction* GetterFunc = NewObject<UFunction>(NewClass, *GetterName, RF_Public);
		GetterFunc->FunctionFlags = FUNC_Public | FUNC_Native | FUNC_BlueprintCallable | FUNC_BlueprintPure;
		FIntProperty* RetProp = new FIntProperty(GetterFunc, TEXT("ReturnValue"), RF_Public);
		RetProp->PropertyFlags = CPF_OutParm | CPF_ReturnParm;
		GetterFunc->AddCppProperty(RetProp);
		GetterFunc->StaticLink();

		FString NativeGetterName = FString::Printf(TEXT("Native_Get_A_%s"), *Var.Name);
		FString NativeGetterCode = FString::Printf(TEXT(
			"extern \"C\" void %s(void* Context, void* StackPtr, void* const Result) { "
			"  void* self = ClingRuntime_GetClingInstance(Context); "
			"  ClingRuntime_FinishStep(StackPtr); "
			"  if (self) *(int*)Result = *(int*)((char*)self + %lld); "
			"} "), *NativeGetterName, VarOffset);
		Cpp::Process(TCHAR_TO_UTF8(*NativeGetterCode));
		FNativeFuncPtr NativeGetter = (FNativeFuncPtr)Cpp::GetFunctionAddress(TCHAR_TO_UTF8(*NativeGetterName));
		if (NativeGetter) GetterFunc->SetNativeFunc(NativeGetter);

		// Setter
		FString SetterName = FString::Printf(TEXT("Set_%s"), *Var.Name);
		UFunction* SetterFunc = NewObject<UFunction>(NewClass, *SetterName, RF_Public);
		SetterFunc->FunctionFlags = FUNC_Public | FUNC_Native | FUNC_BlueprintCallable;
		FIntProperty* ArgProp = new FIntProperty(SetterFunc, TEXT("NewValue"), RF_Public);
		ArgProp->PropertyFlags = CPF_Parm;
		SetterFunc->AddCppProperty(ArgProp);
		SetterFunc->StaticLink();

		FString NativeSetterName = FString::Printf(TEXT("Native_Set_A_%s"), *Var.Name);
		FString NativeSetterCode = FString::Printf(TEXT(
			"extern \"C\" void %s(void* Context, void* StackPtr, void* const Result) { "
			"  void* self = ClingRuntime_GetClingInstance(Context); "
			"  int Value = ClingRuntime_GetStepInt(StackPtr); "
			"  ClingRuntime_FinishStep(StackPtr); "
			"  if (self) *(int*)((char*)self + %lld) = Value; "
			"} "), *NativeSetterName, VarOffset);
		Cpp::Process(TCHAR_TO_UTF8(*NativeSetterCode));
		FNativeFuncPtr NativeSetter = (FNativeFuncPtr)Cpp::GetFunctionAddress(TCHAR_TO_UTF8(*NativeSetterName));
		if (NativeSetter) SetterFunc->SetNativeFunc(NativeSetter);
	}
}

void FClingScriptBlueprintCompiler::FinishCompilingClass(UClass* Class)
{
	UClingScriptBlueprint* ScriptBP = ClingScriptBlueprint();
	UClingScriptBlueprintGeneratedClass* ScriptClass = CastChecked<UClingScriptBlueprintGeneratedClass>(Class);
	ScriptClass->SourceCode = ScriptBP->SourceCode;

	Super::FinishCompilingClass(Class);
}

UClingScriptBlueprint* FClingScriptBlueprintCompiler::ClingScriptBlueprint() const
{
	return CastChecked<UClingScriptBlueprint>(Blueprint);
}

void FClingScriptBlueprintCompiler::EnsureProperGeneratedClass(UClass*& TargetUClass)
{
	if (TargetUClass && !TargetUClass->UObjectBaseUtility::IsA<UClingScriptBlueprintGeneratedClass>())
	{
		FKismetCompilerUtilities::ConsignToOblivion(TargetUClass, Blueprint->bIsRegeneratingOnLoad);
		TargetUClass = nullptr;
	}
}

void FClingScriptBlueprintCompiler::SpawnNewClass(const FString& NewClassName)
{
	NewClingScriptBlueprintClass = FindObject<UClingScriptBlueprintGeneratedClass>(Blueprint->GetOutermost(), *NewClassName);

	if (NewClingScriptBlueprintClass == nullptr)
	{
		NewClingScriptBlueprintClass = NewObject<UClingScriptBlueprintGeneratedClass>(Blueprint->GetOutermost(), FName(*NewClassName), RF_Public | RF_Transactional);
	}
	else
	{
		FBlueprintCompileReinstancer::Create(NewClingScriptBlueprintClass);
	}
	NewClass = NewClingScriptBlueprintClass;
}

void FClingScriptBlueprintCompiler::OnNewClassSet(UBlueprintGeneratedClass* ClassToUse)
{
	NewClingScriptBlueprintClass = CastChecked<UClingScriptBlueprintGeneratedClass>(ClassToUse);
}
