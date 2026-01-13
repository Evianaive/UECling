// Copyright Epic Games, Inc. All Rights Reserved.

#include "CppHighLight/CppRichTextSyntaxHighlightMarshaller.h"
// #include "WhiteSpaceTextRun.h"

#include "Framework/Text/SlateTextRun.h"
#include "Fonts/FontMeasure.h"
#include "ClingRuntime.h"
#include "CppInterOp/CppInterOp.h"
#include "UObject/UObjectIterator.h"
#include "Internationalization/Regex.h"
#include "Misc/Paths.h"

struct FRunInfo;

class FWhiteSpaceTextRun : public FSlateTextRun
{
public:
	static TSharedRef< FWhiteSpaceTextRun > Create( const FRunInfo& InRunInfo, const TSharedRef< const FString >& InText, const FTextBlockStyle& Style, const FTextRange& InRange, int32 NumSpacesPerTab );

public:
	virtual FVector2D Measure( int32 StartIndex, int32 EndIndex, float Scale, const FRunTextContext& TextContext) const override;

protected:
	FWhiteSpaceTextRun( const FRunInfo& InRunInfo, const TSharedRef< const FString >& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange, int32 InNumSpacesPerTab );

private:
	int32 NumSpacesPerTab;

	float TabWidth;

	float SpaceWidth;
};


TSharedRef< FWhiteSpaceTextRun > FWhiteSpaceTextRun::Create( const FRunInfo& InRunInfo, const TSharedRef< const FString >& InText, const FTextBlockStyle& Style, const FTextRange& InRange, int32 NumSpacesPerTab )
{
	return MakeShareable( new FWhiteSpaceTextRun( InRunInfo, InText, Style, InRange, NumSpacesPerTab ) );
}

FVector2D FWhiteSpaceTextRun::Measure( int32 StartIndex, int32 EndIndex, float Scale, const FRunTextContext& TextContext) const
{
	const FVector2D ShadowOffsetToApply((EndIndex == Range.EndIndex) ? FMath::Abs(Style.ShadowOffset.X * Scale) : 0.0f, FMath::Abs(Style.ShadowOffset.Y * Scale));

	if ( EndIndex - StartIndex == 0 )
	{
		return FVector2D( ShadowOffsetToApply.X * Scale, GetMaxHeight( Scale ) );
	}

	// count tabs
	int32 TabCount = 0;
	for(int32 Index = StartIndex; Index < EndIndex; Index++)
	{
		if((*Text)[Index] == TEXT('\t'))
		{
			TabCount++;
		}
	}

	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	FVector2D Size = FontMeasure->Measure( **Text, StartIndex, EndIndex, Style.Font, true, Scale ) + ShadowOffsetToApply;

	Size.X -= TabWidth * (float)TabCount * Scale;
	Size.X += SpaceWidth * (float)(TabCount * NumSpacesPerTab) * Scale;

	return Size;
}

FWhiteSpaceTextRun::FWhiteSpaceTextRun( const FRunInfo& InRunInfo, const TSharedRef< const FString >& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange, int32 InNumSpacesPerTab ) 
	: FSlateTextRun(InRunInfo, InText, InStyle, InRange)
	, NumSpacesPerTab(InNumSpacesPerTab)
{
	// measure tab width
	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	TabWidth = FontMeasure->Measure( TEXT("\t"), 0, 1, Style.Font, true, 1.0f ).X;
	SpaceWidth = FontMeasure->Measure( TEXT(" "), 0, 1, Style.Font, true, 1.0f ).X;
}







