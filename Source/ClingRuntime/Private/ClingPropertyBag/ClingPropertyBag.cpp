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
#include "UObject/PropertyPortFlags.h"
#include "Misc/ScopeLock.h"
#include "Serialization/CustomVersion.h"

#ifndef FUInt32Property
#define FUInt32Property FUInt32Property
#endif

struct FClingPropertyBagCustomVersion
{
	enum Type
	{
		// Before any versioning was added
		BeforeCustomVersion = 0,

		// Added versioning and improved serialization
		AddedVersioning = 1,

		// -----<new versions can be added above this line>-----
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FClingPropertyBagCustomVersion() {}
};

const FGuid FClingPropertyBagCustomVersion::GUID(0x7D3A158E, 0xE6F349B4, 0x9E5F854D, 0xACFEAE42);
// Register the custom version with the engine
FCustomVersionRegistration GClingPropertyBagCustomVersion(FClingPropertyBagCustomVersion::GUID, FClingPropertyBagCustomVersion::LatestVersion, TEXT("ClingPropertyBagVer"));

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

FArchive& operator<<(FArchive& Ar, FClingPropertyBagPropertyDesc& Desc)
{
	Ar << Desc.Name;
	Ar << Desc.ValueType;
	Ar << Desc.ValueTypeObject;
	Ar << Desc.ContainerTypes;
	Ar << Desc.KeyType;
	Ar << Desc.KeyTypeObject;
	return Ar;
}

void FClingInstancedPropertyBag::InitializeWithStruct(const UClingPropertyBag* InStruct)
{
	PropertyBagStruct = const_cast<UClingPropertyBag*>(InStruct);
	if (PropertyBagStruct)
	{
		PropertyDescs = PropertyBagStruct->GetPropertyDescs();
	}
	Value.InitializeAs(const_cast<UClingPropertyBag*>(InStruct));
}

void FClingInstancedPropertyBag::CopyValuesFrom(const FClingInstancedPropertyBag& Other)
{
	const UClingPropertyBag* DestStruct = PropertyBagStruct;
	const UClingPropertyBag* SourceStruct = Other.PropertyBagStruct;

	if (!DestStruct || !SourceStruct)
	{
		return;
	}

	uint8* DestData = Value.GetMutableMemory();
	const uint8* SourceData = Other.Value.GetMemory();

	if (!DestData || !SourceData)
	{
		return;
	}

	for (TFieldIterator<FProperty> DestIt(DestStruct); DestIt; ++DestIt)
	{
		FProperty* DestProp = *DestIt;
		if (DestProp->GetFName() == "PropertyDescs") continue;

		if (FProperty* SourceProp = SourceStruct->FindPropertyByName(DestProp->GetFName()))
		{
			// Check if types are roughly compatible
			if (DestProp->SameType(SourceProp))
			{
				DestProp->CopyCompleteValue(DestProp->ContainerPtrToValuePtr<void>(DestData), SourceProp->ContainerPtrToValuePtr<void>(SourceData));
			}
		}
	}
}

bool FClingInstancedPropertyBag::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FClingPropertyBagCustomVersion::GUID);

	const int32 Version = Ar.CustomVer(FClingPropertyBagCustomVersion::GUID);
	
	if (Version < FClingPropertyBagCustomVersion::AddedVersioning && Ar.IsLoading())
	{
		// Old format was tagged or early binary. Try tagged first for compatibility with older assets.
		UScriptStruct* Struct = FClingInstancedPropertyBag::StaticStruct();
		Struct->SerializeTaggedProperties(Ar, (uint8*)this, Struct, nullptr);

		if (PropertyDescs.Num() > 0)
		{
			PropertyBagStruct = const_cast<UClingPropertyBag*>(UClingPropertyBag::GetOrCreateFromDescs(PropertyDescs));
			Value.InitializeAs(PropertyBagStruct);

			if (SerializedValues.Num() > 0 && PropertyBagStruct)
			{
				uint8* Data = Value.GetMutableMemory();
				for (auto& KVP : SerializedValues)
				{
					if (FProperty* Prop = PropertyBagStruct->FindPropertyByName(KVP.Key))
					{
						const TCHAR* ImportPtr = *KVP.Value;
						Prop->ImportText_Direct(ImportPtr, Prop->ContainerPtrToValuePtr<void>(Data), nullptr, PPF_None);
					}
				}
			}
		}
		return true;
	}

	// New format inspired by FInstancedPropertyBag
	bool bHasData = Value.IsValid();
	Ar << bHasData;

	if (bHasData)
	{
		if (Ar.IsLoading())
		{
			Ar << PropertyDescs;

			for (FClingPropertyBagPropertyDesc& Desc : PropertyDescs)
			{
				if (Desc.ValueTypeObject)
				{
					Ar.Preload(Desc.ValueTypeObject.Get());
				}
				if (Desc.KeyTypeObject)
				{
					Ar.Preload(Desc.KeyTypeObject.Get());
				}
			}

			PropertyBagStruct = const_cast<UClingPropertyBag*>(UClingPropertyBag::GetOrCreateFromDescs(PropertyDescs));
			Value.InitializeAs(PropertyBagStruct);

			int32 SerialSize = 0;
			Ar << SerialSize;

			if (PropertyBagStruct != nullptr && Value.GetMutableMemory() != nullptr)
			{
				PropertyBagStruct->SerializeItem(Ar, Value.GetMutableMemory(), nullptr);
			}
			else
			{
				Ar.Seek(Ar.Tell() + SerialSize);
			}
			
			// Also load SerializedValues for backward compatibility if needed, 
			// though new versions should rely on SerializeItem
			Ar << SerializedValues;
		}
		else
		{
			Ar << PropertyDescs;

			const int64 SizeOffset = Ar.Tell();
			int32 SerialSize = 0;
			Ar << SerialSize;

			const int64 InitialOffset = Ar.Tell();
			if (PropertyBagStruct != nullptr && Value.GetMutableMemory() != nullptr)
			{
				PropertyBagStruct->SerializeItem(Ar, Value.GetMutableMemory(), nullptr);
			}
			const int64 FinalOffset = Ar.Tell();

			Ar.Seek(SizeOffset);
			SerialSize = static_cast<int32>(FinalOffset - InitialOffset);
			Ar << SerialSize;
			Ar.Seek(FinalOffset);

			// Keep SerializedValues for now to maintain consistency with old loading logic if ever needed
			if (PropertyBagStruct)
			{
				SerializedValues.Empty();
				uint8* Data = Value.GetMutableMemory();
				for (TFieldIterator<FProperty> It(PropertyBagStruct); It; ++It)
				{
					if (It->GetFName() == "PropertyDescs") continue;
					FString Val;
					It->ExportText_Direct(Val, It->ContainerPtrToValuePtr<void>(Data), nullptr, nullptr, PPF_None);
					SerializedValues.Add(It->GetFName(), Val);
				}
			}
			Ar << SerializedValues;
		}
	}
	else
	{
		if (Ar.IsLoading())
		{
			PropertyDescs.Empty();
			SerializedValues.Empty();
			Value.Reset();
		}
	}

	return true;
}
