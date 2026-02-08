#include "ClingNotebook.h"
#include "ClingRuntime.h"
#include "ClingSetting.h"
#include "CppInterOp/CppInterOp.h"
#include "Modules/ModuleManager.h"
#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "ClingSourceAccess.h"
#include "Internationalization/Regex.h"
#include "Misc/MessageDialog.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"
#endif

namespace ClingNotebookFile
{
	FString GetPluginDir()
	{
		static FString PluginDir = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin(UE_PLUGIN_NAME)->GetBaseDir());
		return PluginDir;
	}
	
	FString NormalizeIncludePath(FString Path)
	{
		Path.ReplaceCharInline(TEXT('\\'), TEXT('/'));
		return Path;
	}

	FString GetNotebookDir()
	{
		return GetPluginDir() / TEXT("Source/ClingScript/Private/Notebook");
	}

	FString GetNotebookFilePath(const UClingNotebook* Notebook)
	{
		return GetNotebookDir() / (Notebook->GetName() + TEXT(".h"));
	}

	bool BuildNotebookFileContent(const UClingNotebook* Notebook, FString& OutContent, int32* OutSelectedLine)
	{
		int32 CurrentLine = 1;
		int32 SelectedLine = 1;

		auto AppendWithLineCount = [&OutContent, &CurrentLine](const FString& Text)
		{
			OutContent += Text;
			for (TCHAR Ch : Text)
			{
				if (Ch == TEXT('\n'))
				{
					++CurrentLine;
				}
			}
		};

		FString PCHIncludePath;
		{
			const UClingSetting* Setting = GetDefault<UClingSetting>();
			const FClingPCHProfile& DefaultProfile = Setting->GetProfile(Notebook->PCHProfile);
			PCHIncludePath = NormalizeIncludePath(DefaultProfile.GetPCHHeaderPath());
		}
		AppendWithLineCount(TEXT("#if !defined(COMPILE)\n"));
		AppendWithLineCount(FString::Printf(TEXT("#include \"%s\"\n"), *PCHIncludePath));
		AppendWithLineCount(TEXT("#endif\n\n"));

		for (int32 Index = 0; Index < Notebook->Cells.Num(); ++Index)
		{
			AppendWithLineCount(FString::Printf(TEXT("#if !defined(COMPILE) || COMPILE == %d\n"), Index));
			if (OutSelectedLine && Index == Notebook->SelectedCellIndex)
			{
				SelectedLine = CurrentLine;
			}

			FString CellContent = Notebook->Cells[Index].Content;
			CellContent.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
			if (!CellContent.EndsWith(TEXT("\n")))
			{
				CellContent += TEXT("\n");
			}
			AppendWithLineCount(CellContent);
			AppendWithLineCount(TEXT("#endif\n\n"));
		}

		if (OutSelectedLine)
		{
			*OutSelectedLine = SelectedLine;
		}
		return true;
	}

	bool WriteNotebookFile(const UClingNotebook* Notebook, FString& OutFilePath, int32* OutSelectedLine)
	{
		const FString NotebookDir = GetNotebookDir();
		if (!IFileManager::Get().MakeDirectory(*NotebookDir, true))
		{
			return false;
		}

		OutFilePath = GetNotebookFilePath(Notebook);

		FString FileContent;
		BuildNotebookFileContent(Notebook, FileContent, OutSelectedLine);
		return FFileHelper::SaveStringToFile(FileContent, *OutFilePath);
	}

	bool WriteNotebookCompileFile(const UClingNotebook* Notebook, int32 SectionIndex, FString& OutFilePath)
	{
		FString BaseFilePath;
		if (!WriteNotebookFile(Notebook, BaseFilePath, nullptr))
		{
			return false;
		}

		const FString Suffix = FString::Printf(TEXT("_Compile_%d_%s.h"), SectionIndex, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		OutFilePath = GetNotebookDir() / (Notebook->GetName() + Suffix);

		FString BaseContent;
		if (!FFileHelper::LoadFileToString(BaseContent, *BaseFilePath))
		{
			return false;
		}

		return FFileHelper::SaveStringToFile(BaseContent, *OutFilePath,FFileHelper::EEncodingOptions::ForceUTF8);
	}
}

#if WITH_EDITOR
namespace ClingNotebookIDE
{
	bool TryParseSectionIndex(const FString& Line, int32& OutIndex)
	{
		FString Trimmed = Line;
		Trimmed.TrimStartAndEndInline();
		if (!Trimmed.StartsWith(TEXT("#if")))
		{
			return false;
		}
		if (!Trimmed.Contains(TEXT("COMPILE")))
		{
			return false;
		}
		const int32 EqPos = Trimmed.Find(TEXT("=="));
		if (EqPos == INDEX_NONE)
		{
			return false;
		}
		FString Right = Trimmed.Mid(EqPos + 2);
		Right.TrimStartAndEndInline();

		FString Digits;
		for (const TCHAR Ch : Right)
		{
			if (!FChar::IsDigit(Ch))
			{
				break;
			}
			Digits.AppendChar(Ch);
		}
		if (Digits.IsEmpty())
		{
			return false;
		}
		OutIndex = FCString::Atoi(*Digits);
		return true;
	}

	bool IsIfLine(const FString& Line)
	{
		FString Trimmed = Line;
		Trimmed.TrimStartAndEndInline();
		return Trimmed.StartsWith(TEXT("#if"));
	}

	bool IsEndifLine(const FString& Line)
	{
		FString Trimmed = Line;
		Trimmed.TrimStartAndEndInline();
		return Trimmed.StartsWith(TEXT("#endif"));
	}

