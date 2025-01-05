

#include "RecoverCodeGeneration.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/StructureEditorUtils.h"


template<typename T>
concept AllowType =	std::is_same_v<std::_Remove_cvref_t<T>,FEdGraphPinType> || std::is_same_v<std::_Remove_cvref_t<T>,FEdGraphTerminalType>;

const FName& GetCategory(const FEdGraphPinType& Type){return Type.PinCategory;}
const FName& GetCategory(const FEdGraphTerminalType& Type){return Type.TerminalCategory;}
const FName& GetSubCategory(const FEdGraphPinType& Type){return Type.PinSubCategory;}
const FName& GetSubCategory(const FEdGraphTerminalType& Type){return Type.TerminalSubCategory;}
UObject* GetSubCategoryObject(const FEdGraphPinType& Type){return Type.PinSubCategoryObject.Get();}
UObject* GetSubCategoryObject(const FEdGraphTerminalType& Type){return Type.TerminalSubCategoryObject.Get();}
bool GetIsWeakPointer(const FEdGraphPinType& Type){return Type.bIsWeakPointer;}
bool GetIsWeakPointer(const FEdGraphTerminalType& Type){return Type.bTerminalIsWeakPointer;}

template<AllowType T>
void GetCppTypeFromPinType(FString& InOutString, const FName& ValidatedPropertyName, T& Type, UClass* SelfClass = UObject::StaticClass())
{
	const FName& PinCategory = GetCategory(Type);
	const FName& PinSubCategory = GetSubCategory(Type);
	UObject* PinSubCategoryObject = GetSubCategoryObject(Type);
	bool bIsWeakPointer = GetIsWeakPointer(Type);
	if ((PinCategory == UEdGraphSchema_K2::PC_Object) || (PinCategory == UEdGraphSchema_K2::PC_Interface) || (PinCategory == UEdGraphSchema_K2::PC_SoftObject))
	{
		UClass* SubType = (PinSubCategory == UEdGraphSchema_K2::PSC_Self) ? SelfClass : Cast<UClass>(PinSubCategoryObject);

		if (SubType == nullptr)
		{
			// If this is from a degenerate pin, because the object type has been removed, default this to a UObject subtype so we can make a dummy term for it to allow the compiler to continue
			SubType = UObject::StaticClass();
		}

		if (SubType)
		{
			const bool bIsInterface = SubType->HasAnyClassFlags(CLASS_Interface)
				|| ((SubType == SelfClass)
					// Todo We can't get the blueprint class that use thi node before compile
					// Todo But here Self Pin and UObject* will pass bIsInterface now! Fix it 
					// && ensure(SelfClass->ClassGeneratedBy) && FBlueprintEditorUtils::IsInterfaceBlueprint(CastChecked<UBlueprint>(SelfClass->ClassGeneratedBy))
					);

			if (bIsInterface)
			{
				// FInterfaceProperty
				while (SubType && !SubType->HasAnyClassFlags(CLASS_Native))
				{
					SubType = SubType->GetSuperClass();
				}				
				check(SubType);
				if(SubType->HasAnyClassFlags(CLASS_Interface))
				{
					InOutString.Append(FString::Printf(TEXT("TScriptInterface<I%s>"), *SubType->GetName()));
				}
				else
				{
					InOutString.Append(TEXT("UObject*"));
				}
			}
			else
			{
				if (PinCategory == UEdGraphSchema_K2::PC_SoftObject)
				{
					InOutString.Append(FString::Printf(TEXT("TSoftObjectPtr<%s%s>"), SubType->GetPrefixCPP(),*SubType->GetName()));
				}
				else if (bIsWeakPointer)
				{
					InOutString.Append(FString::Printf(TEXT("TWeakObjectPtr<%s%s>"), SubType->GetPrefixCPP(),*SubType->GetName()));
				}
				else
				{
					if (FLinkerLoad::IsImportLazyLoadEnabled())
					{
						InOutString.Append(FString::Printf(TEXT("TObjectPtr<%s%s>"), SubType->GetPrefixCPP(),*SubType->GetName()));
						// NewPropertyObj->SetPropertyFlags(CPF_TObjectPtrWrapper);
					}
					else
					{
						if(SubType->IsNative())
							InOutString.Append(FString::Printf(TEXT("%s%s*"), SubType->GetPrefixCPP(),*SubType->GetName()));
						else
							InOutString.Append(TEXT("UObject*"));
					}
				}
			}
		}
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		if (UScriptStruct* SubType = Cast<UScriptStruct>(PinSubCategoryObject))
		{
			FString StructureError;
			if (FStructureEditorUtils::EStructureError::Ok == FStructureEditorUtils::IsStructureValid(SubType, nullptr, &StructureError))
			{
				InOutString.Append(SubType->GetStructCPPName());
			}
			else
			{
				UE_LOG(LogTemp,Warning,TEXT("Invalid property '%s' structure '%s' error: %s"),
						*ValidatedPropertyName.ToString(),
						*SubType->GetName(),
						*StructureError);
			}
		}
	}
	else if ((PinCategory == UEdGraphSchema_K2::PC_Class) || (PinCategory == UEdGraphSchema_K2::PC_SoftClass))
	{
		UClass* SubType = Cast<UClass>(PinSubCategoryObject);
		if (SubType == nullptr)
		{
			// If this is from a degenerate pin, because the object type has been removed, default this to a UObject subtype so we can make a dummy term for it to allow the compiler to continue
			SubType = UObject::StaticClass();
			UE_LOG(LogTemp,Warning,TEXT("Invalid property '%s' class, replaced with Object.  Please fix or remove."),*ValidatedPropertyName.ToString());
		}
		if (SubType)
		{
			if (PinCategory == UEdGraphSchema_K2::PC_SoftClass)
			{
				InOutString.Append(FString::Printf(TEXT("TSoftClassPtr<%s%s>"),SubType->GetPrefixCPP(),*SubType->GetName()));
			}
			else
			{
				// FClassProperty*
				InOutString.Append(TEXT("UClass*"));
			}
		}
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_Int)
	{
		InOutString.Append(TEXT("int32"));
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_Int64)
	{
		InOutString.Append(TEXT("int64"));
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_Real)
	{
		if(PinSubCategory == UEdGraphSchema_K2::PC_Float || PinSubCategory ==  UEdGraphSchema_K2::PC_Double)
		{
			InOutString.Append(PinSubCategory.ToString());
		}
		else
		{
			checkf(false, TEXT("Erroneous pin subcategory for PC_Real: %s"), *PinSubCategory.ToString());	
		}			
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_Boolean)
	{
		InOutString.Append(TEXT("bool"));
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_String)
	{
		InOutString.Append(TEXT("FString"));
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_Text)
	{
		InOutString.Append(TEXT("FText"));
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_Byte)
	{
		UEnum* Enum = Cast<UEnum>(PinSubCategoryObject);

		if (Enum && Enum->GetCppForm() == UEnum::ECppForm::EnumClass)
		{
			InOutString.Append(TEXT("F")+Enum->GetName());
		}
		else
		{
			InOutString.Append(TEXT("uint64"));
		}
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_Name)
	{
		InOutString.Append(TEXT("FName"));
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_FieldPath)
	{
		InOutString.Append(TEXT("TFieldPath<FProperty>"));
	}
	else
	{
		// Failed to resolve the type-subtype, create a generic property to survive VM bytecode emission
		InOutString.Append(TEXT("UNKNOWN_TYPE"));
	}
}
template void GetCppTypeFromPinType(FString& InOutString, const FName& ValidatedPropertyName, FEdGraphPinType& Type, UClass* SelfClass);
template void GetCppTypeFromPinType(FString& InOutString, const FName& ValidatedPropertyName, FEdGraphTerminalType& Type, UClass* SelfClass);

