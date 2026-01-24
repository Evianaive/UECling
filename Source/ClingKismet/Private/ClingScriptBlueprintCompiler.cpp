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
#include "Internationalization/Regex.h"
#include "ClingKismetLog.h"


namespace ClingCompilerUtils
{
    static FString GLastResultString;
    static void StringCallback(const char* InString)
    {
        GLastResultString = UTF8_TO_TCHAR(InString);
    }

    FString GetName(Cpp::TCppScope_t Scope)
    {
        GLastResultString.Empty();
        Cpp::GetName(Scope, &StringCallback);
        return GLastResultString;
    }

    FString GetTypeString(Cpp::TCppType_t Type)
    {
        GLastResultString.Empty();
        Cpp::GetTypeAsString(Type, &StringCallback);
        return GLastResultString;
    }

    FString GetArgName(Cpp::TCppFunction_t Func, int Index)
    {
        GLastResultString.Empty();
        Cpp::GetFunctionArgName(Func, Index, &StringCallback);
        return GLastResultString;
    }

    FProperty* CreatePropertyFromCppType(Cpp::TCppType_t Type, UObject* Outer, FName Name, int32 Offset)
    {
        FString TypeName = GetTypeString(Type);
        
        const EObjectFlags ObjectFlags = RF_Public;
        const EPropertyFlags PropFlags = CPF_BlueprintVisible | CPF_Edit;

        if (TypeName == TEXT("int") || TypeName == TEXT("int32_t"))
        {
            UECodeGen_Private::FIntPropertyParams Params = { TCHAR_TO_UTF8(*Name.ToString()), nullptr, PropFlags, UECodeGen_Private::EPropertyGenFlags::Int, ObjectFlags, nullptr, nullptr, 1, (uint16)Offset, METADATA_PARAMS(0, nullptr) };
            return new FIntProperty(Outer, Params);
        }
        if (TypeName == TEXT("float"))
        {
            UECodeGen_Private::FFloatPropertyParams Params = { TCHAR_TO_UTF8(*Name.ToString()), nullptr, PropFlags, UECodeGen_Private::EPropertyGenFlags::Float, ObjectFlags, nullptr, nullptr, 1, (uint16)Offset, METADATA_PARAMS(0, nullptr) };
            return new FFloatProperty(Outer, Params);
        }
        if (TypeName == TEXT("double"))
        {
            UECodeGen_Private::FDoublePropertyParams Params = { TCHAR_TO_UTF8(*Name.ToString()), nullptr, PropFlags, UECodeGen_Private::EPropertyGenFlags::Double, ObjectFlags, nullptr, nullptr, 1, (uint16)Offset, METADATA_PARAMS(0, nullptr) };
            return new FDoubleProperty(Outer, Params);
        }
        if (TypeName == TEXT("bool"))
        {
            UECodeGen_Private::FBoolPropertyParams Params = { TCHAR_TO_UTF8(*Name.ToString()), nullptr, PropFlags, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, ObjectFlags, nullptr, nullptr, 1, sizeof(bool), (uint16)Offset, nullptr, METADATA_PARAMS(0, nullptr) };
            return new FBoolProperty(Outer, Params);
        }
        if (TypeName == TEXT("int64_t") || TypeName == TEXT("long long"))
        {
            UECodeGen_Private::FInt64PropertyParams Params = { TCHAR_TO_UTF8(*Name.ToString()), nullptr, PropFlags, UECodeGen_Private::EPropertyGenFlags::Int64, ObjectFlags, nullptr, nullptr, 1, (uint16)Offset, METADATA_PARAMS(0, nullptr) };
            return new FInt64Property(Outer, Params);
        }
        if (TypeName == TEXT("uint8_t") || TypeName == TEXT("unsigned char"))
        {
            UECodeGen_Private::FBytePropertyParams Params = { TCHAR_TO_UTF8(*Name.ToString()), nullptr, PropFlags, UECodeGen_Private::EPropertyGenFlags::Byte, ObjectFlags, nullptr, nullptr, 1, (uint16)Offset, nullptr, METADATA_PARAMS(0, nullptr) };
            return new FByteProperty(Outer, Params);
        }
        return nullptr;
    }

