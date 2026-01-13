// Fill out your copyright notice in the Description page of Project Settings.


#include "ClingEditorSetting.h"

#include "CppHighLight/CodeEditorStyle.h"

// FClingTextBlockStyleWrapper::FClingTextBlockStyleWrapper()
// 	:NormalTextStyle(FClingCodeEditorStyle::NormalText)
// 	,RefToSelf(MakeShareable(&NormalTextStyle))
// {}
//
// FClingTextBlockStyleWrapper::FClingTextBlockStyleWrapper(const FColor& Color)
// 	:NormalTextStyle(FClingCodeEditorStyle::NormalText)
// 	,RefToSelf(MakeShareable(&NormalTextStyle))
// {
// 	NormalTextStyle.SetColorAndOpacity(FLinearColor(Color));
// }

UClingEditorSetting::UClingEditorSetting()
	:SyntaxTextStyle{
		.NormalTextStyle=FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xffdfd706)),// yellow
		.OperatorTextStyle=FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xffcfcfcf)),// light grey
		.KeywordTextStyle=FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xff008ad4)),// blue
		.StringTextStyle=FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xff9e4a1e)),// pinkish
		.NumberTextStyle=FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xff6db3a8)),// cyan
		.CommentTextStyle=FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xff57a64a)),// green
		.PreProcessorKeywordTextStyle=FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xffcfcfcf)),// light grey
		.TypeTextStyle=FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xffd4c006)),// light grey
		.VarTextStyle=FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xffcf8f0f)),// light grey
		.EnumTextStyle=FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xff0fcf4f)),// light grey
		.NamespaceTextStyle=FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xffaf0fef))// light grey
	}
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("ClingEditor");
}

FName UClingEditorSetting::GetCategoryName() const
{
	return Super::GetCategoryName();
}

FName UClingEditorSetting::GetContainerName() const
{
	return Super::GetContainerName();
}

FName UClingEditorSetting::GetSectionName() const
{
	return Super::GetSectionName();
}