FString FRecoverCodeGeneration::GetCppTypeOfProperty(const FProperty* Property)
{
	FString CppType = Property->GetCPPType();
	if(const FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		if(!ObjProp->PropertyClass->IsNative())
		{
			CppType = TEXT("UObject*");
		}
	}
	else if(const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		CppType += TEXT("<")+GetCppTypeOfProperty(ArrayProp->Inner)+TEXT(">");
	}
	else if(const FSetProperty* SetProp = CastField<FSetProperty>(Property))
	{
		CppType += TEXT("<")+GetCppTypeOfProperty(SetProp->ElementProp)+TEXT(">");
	}
	else if(const FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		CppType += TEXT("<")+GetCppTypeOfProperty(MapProp->KeyProp)+TEXT(",")+GetCppTypeOfProperty(MapProp->ValueProp)+TEXT(">");
	}
	return CppType;
}

void FRecoverCodeGeneration::GenerateStructCode(const UScriptStruct* Struct, FString& InOutDeclare)
{
	if(IsValid(Struct) && !Struct->IsNative())
	{
		auto SuperStruct = Cast<UScriptStruct>(Struct->GetSuperStruct());
		GenerateStructCode(SuperStruct,InOutDeclare);
		for (const FProperty* ChildProperty : TFieldRange<FProperty>(Struct))
		{
			RecursivelyGenerateDeclareOfNonNativeProp(ChildProperty,InOutDeclare);
		}
		FString StructName = Struct->GetStructCPPName();
		if(SuperStruct)
			StructName +=TEXT(" : public ") + SuperStruct->GetStructCPPName();
		
		InOutDeclare += FString::Printf(TEXT("struct %s{\n"),*StructName);	
		
		for (const FProperty* ChildProperty : TFieldRange<FProperty>(Struct))
		{
			InOutDeclare += FString::Printf(TEXT("%s %s;\n"),*GetCppTypeOfProperty(ChildProperty),*ChildProperty->GetAuthoredName());
		}
		InOutDeclare += TEXT("};\n");
	}
}

