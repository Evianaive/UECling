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

/**
 * 单元格类型枚举
 */
enum class ECellType : uint8
{
	Code,		// 代码单元格
	Markdown,	// Markdown单元格
	Raw			// 原始文本单元格
};

/**
 * 表示一个笔记本单元格的数据结构
 */
struct FNotebookCellData
{
	ECellType CellType = ECellType::Code;
	FString Content = TEXT("");
	FString Output = TEXT("");
	bool bHasOutput = false;
};

/**
 * 笔记本单元格控件
 */
class CLINGEDITOR_API SClingNotebookCell : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClingNotebookCell) {}
		SLATE_ARGUMENT(FNotebookCellData, CellData)
		SLATE_EVENT(FSimpleDelegate, OnRunCode)
		SLATE_EVENT(FSimpleDelegate, OnDeleteCell)
		SLATE_EVENT(FSimpleDelegate, OnAddCellAbove)
		SLATE_EVENT(FSimpleDelegate, OnAddCellBelow)
		SLATE_EVENT(FOnTextChanged, OnContentChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// 单元格数据
	FNotebookCellData CellData;

	// 文本框控件
	TSharedPtr<SMultiLineEditableTextBox> CodeTextBox;

	// 事件委托
	FSimpleDelegate OnRunCodeDelegate;
	FSimpleDelegate OnDeleteCellDelegate;
	FSimpleDelegate OnAddCellAboveDelegate;
	FSimpleDelegate OnAddCellBelowDelegate;
	FOnTextChanged OnContentChangedDelegate;

	// UI更新方法
	void UpdateCellUI();

	// 按钮点击事件
	FReply OnRunButtonClicked();
	FReply OnDeleteButtonClicked();
	FReply OnAddAboveButtonClicked();
	FReply OnAddBelowButtonClicked();

	// 内容变化事件
	void OnCodeTextChanged(const FText& InText);
};

/**
 * 主要的笔记本文档控件
 */
class CLINGEDITOR_API SNumericNotebook : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNumericNotebook) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	static TSharedPtr<SNumericNotebook> CreateTestWidget();

private:
	// 存储所有单元格的数据
	TArray<FNotebookCellData> Cells;

	// 滚动框控件
	TSharedPtr<SScrollBox> ScrollBox;

	// 创建新的单元格
	TSharedRef<SClingNotebookCell> CreateNewCell(ECellType CellType = ECellType::Code);

	// 添加新单元格
	void AddNewCell(ECellType CellType = ECellType::Code, int32 Index = -1);

	// 删除单元格
	void DeleteCell(int32 Index);

	// 运行单元格代码
	void RunCell(int32 Index);

	// 添加初始单元格
	void AddInitialCells();

	// 更新整个文档UI
	void UpdateDocumentUI();

	// 添加新单元格按钮事件
	FReply OnAddNewCellButtonClicked();
	
};