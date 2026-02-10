// Copyright Epic Games, Inc. All Rights Reserved.

#include "CppHighLight/CodeEditorStyle.h"
#include "CppHighLight/SyntaxTextStyle.h"
#include "PrivateAccessor.hpp"
#include "Brushes/SlateBoxBrush.h"
#include "Styling/SlateStyleRegistry.h"
#include "Brushes/SlateImageBrush.h"
#include "Styling/SlateTypes.h"
#include "Brushes/SlateNoResource.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "Styling/SlateStyle.h"

struct FSlateStyleSetWidgetStyleValuesAccess
{
	using Type=TMap<FName,TSharedRef<struct FSlateWidgetStyle>> FSlateStyleSet::*;
};
template struct TStaticPtrInit<FSlateStyleSetWidgetStyleValuesAccess,&FSlateStyleSet::WidgetStyleValues>;

TSharedPtr< FSlateStyleSet > FClingCodeEditorStyle::StyleSet = nullptr;

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define SVG_BRUSH( RelativePath, ... ) FSlateVectorImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".svg") ), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

// Const icon sizes
static const FVector2D Icon8x8(8.0f, 8.0f);
static const FVector2D Icon9x19(9.0f, 19.0f);
static const FVector2D Icon16x16(16.0f, 16.0f);
static const FVector2D Icon20x20(20.0f, 20.0f);
static const FVector2D Icon22x22(22.0f, 22.0f);
static const FVector2D Icon24x24(24.0f, 24.0f);
static const FVector2D Icon28x28(28.0f, 28.0f);
static const FVector2D Icon27x31(27.0f, 31.0f);
static const FVector2D Icon26x26(26.0f, 26.0f);
static const FVector2D Icon32x32(32.0f, 32.0f);
static const FVector2D Icon40x40(40.0f, 40.0f);
static const FVector2D Icon48x48(48.0f, 48.0f);
static const FVector2D Icon75x82(75.0f, 82.0f);
static const FVector2D Icon360x32(360.0f, 32.0f);
static const FVector2D Icon171x39(171.0f, 39.0f);
static const FVector2D Icon170x50(170.0f, 50.0f);
static const FVector2D Icon267x140(170.0f, 50.0f);
 
FString ContentRoot = FPaths::ProjectPluginsDir() / TEXT("UECling/Resources");
FSlateFontInfo Consolas9  = DEFAULT_FONT("Regular", 9);
FTextBlockStyle FClingCodeEditorStyle::NormalText = FTextBlockStyle()
	.SetFont(Consolas9)
	.SetColorAndOpacity(FLinearColor::White)
	.SetShadowOffset(FVector2D::ZeroVector)
	.SetShadowColorAndOpacity(FLinearColor::Black)
	.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f))
	.SetHighlightShape(FSlateBoxBrush( ContentRoot /"UI/TextBlockHighlightShape"/TEXT(".png") , FMargin(3.f / 8.f) ))
	;

FSlateFontInfo ConsolasBold  = DEFAULT_FONT("Bold", 9);
FTextBlockStyle FClingCodeEditorStyle::BoldText = FTextBlockStyle()
	.SetFont(ConsolasBold)
	.SetColorAndOpacity(FLinearColor::White)
	.SetShadowOffset(FVector2D::ZeroVector)
	.SetShadowColorAndOpacity(FLinearColor::Black)
	.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f))
	.SetHighlightShape(FSlateBoxBrush( ContentRoot /"UI/TextBlockHighlightShape"/TEXT(".png") , FMargin(3.f / 8.f) ))
	;

