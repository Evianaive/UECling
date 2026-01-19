#include "SNumericNotebook.h"
#include "ClingRuntime.h"
#include "CppInterOp/CppInterOp.h"

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

void SClingNotebookCell::Construct(const FArguments& InArgs)
{
	CellData = InArgs._CellData;
	OnRunCodeDelegate = InArgs._OnRunCode;
	OnRunToHereDelegate = InArgs._OnRunToHere;
	OnDeleteCellDelegate = InArgs._OnDeleteCell;
	OnAddCellAboveDelegate = InArgs._OnAddCellAbove;
	OnAddCellBelowDelegate = InArgs._OnAddCellBelow;
	OnContentChangedDelegate = InArgs._OnContentChanged;

	UpdateCellUI();
}

void SClingNotebookCell::UpdateCellUI()
{
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
					.Visibility_Lambda([this]() { return CellData->bIsCompleted ? EVisibility::Visible : EVisibility::Collapsed; })
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
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(INVTEXT("Run"))
					.OnClicked(this, &SClingNotebookCell::OnRunButtonClicked)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
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

FReply SClingNotebookCell::OnRunButtonClicked()
{
	OnRunCodeDelegate.ExecuteIfBound();
	return FReply::Handled();
}

FReply SClingNotebookCell::OnRunToHereButtonClicked()
{
	OnRunToHereDelegate.ExecuteIfBound();
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
					]
					
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5.0f, 0.0f)
					[
						SNew(SButton)
						.Text(INVTEXT("Restart Interpreter"))
						.OnClicked(this, &SNumericNotebook::OnRestartInterpButtonClicked)
						.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
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
	if (CurrentInterp == nullptr)
	{
		FClingRuntimeModule& RuntimeModule = FModuleManager::LoadModuleChecked<FClingRuntimeModule>(TEXT("ClingRuntime"));
		CurrentInterp = RuntimeModule.StartNewInterp();
	}
	return CurrentInterp;
}

void SNumericNotebook::RestartInterp()
{
	FClingRuntimeModule& RuntimeModule = FModuleManager::LoadModuleChecked<FClingRuntimeModule>(TEXT("ClingRuntime"));
	// Note: We don't have a DeleteInterpreter in ClingRuntimeModule for individual interps yet,
	// but we can start a new one and the old one will be cleaned up on shutdown.
	// Actually, StartNewInterp adds to Interps array.
	if (CurrentInterp)
	{
		RuntimeModule.DeleteInterp(CurrentInterp);
	}
	CurrentInterp = RuntimeModule.StartNewInterp();
	
	// Reset completed status for all cells
	if (NotebookAsset)
	{
		for (auto& Cell : NotebookAsset->Cells)
		{
			Cell.bIsCompleted = false;
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
	if (NotebookAsset && NotebookAsset->Cells.IsValidIndex(InIndex))
	{
		NotebookAsset->Cells.RemoveAt(InIndex);
		NotebookAsset->MarkPackageDirty();
		UpdateDocumentUI();
	}
}

bool SNumericNotebook::RunCell(int32 InIndex)
{
	if (!NotebookAsset || !NotebookAsset->Cells.IsValidIndex(InIndex)) return false;

	void* Interp = GetOrStartInterp();
	if (!Interp) return false;

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
	FClingNotebookCellData& Cell = NotebookAsset->Cells[InIndex];
	Cell.Output = ""; // 清空之前的输出
	
	thread_local FString FErrors;
	FErrors.Reset();
	Cpp::BeginStdStreamCapture(Cpp::kStdErr);
	
	int32 CompilationResult = Cpp::Process(TCHAR_TO_ANSI(*Cell.Content));
	
	Cpp::EndStdStreamCapture([](const char* Result)
	{
		FErrors = UTF8_TO_TCHAR(Result);
	});

	if (CompilationResult == 0)
	{
		Cell.bIsCompleted = true;
		if (FErrors.IsEmpty())
		{
			Cell.Output = FString::Printf(TEXT("Success (Executed at %s)"), *FDateTime::Now().ToString());
			Cell.bHasOutput = true;
		}
		else
		{
			Cell.Output = FErrors;
			Cell.bHasOutput = true;
		}
	}
	else
	{
		Cell.bIsCompleted = false;
		Cell.Output = FErrors;
		Cell.bHasOutput = true;
	}
	
	NotebookAsset->MarkPackageDirty();
	UpdateDocumentUI();

	return CompilationResult == 0;
}

void SNumericNotebook::RunToHere(int32 InIndex)
{
	if (!NotebookAsset || !NotebookAsset->Cells.IsValidIndex(InIndex)) return;

	for (int32 i = 0; i <= InIndex; ++i)
	{
		if (!RunCell(i))
		{
			break; // 如果出现报错，需要停止后面的节执行
		}
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
			.OnRunCode_Lambda([this, CurrentIndex]() { RunCell(CurrentIndex); })
			.OnRunToHere_Lambda([this, CurrentIndex]() { RunToHere(CurrentIndex); })
			.OnDeleteCell_Lambda([this, CurrentIndex]() { DeleteCell(CurrentIndex); })
			.OnAddCellAbove_Lambda([this, CurrentIndex]() { AddNewCell(CurrentIndex); })
			.OnAddCellBelow_Lambda([this, CurrentIndex]() { AddNewCell(CurrentIndex + 1); })
			.OnContentChanged_Lambda([this](const FText&) { if (NotebookAsset) NotebookAsset->MarkPackageDirty(); })
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