	bool ParseSectionsFromLines(const TArray<FString>& Lines, TMap<int32, FString>& OutSections)
	{
		OutSections.Reset();

		bool bInSection = false;
		int32 CurrentIndex = INDEX_NONE;
		int32 Depth = 0;
		TArray<FString> CurrentLines;

		for (const FString& Line : Lines)
		{
			if (!bInSection)
			{
				int32 SectionIndex = INDEX_NONE;
				if (TryParseSectionIndex(Line, SectionIndex))
				{
					bInSection = true;
					CurrentIndex = SectionIndex;
					Depth = 1;
					CurrentLines.Reset();
				}
				continue;
			}

			if (IsIfLine(Line))
			{
				++Depth;
				CurrentLines.Add(Line);
				continue;
			}

			if (IsEndifLine(Line))
			{
				--Depth;
				if (Depth == 0)
				{
					OutSections.Add(CurrentIndex, FString::Join(CurrentLines, TEXT("\n")));
					bInSection = false;
					CurrentIndex = INDEX_NONE;
					continue;
				}
				CurrentLines.Add(Line);
				continue;
			}

			CurrentLines.Add(Line);
		}

		if (bInSection)
		{
			OutSections.Add(CurrentIndex, FString::Join(CurrentLines, TEXT("\n")));
		}

		return OutSections.Num() > 0;
	}

	bool LoadSectionsFromFile(const FString& FilePath, TMap<int32, FString>& OutSections)
	{
		TArray<FString> Lines;
		if (!FFileHelper::LoadFileToStringArray(Lines, *FilePath))
		{
			return false;
		}
		return ParseSectionsFromLines(Lines, OutSections);
	}
}

namespace ClingNotebookSymbols
{
	FString StripComments(const FString& InText)
	{
		FString Out;
		Out.Reserve(InText.Len());

		bool bInLineComment = false;
		bool bInBlockComment = false;
		bool bInString = false;
		bool bInChar = false;
		bool bEscaped = false;

		for (int32 Index = 0; Index < InText.Len(); ++Index)
		{
			const TCHAR Ch = InText[Index];
			const TCHAR Next = (Index + 1 < InText.Len()) ? InText[Index + 1] : TEXT('\0');

			if (bInLineComment)
			{
				if (Ch == TEXT('\n'))
				{
					bInLineComment = false;
					Out.AppendChar(Ch);
				}
				continue;
			}

			if (bInBlockComment)
			{
				if (Ch == TEXT('*') && Next == TEXT('/'))
				{
					bInBlockComment = false;
					++Index;
				}
				continue;
			}

			if (!bInString && !bInChar)
			{
				if (Ch == TEXT('/') && Next == TEXT('/'))
				{
					bInLineComment = true;
					++Index;
					continue;
				}
				if (Ch == TEXT('/') && Next == TEXT('*'))
				{
					bInBlockComment = true;
					++Index;
					continue;
				}
			}

			if (bEscaped)
			{
				bEscaped = false;
			}
			else if (Ch == TEXT('\\'))
			{
				bEscaped = true;
			}
			else if (!bInChar && Ch == TEXT('\"'))
			{
				bInString = !bInString;
			}
			else if (!bInString && Ch == TEXT('\''))
			{
				bInChar = !bInChar;
			}

			Out.AppendChar(Ch);
		}

		return Out;
	}

	FString StripQualifiers(const FString& InText)
	{
		static const TSet<FString> Qualifiers = {
			TEXT("const"),
			TEXT("volatile"),
			TEXT("static"),
			TEXT("inline"),
			TEXT("constexpr"),
			TEXT("mutable"),
			TEXT("typename"),
			TEXT("class"),
			TEXT("struct"),
			TEXT("enum")
		};

		TArray<FString> Tokens;
		InText.ParseIntoArrayWS(Tokens);

		TArray<FString> Kept;
		for (const FString& Token : Tokens)
		{
			if (!Qualifiers.Contains(Token))
			{
				Kept.Add(Token);
			}
		}
		return FString::Join(Kept, TEXT(" "));
	}

	bool ExtractTemplateArg(const FString& InText, FString& OutInner)
	{
		const int32 Start = InText.Find(TEXT("<"));
		if (Start == INDEX_NONE)
		{
			return false;
		}

		int32 Depth = 0;
		for (int32 Index = Start; Index < InText.Len(); ++Index)
		{
			const TCHAR Ch = InText[Index];
			if (Ch == TEXT('<'))
			{
				++Depth;
			}
			else if (Ch == TEXT('>'))
			{
				--Depth;
				if (Depth == 0)
				{
					OutInner = InText.Mid(Start + 1, Index - Start - 1);
					return true;
				}
			}
		}
		return false;
	}

	bool TryResolveClass(const FString& TypeName, UClass*& OutClass)
	{
		OutClass = FindObject<UClass>(ANY_PACKAGE, *TypeName);
		if (!OutClass && TypeName == TEXT("UObject"))
		{
			OutClass = UObject::StaticClass();
		}
		return OutClass != nullptr;
	}

	bool TryResolveStruct(const FString& TypeName, UScriptStruct*& OutStruct)
	{
		OutStruct = FindObject<UScriptStruct>(ANY_PACKAGE, *TypeName);
		return OutStruct != nullptr;
	}

	bool TryResolveEnum(const FString& TypeName, UEnum*& OutEnum)
	{
		OutEnum = FindObject<UEnum>(ANY_PACKAGE, *TypeName);
		return OutEnum != nullptr;
	}

