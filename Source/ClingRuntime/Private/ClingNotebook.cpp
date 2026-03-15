#include "ClingNotebook.h"
#include "ClingRuntime.h"
#include "ClingSetting.h"
#include "ClingCoroUtils.h"
#include "CppInterOp/CppInterOp.h"
#include "Modules/ModuleManager.h"
#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "StructUtils/PropertyBag.h"

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
#endif

#if WITH_EDITOR
namespace ClingNotebookSymbols
{
	static FString GLastCppString;
	static void CppStringCallback(const char* Str) { GLastCppString = UTF8_TO_TCHAR(Str); }

	static TArray<CppImpl::TCppFunction_t> GLastCppFunctions;
	static void CppFunctionsCallback(const CppImpl::TCppFunction_t* Funcs, size_t Num)
	{
		UE_LOG(LogTemp, Log, TEXT("ClingNotebook: CppFunctionsCallback called with %llu functions"), (uint64)Num);
		GLastCppFunctions.Empty(Num);
		for (size_t i = 0; i < Num; ++i) GLastCppFunctions.Add(const_cast<CppImpl::TCppFunction_t>(Funcs[i]));
	}

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

	bool TryResolveClass_Legacy(const FString& TypeName, UClass*& OutClass)
	{
		FString CleanName = TypeName;
		if (CleanName.StartsWith(TEXT("::")))
		{
			CleanName.RightChopInline(2);
		}

		auto FindType = [](const FString& Name) -> UClass*
		{
			if (UClass* Found = UClass::TryFindTypeSlow<UClass>(Name))
			{
				return Found;
			}
			return LoadObject<UClass>(nullptr, *Name);
		};

		OutClass = FindType(CleanName);
		if (!OutClass && (CleanName.StartsWith(TEXT("U")) || CleanName.StartsWith(TEXT("A"))))
		{
			OutClass = FindType(CleanName.Mid(1));
		}

		CppImpl::CppInterpWrapper& Wrapper = FClingRuntimeModule::Get().GetInterp();
		if (!OutClass && Wrapper.IsValid())
		{
			if (CppImpl::TCppScope_t Scope = Wrapper.GetScope(TCHAR_TO_ANSI(*CleanName)))
			{
				Wrapper.GetFunctionsUsingName(Scope, "StaticClass", CppFunctionsCallback);
				TArray<CppImpl::TCppFunction_t> LocalFuncs = GLastCppFunctions;
				for (CppImpl::TCppFunction_t Func : LocalFuncs)
				{
					if (Wrapper.GetFunctionNumArgs(Func) == 0)
					{
						CppImpl::JitCall Call = Wrapper.MakeFunctionCallable(Func);
						if (Call.isValid())
						{
							UClass* Result = 0;
							Call.Invoke(&Result);
							if (Result)
							{
								OutClass = Result;
								break;
							}
						}
					}
				}
			}
		}

		if (!OutClass && CleanName == TEXT("UObject"))
		{
			OutClass = UObject::StaticClass();
		}
		return OutClass != nullptr;
	}

	bool TryResolveStruct_Legacy(const FString& TypeName, UScriptStruct*& OutStruct)
	{
		FString CleanName = TypeName;
		if (CleanName.StartsWith(TEXT("::")))
		{
			CleanName.RightChopInline(2);
		}

		auto FindType = [](const FString& Name) -> UScriptStruct*
		{
			if (UScriptStruct* Found = UClass::TryFindTypeSlow<UScriptStruct>(Name))
			{
				return Found;
			}
			return LoadObject<UScriptStruct>(nullptr, *Name);
		};

		OutStruct = FindType(CleanName);
		if (!OutStruct && CleanName.StartsWith(TEXT("F")))
		{
			OutStruct = FindType(CleanName.Mid(1));
		}
		
		CppImpl::CppInterpWrapper& Wrapper = FClingRuntimeModule::Get().GetInterp();
		if (!OutStruct && Wrapper.IsValid())
		{
			FString TBaseName = FString::Printf(TEXT("TBaseStructure<%s>"), *CleanName);
			if (Wrapper.GetScope(TCHAR_TO_ANSI(*TBaseName)))
			{
				Wrapper.GetFunctionsUsingName(Wrapper.GetScope(TCHAR_TO_ANSI(*TBaseName)), "Get", CppFunctionsCallback);
				TArray<CppImpl::TCppFunction_t> LocalFuncs = GLastCppFunctions;
				for (CppImpl::TCppFunction_t Func : LocalFuncs)
				{
					if (Wrapper.GetFunctionNumArgs(Func) == 0)
					{
						CppImpl::JitCall Call = Wrapper.MakeFunctionCallable(Func);
						if (Call.isValid())
						{
							void* Result = nullptr;
							Call.Invoke(&Result);
							if (Result)
							{
								OutStruct = (UScriptStruct*)Result;
								break;
							}
						}
					}
				}
			}

			if (!OutStruct)
			{
				if (Wrapper.GetScope(TCHAR_TO_ANSI(*CleanName)))
				{
					Wrapper.GetFunctionsUsingName(Wrapper.GetScope(TCHAR_TO_ANSI(*CleanName)), "StaticStruct", CppFunctionsCallback);
					TArray<CppImpl::TCppFunction_t> LocalFuncs = GLastCppFunctions;
					for (CppImpl::TCppFunction_t Func : LocalFuncs)
					{
						if (Wrapper.GetFunctionNumArgs(Func) == 0)
						{
							CppImpl::JitCall Call = Wrapper.MakeFunctionCallable(Func);
							if (Call.isValid())
							{
								void* Result = nullptr;
								Call.Invoke(&Result);
								if (Result)
								{
									OutStruct = (UScriptStruct*)Result;
									break;
								}
							}
						}
					}
				}
			}
		}

		return OutStruct != nullptr;
	}

