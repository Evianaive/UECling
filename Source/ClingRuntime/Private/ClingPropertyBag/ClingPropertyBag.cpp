#include "ClingPropertyBag/ClingPropertyBag.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/StrProperty.h"
#include "UObject/Field.h"
#include "UObject/Package.h"
#include "UObject/Class.h"
#include "UObject/UnrealTypePrivate.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Script.h"
#include "Misc/ScopeLock.h"

#ifndef FUInt32Property
#define FUInt32Property FUInt32Property
#endif

static FProperty* CreatePropertyInstance(EClingPropertyBagPropertyType Type, UObject* TypeObject, FFieldVariant Outer, FName Name)
{
	FProperty* NewProperty = nullptr;
	switch (Type)
	{
	case EClingPropertyBagPropertyType::Bool:
		{
			FBoolProperty* BoolProp = new FBoolProperty(Outer, Name, RF_Public);
			BoolProp->SetBoolSize(sizeof(bool), true);
			NewProperty = BoolProp;
		}
		break;
	case EClingPropertyBagPropertyType::Byte:
		{
			FByteProperty* Prop = new FByteProperty(Outer, Name, RF_Public);
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			NewProperty = Prop;
		}
		break;
	case EClingPropertyBagPropertyType::Int32:
		{
			FIntProperty* Prop = new FIntProperty(Outer, Name, RF_Public);
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			NewProperty = Prop;
		}
		break;
	case EClingPropertyBagPropertyType::Int64:
		{
			FInt64Property* Prop = new FInt64Property(Outer, Name, RF_Public);
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			NewProperty = Prop;
		}
		break;
	case EClingPropertyBagPropertyType::Float:
		{
			FFloatProperty* Prop = new FFloatProperty(Outer, Name, RF_Public);
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			NewProperty = Prop;
		}
		break;
	case EClingPropertyBagPropertyType::Double:
		{
			FDoubleProperty* Prop = new FDoubleProperty(Outer, Name, RF_Public);
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			NewProperty = Prop;
		}
		break;
	case EClingPropertyBagPropertyType::Name:
		{
			FNameProperty* Prop = new FNameProperty(Outer, Name, RF_Public);
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			NewProperty = Prop;
		}
		break;
	case EClingPropertyBagPropertyType::String:
		{
			FStrProperty* Prop = new FStrProperty(Outer, Name, RF_Public);
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			NewProperty = Prop;
		}
		break;
	case EClingPropertyBagPropertyType::Text:
		NewProperty = new FTextProperty(Outer, Name, RF_Public);
		break;
	case EClingPropertyBagPropertyType::Enum:
		if (UEnum* Enum = Cast<UEnum>(TypeObject))
		{
			FEnumProperty* EnumProp = new FEnumProperty(Outer, Name, RF_Public);
			EnumProp->SetPropertyFlags(CPF_HasGetValueTypeHash);
			FByteProperty* UnderProp = new FByteProperty(EnumProp, "UnderlyingType", RF_Public);
			EnumProp->AddCppProperty(UnderProp);
			EnumProp->SetEnum(Enum);
			NewProperty = EnumProp;
		}
		break;
	case EClingPropertyBagPropertyType::Struct:
		if (UScriptStruct* Struct = Cast<UScriptStruct>(TypeObject))
		{
			FStructProperty* StructProp = new FStructProperty(Outer, Name, RF_Public);
			StructProp->Struct = Struct;
			NewProperty = StructProp;
		}
		break;
	case EClingPropertyBagPropertyType::Object:
		{
			FObjectProperty* ObjectProp = new FObjectProperty(Outer, Name, RF_Public);
			ObjectProp->SetPropertyFlags(CPF_HasGetValueTypeHash);
			ObjectProp->PropertyClass = Cast<UClass>(TypeObject) ? Cast<UClass>(TypeObject) : UObject::StaticClass();
			NewProperty = ObjectProp;
		}
		break;
	case EClingPropertyBagPropertyType::SoftObject:
		{
			FSoftObjectProperty* SoftObjectProp = new FSoftObjectProperty(Outer, Name, RF_Public);
			SoftObjectProp->SetPropertyFlags(CPF_HasGetValueTypeHash);
			SoftObjectProp->PropertyClass = Cast<UClass>(TypeObject) ? Cast<UClass>(TypeObject) : UObject::StaticClass();
			NewProperty = SoftObjectProp;
		}
		break;
	case EClingPropertyBagPropertyType::Class:
		{
			FClassProperty* ClassProp = new FClassProperty(Outer, Name, RF_Public);
			ClassProp->SetPropertyFlags(CPF_HasGetValueTypeHash);
			ClassProp->PropertyClass = UClass::StaticClass();
			ClassProp->MetaClass = Cast<UClass>(TypeObject) ? Cast<UClass>(TypeObject) : UObject::StaticClass();
			NewProperty = ClassProp;
		}
		break;
	case EClingPropertyBagPropertyType::SoftClass:
		{
			FSoftClassProperty* SoftClassProp = new FSoftClassProperty(Outer, Name, RF_Public);
			SoftClassProp->SetPropertyFlags(CPF_HasGetValueTypeHash);
			SoftClassProp->PropertyClass = UClass::StaticClass();
			SoftClassProp->MetaClass = Cast<UClass>(TypeObject) ? Cast<UClass>(TypeObject) : UObject::StaticClass();
			NewProperty = SoftClassProp;
		}
		break;
	case EClingPropertyBagPropertyType::UInt32:
		{
			FUInt32Property* Prop = new FUInt32Property(Outer, Name, RF_Public);
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			NewProperty = Prop;
		}
		break;
	case EClingPropertyBagPropertyType::UInt64:
		{
			FUInt64Property* Prop = new FUInt64Property(Outer, Name, RF_Public);
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			NewProperty = Prop;
		}
		break;
	default:
		break;
	}
	return NewProperty;
}