	bool ParseTypeRecursive(const FString& InType, FClingNotebookTypeDesc& OutDesc, FPropertyBagContainerTypes& InOutContainers)
	{
		FString Working = StripQualifiers(InType).TrimStartAndEnd();
		Working.ReplaceInline(TEXT("\t"), TEXT(" "));
		Working.TrimStartAndEndInline();

		if (Working.IsEmpty())
		{
			return false;
		}

		const bool bIsVoid = Working.Equals(TEXT("void"), ESearchCase::IgnoreCase);
		if (bIsVoid)
		{
			OutDesc.bIsVoid = true;
			OutDesc.bSupported = false;
			return true;
		}

		bool bPointer = Working.EndsWith(TEXT("*"));
		bool bReference = Working.EndsWith(TEXT("&"));
		while (Working.EndsWith(TEXT("*")) || Working.EndsWith(TEXT("&")))
		{
			Working.LeftChopInline(1);
			Working.TrimEndInline();
		}

		if (Working.StartsWith(TEXT("TArray<")))
		{
			if (!InOutContainers.Add(EPropertyBagContainerType::Array))
			{
				return false;
			}
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner))
			{
				return false;
			}
			return ParseTypeRecursive(Inner, OutDesc, InOutContainers);
		}

		if (Working.StartsWith(TEXT("TSet<")))
		{
			if (!InOutContainers.Add(EPropertyBagContainerType::Set))
			{
				return false;
			}
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner))
			{
				return false;
			}
			return ParseTypeRecursive(Inner, OutDesc, InOutContainers);
		}

		if (Working.StartsWith(TEXT("TSubclassOf<")))
		{
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner))
			{
				return false;
			}
			UClass* ResolvedClass = nullptr;
			if (!TryResolveClass(Inner.TrimStartAndEnd(), ResolvedClass))
			{
				return false;
			}
			OutDesc.ValueType = EPropertyBagPropertyType::Class;
			OutDesc.ValueTypeObject = ResolvedClass;
			OutDesc.ContainerTypes = InOutContainers;
			OutDesc.bSupported = true;
			return true;
		}

		if (Working.StartsWith(TEXT("TSoftObjectPtr<")))
		{
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner))
			{
				return false;
			}
			UClass* ResolvedClass = nullptr;
			if (!TryResolveClass(Inner.TrimStartAndEnd(), ResolvedClass))
			{
				return false;
			}
			OutDesc.ValueType = EPropertyBagPropertyType::SoftObject;
			OutDesc.ValueTypeObject = ResolvedClass;
			OutDesc.ContainerTypes = InOutContainers;
			OutDesc.bSupported = true;
			return true;
		}

		if (Working.StartsWith(TEXT("TSoftClassPtr<")))
		{
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner))
			{
				return false;
			}
			UClass* ResolvedClass = nullptr;
			if (!TryResolveClass(Inner.TrimStartAndEnd(), ResolvedClass))
			{
				return false;
			}
			OutDesc.ValueType = EPropertyBagPropertyType::SoftClass;
			OutDesc.ValueTypeObject = ResolvedClass;
			OutDesc.ContainerTypes = InOutContainers;
			OutDesc.bSupported = true;
			return true;
		}

		if (Working.StartsWith(TEXT("TObjectPtr<")))
		{
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner))
			{
				return false;
			}
			UClass* ResolvedClass = nullptr;
			if (!TryResolveClass(Inner.TrimStartAndEnd(), ResolvedClass))
			{
				return false;
			}
			OutDesc.ValueType = EPropertyBagPropertyType::Object;
			OutDesc.ValueTypeObject = ResolvedClass;
			OutDesc.ContainerTypes = InOutContainers;
			OutDesc.bSupported = true;
			return true;
		}

		if (bPointer || bReference)
		{
			const FString BaseType = Working.TrimStartAndEnd();
			UClass* ResolvedClass = nullptr;
			if (TryResolveClass(BaseType, ResolvedClass))
			{
				OutDesc.ValueType = EPropertyBagPropertyType::Object;
				OutDesc.ValueTypeObject = ResolvedClass;
				OutDesc.ContainerTypes = InOutContainers;
				OutDesc.bSupported = true;
				return true;
			}

			if (BaseType == TEXT("UClass"))
			{
				OutDesc.ValueType = EPropertyBagPropertyType::Class;
				OutDesc.ValueTypeObject = UObject::StaticClass();
				OutDesc.ContainerTypes = InOutContainers;
				OutDesc.bSupported = true;
				return true;
			}
		}

		const FString BaseType = Working.TrimStartAndEnd();
		if (BaseType.Equals(TEXT("bool"), ESearchCase::IgnoreCase))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Bool;
		}
		else if (BaseType.Equals(TEXT("uint8"), ESearchCase::IgnoreCase) || BaseType.Equals(TEXT("uint8_t"), ESearchCase::IgnoreCase)
			|| BaseType.Equals(TEXT("byte"), ESearchCase::IgnoreCase))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Byte;
		}
		else if (BaseType.Equals(TEXT("int32"), ESearchCase::IgnoreCase) || BaseType.Equals(TEXT("int"), ESearchCase::IgnoreCase))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Int32;
		}
		else if (BaseType.Equals(TEXT("int64"), ESearchCase::IgnoreCase) || BaseType.Equals(TEXT("long long"), ESearchCase::IgnoreCase))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Int64;
		}
		else if (BaseType.Equals(TEXT("uint32"), ESearchCase::IgnoreCase) || BaseType.Equals(TEXT("unsigned int"), ESearchCase::IgnoreCase))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::UInt32;
		}
		else if (BaseType.Equals(TEXT("uint64"), ESearchCase::IgnoreCase) || BaseType.Equals(TEXT("unsigned long long"), ESearchCase::IgnoreCase))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::UInt64;
		}
		else if (BaseType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Float;
		}
		else if (BaseType.Equals(TEXT("double"), ESearchCase::IgnoreCase))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Double;
		}
		else if (BaseType.Equals(TEXT("FName"), ESearchCase::IgnoreCase))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Name;
		}
		else if (BaseType.Equals(TEXT("FString"), ESearchCase::IgnoreCase))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::String;
		}
		else if (BaseType.Equals(TEXT("FText"), ESearchCase::IgnoreCase))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Text;
		}
		else
		{
			UScriptStruct* Struct = nullptr;
			if (TryResolveStruct(BaseType, Struct))
			{
				OutDesc.ValueType = EPropertyBagPropertyType::Struct;
				OutDesc.ValueTypeObject = Struct;
			}
			else
			{
				UEnum* Enum = nullptr;
				if (TryResolveEnum(BaseType, Enum))
				{
					OutDesc.ValueType = EPropertyBagPropertyType::Enum;
					OutDesc.ValueTypeObject = Enum;
				}
				else
				{
					return false;
				}
			}
		}

		OutDesc.ContainerTypes = InOutContainers;
		OutDesc.bSupported = OutDesc.ValueType != EPropertyBagPropertyType::None;
		return OutDesc.bSupported;
	}

	bool TryBuildTypeDesc(const FString& InType, FClingNotebookTypeDesc& OutDesc)
	{
		OutDesc = FClingNotebookTypeDesc();
		OutDesc.OriginalType = InType.TrimStartAndEnd();

		FPropertyBagContainerTypes Containers;
		return ParseTypeRecursive(OutDesc.OriginalType, OutDesc, Containers);
	}

	void SplitTopLevel(const FString& InText, TArray<FString>& OutParts)
	{
		OutParts.Reset();
		FString Current;
		int32 DepthAngle = 0;
		int32 DepthParen = 0;
		int32 DepthBracket = 0;

		for (int32 Index = 0; Index < InText.Len(); ++Index)
		{
			const TCHAR Ch = InText[Index];
			if (Ch == TEXT('<')) ++DepthAngle;
			else if (Ch == TEXT('>')) --DepthAngle;
			else if (Ch == TEXT('(')) ++DepthParen;
			else if (Ch == TEXT(')')) --DepthParen;
			else if (Ch == TEXT('[')) ++DepthBracket;
			else if (Ch == TEXT(']')) --DepthBracket;

			if (Ch == TEXT(',') && DepthAngle == 0 && DepthParen == 0 && DepthBracket == 0)
			{
				OutParts.Add(Current.TrimStartAndEnd());
				Current.Reset();
				continue;
			}
			Current.AppendChar(Ch);
		}

		if (!Current.IsEmpty())
		{
			OutParts.Add(Current.TrimStartAndEnd());
		}
	}

	FString TrimDefaultValue(const FString& InParam)
	{
		int32 DepthAngle = 0;
		for (int32 Index = 0; Index < InParam.Len(); ++Index)
		{
			const TCHAR Ch = InParam[Index];
			if (Ch == TEXT('<')) ++DepthAngle;
			else if (Ch == TEXT('>')) --DepthAngle;
			else if (Ch == TEXT('=') && DepthAngle == 0)
			{
				return InParam.Left(Index).TrimStartAndEnd();
			}
		}
		return InParam.TrimStartAndEnd();
	}

	bool TryParseParam(const FString& InParam, FClingNotebookParamDesc& OutParam, int32 ParamIndex)
	{
		FString Param = TrimDefaultValue(InParam);
		if (Param.IsEmpty() || Param.Equals(TEXT("void"), ESearchCase::IgnoreCase))
		{
			return false;
		}

		int32 LastSpace = INDEX_NONE;
		Param.FindLastChar(TEXT(' '), LastSpace);
		if (LastSpace == INDEX_NONE)
		{
			OutParam.Name = *FString::Printf(TEXT("Param%d"), ParamIndex);
			return TryBuildTypeDesc(Param, OutParam.Type);
		}

		FString TypeText = Param.Left(LastSpace).TrimStartAndEnd();
		FString NameText = Param.Mid(LastSpace + 1).TrimStartAndEnd();

		while (NameText.StartsWith(TEXT("*")) || NameText.StartsWith(TEXT("&")))
		{
			TypeText += NameText.Left(1);
			NameText.RightChopInline(1);
			NameText.TrimStartInline();
		}

		OutParam.Name = *NameText;
		return TryBuildTypeDesc(TypeText, OutParam.Type);
	}

	bool ExtractFunctionSymbols(const FString& InText, TArray<FClingNotebookSymbolDesc>& OutSymbols)
	{
		bool bAny = false;
		FRegexPattern Pattern(TEXT("(?m)^\\s*([\\w:<>&\\*\\s]+?)\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\(([^\\)]*)\\)\\s*(?:const\\s*)?\\{"));
		FRegexMatcher Matcher(Pattern, InText);

		while (Matcher.FindNext())
		{
			const FString ReturnType = Matcher.GetCaptureGroup(1).TrimStartAndEnd();
			const FString Name = Matcher.GetCaptureGroup(2).TrimStartAndEnd();
			const FString Params = Matcher.GetCaptureGroup(3).TrimStartAndEnd();

			FClingNotebookSymbolDesc Symbol;
			Symbol.Kind = EClingNotebookSymbolKind::Function;
			Symbol.Name = *Name;

			if (!TryBuildTypeDesc(ReturnType, Symbol.ReturnType))
			{
				if (!Symbol.ReturnType.bIsVoid)
				{
					continue;
				}
			}

			TArray<FString> ParamParts;
			SplitTopLevel(Params, ParamParts);

			bool bParamsOk = true;
			for (int32 Index = 0; Index < ParamParts.Num(); ++Index)
			{
				FClingNotebookParamDesc ParamDesc;
				if (!TryParseParam(ParamParts[Index], ParamDesc, Index))
				{
					continue;
				}
				if (!ParamDesc.Type.bSupported)
				{
					bParamsOk = false;
					break;
				}
				Symbol.Params.Add(MoveTemp(ParamDesc));
			}

			if (!bParamsOk)
			{
				continue;
			}

			OutSymbols.Add(MoveTemp(Symbol));
			bAny = true;
		}

		return bAny;
	}

	bool ExtractVariableSymbols(const FString& InText, TArray<FClingNotebookSymbolDesc>& OutSymbols)
	{
		bool bAny = false;
		TArray<FString> Lines;
		InText.ParseIntoArrayLines(Lines);

		const TSet<FString> SkipStarts = {
			TEXT("if"), TEXT("for"), TEXT("while"), TEXT("switch"), TEXT("case"), TEXT("return"),
			TEXT("else"), TEXT("do"), TEXT("class"), TEXT("struct"), TEXT("enum"), TEXT("typedef"),
			TEXT("using"), TEXT("namespace")
		};

		for (const FString& RawLine : Lines)
		{
			FString Line = RawLine.TrimStartAndEnd();
			if (Line.IsEmpty())
			{
				continue;
			}
			if (!Line.EndsWith(TEXT(";")))
			{
				continue;
			}
			if (Line.Contains(TEXT("(")))
			{
				continue;
			}

			TArray<FString> LineTokens;
			Line.ParseIntoArrayWS(LineTokens);
			if (LineTokens.Num() > 0 && SkipStarts.Contains(LineTokens[0]))
			{
				continue;
			}

			FRegexPattern Pattern(TEXT("^\\s*([\\w:<>&\\*\\s]+?)\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*(?:=|;)"));
			FRegexMatcher Matcher(Pattern, Line);
			if (!Matcher.FindNext())
			{
				continue;
			}

			const FString TypeText = Matcher.GetCaptureGroup(1).TrimStartAndEnd();
			const FString NameText = Matcher.GetCaptureGroup(2).TrimStartAndEnd();

			FClingNotebookTypeDesc TypeDesc;
			if (!TryBuildTypeDesc(TypeText, TypeDesc))
			{
				continue;
			}
			if (!TypeDesc.bSupported)
			{
				continue;
			}

			FClingNotebookSymbolDesc Symbol;
			Symbol.Kind = EClingNotebookSymbolKind::Variable;
			Symbol.Name = *NameText;
			Symbol.ValueType = MoveTemp(TypeDesc);
			OutSymbols.Add(MoveTemp(Symbol));
			bAny = true;
		}

		return bAny;
	}

	bool ExtractSymbolsFromContent(const FString& InContent, TArray<FClingNotebookSymbolDesc>& OutSymbols)
	{
		OutSymbols.Reset();
		const FString Clean = StripComments(InContent);
		ExtractFunctionSymbols(Clean, OutSymbols);
		ExtractVariableSymbols(Clean, OutSymbols);
		return OutSymbols.Num() > 0;
	}
}
#endif