	struct FTypeLookupRequest
	{
		TSet<FName> ClassNames;
		TSet<FName> StructNames;
		TSet<FName> EnumNames;
	};

	struct FTypeLookupResult
	{
		TMap<FName, UClass*> Classes;
		TMap<FName, UScriptStruct*> Structs;
		TMap<FName, UEnum*> Enums;
	};

	enum class EClingTypeCategory : uint8
	{
		Unknown,
		Class,
		Struct,
		Enum
	};

	static FName GetStructFName(const FString& TypeName)
	{
		FString Clean = TypeName.TrimStartAndEnd();
		if (Clean.StartsWith(TEXT("::")))
		{
			Clean.RightChopInline(2);
		}
		if (Clean.StartsWith(TEXT("F")))
		{
			Clean.RightChopInline(1);
		}
		return FName(*Clean);
	}

	static FName GetClassFName(const FString& TypeName)
	{
		FString Clean = TypeName.TrimStartAndEnd();
		if (Clean.StartsWith(TEXT("::")))
		{
			Clean.RightChopInline(2);
		}
		if (Clean.StartsWith(TEXT("U")) || Clean.StartsWith(TEXT("A")))
		{
			Clean.RightChopInline(1);
		}
		return FName(*Clean);
	}

	static FName GetEnumFName(const FString& TypeName)
	{
		FString Clean = TypeName.TrimStartAndEnd();
		if (Clean.StartsWith(TEXT("::")))
		{
			Clean.RightChopInline(2);
		}
		if (Clean.StartsWith(TEXT("E")))
		{
			Clean.RightChopInline(1);
		}
		return FName(*Clean);
	}

	static void ResolveAllTypes(const FTypeLookupRequest& Request, FTypeLookupResult& Result)
	{
		for (TObjectIterator<UStruct> It; It; ++It)
		{
			FName StructFName = It->GetFName();

			if (Request.ClassNames.Contains(StructFName))
			{
				if (UClass* Class = Cast<UClass>(*It))
				{
					Result.Classes.Add(StructFName, Class);
				}
			}

			if (Request.StructNames.Contains(StructFName))
			{
				if (UScriptStruct* Struct = Cast<UScriptStruct>(*It))
				{
					Result.Structs.Add(StructFName, Struct);
				}
			}
		}

		for (TObjectIterator<UEnum> It; It; ++It)
		{
			FName EnumFName = It->GetFName();
			if (Request.EnumNames.Contains(EnumFName))
			{
				Result.Enums.Add(EnumFName, *It);
			}
		}
	}

	static void CollectTypesFromString(const FString& TypeStr, FTypeLookupRequest& Request)
	{
		FString Working = TypeStr.TrimStartAndEnd();
		if (Working.IsEmpty()) return;

		while (Working.EndsWith(TEXT("*")) || Working.EndsWith(TEXT("&")))
		{
			Working.LeftChopInline(1);
			Working.TrimEndInline();
		}

		if (Working.StartsWith(TEXT("TArray<")) || Working.StartsWith(TEXT("TSet<")))
		{
			FString Inner;
			if (ExtractTemplateArg(Working, Inner))
			{
				CollectTypesFromString(Inner, Request);
			}
			return;
		}

		if (Working.StartsWith(TEXT("TSubclassOf<")) ||
			Working.StartsWith(TEXT("TSoftObjectPtr<")) ||
			Working.StartsWith(TEXT("TSoftClassPtr<")) ||
			Working.StartsWith(TEXT("TObjectPtr<")))
		{
			FString Inner;
			if (ExtractTemplateArg(Working, Inner))
			{
				Request.ClassNames.Add(GetClassFName(Inner));
			}
			return;
		}

		Working = StripQualifiers(Working).TrimStartAndEnd();

		static const TSet<FString> BuiltinTypes = {
			TEXT("void"), TEXT("bool"), TEXT("uint8"), TEXT("uint8_t"), TEXT("byte"),
			TEXT("int32"), TEXT("int"), TEXT("int64"), TEXT("uint32"), TEXT("uint64"),
			TEXT("float"), TEXT("double"), TEXT("FName"), TEXT("FString"), TEXT("FText"),
			TEXT("char"), TEXT("wchar_t"), TEXT("size_t"), TEXT("int8"), TEXT("int16"),
			TEXT("uint16"), TEXT("long"), TEXT("short")
		};

		FString LowerWorking = Working.ToLower();
		if (BuiltinTypes.Contains(LowerWorking)) return;

		if (Working == TEXT("UClass"))
		{
			return;
		}

		Request.ClassNames.Add(GetClassFName(Working));
		Request.StructNames.Add(GetStructFName(Working));
		Request.EnumNames.Add(GetEnumFName(Working));
	}