void FClingCodeEditorStyle::Initialize()
{
	// Only register once
	if( StyleSet.IsValid() )
	{
		return;
	}
	
	StyleSet = MakeShareable(new FSlateStyleSet("ClingCodeEditor") );
	StyleSet->SetContentRoot(ContentRoot);
	// Icons
	{
		StyleSet->Set("FClingCodeEditorStyle.TabIcon", new IMAGE_BRUSH("UI/CodeEditor_16x", Icon16x16));
		StyleSet->Set("FClingCodeEditorStyle.Save", new IMAGE_BRUSH("UI/Save_40x", Icon40x40));
		StyleSet->Set("FClingCodeEditorStyle.Save.Small", new IMAGE_BRUSH("UI/Save_40x", Icon16x16));
		StyleSet->Set("FClingCodeEditorStyle.SaveAll", new IMAGE_BRUSH("UI/SaveAll_40x", Icon40x40));
		StyleSet->Set("FClingCodeEditorStyle.SaveAll.Small", new IMAGE_BRUSH("UI/SaveAll_40x", Icon16x16));
		
		// ClingNotebook Asset Icons
		StyleSet->Set("ClassIcon.ClingNotebook", new SVG_BRUSH("UI/ClingNotebook", Icon16x16));
		StyleSet->Set("ClassThumbnail.ClingNotebook", new SVG_BRUSH("UI/ClingNotebook", Icon16x16));
	}
	

	// Text editor
	{
		StyleSet->Set("TextEditor.NormalText", NormalText);
		auto& SyntaxTextStyle = const_cast<FSyntaxTextStyle&>(FSyntaxTextStyle::GetSyntaxTextStyle());
		
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Normal",MakeShareable(&SyntaxTextStyle.NormalTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Operator", MakeShareable(&SyntaxTextStyle.OperatorTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Keyword", MakeShareable(&SyntaxTextStyle.KeywordTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.String", MakeShareable(&SyntaxTextStyle.StringTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Number", MakeShareable(&SyntaxTextStyle.NumberTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Comment", MakeShareable(&SyntaxTextStyle.CommentTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.PreProcessorKeyword", MakeShareable(&SyntaxTextStyle.PreProcessorKeywordTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Type", MakeShareable(&SyntaxTextStyle.TypeTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Class", MakeShareable(&SyntaxTextStyle.ClassTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Struct", MakeShareable(&SyntaxTextStyle.StructTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Var", MakeShareable(&SyntaxTextStyle.VarTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Enum", MakeShareable(&SyntaxTextStyle.EnumTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Namespace", MakeShareable(&SyntaxTextStyle.NamespaceTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Function", MakeShareable(&SyntaxTextStyle.FunctionTextStyle));
		(StyleSet.Get()->*PRIVATE_ACCESS(FSlateStyleSet,WidgetStyleValues)).Add("SyntaxHighlight.CPP.Template", MakeShareable(&SyntaxTextStyle.TemplateTextStyle));
		
		StyleSet->Set("TextEditor.Border", new BOX_BRUSH("UI/TextEditorBorder", FMargin(4.0f/16.0f), FLinearColor(0.02f,0.02f,0.02f,1)));

		const FEditableTextBoxStyle& NormalEditableTextBoxStyle = FAppStyle::GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
		const FEditableTextBoxStyle EditableTextBoxStyle = FEditableTextBoxStyle(NormalEditableTextBoxStyle)
			.SetTextStyle(NormalText)
			// .SetBackgroundImageNormal( FSlateNoResource() )
			// .SetBackgroundImageHovered( FSlateNoResource() )
			// .SetBackgroundImageFocused( FSlateNoResource() )
			// .SetBackgroundImageReadOnly( FSlateNoResource() )
			;
		
		StyleSet->Set("TextEditor.EditableTextBox", EditableTextBoxStyle);
	}

	// Project editor
	{
		StyleSet->Set("ProjectEditor.Border", new BOX_BRUSH("UI/TextEditorBorder", FMargin(4.0f/16.0f), FLinearColor(0.048f,0.048f,0.048f,1)));

		StyleSet->Set("ProjectEditor.Icon.Project", new IMAGE_BRUSH("UI/FolderClosed", Icon16x16, FLinearColor(0.25f,0.25f,0.25f,1)));
		StyleSet->Set("ProjectEditor.Icon.Folder", new IMAGE_BRUSH("UI/FolderClosed", Icon16x16, FLinearColor(0.25f,0.25f,0.25f,1)));
		StyleSet->Set("ProjectEditor.Icon.File", new IMAGE_BRUSH("UI/GenericFile", Icon16x16));
		StyleSet->Set("ProjectEditor.Icon.GenericFile", new IMAGE_BRUSH("UI/GenericFile", Icon16x16));
	}

	FSlateStyleRegistry::RegisterSlateStyle( *StyleSet.Get() );
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef SVG_BRUSH
#undef DEFAULT_FONT

void FClingCodeEditorStyle::Shutdown()
{
	if( StyleSet.IsValid() )
	{
		FSlateStyleRegistry::UnRegisterSlateStyle( *StyleSet.Get() );
		ensure( StyleSet.IsUnique() );
		StyleSet.Reset();
	}
}

const ISlateStyle& FClingCodeEditorStyle::Get()
{
	return *( StyleSet.Get() );
}

const FName& FClingCodeEditorStyle::GetStyleSetName()
{
	return StyleSet->GetStyleSetName();
}
