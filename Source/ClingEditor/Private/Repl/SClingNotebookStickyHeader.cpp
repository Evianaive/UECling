#include "SClingNotebookStickyHeader.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"

// =============================================================================
// SClingNotebookStickyHeader Implementation
// =============================================================================

void SClingNotebookStickyHeader::Construct(const FArguments& InArgs)
{
	CellData = InArgs._CellData;
	NotebookAsset = InArgs._NotebookAsset;
	CellIndex = InArgs._CellIndex;
	OnRunToHereDelegate = InArgs._OnRunToHere;
	OnUndoToHereDelegate = InArgs._OnUndoToHere;
	OnDeleteCellDelegate = InArgs._OnDeleteCell;
	OnAddCellBelowDelegate = InArgs._OnAddCellBelow;
	OnToggleExpandDelegate = InArgs._OnToggleExpand;
	OnJumpToCellDelegate = InArgs._OnJumpToCell;

	UpdateHeader();
}

void SClingNotebookStickyHeader::UpdateHeader()
{
	if (!CellData)
	{
		ChildSlot
		[
			SNew(SBox)
			[
				SNew(STextBlock).Text(INVTEXT("No Cell Selected"))
			]
		];
		return;
	}

	ChildSlot
	[
		SNew(SBorder)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(0.9f, 0.9f, 0.9f, 1.0f))
		.Padding(ClingNotebookLayout::CellBorderPadding)
		[
			SNew(SHorizontalBox)

			// Expand/Collapse button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text_Lambda([this]() { return CellData->bIsExpanded ? INVTEXT("▼") : INVTEXT("▶"); })
				.OnClicked(this, &SClingNotebookStickyHeader::OnToggleExpandClicked)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			]

			// Status label
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5, 0)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() {
					switch (CellData->CompilationState)
					{
					case EClingCellCompilationState::Idle:
						return INVTEXT("[Idle]");
					case EClingCellCompilationState::Compiling:
						return INVTEXT("[Compiling]");
					case EClingCellCompilationState::Completed:
						return INVTEXT("[Completed]");
					default:
						return INVTEXT("[Unknown]");
					}
				})
				.ColorAndOpacity_Lambda([this]() {
					switch (CellData->CompilationState)
					{
					case EClingCellCompilationState::Idle:
						return FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
					case EClingCellCompilationState::Compiling:
						return FSlateColor(FLinearColor::Yellow);
					case EClingCellCompilationState::Completed:
						return FSlateColor(FLinearColor::Green);
					default:
						return FSlateColor(FLinearColor::White);
					}
				})
			]

			// Compiling indicator
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5, 0)
			[
				SNew(SHorizontalBox)
				.Visibility_Lambda([this]() {
					return (CellData->CompilationState == EClingCellCompilationState::Compiling)
						? EVisibility::Visible : EVisibility::Collapsed;
				})
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SThrobber).NumPieces(3)
				]
			]

			// Run in GameThread indicator (read-only in sticky header)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(10, 0)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() {
					return CellData->bExecuteInGameThread ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.IsEnabled(false)
				[
					SNew(STextBlock)
					.Text(INVTEXT("Run in GameThread"))
				]
			]

			// Spacer
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)

			// Action buttons (simplified for sticky header)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(INVTEXT("+ Below"))
				.OnClicked(this, &SClingNotebookStickyHeader::OnAddBelowButtonClicked)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.IsEnabled_Lambda([this]() {
					return NotebookAsset ? NotebookAsset->IsCellAddableBelow(CellIndex) : true;
				})
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(INVTEXT("Run To Here"))
				.OnClicked(this, &SClingNotebookStickyHeader::OnRunToHereButtonClicked)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.IsEnabled_Lambda([this]() {
					return NotebookAsset ? !NotebookAsset->IsCellReadOnly(CellIndex) : true;
				})
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(INVTEXT("Undo To Here"))
				.OnClicked(this, &SClingNotebookStickyHeader::OnUndoToHereButtonClicked)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.IsEnabled_Lambda([this]() {
					if (CellData->CompilationState == EClingCellCompilationState::Compiling) return false;
					if (!NotebookAsset || !NotebookAsset->Cells.IsValidIndex(CellIndex)) return false;
					for (int32 i = CellIndex; i < NotebookAsset->Cells.Num(); ++i)
					{
						if (NotebookAsset->Cells[i].CompilationState == EClingCellCompilationState::Completed) return true;
					}
					return CellData->bHasOutput;
				})
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(INVTEXT("X"))
				.OnClicked(this, &SClingNotebookStickyHeader::OnDeleteButtonClicked)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.IsEnabled_Lambda([this]() {
					return NotebookAsset ? NotebookAsset->IsCellDeletable(CellIndex) : true;
				})
			]
		]
	];
}

FReply SClingNotebookStickyHeader::OnRunToHereButtonClicked()
{
	OnRunToHereDelegate.ExecuteIfBound();
	return FReply::Handled();
}