	bool TryResolveClass_Legacy2(const FString& TypeName, UClass*& OutClass)
	{
		FString CleanName = TypeName;
		if (CleanName.StartsWith(TEXT("::")))
		{
			CleanName.RightChopInline(2);
		}

		auto FindType = [](const FString& Name) -> UClass*
		{
			if (UClass* Found = UClass::TryFindTypeSlow<UClass>(Name))
			{
				return Found;
			}
			return LoadObject<UClass>(nullptr, *Name);
		};

		OutClass = FindType(CleanName);
		if (!OutClass && (CleanName.StartsWith(TEXT("U")) || CleanName.StartsWith(TEXT("A"))))
		{
			OutClass = FindType(CleanName.Mid(1));
		}

		CppImpl::CppInterpWrapper& Wrapper = FClingRuntimeModule::Get().GetInterp();
		if (!OutClass && Wrapper.IsValid())
		{
			if (CppImpl::TCppScope_t Scope = Wrapper.GetScope(TCHAR_TO_ANSI(*CleanName)))
			{
				Wrapper.GetFunctionsUsingName(Scope, "StaticClass", CppFunctionsCallback);
				TArray<CppImpl::TCppFunction_t> LocalFuncs = GLastCppFunctions;
				for (CppImpl::TCppFunction_t Func : LocalFuncs)
				{
					if (Wrapper.GetFunctionNumArgs(Func) == 0)
					{
						CppImpl::JitCall Call = Wrapper.MakeFunctionCallable(Func);
						if (Call.isValid())
						{
							UClass* Result = 0;
							Call.Invoke(&Result);
							if (Result)
							{
								OutClass = Result;
								break;
							}
						}
					}
				}
			}
		}

		if (!OutClass && CleanName == TEXT("UObject"))
		{
			OutClass = UObject::StaticClass();
		}
		return OutClass != nullptr;
	}

	bool TryResolveStruct_Legacy2(const FString& TypeName, UScriptStruct*& OutStruct)
	{
		FString CleanName = TypeName;
		if (CleanName.StartsWith(TEXT("::")))
		{
			CleanName.RightChopInline(2);
		}

		auto FindType = [](const FString& Name) -> UScriptStruct*
		{
			if (UScriptStruct* Found = UClass::TryFindTypeSlow<UScriptStruct>(Name))
			{
				return Found;
			}
			return LoadObject<UScriptStruct>(nullptr, *Name);
		};

		OutStruct = FindType(CleanName);
		if (!OutStruct && CleanName.StartsWith(TEXT("F")))
		{
			OutStruct = FindType(CleanName.Mid(1));
		}

		CppImpl::CppInterpWrapper& Wrapper = FClingRuntimeModule::Get().GetInterp();
		if (!OutStruct && Wrapper.IsValid())
		{
			FString TBaseName = FString::Printf(TEXT("TBaseStructure<%s>"), *CleanName);
			if (Wrapper.GetScope(TCHAR_TO_ANSI(*TBaseName)))
			{
				Wrapper.GetFunctionsUsingName(Wrapper.GetScope(TCHAR_TO_ANSI(*TBaseName)), "Get", CppFunctionsCallback);
				TArray<CppImpl::TCppFunction_t> LocalFuncs = GLastCppFunctions;
				for (CppImpl::TCppFunction_t Func : LocalFuncs)
				{
					if (Wrapper.GetFunctionNumArgs(Func) == 0)
					{
						CppImpl::JitCall Call = Wrapper.MakeFunctionCallable(Func);
						if (Call.isValid())
						{
							void* Result = nullptr;
							Call.Invoke(&Result);
							if (Result)
							{
								OutStruct = (UScriptStruct*)Result;
								break;
							}
						}
					}
				}
			}

			if (!OutStruct)
			{
				if (Wrapper.GetScope(TCHAR_TO_ANSI(*CleanName)))
				{
					Wrapper.GetFunctionsUsingName(Wrapper.GetScope(TCHAR_TO_ANSI(*CleanName)), "StaticStruct", CppFunctionsCallback);
					TArray<CppImpl::TCppFunction_t> LocalFuncs = GLastCppFunctions;
					for (CppImpl::TCppFunction_t Func : LocalFuncs)
					{
						if (Wrapper.GetFunctionNumArgs(Func) == 0)
						{
							CppImpl::JitCall Call = Wrapper.MakeFunctionCallable(Func);
							if (Call.isValid())
							{
								void* Result = nullptr;
								Call.Invoke(&Result);
								if (Result)
								{
									OutStruct = (UScriptStruct*)Result;
									break;
								}
							}
						}
					}
				}
			}
		}

		return OutStruct != nullptr;
	}

	bool TryResolveEnum_Legacy(const FString& TypeName, UEnum*& OutEnum)
	{
		auto FindType = [](const FString& Name) -> UEnum*
		{
			if (UEnum* Found = UClass::TryFindTypeSlow<UEnum>(Name))
			{
				return Found;
			}
			return LoadObject<UEnum>(nullptr, *Name);
		};

		OutEnum = FindType(TypeName);
		return OutEnum != nullptr;
	}

	static UClass* FindClassFromResult(const FString& TypeName, const FTypeLookupResult& Result)
	{
		if (TypeName.TrimStartAndEnd() == TEXT("UObject"))
		{
			return UObject::StaticClass();
		}
		FName Key = GetClassFName(TypeName);
		UClass* const* Found = Result.Classes.Find(Key);
		return Found ? *Found : nullptr;
	}

	static UScriptStruct* FindStructFromResult(const FString& TypeName, const FTypeLookupResult& Result)
	{
		FName Key = GetStructFName(TypeName);
		UScriptStruct* const* Found = Result.Structs.Find(Key);
		return Found ? *Found : nullptr;
	}

	static UEnum* FindEnumFromResult(const FString& TypeName, const FTypeLookupResult& Result)
	{
		FName Key = GetEnumFName(TypeName);
		UEnum* const* Found = Result.Enums.Find(Key);
		return Found ? *Found : nullptr;
	}

