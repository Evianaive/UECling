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
	CellIndex = InArgs._CellIndex;
	OnRunToHereDelegate = InArgs._OnRunToHere;
	OnUndoToHereDelegate = InArgs._OnUndoToHere;
	OnDeleteCellDelegate = InArgs._OnDeleteCell;
	OnAddCellAboveDelegate = InArgs._OnAddCellAbove;
	OnAddCellBelowDelegate = InArgs._OnAddCellBelow;
	OnContentChangedDelegate = InArgs._OnContentChanged;

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
						if (CellData->bIsCompiling) return false;
						if (!NotebookAsset || !NotebookAsset->Cells.IsValidIndex(CellIndex)) return true;
						for (int32 i = CellIndex; i < NotebookAsset->Cells.Num(); ++i)
						{
							if (NotebookAsset->Cells[i].bIsCompleted) return false;
						}
						return true;
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
				
				// 控制按钮
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(INVTEXT("+ Above"))
					.OnClicked(this, &SClingNotebookCell::OnAddAboveButtonClicked)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				]
				
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(INVTEXT("+ Below"))
					.OnClicked(this, &SClingNotebookCell::OnAddBelowButtonClicked)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
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
						if (!NotebookAsset || !NotebookAsset->Cells.IsValidIndex(CellIndex)) return false;
						if (CellData->bIsCompleted || CellData->bIsCompiling) return false;
						for (int32 i = CellIndex + 1; i < NotebookAsset->Cells.Num(); ++i)
						{
							if (NotebookAsset->Cells[i].bIsCompleted) return false;
						}
						return true;
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
					if (CellData->bIsCompiling) return true;
					if (!NotebookAsset || !NotebookAsset->Cells.IsValidIndex(CellIndex)) return false;
					for (int32 i = CellIndex; i < NotebookAsset->Cells.Num(); ++i)
					{
						if (NotebookAsset->Cells[i].bIsCompleted) return true;
					}
					return false;
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

FReply SClingNotebookCell::OnAddAboveButtonClicked()
{
	OnAddCellAboveDelegate.ExecuteIfBound();
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
	int32 InIndex = ExecutionQueue[0];
	ExecutionQueue.RemoveAt(0);

	void* Interp = GetOrStartInterp();
	if (!Interp)
	{
		bIsProcessingQueue = false;
		return;
	}

	FClingNotebookCellData& Cell = NotebookAsset->Cells[InIndex];
	FString Code = Cell.Content;
	bool bExecuteInGameThread = Cell.bExecuteInGameThread;

	auto ExecuteTask = [this, Interp, InIndex, Code]()
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
		
		static thread_local FString AsyncErrors;
		AsyncErrors.Reset();
		
		Cpp::BeginStdStreamCapture(Cpp::kStdErr);
		
		int32 CompilationResult = Cpp::Process(TCHAR_TO_ANSI(*Code));
		
		Cpp::EndStdStreamCapture([](const char* Result)
		{
			AsyncErrors = UTF8_TO_TCHAR(Result);
		});

		FString OutErrors = AsyncErrors;

		AsyncTask(ENamedThreads::GameThread, [this, InIndex, CompilationResult, OutErrors]()
		{
			if (NotebookAsset && NotebookAsset->Cells.IsValidIndex(InIndex))
			{
				FClingNotebookCellData& CellData = NotebookAsset->Cells[InIndex];
				CellData.Output = OutErrors;
				CellData.bHasOutput = true;
				CellData.bIsCompiling = false;
				
				if (CompilationResult == 0)
				{
					CellData.bIsCompleted = true;
					if (CellData.Output.IsEmpty())
					{
						CellData.Output = FString::Printf(TEXT("Success (Executed at %s)"), *FDateTime::Now().ToString());
					}
				}
				else
				{
					CellData.bIsCompleted = false;
				}
				
				NotebookAsset->MarkPackageDirty();
			}

			CompilingCells.Remove(InIndex);
			bIsCompiling = (CompilingCells.Num() > 0);
			bIsProcessingQueue = false;
			ProcessNextInQueue();
		});
	};

	if (bExecuteInGameThread)
	{
		ExecuteTask();
	}
	else
	{
		AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, ExecuteTask);
	}
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
	if (!IsInGameThread())
	{
		AsyncTask(ENamedThreads::GameThread, [this]() { UpdateDocumentUI(); });
		return;
	}

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
			.OnRunToHere_Lambda([this, CurrentIndex]() { RunToHere(CurrentIndex); })
			.OnUndoToHere_Lambda([this, CurrentIndex]() { UndoToHere(CurrentIndex); })
			.OnDeleteCell_Lambda([this, CurrentIndex]() { DeleteCell(CurrentIndex); })
			.OnAddCellAbove_Lambda([this, CurrentIndex]() { AddNewCell(CurrentIndex); })
			.OnAddCellBelow_Lambda([this, CurrentIndex]() { AddNewCell(CurrentIndex + 1); })
			.OnContentChanged_Lambda([this](const FText&) { if (NotebookAsset) NotebookAsset->MarkPackageDirty(); })
			.IsEnabled_Lambda([this, CurrentIndex]() { return !CompilingCells.Contains(CurrentIndex); })
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