const TCHAR* Keywords[] =
{
	TEXT("alignas"),
	TEXT("alignof"),
	TEXT("and"),
	TEXT("and_eq"),
	TEXT("asm"),
	TEXT("auto"),
	TEXT("bitand"),
	TEXT("bitor"),
	TEXT("bool"),
	TEXT("break"),
	TEXT("case"),
	TEXT("catch"),
	TEXT("char"),
	TEXT("char16_t"),
	TEXT("char32_t"),
	TEXT("class"),
	TEXT("compl"),
	TEXT("concept"),
	TEXT("const"),
	TEXT("constexpr"),
	TEXT("const_cast"),
	TEXT("continue"),
	TEXT("decltype"),
	TEXT("default"),
	TEXT("delete"),
	TEXT("do"),
	TEXT("double"),
	TEXT("dynamic_cast"),
	TEXT("else"),
	TEXT("enum"),
	TEXT("explicit"),
	TEXT("export"),
	TEXT("extern"),
	TEXT("false"),
	TEXT("float"),
	TEXT("for"),
	TEXT("friend"),
	TEXT("goto"),
	TEXT("if"),
	TEXT("inline"),
	TEXT("int"),
	TEXT("long"),
	TEXT("mutable"),
	TEXT("namespace"),
	TEXT("new"),
	TEXT("noexcept"),
	TEXT("not"),
	TEXT("not_eq"),
	TEXT("nullptr"),
	TEXT("operator"),
	TEXT("or"),
	TEXT("or_eq"),
	TEXT("private"),
	TEXT("protected"),
	TEXT("public"),
	TEXT("register"),
	TEXT("reinterpret_cast"),
	TEXT("requires"),
	TEXT("return"),
	TEXT("short"),
	TEXT("signed"),
	TEXT("sizeof"),
	TEXT("static"),
	TEXT("static_assert"),
	TEXT("static_cast"),
	TEXT("struct"),
	TEXT("switch"),
	TEXT("template"),
	TEXT("this"),
	TEXT("thread_local"),
	TEXT("throw"),
	TEXT("true"),
	TEXT("try"),
	TEXT("typedef"),
	TEXT("typeid"),
	TEXT("typename"),
	TEXT("union"),
	TEXT("unsigned"),
	TEXT("using"),
	TEXT("virtual"),
	TEXT("void"),
	TEXT("volatile"),
	TEXT("wchar_t"),
	TEXT("while"),
	TEXT("xor"),
	TEXT("xor_eq"),
};

const TCHAR* Operators[] =
{
	TEXT("/*"),
	TEXT("*/"),
	TEXT("//"),
	TEXT("\""),
	TEXT("\'"),
	TEXT("::"),
	TEXT(":"),
	TEXT("+="),
	TEXT("++"),
	TEXT("+"),
	TEXT("--"),
	TEXT("-="),
	TEXT("-"),
	TEXT("("),
	TEXT(")"),
	TEXT("["),
	TEXT("]"),
	TEXT("."),
	TEXT("->"),
	TEXT("!="),
	TEXT("!"),
	TEXT("&="),
	TEXT("~"),
	TEXT("&"),
	TEXT("*="),
	TEXT("*"),
	TEXT("->"),
	TEXT("/="),
	TEXT("/"),
	TEXT("%="),
	TEXT("%"),
	TEXT("<<="),
	TEXT("<<"),
	TEXT("<="),
	TEXT("<"),
	TEXT(">>="),
	TEXT(">>"),
	TEXT(">="),
	TEXT(">"),
	TEXT("=="),
	TEXT("&&"),
	TEXT("&"),
	TEXT("^="),
	TEXT("^"),
	TEXT("|="),
	TEXT("||"),
	TEXT("|"),
	TEXT("?"),
	TEXT("="),
	TEXT(","),
	TEXT("{"),
	TEXT("}"),
	TEXT(";"),
};

const TCHAR* PreProcessorKeywords[] =
{
	TEXT("#include"),
	TEXT("#define"),
	TEXT("#ifndef"),
	TEXT("#ifdef"),
	TEXT("#if"),
	TEXT("#else"),
	TEXT("#endif"),
	TEXT("#pragma"),
	TEXT("#undef"),
};

TSharedRef< FCppRichTextSyntaxHighlightMarshaller > FCppRichTextSyntaxHighlightMarshaller::Create(const FSyntaxTextStyle& InSyntaxTextStyle)
{
	TArray<FSyntaxTokenizer::FRule> TokenizerRules;

	// operators
	for(const auto& Operator : Operators)
	{
		TokenizerRules.Emplace(FSyntaxTokenizer::FRule(Operator));
	}	

	// keywords
	for(const auto& Keyword : Keywords)
	{
		TokenizerRules.Emplace(FSyntaxTokenizer::FRule(Keyword));
	}

	// Pre-processor Keywords
	for(const auto& PreProcessorKeyword : PreProcessorKeywords)
	{
		TokenizerRules.Emplace(FSyntaxTokenizer::FRule(PreProcessorKeyword));
	}

	return MakeShareable(new FCppRichTextSyntaxHighlightMarshaller(FSyntaxTokenizer::Create(TokenizerRules), InSyntaxTextStyle));
}

FCppRichTextSyntaxHighlightMarshaller::~FCppRichTextSyntaxHighlightMarshaller()
{

}

