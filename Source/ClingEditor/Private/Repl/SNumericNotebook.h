#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSeparator.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "SClingNotebookStickyHeader.h"

#include "ClingNotebook.h"

class SNumericNotebook;
class FStructOnScope;

/**
 * Notebook Cell Widget
 */
class CLINGEDITOR_API SClingNotebookCell : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClingNotebookCell) {}
		SLATE_ARGUMENT(FClingNotebookCellData*, CellData)
		SLATE_ARGUMENT(UClingNotebook*, NotebookAsset)
		SLATE_ARGUMENT(int32, CellIndex)
		SLATE_EVENT(FSimpleDelegate, OnRunToHere)
		SLATE_EVENT(FSimpleDelegate, OnUndoToHere)
		SLATE_EVENT(FSimpleDelegate, OnDeleteCell)
		SLATE_EVENT(FSimpleDelegate, OnAddCellBelow)
		SLATE_EVENT(FSimpleDelegate, OnToggleExpand)
		SLATE_EVENT(FOnTextChanged, OnContentChanged)
		SLATE_EVENT(FSimpleDelegate, OnSelected)
		SLATE_ATTRIBUTE(bool, IsSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	FClingNotebookCellData* CellData = nullptr;
	UClingNotebook* NotebookAsset = nullptr;
	int32 CellIndex = -1;
	TAttribute<bool> IsSelected;

	// Cell UI
	TSharedPtr<SMultiLineEditableTextBox> CodeTextBox;
	TSharedPtr<class IStructureDetailsView> SignaturesDetailsView;
	TSharedPtr<FStructOnScope> SignaturesStructData;

	// Delegate
	FSimpleDelegate OnRunToHereDelegate;
	FSimpleDelegate OnUndoToHereDelegate;
	FSimpleDelegate OnDeleteCellDelegate;
	FSimpleDelegate OnAddCellBelowDelegate;
	FSimpleDelegate OnToggleExpandDelegate;
	FOnTextChanged OnContentChangedDelegate;
	FSimpleDelegate OnSelectedDelegate;
	
	void UpdateCellUI();
	TSharedRef<SWidget> GetSignaturesWidget();
	void RefreshSignaturesStructureData();

	// Button press event
	FReply OnRunToHereButtonClicked();
	FReply OnUndoToHereButtonClicked();
	FReply OnDeleteButtonClicked();
	FReply OnAddBelowButtonClicked();
	FReply OnToggleExpandClicked();

	// Content Change Event
	void OnCodeTextChanged(const FText& InText);
};

/**
 * Details Panel for a single cell
 */
class CLINGEDITOR_API SClingNotebookDetailsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClingNotebookDetailsPanel) {}
		SLATE_ARGUMENT(UClingNotebook*, NotebookAsset)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	UClingNotebook* NotebookAsset = nullptr;
	
	void OnSelectionChanged(int32 NewIndex);
	void Refresh();

	TSharedPtr<SMultiLineEditableTextBox> DetailCodeTextBox;

	// Search
	TSharedPtr<class SSearchBox> DetailSearchBox;
	FString SearchFilterText;
	bool bSearchCaseSensitive = false;

	void OnDetailSearchTextChanged(const FText& InText);
	void OnDetailSearchCaseSensitiveChanged(ECheckBoxState NewState);
	void PerformSearch(bool bReverse=false);
};

/**
 * Main Widget of Notebook
 */
class CLINGEDITOR_API SNumericNotebook : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNumericNotebook) {}
		SLATE_ARGUMENT(UClingNotebook*, NotebookAsset)
	SLATE_END_ARGS()

	virtual ~SNumericNotebook() override;
	void Construct(const FArguments& InArgs);

public:
	// Related Asset
	UClingNotebook* NotebookAsset;

private:
	// ScrollBox
	TSharedPtr<SScrollBox> ScrollBox;
	TSharedPtr<SBorder> StickyHeaderContainer;
	FClingNotebookStickyHeaderManager StickyManager;

public:
	void SetSelectedCell(int32 Index);
	void UpdateDocumentUI();
	
	TSharedRef<SWidget> GenerateStickyHeader(int32 CellIndex);
	void OnNotebookScrolled(float InScrollOffset);

	// Exposed for toolbar
	FReply OnFoldAllButtonClicked();
	FReply OnUnfoldAllButtonClicked();

private:
	FReply OnAddNewCellButtonClicked();
	FReply OnRestartInterpButtonClicked();
	FReply OnRefreshHighlightingButtonClicked();
	
	// PCH Profile selection
	TSharedRef<SWidget> GeneratePCHProfileMenu();
	void OnPCHProfileSelected(FName ProfileName);
	void ScheduleStickyHeaderUpdate();

	FDelegateHandle PendingStickyUpdateHandle;
};
