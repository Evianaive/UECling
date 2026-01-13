#include "SNumericNotebook.h"
#include "CppHighLight/CppRichTextSyntaxHighlightMarshaller.h"
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

void SClingNotebookCell::Construct(const FArguments& InArgs)
{
	CellData = InArgs._CellData;
	OnRunCodeDelegate = InArgs._OnRunCode;
	OnDeleteCellDelegate = InArgs._OnDeleteCell;
	OnAddCellAboveDelegate = InArgs._OnAddCellAbove;
	OnAddCellBelowDelegate = InArgs._OnAddCellBelow;
	OnContentChangedDelegate = InArgs._OnContentChanged;

	UpdateCellUI();
}

void SClingNotebookCell::UpdateCellUI()
{
	// 定义单元格背景色根据类型变化
	FSlateColor CellBackgroundColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // 默认深灰色
	if (CellData.CellType == ECellType::Markdown)
	{
		CellBackgroundColor = FLinearColor(0.15f, 0.15f, 0.2f, 1.0f); // Markdown为蓝灰色
	}
	else if (CellData.CellType == ECellType::Raw)
	{
		CellBackgroundColor = FLinearColor(0.18f, 0.15f, 0.15f, 1.0f); // Raw为红灰色
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(2.0f)
		[
			SNew(SVerticalBox)
			
			// 单元格类型标签和控制按钮行
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				
				// 类型标签
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						switch (CellData.CellType)
						{
						case ECellType::Code: return FText::FromString(TEXT("Code"));
						case ECellType::Markdown: return FText::FromString(TEXT("Markdown"));
						case ECellType::Raw: return FText::FromString(TEXT("Raw"));
						default: return FText::FromString(TEXT("Code"));
						}
					})
					.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 1.0f, 1.0f)))
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
					.Text(FText::FromString(TEXT("+ Above")))
					.ToolTipText(FText::FromString(TEXT("Add cell above this one")))
					.OnClicked(this, &SClingNotebookCell::OnAddAboveButtonClicked)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				]
				
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("+ Below")))
					.ToolTipText(FText::FromString(TEXT("Add cell below this one")))
					.OnClicked(this, &SClingNotebookCell::OnAddBelowButtonClicked)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				]
				
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Run")))
					.ToolTipText(FText::FromString(TEXT("Execute this cell")))
					.OnClicked(this, &SClingNotebookCell::OnRunButtonClicked)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.Visibility_Lambda([this]() { return CellData.CellType == ECellType::Code ? EVisibility::Visible : EVisibility::Collapsed; })
				]
				
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("X")))
					.ToolTipText(FText::FromString(TEXT("Delete this cell")))
					.OnClicked(this, &SClingNotebookCell::OnDeleteButtonClicked)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				]
			]
			
			// 分隔线
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 2)
			[
				SNew(SSeparator)
			]
			
			// 代码/文本输入区域
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(CodeTextBox, SMultiLineEditableTextBox)
				.Text(FText::FromString(CellData.Content))
				.OnTextChanged(this, &SClingNotebookCell::OnCodeTextChanged)
				// .Font(FAppStyle::Get().GetFontStyle("SourceCodeFont"))
				// .AutoWrapText(true)
				.Margin(2.0f)
				
				// .Style(&FClingCodeEditorStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("TextEditor.EditableTextBox"))
				.Marshaller(FCppRichTextSyntaxHighlightMarshaller::Create(FSyntaxTextStyle::GetSyntaxTextStyle()))
				.AllowMultiLine(true)
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				// .Margin(0.0f)
				.OnKeyDownHandler_Lambda([](const FGeometry&, const FKeyEvent& KeyEvent) -> FReply
				{
					// avoid changing focus to the next property widget
					if (KeyEvent.GetKey() == EKeys::Tab)
						return FReply::Handled();
					return FReply::Unhandled();
				})
			]
			
			// 输出显示区域 (仅对代码单元格)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.Visibility_Lambda([this]() { return CellData.bHasOutput ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(STextBlock)
					.Text(FText::FromString(CellData.Output))
					.WrapTextAt(800.0f)
					.Margin(5.0f)
				]
			]
		]
	];
}

