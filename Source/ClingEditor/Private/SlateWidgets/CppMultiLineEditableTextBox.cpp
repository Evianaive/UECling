// Fill out your copyright notice in the Description page of Project Settings.


#include "CppMultiLineEditableTextBox.h"

#include "SlateOptMacros.h"
#include "CppHighLight/CppRichTextSyntaxHighlightMarshaller.h"
#include "Widgets/Layout/SPopup.h"

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
	
	// 设置文本变更回调
	if (!OverrideDefaultArgs._OnTextChanged.IsBound())
	{
		OverrideDefaultArgs._OnTextChanged = FOnTextChanged::CreateLambda(
			[this](const FText&) {
				// Todo
				// 可以在这里处理文本变更，例如更新补全列表
		});
	}
	SMultiLineEditableTextBox::Construct(OverrideDefaultArgs);
}


FReply SCppMultiLineEditableTextBox::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    // 如果补全列表可见，处理上下键和回车
    if (CompletionList.IsValid() && CompletionList->GetVisibility().IsVisible())
    {
        if (InKeyEvent.GetKey() == EKeys::Up)
        {
            CompletionList->SelectPrev();
            return FReply::Handled();
        }
        else if (InKeyEvent.GetKey() == EKeys::Down)
        {
            CompletionList->SelectNext();
            return FReply::Handled();
        }
        else if (InKeyEvent.GetKey() == EKeys::Enter || InKeyEvent.GetKey() == EKeys::Tab)
        {
            FString Selected = CompletionList->GetSelectedSuggestion();
            if (!Selected.IsEmpty())
            {
                OnCompletionSelected(Selected);
                return FReply::Handled();
            }
        }
        else if (InKeyEvent.GetKey() == EKeys::Escape)
        {
            HideCompletionList();
            return FReply::Handled();
        }
    }
    
    return SMultiLineEditableTextBox::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SCppMultiLineEditableTextBox::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
    FReply Reply = SMultiLineEditableTextBox::OnKeyChar(MyGeometry, InCharacterEvent);
    
    // 检查是否需要显示补全列表
    // Todo
    // TCHAR Char = InCharacterEvent.GetCharacter();
    // if (Char == '.' || Char == '>' || Char == ':' || FChar::IsAlpha(Char) || Char == '_')
    // {
    //     // 延迟显示补全列表，确保字符已经插入
    //     FSlateApplication::Get().GetPlatformApplication()
    //     ->GetPlatformTicker()->AddTicker(
    //         FTickerDelegate::CreateLambda([this](float) {
    //             ShowCompletionList();
    //             return false; // 只执行一次
    //         }),
    //         0.1f
    //     );
    // }
    
    return Reply;
}

void SCppMultiLineEditableTextBox::ShowCompletionList()
{
    FString Prefix = GetWordBeforeCursor();
    TArray<FString> Suggestions = GetCompletionSuggestions(Prefix);
    
    if (Suggestions.Num() > 0)
    {
        if (!CompletionList.IsValid())
        {
            CompletionList = SNew(SCppCompletionList);
            CompletionList->OnSuggestionSelected.BindSP(this, &SCppMultiLineEditableTextBox::OnCompletionSelected);
        }
        
        CompletionList->SetSuggestions(Suggestions);
        FVector2D CursorPos = GetCursorScreenPosition();
        
        FWidgetPath WidgetPath;
        if (FSlateApplication::Get().GeneratePathToWidgetUnchecked(AsShared(), WidgetPath))
        {
            FSlateApplication::Get().PushMenu(
                AsShared(),
                WidgetPath,
                SNew(SPopup)
                .Content()
                [
                    CompletionList.ToSharedRef()
                ],
                CursorPos,
                FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup)
            );
        }
    }
}

void SCppMultiLineEditableTextBox::HideCompletionList()
{
    FSlateApplication::Get().DismissAllMenus();
}

void SCppMultiLineEditableTextBox::OnCompletionSelected(const FString& SelectedText)
{
    FString CurrentText = GetText().ToString();
    FTextLocation CursorLocation = GetCursorLocation();
    FString LeftText = CurrentText.Left(CursorLocation.GetOffset());
    
    int32 WordStart = LeftText.FindLastCharByPredicate(
        [](TCHAR C) { return !FChar::IsIdentifier(C) && C != '.' && C != '>' && C != ':'; }
    );
    
    int32 ReplaceStart = (WordStart == INDEX_NONE) ? 0 : WordStart + 1;
    int32 ReplaceLength = CursorLocation.GetOffset() - ReplaceStart;
    
    FString NewText = CurrentText.Left(ReplaceStart) + SelectedText + CurrentText.Mid(ReplaceStart + ReplaceLength);
    
    SetText(FText::FromString(NewText));
    
    int32 NewCursorPos = ReplaceStart + SelectedText.Len();
    GoTo(FTextLocation(NewCursorPos));
    
    HideCompletionList();
}

FString SCppMultiLineEditableTextBox::GetWordBeforeCursor() const
{
    FTextLocation CursorLocation = GetCursorLocation();
    FString CurrentText = GetText().ToString();
    FString LeftText = CurrentText.Left(CursorLocation.GetOffset());
    
    int32 WordStart = LeftText.FindLastCharByPredicate(
        [](TCHAR C) { return !FChar::IsIdentifier(C) && C != '.' && C != '>' && C != ':'; }
    );
    
    if (WordStart == INDEX_NONE)
    {
        return LeftText;
    }
    
    return LeftText.RightChop(WordStart + 1);
}

FVector2D SCppMultiLineEditableTextBox::GetCursorScreenPosition() const
{
    
    FTextLocation CursorLocation = GetCursorLocation();
    // FVector2D CursorPos = GetCachedGeometry().LocalToAbsolute()
    
    return {960,540};
}

TArray<FString> SCppMultiLineEditableTextBox::GetCompletionSuggestions(const FString& Prefix)
{
    TArray<FString> Suggestions;
    
    if (Prefix.IsEmpty())
    {
        Suggestions.Add("int");
        Suggestions.Add("float");
        Suggestions.Add("double");
        Suggestions.Add("void");
        Suggestions.Add("class");
        Suggestions.Add("struct");
        Suggestions.Add("namespace");
    }
    else
    {
        static TArray<FString> AllSuggestions = {
            "int", "float", "double", "void", "class", "struct", "namespace",
            "public", "private", "protected", "virtual", "override", "final",
            "if", "else", "for", "while", "do", "switch", "case", "default",
            "return", "break", "continue", "goto", "throw", "try", "catch"
        };
        
        for (const FString& Suggestion : AllSuggestions)
        {
            if (Suggestion.StartsWith(Prefix, ESearchCase::IgnoreCase))
            {
                Suggestions.Add(Suggestion);
            }
        }
    }
    
    return Suggestions;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