    FString GetClingGetStepFunc(Cpp::TCppType_t Type)
    {
        FString TypeName = GetTypeString(Type);
        if (TypeName == TEXT("int") || TypeName == TEXT("int32_t")) return TEXT("ClingRuntime_GetStepInt");
        if (TypeName == TEXT("float")) return TEXT("ClingRuntime_GetStepFloat");
        if (TypeName == TEXT("double")) return TEXT("ClingRuntime_GetStepDouble");
        if (TypeName == TEXT("bool")) return TEXT("ClingRuntime_GetStepBool");
        if (TypeName == TEXT("int64_t") || TypeName == TEXT("long long")) return TEXT("ClingRuntime_GetStepInt64");
        if (TypeName == TEXT("uint8_t") || TypeName == TEXT("unsigned char")) return TEXT("ClingRuntime_GetStepByte");
        return TEXT("");
    }

    FString FindClassName(UClingScriptBlueprint* BP)
    {
        FString BPName = BP->GetName();
        if (Cpp::GetScope(TCHAR_TO_UTF8(*BPName)))
        {
            return BPName;
        }

        FRegexPattern Pattern(TEXT("class\\s+([A-Za-z_]\\w*)"));
        FRegexMatcher Matcher(Pattern, BP->SourceCode);
        if (Matcher.FindNext())
        {
            return Matcher.GetCaptureGroup(1);
        }

        return TEXT("A");
    }
}

FClingScriptBlueprintCompiler::FClingScriptBlueprintCompiler(UClingScriptBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions)
	: Super(SourceSketch, InMessageLog, InCompilerOptions)
	, NewClingScriptBlueprintClass(nullptr)
{
	UE_LOG(LogKismetCling, Log, TEXT("FClingScriptBlueprintCompiler initialized for %s"), *SourceSketch->GetName());
}

FClingScriptBlueprintCompiler::~FClingScriptBlueprintCompiler()
{
}

void FClingScriptBlueprintCompiler::CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& InOldCDO)
{
	UE_LOG(LogKismetCling, Log, TEXT("CleanAndSanitizeClass: %s"), *ClassToClean->GetName());
	Super::CleanAndSanitizeClass(ClassToClean, InOldCDO);
}

void FClingScriptBlueprintCompiler::PrecompileFunction(FKismetFunctionContext& Context,
	EInternalCompilerFlags InternalFlags)
{
	UE_LOG(LogKismetCling, Log, TEXT("PrecompileFunction: %s"), *GetNameSafe(Context.Function));
	FKismetCompilerContext::PrecompileFunction(Context, InternalFlags);
}

void FClingScriptBlueprintCompiler::SetCalculatedMetaDataAndFlags(UFunction* Function, UK2Node_FunctionEntry* EntryNode,
	const UEdGraphSchema_K2* InSchema)
{
	FKismetCompilerContext::SetCalculatedMetaDataAndFlags(Function, EntryNode, InSchema);
}

bool FClingScriptBlueprintCompiler::ShouldForceKeepNode(const UEdGraphNode* Node) const
{
	return FKismetCompilerContext::ShouldForceKeepNode(Node);
}

void FClingScriptBlueprintCompiler::PostExpansionStep(const UEdGraph* Graph)
{
	FKismetCompilerContext::PostExpansionStep(Graph);
}

void FClingScriptBlueprintCompiler::PreCompileUpdateBlueprintOnLoad(UBlueprint* BP)
{
	FKismetCompilerContext::PreCompileUpdateBlueprintOnLoad(BP);
}

