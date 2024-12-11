// Fill out your copyright notice in the Description page of Project Settings.


#include "ClingSetting.h"

#include "UCling.h"

UClingSetting::UClingSetting()
{
	CategoryName = TEXT("Plugin");
	SectionName = TEXT("Cling");
}

FName UClingSetting::GetCategoryName() const
{
	return Super::GetCategoryName();
}

FName UClingSetting::GetContainerName() const
{
	return Super::GetContainerName();
}

FName UClingSetting::GetSectionName() const
{
	return Super::GetSectionName();
}

void UClingSetting::RefreshIncludePaths()
{
	auto& Module = FModuleManager::Get().GetModuleChecked<FUClingModule>(TEXT("UCling"));
}
