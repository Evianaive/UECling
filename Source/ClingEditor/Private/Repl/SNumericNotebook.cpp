#include "SNumericNotebook.h"
#include "IStructureDetailsView.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "ClingRuntime.h"
#include "ClingSetting.h"
#include "CppInterOp/CppInterOp.h"
#include "Async/Async.h"
#include "CppHighLight/CppRichTextSyntaxHighlightMarshaller.h"
#include "ClingSemanticInfoProvider.h"

#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/SlateTypes.h"
#include "Styling/AppStyle.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Engine/Engine.h"
#include "SlateWidgets/CppMultiLineEditableTextBox.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SCheckBox.h"

void SClingNotebookCell::Construct(const FArguments& InArgs)
{
	CellData = InArgs._CellData;
	NotebookAsset = InArgs._NotebookAsset;
	CellIndex = InArgs._CellIndex;
	OnRunToHereDelegate = InArgs._OnRunToHere;
	OnUndoToHereDelegate = InArgs._OnUndoToHere;
	OnDeleteCellDelegate = InArgs._OnDeleteCell;
	OnAddCellBelowDelegate = InArgs._OnAddCellBelow;
	OnContentChangedDelegate = InArgs._OnContentChanged;
	OnSelectedDelegate = InArgs._OnSelected;
	IsSelected = InArgs._IsSelected;

	UpdateCellUI();
}

void SClingNotebookCell::UpdateCellUI()
{
	if (!IsInGameThread())
	{
		AsyncTask(ENamedThreads::GameThread, [this]() { UpdateCellUI(); });
		return;
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor_Lambda([this]() { return IsSelected.Get() ? FLinearColor::Gray : FLinearColor::White; })
		.Padding(2.0f)
		[
			SNew(SVerticalBox)
			
			// 单元格控制行
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				
				// 展开/收起按钮
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text_Lambda([this]() { return CellData->bIsExpanded ? INVTEXT("▼") : INVTEXT("▶"); })
					.OnClicked(this, &SClingNotebookCell::OnToggleExpandClicked)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				]

				// 状态标签
				+SHorizontalBox::Slot()
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
				
				// 编译中标记
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(5, 0)
				[
					SNew(SHorizontalBox)
					.Visibility_Lambda([this]() { return (CellData->CompilationState == EClingCellCompilationState::Compiling) ? EVisibility::Visible : EVisibility::Collapsed; })
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SThrobber)
						.NumPieces(3)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5, 0)
					[
						SNew(STextBlock)
						.Text(INVTEXT("Compiling..."))
						.ColorAndOpacity(FLinearColor::Yellow)
					]
				]

				// Run in GameThread Checkbox
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(10, 0)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return CellData->bExecuteInGameThread ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { CellData->bExecuteInGameThread = (NewState == ECheckBoxState::Checked); })
					.IsEnabled_Lambda([this]() {
						return NotebookAsset ? !NotebookAsset->IsCellReadOnly(CellIndex) : true;
					})
					[
						SNew(STextBlock)
						.Text(INVTEXT("Run in GameThread"))
						.ToolTipText(INVTEXT("Execute this cell in GameThread. Necessary if code creates UI."))
					]
				]

				// 空间填充
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(INVTEXT("+ Below"))
					.OnClicked(this, &SClingNotebookCell::OnAddBelowButtonClicked)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.IsEnabled_Lambda([this]() {
						return NotebookAsset ? NotebookAsset->IsCellAddableBelow(CellIndex) : true;
					})
				]
				
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(INVTEXT("Run To Here"))
					.ToolTipText(INVTEXT("Execute from start to this cell"))
					.OnClicked(this, &SClingNotebookCell::OnRunToHereButtonClicked)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.IsEnabled_Lambda([this]() {
						return NotebookAsset ? !NotebookAsset->IsCellReadOnly(CellIndex) : true;
					})
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(INVTEXT("Undo To Here"))
					.ToolTipText(INVTEXT("Undo this cell and all subsequent cells"))
					.OnClicked(this, &SClingNotebookCell::OnUndoToHereButtonClicked)
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
				
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(INVTEXT("X"))
					.OnClicked(this, &SClingNotebookCell::OnDeleteButtonClicked)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.IsEnabled_Lambda([this]() {
						return NotebookAsset ? NotebookAsset->IsCellDeletable(CellIndex) : true;
					})
				]
			]
			
			// 代码输入区域
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 2)
			[
				SAssignNew(CodeTextBox, SCppMultiLineEditableTextBox)
				.Marshaller(FCppRichTextSyntaxHighlightMarshaller::Create(
					FSyntaxTextStyle::GetSyntaxTextStyle(),
					NotebookAsset->GetUsableSemanticInfoProvider()						
					))
				.Text(FText::FromString(CellData->Content))
				//.HintText(INVTEXT("// Enter your code here...\n// void Function will reflect to a button automatically\n// Run in GameThread if create UI Directly!"))
				.OnTextChanged(this, &SClingNotebookCell::OnCodeTextChanged)
				.Visibility_Lambda([this]() { 
					const bool bShowInline = NotebookAsset ? NotebookAsset->bShowCodeInline : true;
					return (bShowInline && CellData->bIsExpanded) ? EVisibility::Visible : EVisibility::Collapsed; 
				})
				.IsReadOnly_Lambda([this]() {
					return NotebookAsset ? NotebookAsset->IsCellReadOnly(CellIndex) : false;
				})
			]
			
			// 输出显示区域
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.Visibility_Lambda([this]() { 
					if (CellData->bIsExpanded && CellData->bHasOutput)
					{
						if (CellData->LastCompilationResult.bSuccess)
						{
							return EVisibility::Visible;
						}

						const bool bShowInline = NotebookAsset ? NotebookAsset->bShowCodeInline : true;
						return bShowInline ? EVisibility::Visible : EVisibility::Collapsed;
					}
					return EVisibility::Collapsed;
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(CellData->Output); })
					.WrapTextAt(800.0f)
					.Margin(5.0f)
				]
			]

			// 函数签名区域
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.Visibility_Lambda([this]() { 
					return (CellData->bIsExpanded && CellData->SavedSignatures.Signatures.Num() > 0) ? EVisibility::Visible : EVisibility::Collapsed; 
				})
				[
					GetSignaturesWidget()
				]
			]
		]
	];
}