FReply SClingNotebookStickyHeader::OnUndoToHereButtonClicked()
{
	OnUndoToHereDelegate.ExecuteIfBound();
	return FReply::Handled();
}

FReply SClingNotebookStickyHeader::OnDeleteButtonClicked()
{
	OnDeleteCellDelegate.ExecuteIfBound();
	return FReply::Handled();
}

FReply SClingNotebookStickyHeader::OnAddBelowButtonClicked()
{
	OnAddCellBelowDelegate.ExecuteIfBound();
	return FReply::Handled();
}

FReply SClingNotebookStickyHeader::OnToggleExpandClicked()
{
	OnToggleExpandDelegate.ExecuteIfBound();
	return FReply::Handled();
}

FReply SClingNotebookStickyHeader::OnJumpToCellClicked()
{
	OnJumpToCellDelegate.ExecuteIfBound();
	return FReply::Handled();
}

// =============================================================================
// FClingNotebookStickyHeaderManager Implementation
// =============================================================================

FClingNotebookStickyHeaderManager::FClingNotebookStickyHeaderManager()
{
}

FClingNotebookStickyHeaderManager::~FClingNotebookStickyHeaderManager()
{
}

void FClingNotebookStickyHeaderManager::Initialize(
	TWeakPtr<SScrollBox> InScrollBox,
	TWeakObjectPtr<UClingNotebook> InNotebookAsset,
	TWeakPtr<SBorder> InStickyContainer)
{
	ScrollBox = InScrollBox;
	NotebookAsset = InNotebookAsset;
	StickyContainer = InStickyContainer;
}

void FClingNotebookStickyHeaderManager::UpdateStickyHeader()
{
	if (!ScrollBox.IsValid() || !NotebookAsset.IsValid())
	{
		ClearStickyHeader();
		return;
	}

	int32 FirstVisibleIndex = FindFirstVisibleCellIndex();

	if (FirstVisibleIndex == INDEX_NONE)
	{
		ClearStickyHeader();
		return;
	}

	float FirstHeaderTop = 0.0f;
	if (!GetCellHeaderTopInViewport(FirstVisibleIndex, FirstHeaderTop))
	{
		ClearStickyHeader();
		return;
	}

	if (FirstHeaderTop >= 0.0f)
	{
		ClearStickyHeader();
		return;
	}

	float FirstCellLeft = 0.0f;
	float FirstCellTop = 0.0f;
	float FirstCellRight = 0.0f;
	float FirstCellBottom = 0.0f;
	if (!GetCellViewportRect(FirstVisibleIndex, FirstCellLeft, FirstCellTop, FirstCellRight, FirstCellBottom))
	{
		ClearStickyHeader();
		return;
	}

	// Update sticky content if needed
	if (CurrentStickyIndex != FirstVisibleIndex)
	{
		SetStickyHeader(FirstVisibleIndex);
	}

	if (StickyContainer.IsValid() && ScrollBox.IsValid())
	{
		const float ScrollWidth = ScrollBox.Pin()->GetTickSpaceGeometry().GetLocalSize().X;
		const float RightInset = FMath::Max(0.0f, ScrollWidth - FirstCellRight);
		StickyContainer.Pin()->SetPadding(FMargin(FirstCellLeft, 0.0f, RightInset, 0.0f));
	}

	// Calculate Push-off effect
	float OffsetY = 0.0f;
	int32 NextIndex = FirstVisibleIndex + 1;
	if (CellWidgets.IsValidIndex(NextIndex))
	{
		TSharedPtr<SWidget> NextCell = CellWidgets[NextIndex].Pin();
		if (NextCell.IsValid())
		{
			float NextHeaderTopInViewport = 0.0f;
			if (GetCellHeaderTopInViewport(NextIndex, NextHeaderTopInViewport) && StickyContainer.IsValid())
			{
				const float CurrentHeaderHeight = StickyContainer.Pin()->GetTickSpaceGeometry().GetLocalSize().Y;
				if (NextHeaderTopInViewport < CurrentHeaderHeight)
				{
					OffsetY = NextHeaderTopInViewport - CurrentHeaderHeight;
				}
			}
		}
	}

	if (StickyContainer.IsValid())
	{
		StickyContainer.Pin()->SetRenderTransform(FSlateRenderTransform(FVector2D(0.0f, OffsetY)));
	}
}

void FClingNotebookStickyHeaderManager::ClearStickyHeader()
{
	CurrentStickyHeader.Reset();
	CurrentStickyIndex = INDEX_NONE;

	if (StickyContainer.IsValid())
	{
		StickyContainer.Pin()->SetContent(SNullWidget::NullWidget);
		StickyContainer.Pin()->SetVisibility(EVisibility::Collapsed);
		StickyContainer.Pin()->SetPadding(FMargin(0.0f));
		StickyContainer.Pin()->SetRenderTransform(FSlateRenderTransform());
	}
}