	static bool ParseTypeRecursiveWithLookup(const FString& InType, FClingNotebookTypeDesc& OutDesc, FPropertyBagContainerTypes& InOutContainers, const FTypeLookupResult& LookupResult);

	static bool ParseTypeWithLookup(const FString& Working, FClingNotebookTypeDesc& OutDesc, FPropertyBagContainerTypes& InOutContainers, const FTypeLookupResult& LookupResult)
	{
		static const TSet<FString> BuiltinTypes = {
			TEXT("void"), TEXT("bool"), TEXT("uint8"), TEXT("uint8_t"), TEXT("byte"),
			TEXT("int32"), TEXT("int"), TEXT("int64"), TEXT("uint32"), TEXT("uint64"),
			TEXT("float"), TEXT("double"), TEXT("FName"), TEXT("FString"), TEXT("FText"),
			TEXT("char"), TEXT("wchar_t"), TEXT("size_t"), TEXT("int8"), TEXT("int16"),
			TEXT("uint16"), TEXT("long"), TEXT("short")
		};

		FString LowerWorking = Working.ToLower();
		if (BuiltinTypes.Contains(LowerWorking))
		{
			if (Working.Equals(TEXT("void"), ESearchCase::IgnoreCase))
			{
				OutDesc.bIsVoid = true;
				OutDesc.bSupported = false;
				return true;
			}
			if (Working.Equals(TEXT("bool"), ESearchCase::IgnoreCase)) { OutDesc.ValueType = EPropertyBagPropertyType::Bool; }
			else if (Working.Equals(TEXT("uint8"), ESearchCase::IgnoreCase) || Working.Equals(TEXT("uint8_t"), ESearchCase::IgnoreCase) || Working.Equals(TEXT("byte"), ESearchCase::IgnoreCase)) { OutDesc.ValueType = EPropertyBagPropertyType::Byte; }
			else if (Working.Equals(TEXT("int32"), ESearchCase::IgnoreCase) || Working.Equals(TEXT("int"), ESearchCase::IgnoreCase)) { OutDesc.ValueType = EPropertyBagPropertyType::Int32; }
			else if (Working.Equals(TEXT("int64"), ESearchCase::IgnoreCase) || Working.Equals(TEXT("long long"), ESearchCase::IgnoreCase)) { OutDesc.ValueType = EPropertyBagPropertyType::Int64; }
			else if (Working.Equals(TEXT("uint32"), ESearchCase::IgnoreCase) || Working.Equals(TEXT("unsigned int"), ESearchCase::IgnoreCase)) { OutDesc.ValueType = EPropertyBagPropertyType::UInt32; }
			else if (Working.Equals(TEXT("uint64"), ESearchCase::IgnoreCase) || Working.Equals(TEXT("unsigned long long"), ESearchCase::IgnoreCase)) { OutDesc.ValueType = EPropertyBagPropertyType::UInt64; }
			else if (Working.Equals(TEXT("float"), ESearchCase::IgnoreCase)) { OutDesc.ValueType = EPropertyBagPropertyType::Float; }
			else if (Working.Equals(TEXT("double"), ESearchCase::IgnoreCase)) { OutDesc.ValueType = EPropertyBagPropertyType::Double; }
			else if (Working.Equals(TEXT("FName"), ESearchCase::IgnoreCase)) { OutDesc.ValueType = EPropertyBagPropertyType::Name; }
			else if (Working.Equals(TEXT("FString"), ESearchCase::IgnoreCase)) { OutDesc.ValueType = EPropertyBagPropertyType::String; }
			else if (Working.Equals(TEXT("FText"), ESearchCase::IgnoreCase)) { OutDesc.ValueType = EPropertyBagPropertyType::Text; }
			else { return false; }
			OutDesc.ContainerTypes = InOutContainers;
			OutDesc.bSupported = true;
			return true;
		}

		if (Working == TEXT("UClass"))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Class;
			OutDesc.ValueTypeObject = UObject::StaticClass();
			OutDesc.ContainerTypes = InOutContainers;
			OutDesc.bSupported = true;
			return true;
		}

