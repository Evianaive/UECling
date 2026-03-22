#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StructUtils/StructUtils.h"
#include "StructUtils/InstancedStruct.h"
#include "ClingPropertyBag.generated.h"

UENUM(BlueprintType)
enum class EClingPropertyBagPropertyType : uint8
{
	None,
	Bool,
	Byte,
	Int32,
	Int64,
	Float,
	Double,
	Name,
	String,
	Text,
	Enum,
	Struct,
	Object,
	SoftObject,
	Class,
	SoftClass,
	UInt32,
	UInt64
};

UENUM(BlueprintType)
enum class EClingPropertyBagContainerType : uint8
{
	None,
	Array,
	Set,
	Map
};

USTRUCT()
struct FClingPropertyBagPropertyDesc
{
	GENERATED_BODY()

	UPROPERTY()
	FName Name;

	UPROPERTY()
	EClingPropertyBagPropertyType ValueType = EClingPropertyBagPropertyType::None;

	UPROPERTY()
	TObjectPtr<UObject> ValueTypeObject = nullptr;

	UPROPERTY()
	TArray<EClingPropertyBagContainerType> ContainerTypes;

	// For Map Key
	UPROPERTY()
	EClingPropertyBagPropertyType KeyType = EClingPropertyBagPropertyType::None;

	UPROPERTY()
	TObjectPtr<UObject> KeyTypeObject = nullptr;

	FClingPropertyBagPropertyDesc() = default;
	FClingPropertyBagPropertyDesc(const FName InName, const EClingPropertyBagPropertyType InValueType, UObject* InValueTypeObject = nullptr)
		: Name(InName), ValueType(InValueType), ValueTypeObject(InValueTypeObject) {}
};

UCLASS(Transient)
class CLINGRUNTIME_API UClingPropertyBag : public UScriptStruct
{
	GENERATED_BODY()
public:
	static const UClingPropertyBag* GetOrCreateFromDescs(const TArray<FClingPropertyBagPropertyDesc>& InPropertyDescs);

	const TArray<FClingPropertyBagPropertyDesc>& GetPropertyDescs() const { return PropertyDescs; }

private:
	UPROPERTY()
	TArray<FClingPropertyBagPropertyDesc> PropertyDescs;

	friend struct FClingInstancedPropertyBag;
};

USTRUCT(BlueprintType)
struct CLINGRUNTIME_API FClingInstancedPropertyBag
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Cling")
	TObjectPtr<UClingPropertyBag> PropertyBagStruct = nullptr;

	UPROPERTY(EditAnywhere, Category = "Cling")
	FInstancedStruct Value;

	FClingInstancedPropertyBag() = default;

	const UClingPropertyBag* GetPropertyBagStruct() const { return PropertyBagStruct; }
	const FInstancedStruct& GetValue() const { return Value; }

	void InitializeWithStruct(const UClingPropertyBag* InStruct)
	{
		PropertyBagStruct = const_cast<UClingPropertyBag*>(InStruct);
		Value.InitializeAs(const_cast<UClingPropertyBag*>(InStruct));
	}
};