void UClingNotebook::FinishDestroy()
{
	if (Interpreter)
	{
		FClingRuntimeModule::Get().DeleteInterp(Interpreter);
		Interpreter = nullptr;
	}
	Super::FinishDestroy();
}

const FClingSemanticInfoProvider& UClingNotebook::GetUsableSemanticInfoProvider() const
{
	if (SemanticInfoProvider.IsReady())
		return const_cast<FClingSemanticInfoProvider&>(SemanticInfoProvider);
	return *FClingRuntimeModule::Get().GetDefaultSemanticInfoProvider();
}

void UClingNotebook::SetSelectedCell(int32 Index)
{
	if (SelectedCellIndex != Index)
	{
		SelectedCellIndex = Index;
		OnCellSelectionChanged.Broadcast(SelectedCellIndex);
	}
}

void* UClingNotebook::GetInterpreter()
{
	if (!Interpreter)
	{
		Interpreter = FClingRuntimeModule::Get().StartNewInterp(PCHProfile);
	}
	return Interpreter;
}

TFuture<FClingInterpreterResult> UClingNotebook::GetInterpreterAsync()
{
	// If interpreter already exists, return completed future immediately
	if (Interpreter)
	{
		return MakeFulfilledPromise<FClingInterpreterResult>(FClingInterpreterResult(Interpreter)).GetFuture();
	}

	// If already initializing, return a future that will be fulfilled when init completes
	if (bIsInitializingInterpreter && InterpreterPromise.IsValid())
	{
		return InterpreterPromise->GetFuture();
	}

	// Start async initialization
	return StartInterpreterAsync();
}