TSharedRef<SWidget> SClingNotebookCell::GetSignaturesWidget()
{
	if (!SignaturesDetailsView.IsValid())
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = false;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
		DetailsViewArgs.bShowScrollBar = false;

		FStructureDetailsViewArgs StructViewArgs;
		StructViewArgs.bShowObjects = false;
		StructViewArgs.bShowAssets = true;
		StructViewArgs.bShowClasses = true;

		TSharedPtr<FStructOnScope> StructOnScope = MakeShared<FStructOnScope>(FClingFunctionSignatures::StaticStruct(), (uint8*)&CellData->SavedSignatures);
		if (NotebookAsset)
		{
			StructOnScope->SetPackage(NotebookAsset->GetOutermost());
		}

		SignaturesDetailsView = PropertyEditorModule.CreateStructureDetailView(DetailsViewArgs, StructViewArgs, StructOnScope);
	}
	else
	{
		TSharedPtr<FStructOnScope> StructOnScope = MakeShared<FStructOnScope>(FClingFunctionSignatures::StaticStruct(), (uint8*)&CellData->SavedSignatures);
		if (NotebookAsset)
		{
			StructOnScope->SetPackage(NotebookAsset->GetOutermost());
		}
		SignaturesDetailsView->SetStructureData(StructOnScope);
	}

	return SignaturesDetailsView->GetWidget().ToSharedRef();
}

FReply SClingNotebookCell::OnRunToHereButtonClicked()
{
	OnRunToHereDelegate.ExecuteIfBound();
	return FReply::Handled();
}

FReply SClingNotebookCell::OnUndoToHereButtonClicked()
{
	OnUndoToHereDelegate.ExecuteIfBound();
	return FReply::Handled();
}

FReply SClingNotebookCell::OnDeleteButtonClicked()
{
	OnDeleteCellDelegate.ExecuteIfBound();
	return FReply::Handled();
}

FReply SClingNotebookCell::OnAddBelowButtonClicked()
{
	OnAddCellBelowDelegate.ExecuteIfBound();
	return FReply::Handled();
}

FReply SClingNotebookCell::OnToggleExpandClicked()
{
	CellData->bIsExpanded = !CellData->bIsExpanded;
	return FReply::Handled();
}

