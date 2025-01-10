// Fill out your copyright notice in the Description page of Project Settings.


#include "ClingEditorSetting.h"

#include "CppHighLight/CodeEditorStyle.h"

FClingTextBlockStyleWrapper::FClingTextBlockStyleWrapper()
	:NormalTextStyle(FClingCodeEditorStyle::NormalText)
	,RefToSelf(MakeShareable(&NormalTextStyle))
{}

FClingTextBlockStyleWrapper::FClingTextBlockStyleWrapper(const FColor& Color)
	:NormalTextStyle(FClingCodeEditorStyle::NormalText)
	,RefToSelf(MakeShareable(&NormalTextStyle))
{
	NormalTextStyle.SetColorAndOpacity(FLinearColor(Color));
}

UClingEditorSetting::UClingEditorSetting()
	:NormalTextStyle(FColor(0xffdfd706))// yellow
	,OperatorTextStyle(FColor(0xffcfcfcf)) // light grey
	,KeywordTextStyle(FColor(0xff006ab4)) // blue
	,StringTextStyle(FColor(0xff9e4a1e)) // pinkish
	,NumberTextStyle(FColor(0xff6db3a8)) // cyan
	,CommentTextStyle(FColor(0xff57a64a)) // green
	,PreProcessorKeywordTextStyle(FColor(0xffcfcfcf)) // light grey
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
