#pragma once
#include "SyntaxTextStyle.generated.h"

USTRUCT()
struct CLINGEDITOR_API FSyntaxTextStyle
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere)
	FTextBlockStyle NormalTextStyle;
	UPROPERTY(EditAnywhere)
	FTextBlockStyle OperatorTextStyle;
	UPROPERTY(EditAnywhere)
	FTextBlockStyle KeywordTextStyle;
	UPROPERTY(EditAnywhere)
	FTextBlockStyle StringTextStyle;
	UPROPERTY(EditAnywhere)
	FTextBlockStyle NumberTextStyle;
	UPROPERTY(EditAnywhere)
	FTextBlockStyle CommentTextStyle;
	UPROPERTY(EditAnywhere)
	FTextBlockStyle PreProcessorKeywordTextStyle;
	
	UPROPERTY(EditAnywhere)
	FTextBlockStyle TypeTextStyle;
	UPROPERTY(EditAnywhere)
	FTextBlockStyle VarTextStyle;
	UPROPERTY(EditAnywhere)
	FTextBlockStyle EnumTextStyle;
	UPROPERTY(EditAnywhere)
	FTextBlockStyle NamespaceTextStyle;

	static const FSyntaxTextStyle& GetSyntaxTextStyle();
};

//
// struct CLINGEDITOR_API FCurrentSyntaxTextStyle : FSyntaxTextStyle
// {
// 	FCurrentSyntaxTextStyle();
// };