void SClingNotebookCell::OnCodeTextChanged(const FText& InText)
{
	CellData->Content = InText.ToString();
	OnContentChangedDelegate.ExecuteIfBound(InText);
}

FReply SClingNotebookCell::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	OnSelectedDelegate.ExecuteIfBound();
	return FReply::Unhandled();
}

void SClingNotebookDetailsPanel::Construct(const FArguments& InArgs)
{
	NotebookAsset = InArgs._NotebookAsset;

	if (NotebookAsset)
	{
		NotebookAsset->OnCellSelectionChanged.AddSP(this, &SClingNotebookDetailsPanel::OnSelectionChanged);
	}

	Refresh();
}

void SClingNotebookDetailsPanel::OnSelectionChanged(int32 NewIndex)
{
	Refresh();
}

void SClingNotebookDetailsPanel::Refresh()
{
	FClingNotebookCellData* SelectedData = NotebookAsset ? NotebookAsset->GetSelectedCellData() : nullptr;

	if (!SelectedData)
	{
		ChildSlot
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(INVTEXT("Select a cell in the notebook to view details"))
				.Font(FAppStyle::Get().GetFontStyle("HeadingFont"))
			]
		];
		return;
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(10.0f)
		[
			SNew(SVerticalBox)
			
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 10)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() {
					return FText::Format(INVTEXT("Editing Cell #{0}"), FText::AsNumber(NotebookAsset ? NotebookAsset->SelectedCellIndex : -1));
				})
				.Font(FAppStyle::Get().GetFontStyle("HeadingFont"))
			]

			// Search bar
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 5)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(DetailSearchBox, SSearchBox)
					.HintText(INVTEXT("Search in code..."))
					.OnTextChanged(this, &SClingNotebookDetailsPanel::OnDetailSearchTextChanged)
					.DelayChangeNotificationsWhileTyping(true)
					.OnSearch_Lambda([this](SSearchBox::SearchDirection Direction)
					{
						PerformSearch(Direction==SSearchBox::Previous);
					})
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 0, 0, 0)
				[
					
					SNew(SCheckBox)
					.IsChecked(bSearchCaseSensitive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.OnCheckStateChanged(this, &SClingNotebookDetailsPanel::OnDetailSearchCaseSensitiveChanged)
					.ToolTipText(INVTEXT("Match Case"))
					[
						SNew(STextBlock)
						.Text(INVTEXT("Aa"))
						.Font(FAppStyle::Get().GetFontStyle("BoldFont"))
					]
				]
			]

			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.FillHeight(0.7f)
				[
					SAssignNew(DetailCodeTextBox, SCppMultiLineEditableTextBox)
					.Marshaller(FCppRichTextSyntaxHighlightMarshaller::Create(
						FSyntaxTextStyle::GetSyntaxTextStyle(),
						NotebookAsset->GetUsableSemanticInfoProvider()							
						))
					//.HintText(INVTEXT("// Enter your code here...\n// void Function will reflect to a button automatically\n// Run in GameThread if create UI Directly!"))
					.Text(FText::FromString(SelectedData->Content))
					.OnTextChanged_Lambda([this, SelectedData](const FText& InText) {
						SelectedData->Content = InText.ToString();
						if (NotebookAsset) NotebookAsset->MarkPackageDirty();
					})
					.IsReadOnly_Lambda([this]() {
						if (NotebookAsset)
						{
							return NotebookAsset->IsCellReadOnly(NotebookAsset->SelectedCellIndex);
						}
						return false;
					})
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 10, 0, 0)
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)
				]

				+SVerticalBox::Slot()
				.FillHeight(0.3f)
				.Padding(0, 10, 0, 0)
				[
					SNew(SScrollBox)
					+SScrollBox::Slot()
					[
						SNew(STextBlock)
						.Text_Lambda([SelectedData]() { return FText::FromString(SelectedData->Output); })
						.AutoWrapText(true)
					]
				]
			]
		]
	];

	// Perform initial search with existing filter text
	if (!SearchFilterText.IsEmpty())
	{
		PerformSearch();
	}
}

