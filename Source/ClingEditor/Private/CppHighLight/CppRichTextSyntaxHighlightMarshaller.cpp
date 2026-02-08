#include "CppHighLight/CppRichTextSyntaxHighlightMarshaller.h"
#include "ClingIncludeUtils.h"
// #include "WhiteSpaceTextRun.h"

#include "Framework/Text/SlateTextRun.h"
#include "Framework/Text/SlateHyperlinkRun.h"
#include "Fonts/FontMeasure.h"
#include "ClingRuntime.h"
#include "ClingSemanticInfoProvider.h"
#include "CppInterOp/CppInterOp.h"
#include "UObject/UObjectIterator.h"
#include "Internationalization/Regex.h"
#include "Misc/Paths.h"
#include "ClingSetting.h"
#include "HAL/FileManager.h"
#include "ClingSourceAccess.h"

// Helper structures for two-phase parsing

/** Structure to hold include statement information */
struct FIncludeStatementInfo
{
	FString FullIncludePath;      // The complete file path
	FString DisplayIncludePath;   // The path as shown in code (without quotes/brackets)
	bool bIsSystemInclude;        // True if using <>, false if using ""
	
	FIncludeStatementInfo() : bIsSystemInclude(false) {}
};

struct FTokenRunInfo
{
	FString TokenText;
	FTextRange ModelRange;
	bool bIsWhitespace;
	ISyntaxTokenizer::ETokenType TokenType;
	FString RunTypeName;
	FTextBlockStyle Style;
};

struct FIncludeAnalysisResult
{
	bool bIsCompleteInclude = false;
	int32 IncludeKeywordIndex = INDEX_NONE;
	int32 PathStartIndex = INDEX_NONE;
	int32 PathEndIndex = INDEX_NONE;
	FString IncludePath;
	bool bIsSystemInclude = false;  // true for <>, false for ""
};

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

TSharedRef< FCppRichTextSyntaxHighlightMarshaller > FCppRichTextSyntaxHighlightMarshaller::Create(
	const FSyntaxTextStyle& InSyntaxTextStyle, const FClingSemanticInfoProvider& InProvider)
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

	return MakeShareable(new FCppRichTextSyntaxHighlightMarshaller(FSyntaxTokenizer::Create(TokenizerRules), InSyntaxTextStyle, InProvider));
}

FCppRichTextSyntaxHighlightMarshaller::~FCppRichTextSyntaxHighlightMarshaller()
{

}

FIncludeAnalysisResult AnalyzeIncludeStatement(const TArray<FTokenRunInfo>& TokenRunInfos, const FString& LineText)
{
	FIncludeAnalysisResult Result;
	
	// Look for #include pattern
	for (int32 i = 0; i < TokenRunInfos.Num(); ++i)
	{
		const FTokenRunInfo& TokenInfo = TokenRunInfos[i];
		
		// Find #include keyword
		if (TokenInfo.TokenText == TEXT("#include"))
		{
			Result.IncludeKeywordIndex = i;
			
			// Look for path after #include, skipping whitespace
			for (int32 j = i + 1; j < TokenRunInfos.Num(); ++j)
			{
				const FTokenRunInfo& CurrentToken = TokenRunInfos[j];
				
				// Skip whitespace tokens
				if (CurrentToken.bIsWhitespace)
				{
					continue;
				}
				
				// Check for "path" or <path> format
				if (CurrentToken.TokenText == TEXT("\""))
				{
					// Look for closing quote
					for (int32 k = j + 1; k < TokenRunInfos.Num(); ++k)
					{
						if (TokenRunInfos[k].TokenText == TEXT("\""))
						{
							// Found complete quoted include
							Result.bIsCompleteInclude = true;
							Result.PathStartIndex = j + 1;
							Result.PathEndIndex = k - 1;
							Result.bIsSystemInclude = false;
							
							// Safety check: ensure valid range
							if (Result.PathStartIndex <= Result.PathEndIndex && 
								Result.PathStartIndex >= 0 && Result.PathEndIndex < TokenRunInfos.Num())
							{
								// Extract the path
								FString Path;
								for (int32 l = Result.PathStartIndex; l <= Result.PathEndIndex; ++l)
								{
									Path += TokenRunInfos[l].TokenText;
								}
								Result.IncludePath = Path;
							}
							break;
						}
					}
					break; // Exit the search loop regardless of success
				}
				else if (CurrentToken.TokenText == TEXT("<"))
				{
					// Look for closing >
					for (int32 k = j + 1; k < TokenRunInfos.Num(); ++k)
					{
						if (TokenRunInfos[k].TokenText == TEXT(">"))
						{
							// Found complete angle-bracket include
							Result.bIsCompleteInclude = true;
							Result.PathStartIndex = j + 1;
							Result.PathEndIndex = k - 1;
							Result.bIsSystemInclude = true;
							
							// Safety check: ensure valid range
							if (Result.PathStartIndex <= Result.PathEndIndex && 
								Result.PathStartIndex >= 0 && Result.PathEndIndex < TokenRunInfos.Num())
							{
								// Extract the path
								FString Path;
								for (int32 l = Result.PathStartIndex; l <= Result.PathEndIndex; ++l)
								{
									Path += TokenRunInfos[l].TokenText;
								}
								Result.IncludePath = Path;
							}
							break;
						}
					}
					break; // Exit the search loop regardless of success
				}
				else
				{
					// Found non-whitespace token that's not " or <, this is not a valid include
					break;
				}
			}
			break;
		}
	}
	
	return Result;
}