void FRecoverCodeGeneration::RecursivelyGenerateDeclareOfNonNativeProp(
	const FProperty* Property,
	FString& InOutDeclare)
{
	if(auto StructProperty = CastField<FStructProperty>(Property))
	{
		GenerateStructCode(StructProperty->Struct,InOutDeclare);
	}
	else if(const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		RecursivelyGenerateDeclareOfNonNativeProp(ArrayProp->Inner,InOutDeclare);
	}
	else if(const FSetProperty* SetProp = CastField<FSetProperty>(Property))
	{
		RecursivelyGenerateDeclareOfNonNativeProp(SetProp->ElementProp,InOutDeclare);
	}
	else if(const FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		RecursivelyGenerateDeclareOfNonNativeProp(MapProp->KeyProp,InOutDeclare);
		RecursivelyGenerateDeclareOfNonNativeProp(MapProp->ValueProp,InOutDeclare);			
	}
}

void FRecoverCodeGeneration::GetCppTypeFromPin(FString& InOutString, const FEdGraphPinType& PinType,
	const UEdGraphPin* Pin, UClass* SelfClass)
{
	// Pin->PinName;
	FName PinName = NAME_None;
	if(Pin)
		PinName = Pin->PinName;
	const bool bIsMapProperty = PinType.IsMap();
	const bool bIsSetProperty = PinType.IsSet();
	const bool bIsArrayProperty = PinType.IsArray();
	// Because this node will exist in a blueprint, it will always degrade to a UObject
	auto AppendMainType = [&]()
	{
		GetCppTypeFromPinType(InOutString,PinName,PinType,SelfClass);
	};
	if( bIsArrayProperty)
	{
		InOutString += TEXT("TArray<");
		AppendMainType();
		InOutString += TEXT(">");
	}
	else if (bIsSetProperty)
	{
		InOutString += TEXT("TSet<");
		AppendMainType();
		InOutString += TEXT(">");
	}
	else if (bIsMapProperty)
	{
		InOutString += TEXT("TMap<");
		AppendMainType();
		InOutString += TEXT(",");
		GetCppTypeFromPinType(InOutString,PinName,PinType.PinValueType,SelfClass);
		InOutString +=TEXT(">");
	}
	else
	{
		AppendMainType();
	}
}