void FClingScriptBlueprintCompiler::CreateClassVariablesFromBlueprint()
{
	UE_LOG(LogKismetCling, Log, TEXT("CreateClassVariablesFromBlueprint started"));
	Super::CreateClassVariablesFromBlueprint();

	UClingScriptBlueprint* ScriptBP = ClingScriptBlueprint();
	if (!ScriptBP || ScriptBP->SourceCode.IsEmpty())
	{
		UE_LOG(LogKismetCling, Warning, TEXT("CreateClassVariablesFromBlueprint: SourceCode is empty."));
		return;
	}

	// JIT compile the source code
	if (ScriptBP->LastCompiledSourceCode != ScriptBP->SourceCode)
	{
		UE_LOG(LogKismetCling, Log, TEXT("JIT compiling source code..."));
		if (Cpp::Process(TCHAR_TO_UTF8(*ScriptBP->SourceCode)) != 0)
		{
			UE_LOG(LogKismetCling, Error, TEXT("JIT compilation failed!"));
			return;
		}
		ScriptBP->LastCompiledSourceCode = ScriptBP->SourceCode;
	}
	else
	{
		UE_LOG(LogKismetCling, Log, TEXT("Source code already JIT compiled, skipping."));
	}

	FString ClassName = ClingCompilerUtils::FindClassName(ScriptBP);
	UE_LOG(LogKismetCling, Log, TEXT("Found C++ class name: %s"), *ClassName);
	Cpp::TCppScope_t ClassScope = Cpp::GetScope(TCHAR_TO_UTF8(*ClassName));
	if (!ClassScope)
	{
		UE_LOG(LogKismetCling, Error, TEXT("Could not find scope for class %s"), *ClassName);
		return;
	}

	// 1. Add ClingInstance pointer property (stored as Int64)
	// UE_LOG(LogKismetCling, Log, TEXT("Adding ClingInstance property to %s"), *NewClass->GetName());
	// {
	// 	UECodeGen_Private::FInt64PropertyParams Params = { "ClingInstance", nullptr, CPF_BlueprintVisible | CPF_Edit | CPF_Transient | CPF_DuplicateTransient, UECodeGen_Private::EPropertyGenFlags::Int64, RF_Public, nullptr, nullptr, 1, (uint16)NewClass->GetSuperClass()->GetPropertiesSize(), METADATA_PARAMS(0, nullptr) };
	// 	FInt64Property* ClingInstanceProp = new FInt64Property(NewClass, Params);
	// 	// NewClass->AddCppProperty(ClingInstanceProp);
	// }

	// 2. Add properties for C++ class members
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

	UE_LOG(LogKismetCling, Log, TEXT("Processing %d C++ data members"), Vars.Num());
	int32 SuperClassSize = NewClass->GetSuperClass()->GetPropertiesSize();
	int32 ClingInstanceOffset = (int32)Align(SuperClassSize, sizeof(void*));
	
	// 1. Add ClingInstance pointer property (stored as Int64)
	UE_LOG(LogKismetCling, Log, TEXT("Adding ClingInstance property to %s at offset %d"), *NewClass->GetName(), ClingInstanceOffset);
	{
		UECodeGen_Private::FInt64PropertyParams Params = { "ClingInstance", nullptr, CPF_BlueprintVisible | CPF_Edit | CPF_Transient | CPF_DuplicateTransient, UECodeGen_Private::EPropertyGenFlags::Int64, RF_Public, nullptr, nullptr, 1, (uint16)ClingInstanceOffset, METADATA_PARAMS(0, nullptr) };
		FInt64Property* ClingInstanceProp = new FInt64Property(NewClass, Params);
		// NewClass->AddCppProperty(ClingInstanceProp);
	}

	int32 BaseOffset = ClingInstanceOffset + sizeof(void*);
	for (const auto& Var : Vars)
	{
		Cpp::TCppType_t VarType = Cpp::GetVariableType(Var.Var);
		intptr_t ClingOffset = Cpp::GetVariableOffset(Var.Var, ClassScope);
		int32 TotalOffset = BaseOffset + (int32)ClingOffset;

		FProperty* NewProp = ClingCompilerUtils::CreatePropertyFromCppType(VarType, NewClass, *Var.Name, TotalOffset);
		
		if (NewProp)
		{
			// NewClass->AddCppProperty(NewProp);
			UE_LOG(LogKismetCling, Log, TEXT("Added property %s (TotalOffset: %d)"), *Var.Name, TotalOffset);
		}
		else
		{
			UE_LOG(LogKismetCling, Warning, TEXT("Failed to create property for %s (type unknown or unsupported)"), *Var.Name);
		}
	}
}

