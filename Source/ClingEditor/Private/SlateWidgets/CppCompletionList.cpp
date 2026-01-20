
#include "CppCompletionList.h"
#include "SlateOptMacros.h"
#include "CppHighLight/CppRichTextSyntaxHighlightMarshaller.h"
// #include "Widgets/Input/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"
// #include "Widgets/Views/STableViewBuilder.h"
#include "Framework/Application/SlateApplication.h"


// SCppCompletionList implementation
void SCppCompletionList::Construct(const FArguments& InArgs)
{
    SuggestionListView = SNew(SListView<TSharedPtr<FString>>)
        .ListItemsSource(&Suggestions)
        .OnGenerateRow(this, &SCppCompletionList::OnGenerateRow)
        .OnSelectionChanged(this, &SCppCompletionList::OnSelectionChanged);
    
    ChildSlot
    [
        SNew(SBox)
        .WidthOverride(300.0f)
        .MaxDesiredHeight(200.0f)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("Menu.Background"))
            .BorderBackgroundColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
            [
                SNew(SBox)
                .Padding(FMargin(2.0f, 2.0f))
                [
                    SNew(SBox)
                    .WidthOverride(300.0f)
                    .MaxDesiredHeight(200.0f)
                    [
                        SNew(SScrollBox)
                        + SScrollBox::Slot()
                        [
                            SuggestionListView.ToSharedRef()
                        ]
                    ]
                ]
            ]
        ]
    ];
}

TSharedRef<ITableRow> SCppCompletionList::OnGenerateRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
        .Padding(FMargin(4.0f, 2.0f))
        [
            SNew(STextBlock)
            .Text(FText::FromString(*Item))
        ];
}

void SCppCompletionList::OnSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
    if (Item.IsValid() && SelectInfo != ESelectInfo::OnNavigation)
    {
        OnSuggestionSelected.ExecuteIfBound(*Item);
    }
}

void SCppCompletionList::SetSuggestions(const TArray<FString>& InSuggestions)
{
    Suggestions.Reset();
    for (const auto& Suggestion : InSuggestions)
    {
        Suggestions.Add(MakeShared<FString>(Suggestion));
    }
    
    SuggestionListView->RequestListRefresh();
    if (Suggestions.Num() > 0)
    {
        SuggestionListView->SetSelection(Suggestions[0]);
    }
}

void SCppCompletionList::SelectNext()
{
    if (Suggestions.Num() == 0) return;
    
    int32 SelectedIndex = SuggestionListView->GetSelectedItems().Num() > 0 
        ? Suggestions.IndexOfByKey(SuggestionListView->GetSelectedItems()[0]) 
        : -1;
    
    SelectedIndex = FMath::Min(SelectedIndex + 1, Suggestions.Num() - 1);
    SuggestionListView->SetSelection(Suggestions[SelectedIndex]);
    SuggestionListView->RequestScrollIntoView(Suggestions[SelectedIndex]);
}

void SCppCompletionList::SelectPrev()
{
    if (Suggestions.Num() == 0) return;
    
    int32 SelectedIndex = SuggestionListView->GetSelectedItems().Num() > 0 
        ? Suggestions.IndexOfByKey(SuggestionListView->GetSelectedItems()[0]) 
        : 0;
    
    SelectedIndex = FMath::Max(SelectedIndex - 1, 0);
    SuggestionListView->SetSelection(Suggestions[SelectedIndex]);
    SuggestionListView->RequestScrollIntoView(Suggestions[SelectedIndex]);
}

FString SCppCompletionList::GetSelectedSuggestion() const
{
    if (SuggestionListView->GetSelectedItems().Num() > 0)
    {
        return *SuggestionListView->GetSelectedItems()[0].Get();
    }
    return FString();
}