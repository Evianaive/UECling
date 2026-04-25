#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SCheckBox.h"
#include "Styling/AppStyle.h"

#include "ClingNotebook.h"

namespace ClingNotebookLayout
{
	inline constexpr float CellSlotPadding = 5.0f;
	inline constexpr float CellBorderPadding = 2.0f;
	inline constexpr float HeaderViewportOffset = CellBorderPadding;
	inline constexpr float StickyContainerHorizontalPadding = CellSlotPadding;
	inline constexpr float StickyContainerInnerPadding = 0.0f;
}

DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SWidget>, FOnGenerateStickyHeader, int32);

/**
 * Sticky Header Widget for a Notebook Cell
 * Displays the cell's header (control row) in a sticky manner at the top of the notebook
 */
class CLINGEDITOR_API SClingNotebookStickyHeader : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClingNotebookStickyHeader) {}
		SLATE_ARGUMENT(FClingNotebookCellData*, CellData)
		SLATE_ARGUMENT(UClingNotebook*, NotebookAsset)
		SLATE_ARGUMENT(int32, CellIndex)
		SLATE_EVENT(FSimpleDelegate, OnRunToHere)
		SLATE_EVENT(FSimpleDelegate, OnUndoToHere)
		SLATE_EVENT(FSimpleDelegate, OnDeleteCell)
		SLATE_EVENT(FSimpleDelegate, OnAddCellBelow)
		SLATE_EVENT(FSimpleDelegate, OnToggleExpand)
		SLATE_EVENT(FSimpleDelegate, OnJumpToCell)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void UpdateHeader();
	int32 GetCellIndex() const { return CellIndex; }

private:
	FClingNotebookCellData* CellData = nullptr;
	UClingNotebook* NotebookAsset = nullptr;
	int32 CellIndex = -1;

	FSimpleDelegate OnRunToHereDelegate;
	FSimpleDelegate OnUndoToHereDelegate;
	FSimpleDelegate OnDeleteCellDelegate;
	FSimpleDelegate OnAddCellBelowDelegate;
	FSimpleDelegate OnToggleExpandDelegate;
	FSimpleDelegate OnJumpToCellDelegate;

	FReply OnRunToHereButtonClicked();
	FReply OnUndoToHereButtonClicked();
	FReply OnDeleteButtonClicked();
	FReply OnAddBelowButtonClicked();
	FReply OnToggleExpandClicked();
	FReply OnJumpToCellClicked();
};

/**
 * Sticky Header Manager for SNumericNotebook
 * Manages the sticky header container and updates it based on scroll position
 */
class FClingNotebookStickyHeaderManager
{
public:
	FClingNotebookStickyHeaderManager();
	~FClingNotebookStickyHeaderManager();

	/**
	 * Initialize the sticky header manager
	 * @param InScrollBox The scrollbox containing the notebook cells
	 * @param InNotebookAsset The notebook asset
	 * @param InStickyContainer The border widget that will contain the sticky header
	 */
	void Initialize(
		TWeakPtr<SScrollBox> InScrollBox,
		TWeakObjectPtr<UClingNotebook> InNotebookAsset,
		TWeakPtr<SBorder> InStickyContainer);

	/**
	 * Update the sticky header based on current scroll position
	 */
	void UpdateStickyHeader();

	/**
	 * Clear the current sticky header
	 */
	void ClearStickyHeader();

	/**
	 * Called when the scroll offset changes
	 */
	void OnScrollOffsetChanged(float InScrollOffset);

	/**
	 * Set the cell widgets array for visibility tracking
	 */
	void SetCellWidgets(const TArray<TWeakPtr<SWidget>>& InCellWidgets);

	/**
	 * Get the cell widgets array
	 */
	const TArray<TWeakPtr<SWidget>>& GetCellWidgets() const { return CellWidgets; }

	/**
	 * Delegate to generate the sticky header widget for a cell
	 */
	FOnGenerateStickyHeader OnGenerateStickyHeader;

	/**
	 * Check if sticky header is currently showing
	 */
	bool HasStickyHeader() const;

private:
	/**
	 * Find the index of the first visible cell in the scrollbox
	 */
	int32 FindFirstVisibleCellIndex() const;
	bool GetCellViewportRect(int32 CellIndex, float& OutLeft, float& OutTop, float& OutRight, float& OutBottom) const;
	bool GetCellViewportBounds(int32 CellIndex, float& OutTop, float& OutBottom) const;
	bool GetCellHeaderTopInViewport(int32 CellIndex, float& OutTop) const;

	/**
	 * Set the sticky header to display a specific cell
	 */
	void SetStickyHeader(int32 CellIndex);

private:
	TWeakPtr<SScrollBox> ScrollBox;
	TWeakObjectPtr<UClingNotebook> NotebookAsset;
	TWeakPtr<SBorder> StickyContainer;
	TSharedPtr<SClingNotebookStickyHeader> CurrentStickyHeader;

	TArray<TWeakPtr<SWidget>> CellWidgets;
	float LastScrollOffset = 0.0f;
	int32 CurrentStickyIndex = INDEX_NONE;
};
