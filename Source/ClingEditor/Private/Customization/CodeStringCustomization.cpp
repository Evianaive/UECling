#include "CodeStringCustomization.h"

#include "CodeString.h"
#include "DetailWidgetRow.h"
#include "CppHighLight/CodeEditorStyle.h"
#include "CppHighLight/CppRichTextSyntaxHighlightMarshaller.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"


#define LOCTEXT_NAMESPACE "CodeStringCustomization"

void FCodeStringCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

void FCodeStringCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	CodeStringProperty = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCodeString, Code));
	check(CodeStringProperty);

	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MaxDesiredWidth(0.0f)
	.MinDesiredWidth(125.0f)
	[
		SNew(SMultiLineEditableTextBox)
		.Style(&FClingCodeEditorStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("TextEditor.EditableTextBox"))
		.Text(this, &FCodeStringCustomization::GetText)
		.OnTextChanged(this, &FCodeStringCustomization::SetText)
		// .Marshaller(FCppRichTextSyntaxHighlightMarshaller::Create(FCppRichTextSyntaxHighlightMarshaller::FSyntaxTextStyle()))
		.AllowMultiLine(true)
		.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
		.Margin(0.0f)
	];
}

FText FCodeStringCustomization::GetText() const
{
	if(CodeStringProperty.IsValid())
	{
		FString CodeString;
		CodeStringProperty->GetValue(CodeString);

		return FText::FromString(CodeString);
	}

	return FText::GetEmpty();
}

void FCodeStringCustomization::SetText(const FText& InNewText) const
{
	if(CodeStringProperty.IsValid())
	{
		CodeStringProperty->SetValue(InNewText.ToString());
	}
}

FText FCodeStringCustomization::GetToolTip() const
{
	static const FText EmptyText = FText::GetEmpty();

	if(CodeStringProperty.IsValid())
	{
		if(CachedTooltip.IsEmpty())
		{
			FTextBuilder TextBuilder;
			if(CodeStringProperty->GetParentHandle().IsValid())
			{
				TextBuilder.AppendLine(CodeStringProperty->GetParentHandle()->GetToolTipText());
			}

			const TArray<FString>& ValidArgs = GetValidArguments();
			if(!ValidArgs.IsEmpty())
			{
				TextBuilder.AppendLine(LOCTEXT("ValidArgs_ToolTipHeading", "Valid Arguments:"));
				TextBuilder.Indent();

				for(const FString& Arg : ValidArgs)
				{
					TextBuilder.AppendLine(Arg);
				}
			}

			CachedTooltip = TextBuilder.ToText();
		}
		
		return CachedTooltip;
	}

	return EmptyText;
}

const TArray<FString>& FCodeStringCustomization::GetValidArguments() const
{
	static TArray<FString> EmptyArray;

	if(CodeStringProperty.IsValid())
	{
		if(ValidArguments.IsEmpty())
		{
			if(CodeStringProperty->GetParentHandle()->HasMetaData(TEXT("ValidArgs")))
			{
				const FString ValidArgStr = CodeStringProperty->GetParentHandle()->GetMetaData(TEXT("ValidArgs"));
				ValidArgStr.ParseIntoArrayWS(ValidArguments, TEXT(","), true);
			}
		}

		return ValidArguments;
	}

	return EmptyArray;
}

#undef LOCTEXT_NAMESPACE
