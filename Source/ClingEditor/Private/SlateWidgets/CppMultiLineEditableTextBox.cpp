// Fill out your copyright notice in the Description page of Project Settings.


#include "CppMultiLineEditableTextBox.h"

#include "SlateOptMacros.h"
#include "CppHighLight/CppRichTextSyntaxHighlightMarshaller.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SCppMultiLineEditableTextBox::Construct(const FArguments& InArgs)
{
	auto& OverrideDefaultArgs = const_cast<FArguments&>(InArgs);
	if (!OverrideDefaultArgs._OnIsTypedCharValid.IsBound())
	{
		OverrideDefaultArgs._OnIsTypedCharValid
		= FOnIsTypedCharValid::CreateLambda([](const TCHAR InChar)
		{
			// \t is not allowed by default
			// allowing input any char to allow type \t
			return true;
		});
	}
	if (!OverrideDefaultArgs._Marshaller.IsValid())		
		OverrideDefaultArgs._Marshaller=FCppRichTextSyntaxHighlightMarshaller::Create(FSyntaxTextStyle::GetSyntaxTextStyle());
	if (!OverrideDefaultArgs._AllowMultiLine.IsSet())
		OverrideDefaultArgs.AllowMultiLine(true);
	// .Style(&FClingCodeEditorStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("TextEditor.EditableTextBox"))
	SMultiLineEditableTextBox::Construct(OverrideDefaultArgs);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
