#include "SNumericNotebook.h"
#include "ClingRuntime.h"
#include "CppInterOp/CppInterOp.h"
#include "Async/Async.h"

#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
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
	NotebookWidget = InArgs._NotebookWidget;
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
					.Text(INVTEXT("Code"))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 1.0f, 1.0f)))
				]
				
				// 完成标记
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(5, 0)
				[
					SNew(STextBlock)
					.Text(INVTEXT("[Completed]"))
					.ColorAndOpacity(FLinearColor::Green)
					.Visibility_Lambda([this]() { return (CellData->bIsCompleted && !CellData->bIsCompiling) ? EVisibility::Visible : EVisibility::Collapsed; })
				]

				// 编译中标记
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(5, 0)
				[
					SNew(SHorizontalBox)
					.Visibility_Lambda([this]() { return CellData->bIsCompiling ? EVisibility::Visible : EVisibility::Collapsed; })
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
						return NotebookWidget ? !NotebookWidget->IsCellReadOnly(CellIndex) : true;
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
						return NotebookWidget ? NotebookWidget->IsCellAddableBelow(CellIndex) : true;
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
						return NotebookWidget ? !NotebookWidget->IsCellReadOnly(CellIndex) : true;
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
						if (CellData->bIsCompiling) return false;
						if (!NotebookAsset || !NotebookAsset->Cells.IsValidIndex(CellIndex)) return false;
						for (int32 i = CellIndex; i < NotebookAsset->Cells.Num(); ++i)
						{
							if (NotebookAsset->Cells[i].bIsCompleted) return true;
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
						return NotebookWidget ? NotebookWidget->IsCellDeletable(CellIndex) : true;
					})
				]
			]
			
			// 代码输入区域
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 2)
			[
				SAssignNew(CodeTextBox, SCppMultiLineEditableTextBox)
				.Text(FText::FromString(CellData->Content))
				.OnTextChanged(this, &SClingNotebookCell::OnCodeTextChanged)
				.Visibility_Lambda([this]() { return CellData->bIsExpanded ? EVisibility::Visible : EVisibility::Collapsed; })
				.IsReadOnly_Lambda([this]() {
					return NotebookWidget ? NotebookWidget->IsCellReadOnly(CellIndex) : false;
				})
			]
			
			// 输出显示区域
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.Visibility_Lambda([this]() { return (CellData->bIsExpanded && CellData->bHasOutput) ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(CellData->Output); })
					.WrapTextAt(800.0f)
					.Margin(5.0f)
				]
			]
		]
	];
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
	NotebookWidgetPtr = InArgs._NotebookWidget;

	if (TSharedPtr<SNumericNotebook> NotebookWidget = NotebookWidgetPtr.Pin())
	{
		NotebookWidget->OnCellSelectionChanged.AddSP(this, &SClingNotebookDetailsPanel::OnSelectionChanged);
	}

	Refresh();
}

void SClingNotebookDetailsPanel::OnSelectionChanged(int32 NewIndex)
{
	Refresh();
}