void CreateNormalRuns(const TArray<FTokenRunInfo>& TokenRunInfos, const TSharedRef<const FString>& ModelString, TArray<TSharedRef<IRun>>& OutRuns)
{
	// Create runs using the original logic
	for (const FTokenRunInfo& TokenInfo : TokenRunInfos)
	{
		if (TokenInfo.bIsWhitespace)
		{
			FRunInfo RunInfo(TEXT("SyntaxHighlight.CPP.WhiteSpace"));
			TSharedRef<IRun> Run = FWhiteSpaceTextRun::Create(RunInfo, ModelString, TokenInfo.Style, TokenInfo.ModelRange, 4);
			OutRuns.Add(Run);
		}
		else
		{
			FRunInfo RunInfo(*TokenInfo.RunTypeName);
			TSharedRef<IRun> Run = FSlateTextRun::Create(RunInfo, ModelString, TokenInfo.Style, TokenInfo.ModelRange);
			OutRuns.Add(Run);
		}
	}
}

void FCppRichTextSyntaxHighlightMarshaller::ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<ISyntaxTokenizer::FTokenizedLine> TokenizedLines)
{
	TArray<FTextLayout::FNewLineData> LinesToAdd;
	LinesToAdd.Reserve(TokenizedLines.Num());
	
	const FClingSemanticInfoProvider& SemanticInfoProviderToUse = SemanticInfoProvider.IsReady() 
		? SemanticInfoProvider:
		*FClingRuntimeModule::Get().GetDefaultSemanticInfoProvider();

	// Two-phase parsing approach:
	// Phase 1: Collect token information without creating runs
	// Phase 2: Analyze complete line and create appropriate runs
	
	for(const ISyntaxTokenizer::FTokenizedLine& TokenizedLine : TokenizedLines)
	{
		// Phase 1: Collect all token data first
		TSharedRef<FString> ModelString = MakeShareable(new FString());
		TArray<FTokenRunInfo> TokenRunInfos;  // Store run creation info
		
		// Parse all tokens in this line
		for(const ISyntaxTokenizer::FToken& Token : TokenizedLine.Tokens)
		{
			const FString TokenText = SourceString.Mid(Token.Range.BeginIndex, Token.Range.Len());
			const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + TokenText.Len());
			ModelString->Append(TokenText);
			
			// Determine what kind of run this token should become
			FTokenRunInfo RunInfo;
			RunInfo.TokenText = TokenText;
			RunInfo.ModelRange = ModelRange;
			RunInfo.bIsWhitespace = FString(TokenText).TrimEnd().IsEmpty();
			RunInfo.TokenType = Token.Type;
			
			// Basic syntax classification (will be refined in phase 2)
			if (!RunInfo.bIsWhitespace)
			{
				if (TokenText == TEXT("#include"))
				{
					RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.PreProcessorKeyword");
					RunInfo.Style = SyntaxTextStyle.PreProcessorKeywordTextStyle;
				}
				else if (TokenText == TEXT("\""))
				{
					RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.String");
					RunInfo.Style = SyntaxTextStyle.StringTextStyle;
				}
				else if (!TokenText.IsEmpty() && TChar<WIDECHAR>::IsAlpha(TokenText[0]))
				{
					// Check if this is actually a keyword
					bool bIsKeyword = false;
					for (int32 k = 0; k < sizeof(Keywords)/sizeof(Keywords[0]); ++k)
					{
						if (TokenText == Keywords[k])
						{
							bIsKeyword = true;
							break;
						}
					}
					
					if (bIsKeyword)
					{
						RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.Keyword");
						RunInfo.Style = SyntaxTextStyle.KeywordTextStyle;
					}
					else if (SemanticInfoProviderToUse.IsReady())
					{
						if (SemanticInfoProviderToUse.GetNamespaces().Contains(TokenText))
						{
							RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.Namespace");
							RunInfo.Style = SyntaxTextStyle.NamespaceTextStyle;
						}
						else if (SemanticInfoProviderToUse.GetClasses().Contains(TokenText))
						{
							RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.Class");
							RunInfo.Style = SyntaxTextStyle.ClassTextStyle;
						}
						else if (SemanticInfoProviderToUse.GetStructs().Contains(TokenText))
						{
							RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.Struct");
							RunInfo.Style = SyntaxTextStyle.StructTextStyle;
						}
						else if (SemanticInfoProviderToUse.GetEnums().Contains(TokenText))
						{
							RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.Enum");
							RunInfo.Style = SyntaxTextStyle.EnumTextStyle;
						}
						else if (SemanticInfoProviderToUse.GetTypedefs().Contains(TokenText))
						{
							RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.Type");
							RunInfo.Style = SyntaxTextStyle.TypeTextStyle;
						}
						else if (SemanticInfoProviderToUse.GetFunctions().Contains(TokenText)) // Changed from GetFunctions
						{
							RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.Function");
							RunInfo.Style = SyntaxTextStyle.FunctionTextStyle;
						}
						else if (SemanticInfoProviderToUse.GetTemplates().Contains(TokenText))
						{
							RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.Template");
							RunInfo.Style = SyntaxTextStyle.TemplateTextStyle;
						}
						else
						{
							RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.Normal");
							RunInfo.Style = SyntaxTextStyle.NormalTextStyle;
						}
					}
					else
					{
						RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.Normal");
						RunInfo.Style = SyntaxTextStyle.NormalTextStyle;
					}
				}
				else
				{
					RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.Normal");
					RunInfo.Style = SyntaxTextStyle.NormalTextStyle;
				}
			}
			else
			{
				RunInfo.RunTypeName = TEXT("SyntaxHighlight.CPP.WhiteSpace");
				RunInfo.Style = SyntaxTextStyle.NormalTextStyle;
			}
			
			TokenRunInfos.Add(RunInfo);
		}
		
		// Phase 2: Analyze the complete line and create runs
		TArray<TSharedRef<IRun>> Runs;
		
		// Check if this is a complete #include statement
		FIncludeAnalysisResult IncludeResult = AnalyzeIncludeStatement(TokenRunInfos, **ModelString);
		
		auto CreateIncludeHyperlinkRun = [this](
			const FRunInfo& InRunInfo,
			const TSharedRef<const FString>& InText,
			const FTextRange& InRange,
			const FIncludeStatementInfo& IncludeInfo)
		{
			// Create a custom hyperlink style with improved visual appearance
			// Following UE's official pattern from TestStyle.cpp
	
			// Create transparent button style for hyperlink underline
			FButtonStyle HyperlinkButtonStyle = FButtonStyle()
				.SetNormal(FSlateNoResource())
				.SetPressed(FSlateNoResource())
				.SetHovered(FSlateNoResource());
	
			FHyperlinkStyle HyperlinkStyle = FHyperlinkStyle()
				.SetUnderlineStyle(HyperlinkButtonStyle)  // Use transparent underline
				.SetTextStyle(SyntaxTextStyle.StringTextStyle)
				.SetPadding(FMargin(0.0f));
	
			// Enhance the hyperlink appearance
			// Blue color with emphasis
			// HyperlinkStyle.TextStyle.ColorAndOpacity = FLinearColor(0.2f, 0.5f, 0.9f, 1.0f); // Richer blue
			// HyperlinkStyle.TextStyle.Font.Size += 1; // Slightly larger font
	
			// Create the navigating delegate - this will be called when user clicks the hyperlink
			FSlateHyperlinkRun::FOnClick OnClickDelegate = FSlateHyperlinkRun::FOnClick::CreateStatic(
				&FCppRichTextSyntaxHighlightMarshaller::OnIncludeClicked, IncludeInfo.DisplayIncludePath);
	
			// Create tooltip delegate
			FSlateHyperlinkRun::FOnGetTooltipText TooltipTextDelegate = FSlateHyperlinkRun::FOnGetTooltipText::CreateStatic(
				&FCppRichTextSyntaxHighlightMarshaller::GetIncludeTooltip, IncludeInfo.DisplayIncludePath);
	
			// Create the hyperlink run
			return FSlateHyperlinkRun::Create(
				InRunInfo,
				InText,
				HyperlinkStyle,
				OnClickDelegate,
				FSlateHyperlinkRun::FOnGenerateTooltip(), // No custom tooltip widget
				TooltipTextDelegate,
				InRange);
		};

		auto CreateIncludeHyperlinkRuns = [this, &CreateIncludeHyperlinkRun](
			const TArray<FTokenRunInfo>& TokenRunInfos, 
			const FIncludeAnalysisResult& IncludeResult, 
			const TSharedRef<const FString>& ModelString, 
			TArray<TSharedRef<IRun>>& OutRuns)
		{
			// Create runs for the complete line
			for (int32 i = 0; i < TokenRunInfos.Num(); ++i)
			{
				const FTokenRunInfo& TokenInfo = TokenRunInfos[i];
				
				if (TokenInfo.bIsWhitespace)
				{
					FRunInfo RunInfo(TEXT("SyntaxHighlight.CPP.WhiteSpace"));
					TSharedRef<IRun> Run = FWhiteSpaceTextRun::Create(RunInfo, ModelString, TokenInfo.Style, TokenInfo.ModelRange, 4);
					OutRuns.Add(Run);
				}
				else if (i == IncludeResult.PathStartIndex)
				{
					// Safety check: ensure indices are valid
					if (IncludeResult.PathStartIndex >= 0 && IncludeResult.PathEndIndex < TokenRunInfos.Num() && 
						IncludeResult.PathStartIndex <= IncludeResult.PathEndIndex)
					{
						// Create a single hyperlink run covering the entire include path
						// Calculate the range that covers from first to last path token
						FTextRange PathRange(TokenRunInfos[IncludeResult.PathStartIndex].ModelRange.BeginIndex,
							TokenRunInfos[IncludeResult.PathEndIndex].ModelRange.EndIndex);
						
						// Note: We don't call FClingIncludeUtils::FindIncludeFilePath here to avoid performance issues during typing
						// File lookup will happen only when the hyperlink is clicked
						FIncludeStatementInfo IncludeInfo;
						IncludeInfo.DisplayIncludePath = IncludeResult.IncludePath;
						IncludeInfo.FullIncludePath = FString();  // Will be resolved on click
						IncludeInfo.bIsSystemInclude = IncludeResult.bIsSystemInclude;
						
						FRunInfo HyperlinkRunInfo(TEXT("SyntaxHighlight.CPP.IncludeHyperlink"));
						TSharedRef<FSlateHyperlinkRun> HyperlinkRun = CreateIncludeHyperlinkRun(
							HyperlinkRunInfo, ModelString, PathRange, IncludeInfo);
						OutRuns.Add(HyperlinkRun);
						
						// Skip the remaining path tokens since they're covered by the hyperlink run
						// Safety: ensure we don't go beyond array bounds
						i = FMath::Min(IncludeResult.PathEndIndex, TokenRunInfos.Num() - 1);
					}
					else
					{
						// Invalid indices, treat as regular token
						FRunInfo RunInfo(*TokenInfo.RunTypeName);
						TSharedRef<IRun> Run = FSlateTextRun::Create(RunInfo, ModelString, TokenInfo.Style, TokenInfo.ModelRange);
						OutRuns.Add(Run);
					}
				}
				else if (i > IncludeResult.PathStartIndex && i <= IncludeResult.PathEndIndex)
				{
					// Skip path tokens that are already covered by the hyperlink run
					continue;
				}
				else
				{
					// Regular token
					FRunInfo RunInfo(*TokenInfo.RunTypeName);
					TSharedRef<IRun> Run = FSlateTextRun::Create(RunInfo, ModelString, TokenInfo.Style, TokenInfo.ModelRange);
					OutRuns.Add(Run);
				}
			}
		};
		
		if (IncludeResult.bIsCompleteInclude)
		{
			// Create hyperlink for the include path
			CreateIncludeHyperlinkRuns(TokenRunInfos, IncludeResult, ModelString, Runs);
		}
		else
		{
			// Create normal runs using original logic
			CreateNormalRuns(TokenRunInfos, ModelString, Runs);
		}
		
		LinesToAdd.Emplace(MoveTemp(ModelString), MoveTemp(Runs));
	}

	TargetTextLayout.AddLines(LinesToAdd);
}