void SNumericNotebook::Construct(const FArguments& InArgs)
{
	NotebookAsset = InArgs._NotebookAsset;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		[
			SNew(SVerticalBox)
			
			// 标题栏
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				[
					SNew(STextBlock)
					.Text(INVTEXT("Numeric Notebook - Interactive Coding Environment"))
					.Font(FAppStyle::Get().GetFontStyle("HeadingFont"))
					.Justification(ETextJustify::Center)
					.Margin(10.0f)
				]
			]
			
			// 工具栏
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(5.0f)
				[
					SNew(SHorizontalBox)
					
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(INVTEXT("Add New Code Cell"))
						.OnClicked(this, &SNumericNotebook::OnAddNewCellButtonClicked)
						.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
						.IsEnabled_Lambda([this]() { return NotebookAsset ? !NotebookAsset->bIsCompiling : true; })
					]
					
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5.0f, 0.0f)
					[
						SNew(SButton)
						.Text(INVTEXT("Restart Interpreter"))
						.OnClicked(this, &SNumericNotebook::OnRestartInterpButtonClicked)
						.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
						.IsEnabled_Lambda([this]() { return NotebookAsset ? !NotebookAsset->bIsCompiling : true; })
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5.0f, 0.0f)
					[
						SNew(SButton)
						.Text(INVTEXT("Refresh Highlighting"))
						.ToolTipText(INVTEXT("Manually refresh C++ semantic symbols for syntax highlighting"))
						.OnClicked(this, &SNumericNotebook::OnRefreshHighlightingButtonClicked)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.IsEnabled_Lambda([this]() { return NotebookAsset && !NotebookAsset->bIsCompiling; })
					]
					
					// PCH Profile Selector
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(EVerticalAlignment::VAlign_Center)
					.Padding(15.0f, 0.0f, 5.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(INVTEXT("PCH Profile:"))
					]
					
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SComboButton)
						.OnGetMenuContent(this, &SNumericNotebook::GeneratePCHProfileMenu)
						.ButtonContent()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								if (NotebookAsset && !NotebookAsset->PCHProfile.IsNone())
								{
									return FText::FromName(NotebookAsset->PCHProfile);
								}
								return INVTEXT("Default");
							})
						]
					]
										
					// Interpreter Initialization Status
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(EVerticalAlignment::VAlign_Center)
					.Padding(15.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						.Visibility_Lambda([this]() { 
							return (NotebookAsset && NotebookAsset->bIsInitializingInterpreter) 
								? EVisibility::Visible 
								: EVisibility::Collapsed; 
						})
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SThrobber)
							.NumPieces(5)
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(8.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(INVTEXT("Initializing Interpreter..."))
							.ColorAndOpacity(FLinearColor(0.5f, 0.8f, 1.0f, 1.0f))
							.Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))
						]
					]
					
					+SHorizontalBox::Slot()
					.FillWidth(1.0f)
				]
			]
			
			// 单元格列表
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(ScrollBox, SScrollBox)
				.Orientation(Orient_Vertical)
			]
		]
	];
	
	UpdateDocumentUI();
}

void SNumericNotebook::SetSelectedCell(int32 Index)
{
	if (NotebookAsset)
	{
		NotebookAsset->SetSelectedCell(Index);
		UpdateDocumentUI();
	}
}

void SNumericNotebook::UpdateDocumentUI()
{
	if (!ScrollBox.IsValid() || !NotebookAsset) return;

	ScrollBox->ClearChildren();

	for (int32 i = 0; i < NotebookAsset->Cells.Num(); i++)
	{
		int32 CurrentIndex = i;
		
		ScrollBox->AddSlot()
		.Padding(5.0f)
		[
			SNew(SClingNotebookCell)
			.CellData(&NotebookAsset->Cells[i])
			.NotebookAsset(NotebookAsset)
			.CellIndex(CurrentIndex)
			.OnRunToHere_Lambda([this, CurrentIndex]() { 
				if (NotebookAsset) 
				{
					NotebookAsset->RunToHere(CurrentIndex);
					UpdateDocumentUI();
				}
			})
			.OnUndoToHere_Lambda([this, CurrentIndex]() { 
				if (NotebookAsset)
				{
					NotebookAsset->UndoToHere(CurrentIndex);
					UpdateDocumentUI();
				}
			})
			.OnDeleteCell_Lambda([this, CurrentIndex]() { 
				if (NotebookAsset)
				{
					NotebookAsset->DeleteCell(CurrentIndex);
					UpdateDocumentUI();
				}
			})
			.OnAddCellBelow_Lambda([this, CurrentIndex]() { 
				if (NotebookAsset)
				{
					NotebookAsset->AddNewCell(CurrentIndex + 1);
					UpdateDocumentUI();
				}
			})
			.OnContentChanged_Lambda([this](const FText&) { if (NotebookAsset) NotebookAsset->MarkPackageDirty(); })
			.IsEnabled_Lambda([this, CurrentIndex]() { 
				return NotebookAsset ? !NotebookAsset->CompilingCells.Contains(CurrentIndex) : true; 
			})
			.OnSelected_Lambda([this, CurrentIndex]() { SetSelectedCell(CurrentIndex); })
			.IsSelected_Lambda([this, CurrentIndex]() { 
				return NotebookAsset ? NotebookAsset->SelectedCellIndex == CurrentIndex : false; 
			})
		];
	}
}

