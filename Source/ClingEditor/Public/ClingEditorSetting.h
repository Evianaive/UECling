// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ClingEditorSetting.generated.h"


USTRUCT()
struct FClingTextBlockStyleWrapper
{
	GENERATED_BODY()
	FClingTextBlockStyleWrapper();
	FClingTextBlockStyleWrapper(const FColor& Color);
	UPROPERTY(EditAnywhere, meta=(ShowOnlyInnerProperties))
	FTextBlockStyle NormalTextStyle;
	TSharedPtr<FTextBlockStyle> RefToSelf;
};

/**
 * 
 */
UCLASS(Config=EditorPerProjectUserSettings)
class CLINGEDITOR_API UClingEditorSetting : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UClingEditorSetting();
	virtual FName GetCategoryName() const override;
	virtual FName GetContainerName() const override;
	virtual FName GetSectionName() const override;
	UPROPERTY(EditAnywhere)
	FClingTextBlockStyleWrapper NormalTextStyle;
	UPROPERTY(EditAnywhere)
	FClingTextBlockStyleWrapper OperatorTextStyle;
	UPROPERTY(EditAnywhere)
	FClingTextBlockStyleWrapper KeywordTextStyle;
	UPROPERTY(EditAnywhere)
	FClingTextBlockStyleWrapper StringTextStyle;
	UPROPERTY(EditAnywhere)
	FClingTextBlockStyleWrapper NumberTextStyle;
	UPROPERTY(EditAnywhere)
	FClingTextBlockStyleWrapper CommentTextStyle;
	UPROPERTY(EditAnywhere)
	FClingTextBlockStyleWrapper PreProcessorKeywordTextStyle;

};
