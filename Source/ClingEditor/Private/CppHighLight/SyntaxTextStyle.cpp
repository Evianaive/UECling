
#include "CppHighLight/SyntaxTextStyle.h"

#include "ClingEditorSetting.h"
#include "CppHighLight/CodeEditorStyle.h"

// FCurrentSyntaxTextStyle::FCurrentSyntaxTextStyle()
// 	: FSyntaxTextStyle{
// 		 NormalTextStyle = FClingCodeEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.CPP.Normal")
// 		, OperatorTextStyle = FClingCodeEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.CPP.Operator")
// 		, KeywordTextStyle = FClingCodeEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.CPP.Keyword")
// 		, StringTextStyle = FClingCodeEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.CPP.String")
// 		, NumberTextStyle = FClingCodeEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.CPP.Number")
// 		, CommentTextStyle = FClingCodeEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.CPP.Comment")
// 		, PreProcessorKeywordTextStyle = FClingCodeEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.CPP.PreProcessorKeyword")
// 	}
// {
// }
const FSyntaxTextStyle& FSyntaxTextStyle::GetSyntaxTextStyle()
{
	return GetDefault<UClingEditorSetting>()->SyntaxTextStyle;
}