TFuture<FClingInterpreterResult> UClingNotebook::StartInterpreterAsync()
{
	bIsInitializingInterpreter = true;

	// Create shared promise that can fulfill multiple futures
	InterpreterPromise = MakeShared<TPromise<FClingInterpreterResult>>();
	TFuture<FClingInterpreterResult> ResultFuture = InterpreterPromise->GetFuture();

	TWeakObjectPtr<UClingNotebook> WeakThis(this);
	TSharedPtr<TPromise<FClingInterpreterResult>> PromiseCapture = InterpreterPromise;

	// Start interpreter creation on background thread
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, PromiseCapture]()
	{
		UClingNotebook* NotebookPtr = WeakThis.Get();
		FName ProfileName = NotebookPtr ? NotebookPtr->PCHProfile : TEXT("Default");
		
		void* NewInterpreter = FClingRuntimeModule::Get().StartNewInterp(ProfileName);

		// Return to game thread to set the interpreter and fulfill promise
		AsyncTask(ENamedThreads::GameThread, [WeakThis, PromiseCapture, NewInterpreter]()
		{
			if (!WeakThis.IsValid())
			{
				// Notebook destroyed during init, clean up
				if (NewInterpreter)
				{
					FClingRuntimeModule::Get().DeleteInterp(NewInterpreter);
				}
				PromiseCapture->SetValue(FClingInterpreterResult(nullptr, TEXT("Notebook destroyed during initialization")));
				return;
			}

			UClingNotebook* Notebook = WeakThis.Get();
			Notebook->Interpreter = NewInterpreter;
			Notebook->bIsInitializingInterpreter = false;
			Notebook->SemanticInfoProvider.Refresh(NewInterpreter);
			Notebook->InterpreterPromise.Reset();

			// Fulfill the promise - this triggers all waiting futures
			PromiseCapture->SetValue(FClingInterpreterResult(NewInterpreter));
		});
	});

	return ResultFuture;
}

