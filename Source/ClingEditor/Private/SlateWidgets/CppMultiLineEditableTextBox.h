// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "CppCompletionList.h"

/**
 * 
 */
class CLINGEDITOR_API SCppMultiLineEditableTextBox : public SMultiLineEditableTextBox
{
public:
	using SMultiLineEditableTextBox::FArguments;
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	// 处理按键事件
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) override;
    
// private:
	// 显示补全列表
	void ShowCompletionList();
    
	// 隐藏补全列表
	void HideCompletionList();
    
	// 处理补全插入
	void OnCompletionSelected(const FString& SelectedText);
    
	// 获取补全建议
	TArray<FString> GetCompletionSuggestions(const FString& Prefix);
    
	// 获取光标前的单词
	FString GetWordBeforeCursor() const;
    
	// 获取光标位置
	FVector2D GetCursorScreenPosition() const;
    
	TSharedPtr<SCppCompletionList> CompletionList;
	bool bIsCompleting = false;
};