FReply SClingNotebookCell::OnRunButtonClicked()
{
	// TODO: 执行单元格中的代码
	// 这里应该调用Cling或其他代码执行环境来运行代码
	
	// 临时：模拟执行结果
	if (OnRunCodeDelegate.IsBound())
	{
		OnRunCodeDelegate.ExecuteIfBound();
	}
	
	return FReply::Handled();
}

FReply SClingNotebookCell::OnDeleteButtonClicked()
{
	if (OnDeleteCellDelegate.IsBound())
	{
		OnDeleteCellDelegate.ExecuteIfBound();
	}
	return FReply::Handled();
}

FReply SClingNotebookCell::OnAddAboveButtonClicked()
{
	if (OnAddCellAboveDelegate.IsBound())
	{
		OnAddCellAboveDelegate.ExecuteIfBound();
	}
	return FReply::Handled();
}

FReply SClingNotebookCell::OnAddBelowButtonClicked()
{
	if (OnAddCellBelowDelegate.IsBound())
	{
		OnAddCellBelowDelegate.ExecuteIfBound();
	}
	return FReply::Handled();
}

void SClingNotebookCell::OnCodeTextChanged(const FText& InText)
{
	CellData.Content = InText.ToString();
	if (OnContentChangedDelegate.IsBound())
	{
		OnContentChangedDelegate.ExecuteIfBound(InText);
	}
}

void SNumericNotebook::Construct(const FArguments& InArgs)
{
	// 初始化一些默认单元格
	AddInitialCells();
	
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
					.Text(FText::FromString(TEXT("Numeric Notebook - Interactive Coding Environment")))
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
						.Text(FText::FromString(TEXT("Add New Code Cell")))
						.OnClicked(this, &SNumericNotebook::OnAddNewCellButtonClicked)
						.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
					]
					
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5.0f, 0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Clear All Outputs")))
						// TODO: 实现清除输出功能
						.ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
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
	
	// 刷新UI显示
	UpdateDocumentUI();
}

TSharedRef<SClingNotebookCell> SNumericNotebook::CreateNewCell(ECellType CellType)
{
	FNotebookCellData NewCellData;
	NewCellData.CellType = CellType;
	NewCellData.Content = TEXT("");
	NewCellData.Output = TEXT("");
	NewCellData.bHasOutput = false;

	return SNew(SClingNotebookCell)
		.CellData(NewCellData)
		.OnRunCode(FSimpleDelegate::CreateSP(this, &SNumericNotebook::RunCell, Cells.Num())) // 使用当前cells数量作为索引
		.OnDeleteCell(FSimpleDelegate::CreateSP(this, &SNumericNotebook::DeleteCell, Cells.Num()))
		.OnAddCellAbove(FSimpleDelegate::CreateSP(this, &SNumericNotebook::AddNewCell, CellType, Cells.Num()))
		.OnAddCellBelow(FSimpleDelegate::CreateSP(this, &SNumericNotebook::AddNewCell, CellType, Cells.Num() + 1));
}

void SNumericNotebook::AddNewCell(ECellType CellType, int32 Index)
{
	FNotebookCellData NewCellData;
	NewCellData.CellType = CellType;
	NewCellData.Content = TEXT("");
	NewCellData.Output = TEXT("");
	NewCellData.bHasOutput = false;

	if (Index == -1 || Index >= Cells.Num())
	{
		// 添加到末尾
		Cells.Add(NewCellData);
	}
	else
	{
		// 插入到指定位置
		Cells.Insert(NewCellData, Index);
	}

	// 更新UI
	UpdateDocumentUI();
}

void SNumericNotebook::DeleteCell(int32 Index)
{
	if (Index >= 0 && Index < Cells.Num())
	{
		Cells.RemoveAt(Index);
		UpdateDocumentUI();
	}
}