FClingCellCompilationResult UClingNotebook::ExecuteCellCompilation(void* Interp, const FString& Code)
{
	FScopeLock Lock(&FClingRuntimeModule::Get().GetCppInterOpLock());
	void* StoreInterp = Cpp::GetInterpreter();
	Cpp::ActivateInterpreter(Interp);

	static FString StaticErrors;
	StaticErrors.Reset();

	Cpp::BeginStdStreamCapture(Cpp::kStdErr);
	int32 ResultCode = Cpp::Process(TCHAR_TO_ANSI(*Code));
	Cpp::EndStdStreamCapture([](const char* Result)
	{
		StaticErrors = UTF8_TO_TCHAR(Result);
	});

	Cpp::ActivateInterpreter(StoreInterp);

	FClingCellCompilationResult CompilationResult;
	CompilationResult.ErrorOutput = StaticErrors;
	CompilationResult.bSuccess = (ResultCode == 0);
	return CompilationResult;
}

TFuture<FClingCellCompilationResult> UClingNotebook::CompileCellAsync(void* Interp, int32 CellIndex)
{
	TSharedRef<TPromise<FClingCellCompilationResult>> CompilePromise = MakeShared<TPromise<FClingCellCompilationResult>>();
	TFuture<FClingCellCompilationResult> ResultFuture = CompilePromise->GetFuture();

	if (!Cells.IsValidIndex(CellIndex) || !Interp)
	{
		FClingCellCompilationResult ErrorResult;
		ErrorResult.ErrorOutput = TEXT("Invalid cell index or interpreter");
		ErrorResult.bSuccess = false;
		CompilePromise->SetValue(ErrorResult);
		return ResultFuture;
	}

	// Generate compilation file
	FString NotebookFilePath;
	if (!ClingNotebookFile::WriteNotebookCompileFile(this, CellIndex, NotebookFilePath))
	{
		FClingCellCompilationResult ErrorResult;
		ErrorResult.ErrorOutput = TEXT("Failed to generate compilation file");
		ErrorResult.bSuccess = false;
		CompilePromise->SetValue(ErrorResult);
		return ResultFuture;
	}

	const FString IncludePath = ClingNotebookFile::NormalizeIncludePath(NotebookFilePath);
	const FString Code = FString::Printf(TEXT("#undef COMPILE\n#define COMPILE %d\n#include \"%s\"\n"), CellIndex, *IncludePath);

	bool bRunInGameThread = Cells[CellIndex].bExecuteInGameThread;
	TWeakObjectPtr<UClingNotebook> WeakThis(this);

	if (bRunInGameThread)
	{
		// Execute synchronously on game thread
		FClingCellCompilationResult Result = ExecuteCellCompilation(Interp, Code);
		OnCellCompilationComplete(CellIndex, Result);
		CompilePromise->SetValue(Result);
	}
	else
	{
		// Execute on background thread
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, Interp, Code, CellIndex, CompilePromise]()
		{
			FClingCellCompilationResult Result;
			Result.ErrorOutput = TEXT("Compilation failed");
			Result.bSuccess = false;

			if (WeakThis.IsValid())
			{
				UClingNotebook* Notebook = WeakThis.Get();
				Result = Notebook->ExecuteCellCompilation(Interp, Code);
			}

			// Return to game thread to update state
			AsyncTask(ENamedThreads::GameThread, [WeakThis, CellIndex, Result, CompilePromise]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->OnCellCompilationComplete(CellIndex, Result);
				}
				CompilePromise->SetValue(Result);
			});
		});
	}

	return ResultFuture;
}

void UClingNotebook::OnCellCompilationComplete(int32 CellIndex, const FClingCellCompilationResult& Result)
{
	if (!Cells.IsValidIndex(CellIndex))
	{
		CompilingCells.Remove(CellIndex);
		return;
	}

	FClingNotebookCellData& CellData = Cells[CellIndex];
	CellData.LastCompilationResult = Result;
	CellData.Output = Result.ErrorOutput;
	CellData.bHasOutput = true;
	CellData.CompilationState = EClingCellCompilationState::Idle;

	if (Result.bSuccess)
	{
		CellData.CompilationState = EClingCellCompilationState::Completed;
		SemanticInfoProvider.Refresh(GetInterpreter());
		if (CellData.Output.IsEmpty())
		{
			CellData.Output = FString::Printf(TEXT("Success (Executed at %s)"), *FDateTime::Now().ToString());
		}
	}

	CompilingCells.Remove(CellIndex);
	MarkPackageDirty();
}

void UClingNotebook::RestartInterpreter()
{
	if (Interpreter)
	{
		FClingRuntimeModule::Get().DeleteInterp(Interpreter);
	}
	Interpreter = FClingRuntimeModule::Get().StartNewInterp(PCHProfile);

	// Reset status for all cells
	for (auto& Cell : Cells)
	{
		Cell.CompilationState = EClingCellCompilationState::Idle;
		Cell.bHasOutput = false;
		Cell.Output.Empty();
	}
	MarkPackageDirty();
}