void SClingNotebookDetailsPanel::Refresh()
{
	TSharedPtr<SNumericNotebook> NotebookWidget = NotebookWidgetPtr.Pin();
	FClingNotebookCellData* SelectedData = NotebookWidget.IsValid() ? NotebookWidget->GetSelectedCellData() : nullptr;

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

	TSharedPtr<SVerticalBox> FunctionButtonsBox;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(10.0f)
		[
			SNew(SVerticalBox)
			
			// Top Bar for Function Buttons
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 10)
			[
				SAssignNew(FunctionButtonsBox, SVerticalBox)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 10, 0, 0)
			[
				SNew(SSeparator)
				.Orientation(Orient_Horizontal)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 10)
			[
				SNew(STextBlock)
				.Text(FText::Format(INVTEXT("Editing Cell #{0}"), FText::AsNumber(NotebookWidget->GetSelectedCellIndex())))
				.Font(FAppStyle::Get().GetFontStyle("HeadingFont"))
			]

			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.FillHeight(0.7f)
				[
					SAssignNew(DetailCodeTextBox, SCppMultiLineEditableTextBox)
					.Text(FText::FromString(SelectedData->Content))
					.OnTextChanged_Lambda([this, SelectedData](const FText& InText) {
						SelectedData->Content = InText.ToString();
						if (NotebookAsset) NotebookAsset->MarkPackageDirty();
					})
					.IsReadOnly_Lambda([this]() {
						if (TSharedPtr<SNumericNotebook> NW = NotebookWidgetPtr.Pin())
						{
							return NW->IsCellReadOnly(NW->GetSelectedCellIndex());
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

	// Detect void functions with no arguments and add buttons
	FString Content = SelectedData->Content;
	FRegexPattern Pattern(TEXT("void\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(\\s*\\)"));
	FRegexMatcher Matcher(Pattern, Content);

	TArray<FString> DetectedFunctions;
	while (Matcher.FindNext())
	{
		DetectedFunctions.Add(Matcher.GetCaptureGroup(1));
	}

	if (DetectedFunctions.Num() > 0)
	{
		TSharedPtr<SHorizontalBox> HBox;
		FunctionButtonsBox->AddSlot()
		.AutoHeight()
		[
			SAssignNew(HBox, SHorizontalBox)
		];

		for (const FString& FuncName : DetectedFunctions)
		{
			HBox->AddSlot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(FText::FromString(FuncName))
				// .ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.OnClicked_Lambda([this, FuncName]() -> FReply
				{
					if (TSharedPtr<SNumericNotebook> NW = NotebookWidgetPtr.Pin())
					{
						void* Interp = NW->NotebookAsset ? NW->NotebookAsset->GetInterpreter() : nullptr;
						if (Interp)
						{
							FScopeLock Lock(&FClingRuntimeModule::Get().GetCppInterOpLock());
							void* StoreInterp = Cpp::GetInterpreter();
							Cpp::ActivateInterpreter(Interp);
							
							Cpp::TCppScope_t FuncHandle = Cpp::GetNamed(TCHAR_TO_ANSI(*FuncName), Cpp::GetGlobalScope());
							if (FuncHandle && Cpp::IsFunction(FuncHandle))
							{
								if (Cpp::GetFunctionNumArgs((Cpp::TCppFunction_t)FuncHandle) == 0)
								{
									void* Addr = Cpp::GetFunctionAddress((Cpp::TCppFunction_t)FuncHandle);
									if (Addr)
									{
										typedef void (*VoidFunc)();
										VoidFunc f = (VoidFunc)Addr;
										f();
									}
								}
							}
							
							Cpp::ActivateInterpreter(StoreInterp);
						}
					}
					return FReply::Handled();
				})
			];
		}
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
						.IsEnabled_Lambda([this]() { return !bIsCompiling; })
					]
					
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5.0f, 0.0f)
					[
						SNew(SButton)
						.Text(INVTEXT("Restart Interpreter"))
						.OnClicked(this, &SNumericNotebook::OnRestartInterpButtonClicked)
						.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
						.IsEnabled_Lambda([this]() { return !bIsCompiling; })
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5.0f, 0.0f)
					[
						SNew(SButton)
						.Text(INVTEXT("Fold All"))
						.OnClicked(this, &SNumericNotebook::OnFoldAllButtonClicked)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5.0f, 0.0f)
					[
						SNew(SButton)
						.Text(INVTEXT("Unfold All"))
						.OnClicked(this, &SNumericNotebook::OnUnfoldAllButtonClicked)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
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

void* SNumericNotebook::GetOrStartInterp()
{
	if (NotebookAsset)
	{
		return NotebookAsset->GetInterpreter();
	}
	return nullptr;
}

void SNumericNotebook::RestartInterp()
{
	if (NotebookAsset)
	{
		NotebookAsset->RestartInterpreter();
		
		// Reset status for all cells
		for (auto& Cell : NotebookAsset->Cells)
		{
			Cell.bIsCompleted = false;
			Cell.bHasOutput = false;
			Cell.Output.Empty();
		}
	}
	
	UpdateDocumentUI();
}

void SNumericNotebook::AddNewCell(int32 InIndex)
{
	if (!NotebookAsset) return;

	FClingNotebookCellData NewCellData;
	NewCellData.bIsExpanded = true;
	NewCellData.bIsCompleted = false;

	if (InIndex == -1 || InIndex >= NotebookAsset->Cells.Num())
	{
		NotebookAsset->Cells.Add(NewCellData);
	}
	else
	{
		NotebookAsset->Cells.Insert(NewCellData, InIndex);
	}

	NotebookAsset->MarkPackageDirty();
	UpdateDocumentUI();
}

void SNumericNotebook::DeleteCell(int32 InIndex)
{
	if (NotebookAsset)
	{
		if (NotebookAsset->Cells.IsValidIndex(InIndex))
		{
			NotebookAsset->Cells.RemoveAt(InIndex);
			
			// Adjust selection
			if (SelectedCellIndex == InIndex)
			{
				SetSelectedCell(-1);
			}
			else if (SelectedCellIndex > InIndex)
			{
				SetSelectedCell(SelectedCellIndex - 1);
			}

			NotebookAsset->MarkPackageDirty();
			UpdateDocumentUI();
		}
	}
}

void SNumericNotebook::RunCell(int32 InIndex)
{
	if (!NotebookAsset || !NotebookAsset->Cells.IsValidIndex(InIndex)) return;
	
	if (ExecutionQueue.Contains(InIndex) || CompilingCells.Contains(InIndex)) return;

	ExecutionQueue.Add(InIndex);
	NotebookAsset->Cells[InIndex].bIsCompiling = true;
	CompilingCells.Add(InIndex);
	bIsCompiling = true;

	ProcessNextInQueue();
}

void SNumericNotebook::ProcessNextInQueue()
{
	if (bIsProcessingQueue || ExecutionQueue.Num() == 0) return;

	bIsProcessingQueue = true;

	while (ExecutionQueue.Num() > 0)
	{
		int32 InIndex = ExecutionQueue[0];
		ExecutionQueue.RemoveAt(0);

		void* Interp = GetOrStartInterp();
		if (!Interp)
		{
			continue;
		}

		FClingNotebookCellData& Cell = NotebookAsset->Cells[InIndex];
		FString Code = Cell.Content;

		// Execute Synchronously on GameThread
		{
			FScopeLock Lock(&FClingRuntimeModule::Get().GetCppInterOpLock());

			struct FLocal
			{
				FLocal(void* InInterp)
				{
					StoreInterp = Cpp::GetInterpreter();
					Cpp::ActivateInterpreter(InInterp);
				}
				~FLocal()
				{
					Cpp::ActivateInterpreter(StoreInterp);
				}
				void* StoreInterp;
			};
			FLocal Guard{Interp};
			
			static FString StaticErrors;
			StaticErrors.Reset();
			
			Cpp::BeginStdStreamCapture(Cpp::kStdErr);
			
			int32 CompilationResult = Cpp::Process(TCHAR_TO_ANSI(*Code));
			
			Cpp::EndStdStreamCapture([](const char* Result)
			{
				StaticErrors = UTF8_TO_TCHAR(Result);
			});

			FString OutErrors = StaticErrors;

			if (NotebookAsset && (NotebookAsset->Cells.Num() > InIndex))
			{
				FClingNotebookCellData& CellDataAt = NotebookAsset->Cells[InIndex];
				CellDataAt.Output = OutErrors;
				CellDataAt.bHasOutput = true;
				CellDataAt.bIsCompiling = false;
				
				if (CompilationResult == 0)
				{
					CellDataAt.bIsCompleted = true;
					if (CellDataAt.Output.IsEmpty())
					{
						CellDataAt.Output = FString::Printf(TEXT("Success (Executed at %s)"), *FDateTime::Now().ToString());
					}
				}
				else
				{
					CellDataAt.bIsCompleted = false;
				}
				
				NotebookAsset->MarkPackageDirty();
			}

			CompilingCells.Remove(InIndex);
		}
	}

	bIsCompiling = (CompilingCells.Num() > 0);
	bIsProcessingQueue = false;
}

void SNumericNotebook::RunToHere(int32 InIndex)
{
	if (!NotebookAsset || !NotebookAsset->Cells.IsValidIndex(InIndex)) return;

	for (int32 i = 0; i <= InIndex; ++i)
	{
		if (!ExecutionQueue.Contains(i) && !CompilingCells.Contains(i) && !NotebookAsset->Cells[i].bIsCompleted)
		{
			ExecutionQueue.Add(i);
			NotebookAsset->Cells[i].bIsCompiling = true;
			CompilingCells.Add(i);
		}
	}
	
	if (CompilingCells.Num() > 0)
	{
		bIsCompiling = true;
		ProcessNextInQueue();
	}
}

void SNumericNotebook::UndoToHere(int32 InIndex)
{
	if (!NotebookAsset || !NotebookAsset->Cells.IsValidIndex(InIndex)) return;
	if (bIsCompiling) return;

	int32 UndoCount = 0;
	bool bNeedsReset = false;
	for (int32 i = InIndex; i < NotebookAsset->Cells.Num(); ++i)
	{
		if (NotebookAsset->Cells[i].bIsCompleted)
		{
			UndoCount++;
		}
		if (NotebookAsset->Cells[i].bIsCompleted || NotebookAsset->Cells[i].bHasOutput)
		{
			bNeedsReset = true;
		}
	}

	if (UndoCount > 0)
	{
		FScopeLock Lock(&FClingRuntimeModule::Get().GetCppInterOpLock());
		void* Interp = GetOrStartInterp();
		if (Interp)
		{
			void* StoreInterp = Cpp::GetInterpreter();
			Cpp::ActivateInterpreter(Interp);
			Cpp::Undo(UndoCount);
			Cpp::ActivateInterpreter(StoreInterp);
		}
	}

	if (bNeedsReset)
	{
		for (int32 i = InIndex; i < NotebookAsset->Cells.Num(); ++i)
		{
			NotebookAsset->Cells[i].bIsCompleted = false;
			NotebookAsset->Cells[i].bHasOutput = false;
			NotebookAsset->Cells[i].Output.Empty();
		}

		NotebookAsset->MarkPackageDirty();
		UpdateDocumentUI();
	}
}

void SNumericNotebook::UpdateDocumentUI()
{
	// if (!IsInGameThread())
	// {
	// 	AsyncTask(ENamedThreads::GameThread, [this]() { UpdateDocumentUI(); });
	// 	return;
	// }

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
			.NotebookWidget(this)
			.CellIndex(CurrentIndex)
			.OnRunToHere_Lambda([this, CurrentIndex]() { RunToHere(CurrentIndex); })
			.OnUndoToHere_Lambda([this, CurrentIndex]() { UndoToHere(CurrentIndex); })
			.OnDeleteCell_Lambda([this, CurrentIndex]() { DeleteCell(CurrentIndex); })
			.OnAddCellBelow_Lambda([this, CurrentIndex]() { AddNewCell(CurrentIndex + 1); })
			.OnContentChanged_Lambda([this](const FText&) { if (NotebookAsset) NotebookAsset->MarkPackageDirty(); })
			.IsEnabled_Lambda([this, CurrentIndex]() { return !CompilingCells.Contains(CurrentIndex); })
			.OnSelected_Lambda([this, CurrentIndex]() { SetSelectedCell(CurrentIndex); })
			.IsSelected_Lambda([this, CurrentIndex]() { return SelectedCellIndex == CurrentIndex; })
		];
	}
}

FReply SNumericNotebook::OnAddNewCellButtonClicked()
{
	AddNewCell();
	return FReply::Handled();
}

FReply SNumericNotebook::OnRestartInterpButtonClicked()
{
	RestartInterp();
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

void SNumericNotebook::SetSelectedCell(int32 Index)
{
	if (SelectedCellIndex != Index)
	{
		SelectedCellIndex = Index;
		OnCellSelectionChanged.Broadcast(SelectedCellIndex);
		
		// Optional: Refresh UI to show highlight
		UpdateDocumentUI();
	}
}

FClingNotebookCellData* SNumericNotebook::GetSelectedCellData() const
{
	if (NotebookAsset && (NotebookAsset->Cells.Num() > SelectedCellIndex) && (SelectedCellIndex >= 0))
	{
		auto& CellAt = NotebookAsset->Cells[SelectedCellIndex];
		return &CellAt;
	}
	return nullptr;
}

bool SNumericNotebook::IsCellReadOnly(int32 Index) const
{
	if (!NotebookAsset || !NotebookAsset->Cells.IsValidIndex(Index)) return false;
	if (NotebookAsset->Cells[Index].bIsCompiling) return true;
	
	for (int32 i = Index; i < NotebookAsset->Cells.Num(); ++i)
	{
		if (NotebookAsset->Cells[i].bIsCompleted) return true;
	}
	return false;
}

bool SNumericNotebook::IsCellDeletable(int32 Index) const
{
	// 只有非只读（未编译且其后无编译节）的节才可以删除
	return !IsCellReadOnly(Index);
}

bool SNumericNotebook::IsCellAddableBelow(int32 Index) const
{
	// 下方添加节时要看下方节的状态
	if (!NotebookAsset) return false;
	int32 NextIndex = Index + 1;
	if (NextIndex >= NotebookAsset->Cells.Num()) return true; // 最后一个节下方总是可以添加
	
	// 如果下方节是只读的（即它是编译序列的一部分），则不允许在其上方插入
	return !IsCellReadOnly(NextIndex);
}