void FCppRichTextSyntaxHighlightMarshaller::ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<ISyntaxTokenizer::FTokenizedLine> TokenizedLines)
{
	enum class EParseState : uint8
	{
		None,
		LookingForString,
		LookingForCharacter,
		LookingForSingleLineComment,
		LookingForMultiLineComment,
	};

	TArray<FTextLayout::FNewLineData> LinesToAdd;
	LinesToAdd.Reserve(TokenizedLines.Num());

	// Parse the tokens, generating the styled runs for each line
	EParseState ParseState = EParseState::None;
	for(const ISyntaxTokenizer::FTokenizedLine& TokenizedLine : TokenizedLines)
	{
		TSharedRef<FString> ModelString = MakeShareable(new FString());
		TArray< TSharedRef< IRun > > Runs;

		if(ParseState == EParseState::LookingForSingleLineComment)
		{
			ParseState = EParseState::None;
		}

		for(const ISyntaxTokenizer::FToken& Token : TokenizedLine.Tokens)
		{
			const FString TokenText = SourceString.Mid(Token.Range.BeginIndex, Token.Range.Len());

			const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + TokenText.Len());
			ModelString->Append(TokenText);

			FRunInfo RunInfo(TEXT("SyntaxHighlight.CPP.Normal"));
			FTextBlockStyle TextBlockStyle = SyntaxTextStyle.NormalTextStyle;
			
			const bool bIsWhitespace = FString(TokenText).TrimEnd().IsEmpty();
			if(!bIsWhitespace)
			{
				bool bHasMatchedSyntax = false;
				if(Token.Type == ISyntaxTokenizer::ETokenType::Syntax)
				{
					if(ParseState == EParseState::None && TokenText == TEXT("\""))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.String");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
						ParseState = EParseState::LookingForString;
						bHasMatchedSyntax = true;
					}
					else if(ParseState == EParseState::LookingForString && TokenText == TEXT("\""))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Normal");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
						ParseState = EParseState::None;
					}
					else if(ParseState == EParseState::None && TokenText == TEXT("\'"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.String");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
						ParseState = EParseState::LookingForCharacter;
						bHasMatchedSyntax = true;
					}
					else if(ParseState == EParseState::LookingForCharacter && TokenText == TEXT("\'"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Normal");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
						ParseState = EParseState::None;
					}
					else if(ParseState == EParseState::None && TokenText.StartsWith(TEXT("#")))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.PreProcessorKeyword");
						TextBlockStyle = SyntaxTextStyle.PreProcessorKeywordTextStyle;
						ParseState = EParseState::None;
					}
					else if(ParseState == EParseState::None && TokenText == TEXT("//"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Comment");
						TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
						ParseState = EParseState::LookingForSingleLineComment;
					}
					else if(ParseState == EParseState::None && TokenText == TEXT("/*"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Comment");
						TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
						ParseState = EParseState::LookingForMultiLineComment;
					}
					else if(ParseState == EParseState::LookingForMultiLineComment && TokenText == TEXT("*/"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Comment");
						TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
						ParseState = EParseState::None;
					}
					else if(ParseState == EParseState::None && TChar<WIDECHAR>::IsAlpha(TokenText[0]))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Keyword");
						TextBlockStyle = SyntaxTextStyle.KeywordTextStyle;
						ParseState = EParseState::None;
					}
					else if(ParseState == EParseState::None && !TChar<WIDECHAR>::IsAlpha(TokenText[0]))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Operator");
						TextBlockStyle = SyntaxTextStyle.OperatorTextStyle;
						ParseState = EParseState::None;
					}
				}
				
				// It's possible that we fail to match a syntax token if we're in a state where it isn't parsed
				// In this case, we treat it as a literal token
				if(Token.Type == ISyntaxTokenizer::ETokenType::Literal || !bHasMatchedSyntax)
				{
					if(ParseState == EParseState::LookingForString)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.String");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
					}
					else if(ParseState == EParseState::LookingForCharacter)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.String");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
					}
					else if(ParseState == EParseState::LookingForSingleLineComment)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Comment");
						TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
					}
					else if(ParseState == EParseState::LookingForMultiLineComment)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Comment");
						TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
					}
					else if(ParseState == EParseState::None)
					{
						if (KnownEnums.Contains(TokenText))
						{
							// 使用语义着色
							RunInfo.Name = TEXT("SyntaxHighlight.CPP.Enum");
							TextBlockStyle = SyntaxTextStyle.EnumTextStyle;
							// UE_LOG(LogTemp, Warning, TEXT("Enum token: %s"), *TokenText);
						}
						else if (KnownTypes.Contains(TokenText))
						{
							// 使用语义着色
							RunInfo.Name = TEXT("SyntaxHighlight.CPP.Type");
							TextBlockStyle = SyntaxTextStyle.TypeTextStyle;
							// UE_LOG(LogTemp, Warning, TEXT("Type token: %s"), *TokenText);
						}
						else if (KnownNamespaces.Contains(TokenText))
						{
							// 使用语义着色
							RunInfo.Name = TEXT("SyntaxHighlight.CPP.Namespace");
							TextBlockStyle = SyntaxTextStyle.NamespaceTextStyle;
							// UE_LOG(LogTemp, Warning, TEXT("Namespace token: %s"), *TokenText);
						}
						// else
						// {
						// 	RunInfo.Name = TEXT("SyntaxHighlight.CPP.Var");
						// 	TextBlockStyle = SyntaxTextStyle.VarTextStyle;
						// 	UE_LOG(LogTemp, Warning, TEXT("Var token: %s"), *TokenText);
						// }
						// else if (TChar<WIDECHAR>::IsDigit(TokenText[0]))
						// {
						// 	RunInfo.Name = TEXT("SyntaxHighlight.CPP.Number");
						// 	TextBlockStyle = SyntaxTextStyle.NumberTextStyle;
						// }
					}
				}

				TSharedRef< ISlateRun > Run = FSlateTextRun::Create(RunInfo, ModelString, TextBlockStyle, ModelRange);
				Runs.Add(Run);
			}
			else
			{
				RunInfo.Name = TEXT("SyntaxHighlight.CPP.WhiteSpace");
				TSharedRef< ISlateRun > Run = FWhiteSpaceTextRun::Create(RunInfo, ModelString, TextBlockStyle, ModelRange, 4);
				Runs.Add(Run);
			}
		}

		LinesToAdd.Emplace(MoveTemp(ModelString), MoveTemp(Runs));
	}

	TargetTextLayout.AddLines(LinesToAdd);
}