void UClingNotebook::AddNewCell(int32 InIndex)
{
	FClingNotebookCellData NewCellData;
	NewCellData.bIsExpanded = true;
	NewCellData.CompilationState = EClingCellCompilationState::Idle;

	if (InIndex == -1 || InIndex >= Cells.Num())
	{
		Cells.Add(NewCellData);
	}
	else
	{
		Cells.Insert(NewCellData, InIndex);
	}

	MarkPackageDirty();
}

void UClingNotebook::DeleteCell(int32 InIndex)
{
	if (Cells.IsValidIndex(InIndex))
	{
		Cells.RemoveAt(InIndex);

		// Adjust selection
		if (SelectedCellIndex == InIndex)
		{
			SelectedCellIndex = -1;
		}
		else if (SelectedCellIndex > InIndex)
		{
			SelectedCellIndex--;
		}

		MarkPackageDirty();
	}
}

void UClingNotebook::RunCell(int32 InIndex)
{
	if (!Cells.IsValidIndex(InIndex)) return;
	if (ExecutionQueue.Contains(InIndex) || CompilingCells.Contains(InIndex)) return;

	ExecutionQueue.Add(InIndex);
	Cells[InIndex].CompilationState = EClingCellCompilationState::Compiling;
	CompilingCells.Add(InIndex);
	bIsCompiling = true;

	ProcessNextInQueue();
}

void UClingNotebook::RunToHere(int32 InIndex)
{
	if (!Cells.IsValidIndex(InIndex)) return;

	for (int32 i = 0; i <= InIndex; ++i)
	{
		if (!ExecutionQueue.Contains(i) && !CompilingCells.Contains(i) && Cells[i].CompilationState != EClingCellCompilationState::Completed)
		{
			ExecutionQueue.Add(i);
			Cells[i].CompilationState = EClingCellCompilationState::Compiling;
			CompilingCells.Add(i);
		}
	}

	if (CompilingCells.Num() > 0)
	{
		bIsCompiling = true;
		ProcessNextInQueue();
	}
}

void UClingNotebook::UndoToHere(int32 InIndex)
{
	if (!Cells.IsValidIndex(InIndex)) return;
	if (bIsCompiling) return;

	int32 UndoCount = 0;
	bool bNeedsReset = false;
	for (int32 i = InIndex; i < Cells.Num(); ++i)
	{
		if (Cells[i].CompilationState == EClingCellCompilationState::Completed)
		{
			UndoCount++;
		}
		if (Cells[i].CompilationState != EClingCellCompilationState::Idle || Cells[i].bHasOutput)
		{
			bNeedsReset = true;
		}
	}

	if (UndoCount > 0)
	{
		FScopeLock Lock(&FClingRuntimeModule::Get().GetCppInterOpLock());
		void* Interp = GetInterpreter();
		if (Interp)
		{
			void* StoreInterp = Cpp::GetInterpreter();
			Cpp::ActivateInterpreter(Interp);
			Cpp::Undo(UndoCount);
			Cpp::ActivateInterpreter(StoreInterp);
		}
	}

	if (bNeedsReset)
	{
		for (int32 i = InIndex; i < Cells.Num(); ++i)
		{
			Cells[i].CompilationState = EClingCellCompilationState::Idle;
			Cells[i].bHasOutput = false;
			Cells[i].Output.Empty();
		}

		MarkPackageDirty();
	}
}

void UClingNotebook::ProcessNextInQueue()
{
	if (bIsProcessingQueue || ExecutionQueue.Num() == 0)
	{
		if (!bIsProcessingQueue)
		{
			bIsCompiling = (CompilingCells.Num() > 0);
		}
		return;
	}

	bIsProcessingQueue = true;
	int32 CellIndex = ExecutionQueue[0];
	ExecutionQueue.RemoveAt(0);

	if (!Cells.IsValidIndex(CellIndex))
	{
		bIsProcessingQueue = false;
		ProcessNextInQueue();
		return;
	}

	TWeakObjectPtr<UClingNotebook> WeakThis(this);

	/*
	 * TFuture链式执行流程:
	 *
	 * 1. GetInterpreterAsync() 返回 TFuture<FClingInterpreterResult>
	 *    - 如果Interpreter已存在: 返回立即完成的Future
	 *    - 如果正在初始化: 返回等待中的Future(共享同一个Promise)
	 *    - 如果需要初始化: 启动后台线程创建,返回Future
	 *
	 * 2. Next() 非阻塞回调
	 *    - Future完成时在调用SetValue的线程执行
	 *    - 使用AsyncTask回到GameThread安全更新状态
	 *
	 * 3. CompileCellAsync() 返回 TFuture<FClingCellCompilationResult>
	 *    - 在后台线程执行编译
	 *    - 完成后自动触发状态更新和队列处理
	 *
	 * 对象持有关系:
	 * - WeakThis: 弱引用,不延长Notebook生命周期
	 * - Promise被TSharedRef持有,Future持有Promise的State
	 * - Lambda捕获的变量在Future完成前保持有效
	 */
	GetInterpreterAsync().Next([WeakThis, CellIndex](const FClingInterpreterResult& InterpResult)
	{
		// 此回调可能在后台线程执行,需要回到GameThread
		AsyncTask(ENamedThreads::GameThread, [WeakThis, CellIndex, InterpResult]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
	
			UClingNotebook* Notebook = WeakThis.Get();
	
			if (!InterpResult.bSuccess || !InterpResult.Interpreter)
			{
				// Interpreter创建失败
				if (Notebook->Cells.IsValidIndex(CellIndex))
				{
					FClingCellCompilationResult FailedResult;
					FailedResult.ErrorOutput = InterpResult.ErrorMessage.IsEmpty() 
						? TEXT("Failed to create interpreter") 
						: InterpResult.ErrorMessage;
					FailedResult.bSuccess = false;
					Notebook->OnCellCompilationComplete(CellIndex, FailedResult);
				}
				Notebook->CompilingCells.Remove(CellIndex);
				Notebook->bIsProcessingQueue = false;
				Notebook->bIsCompiling = (Notebook->CompilingCells.Num() > 0);
				Notebook->ProcessNextInQueue();
				return;
			}
	
			// 启动编译,编译完成后继续处理队列
			Notebook->CompileCellAsync(InterpResult.Interpreter, CellIndex).Next([WeakThis](const FClingCellCompilationResult& CompileResult)
			{
				AsyncTask(ENamedThreads::GameThread, [WeakThis]()
				{
					if (!WeakThis.IsValid())
					{
						return;
					}
	
					UClingNotebook* Notebook = WeakThis.Get();
					Notebook->bIsProcessingQueue = false;
					Notebook->bIsCompiling = (Notebook->CompilingCells.Num() > 0);
					Notebook->ProcessNextInQueue();
				});
			});
		});
	});
}

