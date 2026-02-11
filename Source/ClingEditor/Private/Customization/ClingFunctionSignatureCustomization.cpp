#include "ClingFunctionSignatureCustomization.h"
#include "ClingNotebook.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Input/SButton.h"
#include "PropertyHandle.h"

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