void FClingScriptBlueprintCompiler::CreateFunctionList()
{
	UE_LOG(LogKismetCling, Log, TEXT("CreateFunctionList started"));
	Super::CreateFunctionList();

	UClingScriptBlueprint* ScriptBP = ClingScriptBlueprint();
	if (!ScriptBP || ScriptBP->SourceCode.IsEmpty())
	{
		UE_LOG(LogKismetCling, Warning, TEXT("CreateFunctionList: SourceCode is empty."));
		return;
	}

	FString ClassName = ClingCompilerUtils::FindClassName(ScriptBP);
	Cpp::TCppScope_t ClassScope = Cpp::GetScope(TCHAR_TO_UTF8(*ClassName));
	if (!ClassScope)
	{
		UE_LOG(LogKismetCling, Error, TEXT("CreateFunctionList: Could not find scope for class %s"), *ClassName);
		return;
	}

	// Declare helper functions in JIT for the wrappers
	UE_LOG(LogKismetCling, Log, TEXT("Declaring helper functions in JIT..."));
	Cpp::Declare(
		"extern \"C\" void* ClingRuntime_GetClingInstance(void* Obj);"
		"extern \"C\" int ClingRuntime_GetStepInt(void* StackPtr);"
		"extern \"C\" float ClingRuntime_GetStepFloat(void* StackPtr);"
		"extern \"C\" double ClingRuntime_GetStepDouble(void* StackPtr);"
		"extern \"C\" long long ClingRuntime_GetStepInt64(void* StackPtr);"
		"extern \"C\" bool ClingRuntime_GetStepBool(void* StackPtr);"
		"extern \"C\" unsigned char ClingRuntime_GetStepByte(void* StackPtr);"
		"extern \"C\" void ClingRuntime_FinishStep(void* StackPtr);"
	);

	// --- 1. Process Methods ---
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

			if (!Cpp::IsConstructor(funcs[i]) && !Cpp::IsDestructor(funcs[i]) && !GMethodName.IsEmpty() && !GMethodName.StartsWith(TEXT("operator")))
			{
				GMethodsPtr->Add({GMethodName, funcs[i]});
			}
		}
	});

	UE_LOG(LogKismetCling, Log, TEXT("Processing %d C++ methods"), Methods.Num());
	int32 MethodsBaseOffset = NewClass->GetSuperClass()->GetPropertiesSize() + sizeof(void*) + (int32)Align(NewClass->GetSuperClass()->GetPropertiesSize(), sizeof(void*)) - NewClass->GetSuperClass()->GetPropertiesSize();
	// Actually just use the same logic as variables
	int32 MethodsSuperClassSize = NewClass->GetSuperClass()->GetPropertiesSize();
	int32 MethodsClingInstanceOffset = (int32)Align(MethodsSuperClassSize, sizeof(void*));
	int32 MethodsBaseOffsetActual = MethodsClingInstanceOffset + sizeof(void*);

	for (const auto& Method : Methods)
	{
		UE_LOG(LogKismetCling, Log, TEXT("Generating UFunction and Wrapper for method: %s"), *Method.Name);
		
		int NumArgs = Cpp::GetFunctionNumArgs(Method.Func);
		Cpp::TCppType_t RetType = Cpp::GetFunctionReturnType(Method.Func);
		FString RetTypeStr = ClingCompilerUtils::GetTypeString(RetType);

		// Prepare Function Params
		TArray<FProperty*> FuncProperties;
		
		if (RetTypeStr != TEXT("void"))
		{
			FProperty* ReturnProp = ClingCompilerUtils::CreatePropertyFromCppType(RetType, nullptr, TEXT("ReturnValue"), 0);
			if (ReturnProp)
			{
				ReturnProp->PropertyFlags |= CPF_OutParm | CPF_ReturnParm;
				FuncProperties.Add(ReturnProp);
			}
		}

		for (int i = 0; i < NumArgs; ++i)
		{
			Cpp::TCppType_t ArgType = Cpp::GetFunctionArgType(Method.Func, i);
			FString ArgName = ClingCompilerUtils::GetArgName(Method.Func, i);
			if (ArgName.IsEmpty()) ArgName = FString::Printf(TEXT("Arg%d"), i);
			
			FProperty* ArgProp = ClingCompilerUtils::CreatePropertyFromCppType(ArgType, nullptr, *ArgName, 0);
			if (ArgProp)
			{
				ArgProp->PropertyFlags |= CPF_Parm;
				FuncProperties.Add(ArgProp);
			}
		}

		// Calculate total structure size for the function
		uint32 StructureSize = 0;
		for (FProperty* Prop : FuncProperties)
		{
			// Align offset
			StructureSize = (uint32)Align(StructureSize, Prop->GetMinAlignment());
			// Use LinkWithoutChangingOffset to set internal state without messing up our calculated offset
			Prop->LinkWithoutChangingOffset(*(FArchive*)nullptr);
			// We can't easily set Offset_Internal if it's private, but StaticLink usually does it.
			// However, if we use UECodeGen_Private::ConstructUFunction, it might handle it if we provide PropPointers.
			// But we are adding them manually.
			StructureSize += (uint32)Prop->GetSize();
		}

		UECodeGen_Private::FFunctionParams Params = {};
		Params.OuterFunc = nullptr; 
		Params.SuperFunc = nullptr;
		Params.NameUTF8 = TCHAR_TO_UTF8(*Method.Name);
		Params.FunctionFlags = FUNC_Public | FUNC_BlueprintCallable | FUNC_Native;
		Params.NumProperties = FuncProperties.Num();
		Params.StructureSize = (uint16)StructureSize;
		Params.ObjectFlags = RF_Public;

		UFunction* NewFunction = nullptr;
		// Use the version that takes OutFunction reference
		UECodeGen_Private::ConstructUFunction(&NewFunction, Params);
		
		// Add properties to the function
		// for (FProperty* Prop : FuncProperties)
		// {
		// 	NewFunction->AddCppProperty(Prop);
		// }

		NewFunction->StaticLink();

		// JIT Wrapper
		FString WrapperName = FString::Printf(TEXT("Wrapper_%s_%s"), *ClassName, *Method.Name);
		FString WrapperCode = FString::Printf(TEXT("extern \"C\" void %s(void* self, size_t nargs, void** args, void* res) { "), *WrapperName);
		if (RetTypeStr != TEXT("void"))
		{
			WrapperCode += FString::Printf(TEXT("*(%s*)res = "), *RetTypeStr);
		}
		WrapperCode += FString::Printf(TEXT("((%s*)self)->%s("), *ClassName, *Method.Name);
		for (int i = 0; i < NumArgs; ++i)
		{
			Cpp::TCppType_t ArgType = Cpp::GetFunctionArgType(Method.Func, i);
			FString ArgTypeStr = ClingCompilerUtils::GetTypeString(ArgType);
			WrapperCode += FString::Printf(TEXT("*(%s*)args[%d]"), *ArgTypeStr, i);
			if (i < NumArgs - 1) WrapperCode += TEXT(", ");
		}
		WrapperCode += TEXT("); }");

		Cpp::Process(TCHAR_TO_UTF8(*WrapperCode));
		void* WrapperPtr = (void*)Cpp::GetFunctionAddress(TCHAR_TO_UTF8(*WrapperName));

		if (WrapperPtr)
		{
			UE_LOG(LogKismetCling, Log, TEXT("JIT Wrapper created for %s at address %p"), *Method.Name, WrapperPtr);
			FString NativeFuncName = FString::Printf(TEXT("Native_%s_%s"), *ClassName, *Method.Name);
			FString NativeFuncCode = FString::Printf(TEXT("extern \"C\" void %s(void* Context, void* StackPtr, void* const Result) { "), *NativeFuncName);
			NativeFuncCode += FString::Printf(TEXT("void* self = (char*)Context + %d; "), MethodsBaseOffsetActual); 
			
			TArray<FString> ArgVarNames;
			for (int i = 0; i < NumArgs; ++i)
			{
				Cpp::TCppType_t ArgType = Cpp::GetFunctionArgType(Method.Func, i);
				FString ArgTypeStr = ClingCompilerUtils::GetTypeString(ArgType);
				FString GetStepFunc = ClingCompilerUtils::GetClingGetStepFunc(ArgType);
				FString VarName = FString::Printf(TEXT("arg%d"), i);
				NativeFuncCode += FString::Printf(TEXT("%s %s = %s(StackPtr); "), *ArgTypeStr, *VarName, *GetStepFunc);
				ArgVarNames.Add(VarName);
			}
			NativeFuncCode += TEXT("ClingRuntime_FinishStep(StackPtr); ");
			NativeFuncCode += TEXT("if (self) { ");
			if (NumArgs > 0)
			{
				NativeFuncCode += FString::Printf(TEXT("void* args[%d] = { "), NumArgs);
				for (int i = 0; i < NumArgs; ++i)
				{
					NativeFuncCode += FString::Printf(TEXT("&%s"), *ArgVarNames[i]);
					if (i < NumArgs - 1) NativeFuncCode += TEXT(", ");
				}
				NativeFuncCode += TEXT(" }; ");
				NativeFuncCode += FString::Printf(TEXT("((void(*)(void*, size_t, void**, void*))%p)(self, %d, args, Result); "), WrapperPtr, NumArgs);
			}
			else
			{
				NativeFuncCode += FString::Printf(TEXT("((void(*)(void*, size_t, void**, void*))%p)(self, 0, nullptr, Result); "), WrapperPtr);
			}
			NativeFuncCode += TEXT("} }");
			
			Cpp::Process(TCHAR_TO_UTF8(*NativeFuncCode));
			FNativeFuncPtr NativeFunc = (FNativeFuncPtr)Cpp::GetFunctionAddress(TCHAR_TO_UTF8(*NativeFuncName));
			
			if (NativeFunc)
			{
				NewFunction->SetNativeFunc(NativeFunc);
				UE_LOG(LogKismetCling, Log, TEXT("Native bound for %s at address %p"), *Method.Name, NativeFunc);
			}
		}
		else
		{
			UE_LOG(LogKismetCling, Error, TEXT("Failed to create JIT Wrapper for method: %s"), *Method.Name);
		}
		
		NewClass->AddFunctionToFunctionMap(NewFunction, *Method.Name);
	}

	// --- 2. Process Variables (Generate Getters/Setters) ---
	// Only generate if we don't use direct offset access
	/*
	UE_LOG(LogKismetCling, Log, TEXT("Generating Getters/Setters for properties..."));
	...
	*/
}