		if (UScriptStruct* Struct = FindStructFromResult(Working, LookupResult))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Struct;
			OutDesc.ValueTypeObject = Struct;
			OutDesc.ContainerTypes = InOutContainers;
			OutDesc.bSupported = true;
			return true;
		}

		if (UEnum* Enum = FindEnumFromResult(Working, LookupResult))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Enum;
			OutDesc.ValueTypeObject = Enum;
			OutDesc.ContainerTypes = InOutContainers;
			OutDesc.bSupported = true;
			return true;
		}

		return false;
	}

	static bool ParseTypeRecursiveWithLookup(const FString& InType, FClingNotebookTypeDesc& OutDesc, FPropertyBagContainerTypes& InOutContainers, const FTypeLookupResult& LookupResult)
	{
		FString Working = StripQualifiers(InType).TrimStartAndEnd();
		Working.ReplaceInline(TEXT("\t"), TEXT(" "));
		Working.TrimStartAndEndInline();

		if (Working.IsEmpty()) return false;

		bool bPointer = Working.EndsWith(TEXT("*"));
		bool bReference = Working.EndsWith(TEXT("&"));
		while (Working.EndsWith(TEXT("*")) || Working.EndsWith(TEXT("&")))
		{
			Working.LeftChopInline(1);
			Working.TrimEndInline();
		}

		if (Working.StartsWith(TEXT("TArray<")))
		{
			if (!InOutContainers.Add(EPropertyBagContainerType::Array)) return false;
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner)) return false;
			return ParseTypeRecursiveWithLookup(Inner, OutDesc, InOutContainers, LookupResult);
		}

		if (Working.StartsWith(TEXT("TSet<")))
		{
			if (!InOutContainers.Add(EPropertyBagContainerType::Set)) return false;
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner)) return false;
			return ParseTypeRecursiveWithLookup(Inner, OutDesc, InOutContainers, LookupResult);
		}

		if (Working.StartsWith(TEXT("TSubclassOf<")))
		{
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner)) return false;
			if (UClass* Class = FindClassFromResult(Inner.TrimStartAndEnd(), LookupResult))
			{
				OutDesc.ValueType = EPropertyBagPropertyType::Class;
				OutDesc.ValueTypeObject = Class;
				OutDesc.ContainerTypes = InOutContainers;
				OutDesc.bSupported = true;
				return true;
			}
			return false;
		}

		if (Working.StartsWith(TEXT("TSoftObjectPtr<")))
		{
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner)) return false;
			if (UClass* Class = FindClassFromResult(Inner.TrimStartAndEnd(), LookupResult))
			{
				OutDesc.ValueType = EPropertyBagPropertyType::SoftObject;
				OutDesc.ValueTypeObject = Class;
				OutDesc.ContainerTypes = InOutContainers;
				OutDesc.bSupported = true;
				return true;
			}
			return false;
		}

		if (Working.StartsWith(TEXT("TSoftClassPtr<")))
		{
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner)) return false;
			if (UClass* Class = FindClassFromResult(Inner.TrimStartAndEnd(), LookupResult))
			{
				OutDesc.ValueType = EPropertyBagPropertyType::SoftClass;
				OutDesc.ValueTypeObject = Class;
				OutDesc.ContainerTypes = InOutContainers;
				OutDesc.bSupported = true;
				return true;
			}
			return false;
		}

		if (Working.StartsWith(TEXT("TObjectPtr<")))
		{
			FString Inner;
			if (!ExtractTemplateArg(Working, Inner)) return false;
			if (UClass* Class = FindClassFromResult(Inner.TrimStartAndEnd(), LookupResult))
			{
				OutDesc.ValueType = EPropertyBagPropertyType::Object;
				OutDesc.ValueTypeObject = Class;
				OutDesc.ContainerTypes = InOutContainers;
				OutDesc.bSupported = true;
				return true;
			}
			return false;
		}

		if (bPointer || bReference)
		{
			const FString BaseType = Working.TrimStartAndEnd();
			if (UClass* Class = FindClassFromResult(BaseType, LookupResult))
			{
				OutDesc.ValueType = EPropertyBagPropertyType::Object;
				OutDesc.ValueTypeObject = Class;
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
			return false;
		}

		return ParseTypeWithLookup(Working, OutDesc, InOutContainers, LookupResult);
	}

	static bool BuildTypeDescWithLookup(const FString& InType, FClingNotebookTypeDesc& OutDesc, const FTypeLookupResult& LookupResult)
	{
		OutDesc = FClingNotebookTypeDesc();
		OutDesc.OriginalType = InType.TrimStartAndEnd();
		FPropertyBagContainerTypes Containers;
		return ParseTypeRecursiveWithLookup(OutDesc.OriginalType, OutDesc, Containers, LookupResult);
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

	// Handle templates
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
		if (!TryResolveClass_Legacy(Inner.TrimStartAndEnd(), ResolvedClass))
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
		if (!TryResolveClass_Legacy(Inner.TrimStartAndEnd(), ResolvedClass))
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
		if (!TryResolveClass_Legacy(Inner.TrimStartAndEnd(), ResolvedClass))
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
		if (!TryResolveClass_Legacy(Inner.TrimStartAndEnd(), ResolvedClass))
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
		if (TryResolveClass_Legacy(BaseType, ResolvedClass))
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
		if (TryResolveStruct_Legacy(BaseType, Struct))
		{
			OutDesc.ValueType = EPropertyBagPropertyType::Struct;
			OutDesc.ValueTypeObject = Struct;
		}
		else
		{
			UEnum* Enum = nullptr;
			if (TryResolveEnum_Legacy(BaseType, Enum))
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

	bool ExtractFunctionSignatures(CppImpl::CppInterpWrapper& Interp, const FString& InContent, TArray<FClingFunctionSignature>& OutSignatures)
	{
		if (!Interp.IsValid()) return false;

		FScopeLock Lock(&FClingRuntimeModule::Get().GetCppInterOpLock());

		FString Clean = StripComments(InContent);
		FRegexPattern Pattern(TEXT("(?m)^\\s*(?:[\\w:<>&\\*\\s]+?)\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\("));
		FRegexMatcher Matcher(Pattern, Clean);

		TSet<FString> FoundNames;
		while (Matcher.FindNext())
		{
			FoundNames.Add(Matcher.GetCaptureGroup(1));
		}

		struct FArgInfo
		{
			FString TypeStr;
			FString NameStr;
		};
		struct FFuncInfo
		{
			FString Name;
			FString RetTypeStr;
			TArray<FArgInfo> Args;
			CppImpl::TCppFunction_t Func;
		};
		TArray<FFuncInfo> FuncInfos;

		FTypeLookupRequest TypeRequest;

		for (const FString& NameStr : FoundNames)
		{
			Interp.GetFunctionsUsingName(Interp.GetGlobalScope(), TCHAR_TO_ANSI(*NameStr), CppFunctionsCallback);
			TArray<CppImpl::TCppFunction_t> LocalFunctions = GLastCppFunctions;

			for (CppImpl::TCppFunction_t Func : LocalFunctions)
			{
				FFuncInfo Info;
				Info.Name = NameStr;
				Info.Func = Func;

				CppImpl::TCppType_t RetType = Interp.GetFunctionReturnType(Func);
				Interp.GetTypeAsString(RetType, CppStringCallback);
				Info.RetTypeStr = GLastCppString;
				CollectTypesFromString(Info.RetTypeStr, TypeRequest);

				size_t NumArgs = (size_t)Interp.GetFunctionNumArgs(Func);
				for (size_t i = 0; i < NumArgs; ++i)
				{
					CppImpl::TCppType_t ArgType = Interp.GetFunctionArgType(Func, (CppImpl::TCppIndex_t)i);
					Interp.GetTypeAsString(ArgType, CppStringCallback);

					FArgInfo Arg;
					Arg.TypeStr = GLastCppString;

					Interp.GetFunctionArgName(Func, (CppImpl::TCppIndex_t)i, CppStringCallback);
					Arg.NameStr = GLastCppString;
					if (Arg.NameStr.IsEmpty()) Arg.NameStr = FString::Printf(TEXT("Param%d"), (int32)i);

					CollectTypesFromString(Arg.TypeStr, TypeRequest);
					Info.Args.Add(MoveTemp(Arg));
				}

				FuncInfos.Add(MoveTemp(Info));
			}
		}

		FTypeLookupResult TypeResult;
		ResolveAllTypes(TypeRequest, TypeResult);

		for (const FFuncInfo& Info : FuncInfos)
		{
			FClingFunctionSignature Sig;
			Sig.Name = *Info.Name;

			BuildTypeDescWithLookup(Info.RetTypeStr, Sig.ReturnType, TypeResult);

			Sig.OriginalNumArgs = Info.Args.Num();
			TArray<FPropertyBagPropertyDesc> PropDescs;
			for (const FArgInfo& Arg : Info.Args)
			{
				FClingNotebookTypeDesc ArgTypeDesc;
				if (BuildTypeDescWithLookup(Arg.TypeStr, ArgTypeDesc, TypeResult) && ArgTypeDesc.bSupported)
				{
					PropDescs.Add(FPropertyBagPropertyDesc(*Arg.NameStr, ArgTypeDesc.ValueType, ArgTypeDesc.ValueTypeObject));
					PropDescs.Last().ContainerTypes = ArgTypeDesc.ContainerTypes;
				}
			}

			if (PropDescs.Num() > 0 || Info.Args.Num() == 0)
			{
				Sig.Parameters.AddProperties(PropDescs);
				OutSignatures.Add(MoveTemp(Sig));
			}
		}

		return OutSignatures.Num() > 0;
	}
}
#endif

void UClingNotebook::FinishDestroy()
{
	if (Interpreter.GetInterpreter())
	{
		int32 Index = FClingRuntimeModule::Get().FindInterpIndex(Interpreter);
		if (Index >= 0)
		{
			FClingRuntimeModule::Get().DeleteInterp(Index);
		}
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

CppImpl::CppInterpWrapper& UClingNotebook::GetInterpreter()
{
	if (!Interpreter.IsValid())
	{
		Interpreter = FClingRuntimeModule::Get().StartNewInterp(PCHProfile);
	}
	return Interpreter;
}

ClingCoro::TClingTask<FClingInterpreterResult> UClingNotebook::GetInterpreterAsync()
{
	UE_LOG(LogTemp, Log, TEXT("[GetInterpreterAsync] Entry, Interpreter=%p, GameThread=%d"), Interpreter.GetInterpreter(), (int32)IsInGameThread());
	// If interpreter already exists, return completed task immediately
	if (Interpreter.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("[GetInterpreterAsync] Returning existing interpreter immediately"));
		co_return FClingInterpreterResult(Interpreter);
	}

	// Start async initialization
	UE_LOG(LogTemp, Log, TEXT("[GetInterpreterAsync] Launching StartInterpreterAsync"));
	auto Task = StartInterpreterAsync();
	Task.Launch(TEXT("GetInterpreterAsync"));
	UE_LOG(LogTemp, Log, TEXT("[GetInterpreterAsync] Waiting for StartInterpreterAsync..."));
	auto Result = co_await Task;
	UE_LOG(LogTemp, Log, TEXT("[GetInterpreterAsync] StartInterpreterAsync completed, bSuccess=%d"), (int32)Result.bSuccess);
	co_return Result;
}

ClingCoro::TClingTask<FClingInterpreterResult> UClingNotebook::StartInterpreterAsync()
{
	bIsInitializingInterpreter = true;

	TWeakObjectPtr<UClingNotebook> WeakThis(this);
	FName ProfileName = PCHProfile;

	// Acquire interpreter from pool (async)
	CppImpl::CppInterpWrapper NewInterpreter = co_await FClingRuntimeModule::Get().GetPool().AcquireAsync(ProfileName);

	// Return to game thread to update notebook state
	co_await ClingCoro::MoveToGameThread();

	FClingInterpreterResult Result(NewInterpreter);

	if (!WeakThis.IsValid())
	{
		if (NewInterpreter.IsValid())
		{
			NewInterpreter.DeleteInterpreter();
		}
		co_return FClingInterpreterResult(CppImpl::CppInterpWrapper(), TEXT("Notebook destroyed during initialization"));
	}

	UClingNotebook* Notebook = WeakThis.Get();
	Notebook->Interpreter = NewInterpreter;
	Notebook->bIsInitializingInterpreter = false;
	if (NewInterpreter.IsValid())
	{
		Notebook->SemanticInfoProvider.Refresh(NewInterpreter);
		UE_LOG(LogTemp, Log, TEXT("[Notebook] Interpreter ready, semantic info refreshed."));
	}

	UE_LOG(LogTemp, Log, TEXT("[StartInterpreterAsync] About to co_return, Interpreter=%p"), NewInterpreter.GetInterpreter());
	co_return Result;
}

FClingCellCompilationResult UClingNotebook::ExecuteCellCompilation(CppImpl::CppInterpWrapper& Interp, const FString& Code)
{
	FScopeLock Lock(&FClingRuntimeModule::Get().GetCppInterOpLock());

	static FString StaticErrors;
	StaticErrors.Reset();

	Interp.BeginStdStreamCapture(CppImpl::CaptureStreamKind::kStdErr);
	int32 ResultCode = Interp.Process(TCHAR_TO_ANSI(*Code));
	Interp.EndStdStreamCapture([](const char* Result)
	{
		StaticErrors = UTF8_TO_TCHAR(Result);
	});

	FClingCellCompilationResult CompilationResult;
	CompilationResult.ErrorOutput = StaticErrors;
	CompilationResult.bSuccess = (ResultCode == 0);

	return CompilationResult;
}

ClingCoro::TClingTask<FClingCellCompilationResult> UClingNotebook::CompileCellAsync(CppImpl::CppInterpWrapper& Interp, int32 CellIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[CompileCellAsync] Cell %d: entry, Interp=%p, GameThread=%d"), CellIndex, Interp.GetInterpreter(), (int32)IsInGameThread());
	if (!Cells.IsValidIndex(CellIndex) || !Interp.IsValid())
	{
		FClingCellCompilationResult ErrorResult;
		ErrorResult.ErrorOutput = TEXT("Invalid cell index or interpreter");
		ErrorResult.bSuccess = false;
		co_return ErrorResult;
	}

	// Generate compilation file
	FString NotebookFilePath;
	if (!ClingNotebookFile::WriteNotebookCompileFile(this, CellIndex, NotebookFilePath))
	{
		FClingCellCompilationResult ErrorResult;
		ErrorResult.ErrorOutput = TEXT("Failed to generate compilation file");
		ErrorResult.bSuccess = false;
		co_return ErrorResult;
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
		co_return Result;
	}
	else
	{
		// Execute on background thread using Coroutine
		co_await ClingCoro::MoveToTask();

		FClingCellCompilationResult Result;
		if (WeakThis.IsValid())
		{
			Result = WeakThis->ExecuteCellCompilation(Interp, Code);
		}
		else
		{
			Result.ErrorOutput = TEXT("Notebook destroyed during compilation");
			Result.bSuccess = false;
		}

		// Jump back to game thread
		co_await ClingCoro::MoveToGameThread();

		if (WeakThis.IsValid())
		{
			WeakThis->OnCellCompilationComplete(CellIndex, Result);
		}
		co_return Result;
	}
}

#if WITH_EDITOR
void UClingNotebook::UpdateCellSignatures(int32 CellIndex)
{
	if (!Cells.IsValidIndex(CellIndex)) return;
	FClingNotebookCellData& CellData = Cells[CellIndex];

	TArray<FClingFunctionSignature> NewSignatures;
	if (ClingNotebookSymbols::ExtractFunctionSignatures(GetInterpreter(), CellData.Content, NewSignatures))
	{
		for (FClingFunctionSignature& NewSig : NewSignatures)
		{
			NewSig.OnExecute.BindUObject(this, &UClingNotebook::ExecuteFunction);
			for (const FClingFunctionSignature& OldSig : CellData.SavedSignatures.Signatures)
			{
				if (OldSig.Name == NewSig.Name)
				{
					NewSig.Parameters = OldSig.Parameters;
					break;
				}
			}
		}
		CellData.SavedSignatures.Signatures = MoveTemp(NewSignatures);
	}
	else
	{
		CellData.SavedSignatures.Signatures.Empty();
	}
}

void UClingNotebook::ExecuteFunction(const FClingFunctionSignature& Signature)
{
	CppImpl::CppInterpWrapper& Interp = GetInterpreter();
	if (!Interp.GetInterpreter())
	{
		UE_LOG(LogTemp, Error, TEXT("ClingNotebook: Cannot execute function, interpreter is null"));
		return;
	}

	FScopeLock Lock(&FClingRuntimeModule::Get().GetCppInterOpLock());

	FString NameStr = Signature.Name.ToString();
	UE_LOG(LogTemp, Log, TEXT("ClingNotebook: Executing function %s"), *NameStr);

	Interp.GetFunctionsUsingName(Interp.GetGlobalScope(), TCHAR_TO_ANSI(*NameStr), ClingNotebookSymbols::CppFunctionsCallback);
	TArray<CppImpl::TCppFunction_t> LocalFunctions = ClingNotebookSymbols::GLastCppFunctions;

	const UPropertyBag* BagStruct = Signature.Parameters.GetPropertyBagStruct();
	int32 ExpectedArgs = Signature.OriginalNumArgs;
	// Fallback if OriginalNumArgs is not set
	if (ExpectedArgs == 0 && BagStruct && BagStruct->GetPropertyDescs().Num() > 0)
	{
		ExpectedArgs = BagStruct->GetPropertyDescs().Num();
	}

	UE_LOG(LogTemp, Log, TEXT("ClingNotebook: Found %d candidates for %s, expecting %d args"), LocalFunctions.Num(), *NameStr, ExpectedArgs);

	bool bExecuted = false;
	for (CppImpl::TCppFunction_t Func : LocalFunctions)
	{
		int32 FuncNumArgs = (int32)Interp.GetFunctionNumArgs(Func);
		if (FuncNumArgs == ExpectedArgs)
		{
			UE_LOG(LogTemp, Log, TEXT("ClingNotebook: Found candidate with matching args (%d), creating JitCall"), FuncNumArgs);
			CppImpl::JitCall Call = Interp.MakeFunctionCallable(Func);
			if (Call.isValid())
			{
				UE_LOG(LogTemp, Log, TEXT("ClingNotebook: JitCall is valid, invoking..."));
				TArray<void*> ArgPointers;
				if (BagStruct)
				{
					const uint8* BagMem = Signature.Parameters.GetValue().GetMemory();
					for (const FPropertyBagPropertyDesc& Desc : BagStruct->GetPropertyDescs())
					{
						if (const FProperty* Prop = Desc.CachedProperty)
						{
							ArgPointers.Add((void*)Prop->ContainerPtrToValuePtr<void>(BagMem));
						}
					}
				}

				CppImpl::JitCall::ArgList Args(ArgPointers.GetData(), ArgPointers.Num());
				Call.Invoke(Args);
				bExecuted = true;
				break;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ClingNotebook: JitCall is invalid for candidate function %p"), Func);
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("ClingNotebook: Candidate function %p has %d args, mismatch"), Func, FuncNumArgs);
		}
	}

	if (!bExecuted)
	{
		UE_LOG(LogTemp, Error, TEXT("ClingNotebook: Failed to execute function %s - no matching valid function found"), *NameStr);
	}
}
#endif

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
#if WITH_EDITOR
		UpdateCellSignatures(CellIndex);
#endif
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
	if (Interpreter.IsValid())
	{
		Interpreter.DeleteInterpreter();
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
		CppImpl::CppInterpWrapper& Interp = GetInterpreter();
		if (Interp.GetInterpreter())
		{
			Interp.Undo(UndoCount);
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

// ---------------------------------------------------------------------------
// Static coroutine to process one cell — avoids MSVC lambda-coroutine
// capture-by-pointer bug (lambda closure lives on the caller's stack and
// becomes a dangling pointer once ProcessNextInQueue returns).
// ---------------------------------------------------------------------------
ClingCoro::TClingTask<void> UClingNotebook::ProcessCellCoro(
	TWeakObjectPtr<UClingNotebook> WeakThis,
	int32 CellIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[ProcessCoro] Cell %d: started on GameThread=%d"), CellIndex, (int32)IsInGameThread());
	FClingInterpreterResult InterpResult = co_await WeakThis->GetInterpreterAsync();
	UE_LOG(LogTemp, Log, TEXT("[ProcessCoro] Cell %d: GetInterpreterAsync returned, bSuccess=%d, GameThread=%d"), CellIndex, (int32)InterpResult.bSuccess, (int32)IsInGameThread());

	// Always continue on GameThread: CompileCellAsync accesses UClingNotebook members
	// (Cells, GetName, etc.). When the interpreter was already set, GetInterpreterAsync
	// completes synchronously inside the symmetric-transfer chain on a background thread,
	// so we must explicitly move to GameThread here.
	co_await ClingCoro::MoveToGameThread();
	UE_LOG(LogTemp, Log, TEXT("[ProcessCoro] Cell %d: now on GameThread=%d"), CellIndex, (int32)IsInGameThread());

	if (!WeakThis.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProcessCoro] Cell %d: WeakThis INVALID, notebook was GC'd — aborting."), CellIndex);
		co_return;
	}

	UClingNotebook* Notebook = WeakThis.Get();
	if (!InterpResult.bSuccess || !InterpResult.Interpreter.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Notebook] Cell %d: failed to get interpreter: %s"), CellIndex, *InterpResult.ErrorMessage);
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
		co_return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Notebook] Cell %d: starting compilation (GameThread=%d)"), CellIndex, (int32)IsInGameThread());
	co_await Notebook->CompileCellAsync(InterpResult.Interpreter, CellIndex);

	if (!WeakThis.IsValid()) co_return;
	UE_LOG(LogTemp, Log, TEXT("[Notebook] Cell %d: compilation complete"), CellIndex);
	Notebook->bIsProcessingQueue = false;
	Notebook->bIsCompiling = (Notebook->CompilingCells.Num() > 0);
	Notebook->ProcessNextInQueue();
	co_return;
}

void UClingNotebook::ProcessNextInQueue()
{
	if (bIsProcessingQueue || ExecutionQueue.Num() == 0)
	{
		if (!bIsProcessingQueue)
		{
			bIsCompiling = (CompilingCells.Num() > 0);

			// Notebook is idle: queue empty and no active compilation.
			// Safe to refill the interpreter pool now — pool workers hold CppInterOpLock
			// for ~30s, so we must never trigger this while cells are compiling.
			if (!bIsCompiling)
			{
				FClingRuntimeModule::Get().GetPool().Refill();
			}
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

	// Use a static coroutine function (not a lambda) to avoid MSVC's
	// lambda-coroutine capture bug where the lambda closure (on the
	// caller's stack) becomes a dangling pointer after ProcessNextInQueue returns.
	UE_LOG(LogTemp, Log, TEXT("[Notebook] Processing cell %d from queue."), CellIndex);
	ProcessCellCoro(WeakThis, CellIndex).Launch(TEXT("ProcessNextInQueue"));
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