void SNumericNotebook::RunCell(int32 Index)
{
	if (Index >= 0 && Index < Cells.Num())
	{
		FNotebookCellData& Cell = Cells[Index];
		
		// TODO: 实际执行代码的逻辑 - 使用Cling或其他执行环境
		// 这里只是模拟执行过程
		
		// 示例：对于特定内容执行模拟操作
		if (Cell.Content.Contains(TEXT("UE_LOG")) || Cell.Content.Contains(TEXT("Print")))
		{
			Cell.Output = TEXT("Executed successfully! Check Output Log for details.");
			Cell.bHasOutput = true;
		}
		else if (Cell.Content.TrimStartAndEnd().IsEmpty())
		{
			Cell.Output = TEXT("Empty cell - nothing to execute");
			Cell.bHasOutput = true;
		}
		else
		{
			Cell.Output = FString::Printf(TEXT("Executed: %s\nResult: Success"), *Cell.Content.Left(50));
			Cell.bHasOutput = true;
		}
		
		// 更新UI显示
		UpdateDocumentUI();
		
		// 如果有可执行代码，这里应该实际调用Cling执行
		/* 注释掉的Cling执行代码 - 需要在后续实现
		if (Cell.CellType == ECellType::Code)
		{
			// 获取Cling实例并执行代码
			// UEClingManager::ExecuteCode(Cell.Content);
		}
		*/
	}
}

void SNumericNotebook::AddInitialCells()
{
	// 添加一个示例代码单元格
	FNotebookCellData ExampleCodeCell;
	ExampleCodeCell.CellType = ECellType::Code;
	ExampleCodeCell.Content = TEXT("// Example C++ code cell\n// Type your C++ code here and click Run\nint x = 42;\nUE_LOG(LogTemp, Warning, TEXT(\"Hello from notebook! x = %d\"), x);\n");
	ExampleCodeCell.Output = TEXT("");
	ExampleCodeCell.bHasOutput = false;
	Cells.Add(ExampleCodeCell);

	// 添加一个示例Markdown单元格
	FNotebookCellData ExampleMarkdownCell;
	ExampleMarkdownCell.CellType = ECellType::Markdown;
	ExampleMarkdownCell.Content = TEXT("# Welcome to Numeric Notebook\n\nThis is an interactive coding environment similar to Jupyter Notebook.\nYou can write and execute C++ code directly in this editor.");
	ExampleMarkdownCell.Output = TEXT("");
	ExampleMarkdownCell.bHasOutput = false;
	Cells.Add(ExampleMarkdownCell);
}

void SNumericNotebook::UpdateDocumentUI()
{
	// 清空滚动框内容
	ScrollBox->ClearChildren();

	// 重新添加所有单元格
	for (int32 i = 0; i < Cells.Num(); i++)
	{
		const FNotebookCellData& CellData = Cells[i];
		
		// 使用lambda捕获正确的索引
		int32 CurrentIndex = i;
		
		ScrollBox->AddSlot()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
			.Padding(5.0f)
			[
				SNew(SClingNotebookCell)
				.CellData(CellData)
				.OnRunCode(FSimpleDelegate::CreateSP(this, &SNumericNotebook::RunCell, CurrentIndex))
				.OnDeleteCell(FSimpleDelegate::CreateSP(this, &SNumericNotebook::DeleteCell, CurrentIndex))
				.OnAddCellAbove_Lambda([this, CurrentIndex]() 
				{
					AddNewCell(ECellType::Code, CurrentIndex);
				})
				.OnAddCellBelow_Lambda([this, CurrentIndex]() 
				{
					AddNewCell(ECellType::Code, CurrentIndex + 1);
				})
			]
		];
	}
}

FReply SNumericNotebook::OnAddNewCellButtonClicked()
{
	AddNewCell(ECellType::Code);
	return FReply::Handled();
}

TSharedPtr<SNumericNotebook> SNumericNotebook::CreateTestWidget()
{
	TSharedRef<SNumericNotebook> NotebookWidget = SNew(SNumericNotebook);
    
	// 创建一个窗口来承载Notebook控件
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(TEXT("Numeric Notebook - Interactive C++ Environment")))
		.ClientSize(FVector2D(1000, 700))
		.Content()
		[
			NotebookWidget
		];
    
	// 将窗口添加到应用程序中并显示
	FSlateApplication::Get().AddWindow(Window);
	return NotebookWidget.ToSharedPtr();
};