FReply SNumericNotebook::OnAddNewCellButtonClicked()
{
	if (NotebookAsset)
	{
		NotebookAsset->AddNewCell();
		UpdateDocumentUI();
	}
	return FReply::Handled();
}

FReply SNumericNotebook::OnRestartInterpButtonClicked()
{
	if (NotebookAsset)
	{
		NotebookAsset->RestartInterpreter();
		UpdateDocumentUI();
	}
	return FReply::Handled();
}

FReply SNumericNotebook::OnRefreshHighlightingButtonClicked()
{
	if (NotebookAsset)
	{
		NotebookAsset->SemanticInfoProvider.Refresh(NotebookAsset->GetInterpreter());
		UpdateDocumentUI();
	}
	return FReply::Handled();
}

FReply SNumericNotebook::OnFoldAllButtonClicked()
{
	if (NotebookAsset)
	{
		for (auto& Cell : NotebookAsset->Cells)
		{
			Cell.bIsExpanded = false;
		}
		UpdateDocumentUI();
	}
	return FReply::Handled();
}

FReply SNumericNotebook::OnUnfoldAllButtonClicked()
{
	if (NotebookAsset)
	{
		for (auto& Cell : NotebookAsset->Cells)
		{
			Cell.bIsExpanded = true;
		}
		UpdateDocumentUI();
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SNumericNotebook::GeneratePCHProfileMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	const UClingSetting* Settings = GetDefault<UClingSetting>();
	if (!Settings)
	{
		return MenuBuilder.MakeWidget();
	}

	// Add Default profile
	MenuBuilder.AddMenuEntry(
		FText::FromName(Settings->DefaultPCHProfile.ProfileName),
		INVTEXT("Use default PCH profile"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SNumericNotebook::OnPCHProfileSelected, Settings->DefaultPCHProfile.ProfileName)
		)
	);

	// Add custom profiles
	for (const FClingPCHProfile& Profile : Settings->PCHProfiles)
	{
		if (Profile.bEnabled)
		{
			FText DisplayText = Profile.ProfileName.IsNone() 
				? FText::FromString(TEXT("Unnamed Profile"))
				: FText::FromName(Profile.ProfileName);

			MenuBuilder.AddMenuEntry(
				DisplayText,
				FText::Format(INVTEXT("Use PCH profile: {0}"), DisplayText),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(this, &SNumericNotebook::OnPCHProfileSelected, Profile.ProfileName)
				)
			);
		}
	}

	return MenuBuilder.MakeWidget();
}

void SNumericNotebook::OnPCHProfileSelected(FName ProfileName)
{
	if (NotebookAsset)
	{
		NotebookAsset->PCHProfile = ProfileName;
		NotebookAsset->MarkPackageDirty();
	}
}

void SClingNotebookDetailsPanel::OnDetailSearchTextChanged(const FText& InText)
{
	SearchFilterText = InText.ToString();
	PerformSearch();
}

void SClingNotebookDetailsPanel::OnDetailSearchCaseSensitiveChanged(ECheckBoxState NewState)
{
	bSearchCaseSensitive = (NewState == ECheckBoxState::Checked);
	PerformSearch();
}

void SClingNotebookDetailsPanel::PerformSearch(bool bReverse)
{
	if (!DetailCodeTextBox.IsValid() || SearchFilterText.IsEmpty())
	{
		return;
	}

	ESearchCase::Type SearchCase = bSearchCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	DetailCodeTextBox->BeginSearch(FText::FromString(SearchFilterText), SearchCase, bReverse);
}