void FClingNotebookStickyHeaderManager::OnScrollOffsetChanged(float InScrollOffset)
{
	// Only update if scroll offset changed significantly (optimization)
	if (FMath::Abs(InScrollOffset - LastScrollOffset) > 1.0f)
	{
		LastScrollOffset = InScrollOffset;
		UpdateStickyHeader();
	}
}

void FClingNotebookStickyHeaderManager::SetCellWidgets(const TArray<TWeakPtr<SWidget>>& InCellWidgets)
{
	CellWidgets = InCellWidgets;
}

bool FClingNotebookStickyHeaderManager::HasStickyHeader() const
{
	return CurrentStickyIndex != INDEX_NONE;
}

int32 FClingNotebookStickyHeaderManager::FindFirstVisibleCellIndex() const
{
	if (!ScrollBox.IsValid() || CellWidgets.Num() == 0)
	{
		return INDEX_NONE;
	}

	// Sticky boundary is at the top of the viewport
	float StickyBoundaryY = 0.0f;

	for (int32 i = 0; i < CellWidgets.Num(); ++i)
	{
		float CellTopInViewport = 0.0f;
		float CellBottomInViewport = 0.0f;
		if (!GetCellViewportBounds(i, CellTopInViewport, CellBottomInViewport))
		{
			continue;
		}

		// First item whose bottom is below the boundary
		if (CellBottomInViewport > StickyBoundaryY)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

bool FClingNotebookStickyHeaderManager::GetCellViewportRect(int32 CellIndex, float& OutLeft, float& OutTop, float& OutRight, float& OutBottom) const
{
	OutLeft = 0.0f;
	OutTop = 0.0f;
	OutRight = 0.0f;
	OutBottom = 0.0f;

	if (!ScrollBox.IsValid() || !CellWidgets.IsValidIndex(CellIndex))
	{
		return false;
	}

	TSharedPtr<SWidget> CellWidget = CellWidgets[CellIndex].Pin();
	TSharedPtr<SScrollBox> ScrollBoxPtr = ScrollBox.Pin();
	if (!CellWidget.IsValid() || !ScrollBoxPtr.IsValid())
	{
		return false;
	}

	const FGeometry ScrollGeometry = ScrollBoxPtr->GetTickSpaceGeometry();
	const FGeometry CellGeometry = CellWidget->GetTickSpaceGeometry();
	const FVector2D CellTopLeft = ScrollGeometry.AbsoluteToLocal(CellGeometry.LocalToAbsolute(FVector2D::ZeroVector));
	const FVector2D CellTopRight = ScrollGeometry.AbsoluteToLocal(CellGeometry.LocalToAbsolute(FVector2D(CellGeometry.GetLocalSize().X, 0.0f)));
	const FVector2D CellBottomLeft = ScrollGeometry.AbsoluteToLocal(CellGeometry.LocalToAbsolute(FVector2D(0.0f, CellGeometry.GetLocalSize().Y)));

	OutLeft = CellTopLeft.X;
	OutTop = CellTopLeft.Y;
	OutRight = CellTopRight.X;
	OutBottom = CellBottomLeft.Y;
	return true;
}

bool FClingNotebookStickyHeaderManager::GetCellViewportBounds(int32 CellIndex, float& OutTop, float& OutBottom) const
{
	float Left = 0.0f;
	float Right = 0.0f;
	return GetCellViewportRect(CellIndex, Left, OutTop, Right, OutBottom);
}

bool FClingNotebookStickyHeaderManager::GetCellHeaderTopInViewport(int32 CellIndex, float& OutTop) const
{
	float CellTop = 0.0f;
	float CellBottom = 0.0f;
	if (!GetCellViewportBounds(CellIndex, CellTop, CellBottom))
	{
		OutTop = 0.0f;
		return false;
	}

	OutTop = CellTop + ClingNotebookLayout::HeaderViewportOffset;
	return true;
}

void FClingNotebookStickyHeaderManager::SetStickyHeader(int32 CellIndex)
{
	if (!NotebookAsset.IsValid() || !StickyContainer.IsValid() || !OnGenerateStickyHeader.IsBound())
	{
		return;
	}

	if (!NotebookAsset->Cells.IsValidIndex(CellIndex))
	{
		ClearStickyHeader();
		return;
	}

	// Generate the sticky header widget using the delegate
	TSharedRef<SWidget> NewStickyWidget = OnGenerateStickyHeader.Execute(CellIndex);
	CurrentStickyHeader.Reset();
	CurrentStickyIndex = CellIndex;

	// Update the container
	TSharedPtr<SBorder> ContainerPtr = StickyContainer.Pin();
	ContainerPtr->SetContent(NewStickyWidget);
	ContainerPtr->SetVisibility(EVisibility::SelfHitTestInvisible);
	ContainerPtr->SetRenderTransform(FSlateRenderTransform());
}
