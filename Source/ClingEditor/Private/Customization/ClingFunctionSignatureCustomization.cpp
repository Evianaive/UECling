#include "ClingFunctionSignatureCustomization.h"
#include "ClingNotebook.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Input/SButton.h"
#include "PropertyHandle.h"
#include "IStructureDataProvider.h"
#include "StructUtils/InstancedStruct.h"

class FClingInstancedPropertyBagStructureDataProvider : public IStructureDataProvider
{
public:
	FClingInstancedPropertyBagStructureDataProvider(FClingInstancedPropertyBag& InPropertyBag)
		: PropertyBag(InPropertyBag)
	{
	}

	virtual bool IsValid() const override
	{
		return PropertyBag.GetPropertyBagStruct() != nullptr && PropertyBag.GetValue().IsValid();
	}

	virtual const UStruct* GetBaseStructure() const override
	{
		return PropertyBag.GetPropertyBagStruct();
	}

	virtual void GetInstances(TArray<TSharedPtr<FStructOnScope>>& OutInstances, const UStruct* ExpectedBaseStructure) const override
	{
		const UClingPropertyBag* Struct = PropertyBag.GetPropertyBagStruct();
		if (Struct && Struct->IsChildOf(ExpectedBaseStructure))
		{
			OutInstances.Add(MakeShared<FStructOnScope>(Struct, const_cast<uint8*>(PropertyBag.GetValue().GetMemory())));
		}
	}

protected:
	FClingInstancedPropertyBag& PropertyBag;
};

TSharedRef<IPropertyTypeCustomization> FClingFunctionSignatureCustomization::MakeInstance()
{
	return MakeShareable(new FClingFunctionSignatureCustomization);
}

void FClingFunctionSignatureCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;
	TSharedPtr<IPropertyHandle> NameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClingFunctionSignature, Name));

	HeaderRow.NameContent()
	[
		NameHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			NameHandle->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(5, 0)
		[
			SNew(SButton)
			.Text(INVTEXT("Execute"))
			.OnClicked(this, &FClingFunctionSignatureCustomization::OnExecuteClicked)
			.IsEnabled(this, &FClingFunctionSignatureCustomization::IsExecuteEnabled)
		]
	];
}

void FClingFunctionSignatureCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> ParamsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClingFunctionSignature, Parameters));
	if (ParamsHandle.IsValid())
	{
		void* ParamsPtr = nullptr;
		if (ParamsHandle->GetValueData(ParamsPtr) == FPropertyAccess::Success && ParamsPtr != nullptr)
		{
			FClingInstancedPropertyBag* PropertyBag = (FClingInstancedPropertyBag*)ParamsPtr;
			if (PropertyBag->GetPropertyBagStruct())
			{
				TSharedRef<FClingInstancedPropertyBagStructureDataProvider> Provider = MakeShared<FClingInstancedPropertyBagStructureDataProvider>(*PropertyBag);
				
				// Use AddChildStructure to flatten the properties directly into this customization's children
				TArray<TSharedPtr<IPropertyHandle>> ChildProperties = ParamsHandle->AddChildStructure(Provider);
				for (TSharedPtr<IPropertyHandle> ChildHandle : ChildProperties)
				{
					ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
				}
				return;
			}
		}

		// Fallback to default if pointer extraction fails or no struct is set
		ChildBuilder.AddProperty(ParamsHandle.ToSharedRef());
	}
}

FReply FClingFunctionSignatureCustomization::OnExecuteClicked()
{
	UE_LOG(LogTemp, Log, TEXT("ClingEditor: OnExecuteClicked called"));
	
	void* StructPtr = nullptr;
	if (StructHandle->GetValueData(StructPtr) == FPropertyAccess::Success)
	{
		FClingFunctionSignature* Signature = (FClingFunctionSignature*)StructPtr;
		if (Signature->OnExecute.IsBound())
		{
			UE_LOG(LogTemp, Log, TEXT("ClingEditor: Invoking OnExecute delegate for %s"), *Signature->Name.ToString());
			Signature->OnExecute.Execute(*Signature);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ClingEditor: OnExecute delegate is not bound for %s"), *Signature->Name.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ClingEditor: Failed to get StructPtr data"));
	}

	return FReply::Handled();
}

bool FClingFunctionSignatureCustomization::IsExecuteEnabled() const
{
	void* StructPtr = nullptr;
	if (StructHandle.IsValid() && StructHandle->GetValueData(StructPtr) == FPropertyAccess::Success)
	{
		FClingFunctionSignature* Signature = (FClingFunctionSignature*)StructPtr;
		return Signature && Signature->OnExecute.IsBound();
	}
	return false;
}