FCppRichTextSyntaxHighlightMarshaller::FCppRichTextSyntaxHighlightMarshaller(TSharedPtr< ISyntaxTokenizer > InTokenizer, const FSyntaxTextStyle& InSyntaxTextStyle)
	: FSyntaxHighlighterTextLayoutMarshaller(MoveTemp(InTokenizer))
	, SyntaxTextStyle(InSyntaxTextStyle)
{
	RefreshSemanticNames();
}

void FCppRichTextSyntaxHighlightMarshaller::RefreshSemanticNames()
{
	FClingRuntimeModule* Module = FModuleManager::GetModulePtr<FClingRuntimeModule>(TEXT("ClingRuntime"));
	if (Module && Module->BaseInterp)
	{
		// Cpp::BeginStdStreamCapture(Cpp::kStdErr);
		// Cpp::DumpScope(Cpp::GetGlobalScope());
		// Cpp::EndStdStreamCapture([](const char* In){UE_LOG(LogTemp,Log,TEXT("%hs"),In)});
		
		// Cpp::ActivateInterpreter(Module->BaseInterp);
		thread_local TSet<FString>* LocalKnownNames = nullptr;
		
		TGuardValue Guard{LocalKnownNames,&KnownTypes};
		Cpp::GetAllCppNames(Cpp::GetGlobalScope(),[](const char* const* Names, size_t Size)
		{
			for (size_t i = 0; i < Size; ++i)
			{
				LocalKnownNames->Add(UTF8_TO_TCHAR(Names[i]));
			}
		});		
		
		LocalKnownNames = &KnownEnums;
		Cpp::GetEnums(Cpp::GetGlobalScope(),[](const char* const* Names, size_t Size)
		{
			for (size_t i = 0; i < Size; ++i)
			{
				LocalKnownNames->Add(UTF8_TO_TCHAR(Names[i]));
			}
		});
		
		LocalKnownNames = &KnownNamespaces;
		Cpp::GetUsingNamespaces(Cpp::GetGlobalScope(),[](const Cpp::TCppScope_t* Scopes, size_t Size)
		{
			for (size_t i = 0; i < Size; ++i)
			{
				Cpp::GetName(Scopes[i],[](const char* Name)
				{
					LocalKnownNames->Add(UTF8_TO_TCHAR(Name));
				});
			}
		});
	}

	// 补充 UE UObject 符号，这在处理 UE 专属代码时非常有用
	// for (TObjectIterator<UStruct> It; It; ++It)
	// {
	// 	KnownNames.Add(It->GetName());
	// }
}

