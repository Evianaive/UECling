// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CodeEditorStyle.h"
#include "SyntaxTextStyle.h"
#include "Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"
#include "Framework/Text/SlateHyperlinkRun.h"

namespace Cpp {
	typedef void* TInterp_t;
}

class FTextLayout;

/**
 * Get/set the raw text to/from a text layout, and also inject syntax highlighting for our rich-text markup
 */
class CLINGEDITOR_API FCppRichTextSyntaxHighlightMarshaller : public FSyntaxHighlighterTextLayoutMarshaller
{
public:

	static TSharedRef< FCppRichTextSyntaxHighlightMarshaller > Create(
		const FSyntaxTextStyle& InSyntaxTextStyle, const struct FClingSemanticInfoProvider& InProvider);

	virtual ~FCppRichTextSyntaxHighlightMarshaller();

	/** 尝试分析并修复缺失的 include */
	FString TryFixIncludes(const FString& InCode);

protected:

	virtual void ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<ISyntaxTokenizer::FTokenizedLine> TokenizedLines) override;

	FCppRichTextSyntaxHighlightMarshaller(
		TSharedPtr< ISyntaxTokenizer > InTokenizer, 
		const FSyntaxTextStyle& InSyntaxTextStyle,
		const FClingSemanticInfoProvider& InProvider);

	/** Styles used to display the text */
	const FSyntaxTextStyle& SyntaxTextStyle;

	const FClingSemanticInfoProvider& SemanticInfoProvider;

private:
	/** Callback when include hyperlink is clicked */
	static void OnIncludeClicked(const FSlateHyperlinkRun::FMetadata& Metadata, FString FullPath);
	
	/** Get tooltip text for include hyperlink */
	static FText GetIncludeTooltip(const FSlateHyperlinkRun::FMetadata& Metadata, FString FullPath);
};
