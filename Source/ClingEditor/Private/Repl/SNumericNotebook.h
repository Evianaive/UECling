#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSeparator.h"

#include "ClingNotebook.h"

class SNumericNotebook;

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

	// Delegate
	FSimpleDelegate OnRunToHereDelegate;
	FSimpleDelegate OnUndoToHereDelegate;
	FSimpleDelegate OnDeleteCellDelegate;
	FSimpleDelegate OnAddCellBelowDelegate;
	FOnTextChanged OnContentChangedDelegate;
	FSimpleDelegate OnSelectedDelegate;
	
	void UpdateCellUI();

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

	void Construct(const FArguments& InArgs);

public:
	// Related Asset
	UClingNotebook* NotebookAsset;

private:
	// ScrollBox
	TSharedPtr<SScrollBox> ScrollBox;

public:
	void SetSelectedCell(int32 Index);

private:
	// UI Update
	void UpdateDocumentUI();
	
	FReply OnAddNewCellButtonClicked();
	FReply OnRestartInterpButtonClicked();
	FReply OnFoldAllButtonClicked();
	FReply OnUnfoldAllButtonClicked();
};