FCppRichTextSyntaxHighlightMarshaller::FCppRichTextSyntaxHighlightMarshaller(
	TSharedPtr< ISyntaxTokenizer > InTokenizer, 
	const FSyntaxTextStyle& InSyntaxTextStyle,
	const FClingSemanticInfoProvider& InProvider)
	: FSyntaxHighlighterTextLayoutMarshaller(MoveTemp(InTokenizer))
	, SyntaxTextStyle(InSyntaxTextStyle)
	, SemanticInfoProvider(InProvider)
{

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


void FCppRichTextSyntaxHighlightMarshaller::OnIncludeClicked(const FSlateHyperlinkRun::FMetadata& Metadata, FString DisplayPath)
{
	// This is called when user clicks the hyperlink
	// Now we perform the file lookup that was deferred during parsing
	UE_LOG(LogTemp, Log, TEXT("Include hyperlink clicked: '%s'"), *DisplayPath);
	
	FString FullPath = FClingIncludeUtils::FindIncludeFilePath(DisplayPath);
	
	if (FullPath.IsEmpty())
	{
		// Show error message when file is not found
		UE_LOG(LogTemp, Warning, TEXT("Include file not found: %s"), *DisplayPath);
		
		// TODO: Could show a notification to user instead of just logging
		// FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
		// 	NSLOCTEXT("CppHighlight", "FileNotFound", "Could not find include file: {0}"), 
		// 	FText::FromString(DisplayPath)));
		return;
	}
	
	FClingSourceAccessModule::EnsureFileOpenInIDE(FullPath, 1);
}

FText FCppRichTextSyntaxHighlightMarshaller::GetIncludeTooltip(const FSlateHyperlinkRun::FMetadata& Metadata, FString FullPath)
{
	// Don't check file existence here to avoid performance issues during editing
	// Just show the include path - file validation will happen on click
	return FText::Format(NSLOCTEXT("CppHighlight", "IncludeTooltip", "Click to open: {0}"),
		FText::FromString(FullPath));
}