void FClingScriptBlueprintCompiler::FinishCompilingClass(UClass* Class)
{
	UE_LOG(LogKismetCling, Log, TEXT("FinishCompilingClass started for %s"), *Class->GetName());
	UClingScriptBlueprint* ScriptBP = ClingScriptBlueprint();
	UClingScriptBlueprintGeneratedClass* ScriptClass = CastChecked<UClingScriptBlueprintGeneratedClass>(Class);
	ScriptClass->SourceCode = ScriptBP->SourceCode;

	// Finalize class layout
	UE_LOG(LogKismetCling, Log, TEXT("Binding and Linking class..."));
	Class->Bind();
	Class->StaticLink();
	Class->AssembleReferenceTokenStream();

	Super::FinishCompilingClass(Class);
	// Ff context property has been created, create a DSO and set it on the CDO
	if (ScriptBP->SourceCode.Len())
	{
		// Create CDO
		UE_LOG(LogKismetCling, Log, TEXT("Creating Class Default Object (CDO)..."));
		// Class->GetDefaultObject();
		UObject* CDO = Class->GetDefaultObject();
		// UObject* ContextDefaultSubobject = NewObject<UObject>(CDO, ContextProperty->PropertyClass, "ScriptContext", RF_DefaultSubObject | RF_Public);
		// ContextProperty->SetObjectPropertyValue(ContextProperty->ContainerPtrToValuePtr<UObject*>(CDO), ContextDefaultSubobject);
	}
	UE_LOG(LogKismetCling, Log, TEXT("FinishCompilingClass finished for %s"), *Class->GetName());
}

