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
		FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xffdfd706)),// yellow
		FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xffcfcfcf)),// light grey
		FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xff006ab4)),// blue
		FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xff9e4a1e)),// pinkish
		FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xff6db3a8)),// cyan
		FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xff57a64a)),// green
		FTextBlockStyle(FClingCodeEditorStyle::NormalText).SetColorAndOpacity(FColor(0xffcfcfcf))// light grey
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
