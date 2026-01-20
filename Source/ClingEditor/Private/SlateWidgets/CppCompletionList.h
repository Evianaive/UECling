// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

// Forward declarations
class STextBlock;

class CLINGEDITOR_API SCppCompletionList : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCppCompletionList) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    
    // 设置补全建议
    void SetSuggestions(const TArray<FString>& InSuggestions);
    
    // 选择下一个/上一个建议
    void SelectNext();
    void SelectPrev();
    
    // 获取当前选中的建议
    FString GetSelectedSuggestion() const;
    
    // 选择变化委托
    DECLARE_DELEGATE_OneParam(FOnSuggestionSelected, const FString&);
    FOnSuggestionSelected OnSuggestionSelected;
    
private:
    TSharedPtr<SListView<TSharedPtr<FString>>> SuggestionListView;
    TArray<TSharedPtr<FString>> Suggestions;
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);
    void OnSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
};