UClingScriptBlueprint* FClingScriptBlueprintCompiler::ClingScriptBlueprint() const
{
	return CastChecked<UClingScriptBlueprint>(Blueprint);
}

void FClingScriptBlueprintCompiler::EnsureProperGeneratedClass(UClass*& TargetUClass)
{
	if (TargetUClass && !TargetUClass->UObjectBaseUtility::IsA<UClingScriptBlueprintGeneratedClass>())
	{
		UE_LOG(LogKismetCling, Log, TEXT("Consigning improper class %s to oblivion"), *TargetUClass->GetName());
		FKismetCompilerUtilities::ConsignToOblivion(TargetUClass, Blueprint->bIsRegeneratingOnLoad);
		TargetUClass = nullptr;
	}
}

void FClingScriptBlueprintCompiler::SpawnNewClass(const FString& NewClassName)
{
	UE_LOG(LogKismetCling, Log, TEXT("SpawnNewClass: %s"), *NewClassName);
	NewClingScriptBlueprintClass = FindObject<UClingScriptBlueprintGeneratedClass>(Blueprint->GetOutermost(), *NewClassName);

	if (NewClingScriptBlueprintClass == nullptr)
	{
		UE_LOG(LogKismetCling, Log, TEXT("Creating new UClingScriptBlueprintGeneratedClass: %s"), *NewClassName);
		NewClingScriptBlueprintClass = NewObject<UClingScriptBlueprintGeneratedClass>(Blueprint->GetOutermost(), FName(*NewClassName), RF_Public | RF_Transactional);
	}
	else
	{
		UE_LOG(LogKismetCling, Log, TEXT("Reusing existing UClingScriptBlueprintGeneratedClass: %s, creating reinstancer"), *NewClassName);
		FBlueprintCompileReinstancer::Create(NewClingScriptBlueprintClass);
	}
	NewClass = NewClingScriptBlueprintClass;
}