static FCriticalSection GClingPropertyBagLock;
static TMap<uint32, TWeakObjectPtr<const UClingPropertyBag>> GClingPropertyBagCache;

const UClingPropertyBag* UClingPropertyBag::GetOrCreateFromDescs(const TArray<FClingPropertyBagPropertyDesc>& InPropertyDescs)
{
	uint32 Hash = 0;
	for (const FClingPropertyBagPropertyDesc& Desc : InPropertyDescs)
	{
		Hash = HashCombine(Hash, GetTypeHash(Desc.Name));
		Hash = HashCombine(Hash, GetTypeHash(Desc.ValueType));
		for (EClingPropertyBagContainerType CT : Desc.ContainerTypes)
		{
			Hash = HashCombine(Hash, (uint32)CT);
		}
		if (Desc.ValueTypeObject) Hash = HashCombine(Hash, GetTypeHash(Desc.ValueTypeObject->GetName()));
		if (Desc.ContainerTypes.Contains(EClingPropertyBagContainerType::Map))
		{
			Hash = HashCombine(Hash, GetTypeHash(Desc.KeyType));
			if (Desc.KeyTypeObject) Hash = HashCombine(Hash, GetTypeHash(Desc.KeyTypeObject->GetName()));
		}
	}

	{
		FScopeLock Lock(&GClingPropertyBagLock);
		if (TWeakObjectPtr<const UClingPropertyBag>* Cached = GClingPropertyBagCache.Find(Hash))
		{
			if (Cached->IsValid())
			{
				return Cached->Get();
			}
		}
	}

	FString StructName = FString::Printf(TEXT("ClingPropertyBag_%u"), Hash);
	UClingPropertyBag* NewStruct = NewObject<UClingPropertyBag>(GetTransientPackage(), *StructName, RF_Public | RF_Standalone | RF_Transient);
	NewStruct->PropertyDescs = InPropertyDescs;

	struct FLocal
	{
		static FProperty* CreateRecursive(const FClingPropertyBagPropertyDesc& Desc, int32 ContainerIndex, FFieldVariant Outer, FName Name)
		{
			if (ContainerIndex < Desc.ContainerTypes.Num())
			{
				EClingPropertyBagContainerType Type = Desc.ContainerTypes[ContainerIndex];
				if (Type == EClingPropertyBagContainerType::Array)
				{
					FArrayProperty* ArrayProp = new FArrayProperty(Outer, Name, RF_Public);
					ArrayProp->Inner = CreateRecursive(Desc, ContainerIndex + 1, ArrayProp, "Inner");
					return ArrayProp;
				}
				else if (Type == EClingPropertyBagContainerType::Set)
				{
					FSetProperty* SetProp = new FSetProperty(Outer, Name, RF_Public);
					SetProp->ElementProp = CreateRecursive(Desc, ContainerIndex + 1, SetProp, "Element");
					return SetProp;
				}
				else if (Type == EClingPropertyBagContainerType::Map)
				{
					FMapProperty* MapProp = new FMapProperty(Outer, Name, RF_Public);
					MapProp->KeyProp = CreatePropertyInstance(Desc.KeyType, Desc.KeyTypeObject, MapProp, "Key");
					MapProp->ValueProp = CreateRecursive(Desc, ContainerIndex + 1, MapProp, "Value");
					return MapProp;
				}
			}
			return CreatePropertyInstance(Desc.ValueType, Desc.ValueTypeObject, Outer, Name);
		}
	};

	for (int32 i = InPropertyDescs.Num() - 1; i >= 0; i--)
	{
		const FClingPropertyBagPropertyDesc& Desc = InPropertyDescs[i];
		FProperty* Prop = FLocal::CreateRecursive(Desc, 0, NewStruct, Desc.Name);
		if (Prop)
		{
			Prop->SetPropertyFlags(CPF_Edit | CPF_BlueprintVisible);
			NewStruct->AddCppProperty(Prop);
		}
	}

	NewStruct->PrepareCppStructOps();
	NewStruct->Bind();
	NewStruct->StaticLink(true);
	
	{
		FScopeLock Lock(&GClingPropertyBagLock);
		GClingPropertyBagCache.Add(Hash, NewStruct);
	}
	return NewStruct;
}
