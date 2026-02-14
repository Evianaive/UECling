// Fill out your copyright notice in the Description page of Project Settings.


#include "CppMultiLineEditableTextBox.h"

#include "ClingSemanticInfoProvider.h"
#include "SlateOptMacros.h"
#include "CppHighLight/CppRichTextSyntaxHighlightMarshaller.h"
#include "Fonts/FontMeasure.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


// // Copyright Epic Games, Inc. All Rights Reserved.
// #pragma once

#include "CoreMinimal.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Styling/SlateTypes.h"

/**
 * Text layout class with line number display
 * Inherits from FSlateTextLayout, draws line numbers additionally in OnPaint
 * 
 * Note: Margin needs to be set at SMultiLineEditableText level,
 * because FSlateEditableTextLayout::CacheDesiredSize will override TextLayout's Margin
 */
class FLineNumberTextLayout : public FSlateTextLayout
{
public:
    using Super = FSlateTextLayout;
    static TSharedRef<FSlateTextLayout> Create(
        SWidget* InOwner, 
        const FTextBlockStyle& InDefaultTextStyle)
    {
        return MakeShareable(new FLineNumberTextLayout(InOwner, InDefaultTextStyle));
    }

    virtual ~FLineNumberTextLayout() {}
    virtual void UpdateIfNeeded() override
    {
        Super::UpdateIfNeeded();
    };
    virtual void UpdateLayout() override
    {
        LineNumberWidth = Margin.Left;
        Super::UpdateLayout();
    };
    
    void SetShowLineNumbers(bool bInShow) 
    { 
        if (bShowLineNumbers != bInShow)
        {
            bShowLineNumbers = bInShow;
            DirtyLayout();
        }
    }
    
    bool GetShowLineNumbers() const { return bShowLineNumbers; }    
    void SetLineNumberWidth(float InWidth) 
    { 
        if (LineNumberWidth != InWidth)
        {
            LineNumberWidth = InWidth;
            DirtyLayout();
        }
    }
    
    float GetLineNumberWidth() const { return LineNumberWidth; }    
    void SetLineNumberStyle(const FTextBlockStyle& InStyle)
    {
        LineNumberStyle = InStyle;
    }    
    const FTextBlockStyle& GetLineNumberStyle() const { return LineNumberStyle; }

protected:
    FLineNumberTextLayout(SWidget* InOwner, FTextBlockStyle InDefaultTextStyle)
        : FSlateTextLayout(InOwner, MoveTemp(InDefaultTextStyle))
        , bShowLineNumbers(true)
    {
        LineNumberStyle = GetDefaultTextStyle();
        LineNumberStyle.Font.Size = 10;
        LineNumberStyle.ColorAndOpacity = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
    }

    // Override OnPaint to draw line numbers
    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;

private:
    /** Whether to show line numbers */
    bool bShowLineNumbers;
    
    /** Line number area width */
    float LineNumberWidth;
    
    /** Line number text style */
    FTextBlockStyle LineNumberStyle;
};

// Copyright Epic Games, Inc. All Rights Reserved.

// #include "LineNumberTextLayout.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