FString FCppRichTextSyntaxHighlightMarshaller::TryFixIncludes(const FString& InCode)
{
	FClingRuntimeModule* Module = FModuleManager::GetModulePtr<FClingRuntimeModule>(TEXT("ClingRuntime"));
	if (!Module || !Module->BaseInterp) return InCode;

	Cpp::ActivateInterpreter(Module->BaseInterp);

	// 1. 捕获 CppInterOp 的编译错误 (Clang 错误通常输出到 stderr)
	Cpp::BeginStdStreamCapture(Cpp::kStdErr);
	Cpp::Declare(TCHAR_TO_UTF8(*InCode), true);
	thread_local FString FErrors;
	FErrors.Reset();
	Cpp::EndStdStreamCapture([](const char* Result)
	{
		FErrors = UTF8_TO_TCHAR(Result);
	});	

	// 2. 使用正则表达式提取未定义的符号
	TArray<FString> MissingSymbols;
	{
		FRegexPattern Pattern(TEXT("use of undeclared identifier '([^']*)'"));
		FRegexMatcher Matcher(Pattern, FErrors);
		while (Matcher.FindNext()) { MissingSymbols.AddUnique(Matcher.GetCaptureGroup(1)); }
	}
	{
		FRegexPattern Pattern(TEXT("unknown type name '([^']*)'"));
		FRegexMatcher Matcher(Pattern, FErrors);
		while (Matcher.FindNext()) { MissingSymbols.AddUnique(Matcher.GetCaptureGroup(1)); }
	}

	if (MissingSymbols.Num() == 0) return InCode;

	// 3. 寻找符号对应的头文件
	TSet<FString> NewIncludes;
	for (const FString& Symbol : MissingSymbols)
	{
		// 策略 A: 利用 Unreal Engine 的反射系统寻找 U 类型
		UStruct* FoundStruct = FindObject<UStruct>(nullptr, *Symbol, true);
		if (!FoundStruct) FoundStruct = FindObject<UStruct>(nullptr, *(TEXT("U") + Symbol), true);
		if (!FoundStruct) FoundStruct = FindObject<UStruct>(nullptr, *(TEXT("A") + Symbol), true);
		if (!FoundStruct) FoundStruct = FindObject<UStruct>(nullptr, *(TEXT("F") + Symbol), true);

		if (FoundStruct)
		{
			FString HeaderPath = FoundStruct->GetMetaData(TEXT("ModuleRelativePath"));
			if (!HeaderPath.IsEmpty())
			{
				NewIncludes.Add(FString::Printf(TEXT("#include \"%s\""), *FPaths::GetCleanFilename(HeaderPath)));
			}
		}
		
		// 策略 B: 利用 RiderLink 插件及其对项目的索引能力
		// if (FModuleManager::Get().IsModuleLoaded("RiderLink"))
		// {
		// 	// 获取 RiderLink 模块以确保连接
		// 	IRiderLinkModule& RiderLinkModule = IRiderLinkModule::Get();
		//
		// 	// 尝试在项目 Source 目录下直接搜索匹配符号名称的头文件
		// 	// 这是基于 Rider 用户通常拥有完整的项目源码索引这一假设
		// 	TArray<FString> FoundFiles;
		// 	FString SearchDir = FPaths::ProjectDir() / TEXT("Source");
		// 	IFileManager::Get().FindFilesRecursive(FoundFiles, *SearchDir, *(Symbol + TEXT(".h")), true, false);
		// 	
		// 	// 处理带有前缀的情况 (U/A/F/S/T)
		// 	if (FoundFiles.Num() == 0 && Symbol.Len() > 1)
		// 	{
		// 		IFileManager::Get().FindFilesRecursive(FoundFiles, *SearchDir, *(Symbol.Mid(1) + TEXT(".h")), true, false);
		// 	}
		//
		// 	if (FoundFiles.Num() > 0)
		// 	{
		// 		NewIncludes.Add(FString::Printf(TEXT("#include \"%s\""), *FPaths::GetCleanFilename(FoundFiles[0])));
		// 	}
		// }
	}

	if (NewIncludes.Num() == 0) return InCode;

	// 4. 将缺失的 include 插入代码顶部
	TArray<FString> Lines;
	InCode.ParseIntoArrayLines(Lines, false);
	int32 InsertIdx = 0;
	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		if (Lines[i].StartsWith(TEXT("#include")) || Lines[i].StartsWith(TEXT("#pragma"))) { InsertIdx = i + 1; }
		else if (!Lines[i].TrimStartAndEnd().IsEmpty()) { break; }
	}

	for (const FString& Inc : NewIncludes)
	{
		if (!InCode.Contains(Inc)) { Lines.Insert(Inc, InsertIdx++); }
	}

	return FString::Join(Lines, TEXT("\n"));
}