void FClingScriptBlueprintCompiler::OnNewClassSet(UBlueprintGeneratedClass* ClassToUse)
{
	NewClingScriptBlueprintClass = CastChecked<UClingScriptBlueprintGeneratedClass>(ClassToUse);
}

void FClingScriptBlueprintCompiler::MergeUbergraphPagesIn(UEdGraph* Ubergraph)
{
	FKismetCompilerContext::MergeUbergraphPagesIn(Ubergraph);
}

void FClingScriptBlueprintCompiler::ProcessOneFunctionGraph(UEdGraph* SourceGraph, bool bInternalFunction)
{
	FKismetCompilerContext::ProcessOneFunctionGraph(SourceGraph, bInternalFunction);
}

void FClingScriptBlueprintCompiler::OnPostCDOCompiled(const UObject::FPostCDOCompiledContext& Context)
{
	UE_LOG(LogKismetCling, Log, TEXT("OnPostCDOCompiled for %s"), *NewClass->GetName());
	FKismetCompilerContext::OnPostCDOCompiled(Context);
}

void FClingScriptBlueprintCompiler::CopyTermDefaultsToDefaultObject(UObject* DefaultObject)
{
	UE_LOG(LogKismetCling, Log, TEXT("CopyTermDefaultsToDefaultObject for %s"), *DefaultObject->GetName());
	FKismetCompilerContext::CopyTermDefaultsToDefaultObject(DefaultObject);
}

void FClingScriptBlueprintCompiler::PostCompile()
{
	UE_LOG(LogKismetCling, Log, TEXT("PostCompile started"));
	FKismetCompilerContext::PostCompile();
}

void FClingScriptBlueprintCompiler::PostCompileDiagnostics()
{
	FKismetCompilerContext::PostCompileDiagnostics();
}
