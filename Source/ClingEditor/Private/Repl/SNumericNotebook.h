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

/**
 * Notebook Cell Widget
 */
class CLINGEDITOR_API SClingNotebookCell : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClingNotebookCell) {}
		SLATE_ARGUMENT(FClingNotebookCellData*, CellData)
		SLATE_EVENT(FSimpleDelegate, OnRunCode)
		SLATE_EVENT(FSimpleDelegate, OnRunToHere)
		SLATE_EVENT(FSimpleDelegate, OnDeleteCell)
		SLATE_EVENT(FSimpleDelegate, OnAddCellAbove)
		SLATE_EVENT(FSimpleDelegate, OnAddCellBelow)
		SLATE_EVENT(FOnTextChanged, OnContentChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FClingNotebookCellData* CellData = nullptr;

	// Cell UI
	TSharedPtr<SMultiLineEditableTextBox> CodeTextBox;

	// Delegate
	FSimpleDelegate OnRunCodeDelegate;
	FSimpleDelegate OnRunToHereDelegate;
	FSimpleDelegate OnDeleteCellDelegate;
	FSimpleDelegate OnAddCellAboveDelegate;
	FSimpleDelegate OnAddCellBelowDelegate;
	FOnTextChanged OnContentChangedDelegate;
	
	void UpdateCellUI();

	// Button press event
	FReply OnRunButtonClicked();
	FReply OnRunToHereButtonClicked();
	FReply OnDeleteButtonClicked();
	FReply OnAddAboveButtonClicked();
	FReply OnAddBelowButtonClicked();
	FReply OnToggleExpandClicked();

	// Content Change Event
	void OnCodeTextChanged(const FText& InText);
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
	// static TSharedPtr<SNumericNotebook> CreateTestWidget();

private:
	// Related Asset
	UClingNotebook* NotebookAsset;

	// ScrollBox
	TSharedPtr<SScrollBox> ScrollBox;

	// Interpreter used by this asset
	void* GetOrStartInterp();

	// Restart Interpreter
	void RestartInterp();
	
	void AddNewCell(int32 InIndex = -1);
	void DeleteCell(int32 InIndex);
	void RunCell(int32 InIndex);
	void RunToHere(int32 InIndex);

	// Compilation state
	bool bIsCompiling = false;
	TSet<int32> CompilingCells;
	
	TArray<int32> ExecutionQueue;
	bool bIsProcessingQueue = false;
	void ProcessNextInQueue();

	// UI Update
	void UpdateDocumentUI();
	
	FReply OnAddNewCellButtonClicked();
	FReply OnRestartInterpButtonClicked();
};