int32 FLineNumberTextLayout::OnPaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    int32 TextLayer =  FSlateTextLayout::OnPaint(
        Args, AllottedGeometry, MyCullingRect, 
        OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
    
    if (!bShowLineNumbers)    
        return TextLayer;
    
    const ESlateDrawEffect DrawEffects = bParentEnabled 
        ? ESlateDrawEffect::None 
        : ESlateDrawEffect::DisabledEffect;

    const TSharedRef<FSlateFontMeasure> FontMeasure = 
        FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

    const TArray<FTextLayout::FLineView>& LocalLineViews = GetLineViews();

    // === Draw line number area background ===
    const float LineNumberAreaWidth = LineNumberWidth;
    const float LineNumberAreaHeight = AllottedGeometry.GetLocalSize().Y;

    // === Draw separator line ===
    FSlateDrawElement::MakeBox(
        OutDrawElements, TextLayer + 1,
        AllottedGeometry.ToPaintGeometry(
            FVector2f(LineNumberAreaWidth - 5.0f, 0.0f),
            FVector2f(1, LineNumberAreaHeight)
        ),
        FCoreStyle::Get().GetBrush("BlackBrush"),
        DrawEffects,
        FLinearColor(0.3f, 0.3f, 0.3f, 1.0f)
    );
    // Draw background
    FSlateDrawElement::MakeBox(
        OutDrawElements, TextLayer + 1,
        AllottedGeometry.ToPaintGeometry(
            FVector2f(0, 0.0f),
            FVector2f(LineNumberAreaWidth - 5.0f, LineNumberAreaHeight)
        ),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        DrawEffects,
        FLinearColor(0, 0, 0, 1.0f)
    );

    int32 HighestLayerId = TextLayer + 2;

    // === Draw line numbers ===
    // Track displayed logical lines to avoid duplicate line numbers for auto-wrapped continuation lines
    TSet<int32> DisplayedModelIndices;

    for (const FTextLayout::FLineView& LineView : LocalLineViews)
    {
        const int32 ModelIdx = LineView.ModelIndex;

        // Skip already displayed logical lines (continuation lines from auto-wrapping)
        if (DisplayedModelIndices.Contains(ModelIdx))
        {
            continue;
        }
        DisplayedModelIndices.Add(ModelIdx);

        // Line number = logical line index + 1
        const int32 LineNumber = ModelIdx + 1;
        const FString LineNumberText = FString::FromInt(LineNumber);
        const FVector2D TextSize = FontMeasure->Measure(LineNumberText, LineNumberStyle.Font);

        // Calculate line number position
        // Y: Use LineView.Offset.Y (already includes Margin.Top)
        float LineNumberY = LineView.Offset.Y;
        // Vertical centering
        LineNumberY += (LineView.Size.Y - TextSize.Y) * 0.5f;
        
        
        float LineNumberX = LineNumberAreaWidth - TextSize.X - 8.0f;
        // float LineNumberX = 8.0f;

        FSlateDrawElement::MakeText(
            OutDrawElements,
            HighestLayerId,
            AllottedGeometry.ToPaintGeometry(
                FSlateLayoutTransform(FVector2D(LineNumberX, LineNumberY))
            ),
            LineNumberText,
            LineNumberStyle.Font,
            DrawEffects,
            LineNumberStyle.ColorAndOpacity.GetColor(InWidgetStyle)
        );
    }
    return HighestLayerId + 1;
}

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
		OverrideDefaultArgs._Marshaller=FCppRichTextSyntaxHighlightMarshaller::Create(FSyntaxTextStyle::GetSyntaxTextStyle(),FClingSemanticInfoProvider());
	if (!OverrideDefaultArgs._AllowMultiLine.IsSet())
		OverrideDefaultArgs.AllowMultiLine(true);
	// .Style(&FClingCodeEditorStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("TextEditor.EditableTextBox"))
	if (!InArgs._CreateSlateTextLayout.IsBound())
	{
	    OverrideDefaultArgs._CreateSlateTextLayout = 
	        FCreateSlateTextLayout::CreateStatic(FLineNumberTextLayout::Create);
	    OverrideDefaultArgs.Margin(FMargin(30,0,0,0));
	}
    if (!InArgs._AutoWrapText.IsSet())
    {
        OverrideDefaultArgs._AutoWrapText = true;
    }
    if (!InArgs._WrappingPolicy.IsSet())
    {
        OverrideDefaultArgs._WrappingPolicy = ETextWrappingPolicy::AllowPerCharacterWrapping;
    }
    // if (!InArgs._WrapTextAt.IsSet())
    // {
    //     OverrideDefaultArgs._WrapTextAt = 80;
    // }
	SMultiLineEditableTextBox::Construct(OverrideDefaultArgs); 
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