FClingNotebookCellData* UClingNotebook::GetSelectedCellData()
{
	if (Cells.IsValidIndex(SelectedCellIndex))
	{
		return &Cells[SelectedCellIndex];
	}
	return nullptr;
}

bool UClingNotebook::IsCellReadOnly(int32 Index) const
{
	if (!Cells.IsValidIndex(Index)) return false;
	if (Cells[Index].CompilationState == EClingCellCompilationState::Compiling) return true;

	for (int32 i = Index; i < Cells.Num(); ++i)
	{
		if (Cells[i].CompilationState == EClingCellCompilationState::Completed) return true;
	}
	return false;
}

bool UClingNotebook::IsCellDeletable(int32 Index) const
{
	return !IsCellReadOnly(Index);
}

bool UClingNotebook::IsCellAddableBelow(int32 Index) const
{
	int32 NextIndex = Index + 1;
	if (NextIndex >= Cells.Num()) return true;
	return !IsCellReadOnly(NextIndex);
}

#if WITH_EDITOR
void UClingNotebook::OpenInIDE()
{
	FClingSourceFileOpener FileOpener;
	ClingNotebookFile::WriteNotebookFile(this, FileOpener.FilePath, &FileOpener.LineNumber);
}

void UClingNotebook::BackFromIDE()
{
	if (bIsCompiling)
	{
		FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Notebook is compiling. Please wait before reading from IDE."));
		return;
	}

	const FString FilePath = ClingNotebookFile::GetNotebookFilePath(this);
	if (!FPaths::FileExists(FilePath))
	{
		FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Notebook IDE file not found."));
		return;
	}

	TMap<int32, FString> ParsedSections;
	if (!ClingNotebookIDE::LoadSectionsFromFile(FilePath, ParsedSections))
	{
		FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("No notebook sections found in the IDE file."));
		return;
	}

	auto NormalizeContent = [](const FString& Text)
	{
		FString Normalized = Text;
		Normalized.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
		return Normalized;
	};

	TArray<int32> ChangedIndices;
	int32 FirstCompiledChanged = INDEX_NONE;

	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		const FString* Parsed = ParsedSections.Find(Index);
		if (!Parsed)
		{
			continue;
		}
		if (!NormalizeContent(*Parsed).Equals(NormalizeContent(Cells[Index].Content), ESearchCase::CaseSensitive))
		{
			ChangedIndices.Add(Index);
			if (Cells[Index].CompilationState == EClingCellCompilationState::Completed && FirstCompiledChanged == INDEX_NONE)
			{
				FirstCompiledChanged = Index;
			}
		}
	}

	if (ChangedIndices.Num() == 0)
	{
		return;
	}

	if (FirstCompiledChanged != INDEX_NONE)
	{
		const FText Message = FText::Format(INVTEXT("Cell #{0} was already compiled but has changed in the IDE file. Undo to this cell and apply changes?"), FText::AsNumber(FirstCompiledChanged));
		const EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::YesNo, Message);
		if (Response != EAppReturnType::Yes)
		{
			return;
		}
		UndoToHere(FirstCompiledChanged);
	}

	for (int32 Index : ChangedIndices)
	{
		const FString* Parsed = ParsedSections.Find(Index);
		if (Parsed)
		{
			Cells[Index].Content = *Parsed;
		}
	}

	MarkPackageDirty();
}

bool UClingNotebook::TryGetSectionContentFromFile(int32 SectionIndex, FString& OutContent) const
{
	const FString FilePath = ClingNotebookFile::GetNotebookFilePath(this);
	if (!FPaths::FileExists(FilePath))
	{
		return false;
	}

	TMap<int32, FString> ParsedSections;
	if (!ClingNotebookIDE::LoadSectionsFromFile(FilePath, ParsedSections))
	{
		return false;
	}

	const FString* Parsed = ParsedSections.Find(SectionIndex);
	if (!Parsed)
	{
		return false;
	}

	OutContent = *Parsed;
	return true;
}

bool UClingNotebook::TryGetSectionSymbolsFromFile(int32 SectionIndex, TArray<FClingNotebookSymbolDesc>& OutSymbols) const
{
	FString Content;
	if (!TryGetSectionContentFromFile(SectionIndex, Content))
	{
		return false;
	}

	return ClingNotebookSymbols::ExtractSymbolsFromContent(Content, OutSymbols);
}
#endif
