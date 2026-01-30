#include "ClingNotebook.h"
#include "ClingRuntime.h"
#include "ClingSetting.h"
#include "CppInterOp/CppInterOp.h"
#include "Modules/ModuleManager.h"
#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "ISourceCodeAccessModule.h"
#include "ISourceCodeAccessor.h"
#include "Misc/MessageDialog.h"
#endif

namespace ClingNotebookFile
{
	FString NormalizeIncludePath(FString Path)
	{
		Path.ReplaceCharInline(TEXT('\\'), TEXT('/'));
		return Path;
	}

	FString GetNotebookDir()
	{
		const FString PCHPath = UClingSetting::GetPCHSourceFilePath();
		const FString PluginDir = FPaths::GetPath(PCHPath);
		return PluginDir / TEXT("Source/ClingScript/Private/Notebook");
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

		FString PCHIncludePath = NormalizeIncludePath(UClingSetting::GetPCHSourceFilePath());
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

void UClingNotebook::FinishDestroy()
{
	if (Interpreter)
	{
		FClingRuntimeModule::Get().DeleteInterp(Interpreter);
		Interpreter = nullptr;
	}
	Super::FinishDestroy();
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
		Interpreter = FClingRuntimeModule::Get().StartNewInterp();
	}
	return Interpreter;
}

void UClingNotebook::RestartInterpreter()
{
	if (Interpreter)
	{
		FClingRuntimeModule::Get().DeleteInterp(Interpreter);
	}
	Interpreter = FClingRuntimeModule::Get().StartNewInterp();

	// Reset status for all cells
	for (auto& Cell : Cells)
	{
		Cell.bIsCompleted = false;
		Cell.bHasOutput = false;
		Cell.Output.Empty();
	}
	MarkPackageDirty();
}

void UClingNotebook::AddNewCell(int32 InIndex)
{
	FClingNotebookCellData NewCellData;
	NewCellData.bIsExpanded = true;
	NewCellData.bIsCompleted = false;

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
	Cells[InIndex].bIsCompiling = true;
	CompilingCells.Add(InIndex);
	bIsCompiling = true;

	ProcessNextInQueue();
}

void UClingNotebook::RunToHere(int32 InIndex)
{
	if (!Cells.IsValidIndex(InIndex)) return;

	for (int32 i = 0; i <= InIndex; ++i)
	{
		if (!ExecutionQueue.Contains(i) && !CompilingCells.Contains(i) && !Cells[i].bIsCompleted)
		{
			ExecutionQueue.Add(i);
			Cells[i].bIsCompiling = true;
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
		if (Cells[i].bIsCompleted)
		{
			UndoCount++;
		}
		if (Cells[i].bIsCompleted || Cells[i].bHasOutput)
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
			Cells[i].bIsCompleted = false;
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
	int32 InIndex = ExecutionQueue[0];
	ExecutionQueue.RemoveAt(0);

	if (!Cells.IsValidIndex(InIndex))
	{
		bIsProcessingQueue = false;
		ProcessNextInQueue();
		return;
	}

	void* Interp = GetInterpreter();
	if (!Interp)
	{
		Cells[InIndex].bIsCompiling = false;
		CompilingCells.Remove(InIndex);
		bIsProcessingQueue = false;
		ProcessNextInQueue();
		return;
	}

	FClingNotebookCellData& Cell = Cells[InIndex];
	bool bRunInGameThread = Cell.bExecuteInGameThread;

	TWeakObjectPtr<UClingNotebook> WeakThis(this);

	auto CompletionHandler = [WeakThis, InIndex](int32 CompilationResult, FString OutErrors)
	{
		if (!WeakThis.IsValid()) return;
		UClingNotebook* Notebook = WeakThis.Get();

		if (Notebook->Cells.IsValidIndex(InIndex))
		{
			FClingNotebookCellData& CellDataAt = Notebook->Cells[InIndex];
			CellDataAt.Output = OutErrors;
			CellDataAt.bHasOutput = true;
			CellDataAt.bIsCompiling = false;

			if (CompilationResult == 0)
			{
				CellDataAt.bIsCompleted = true;
				if (CellDataAt.Output.IsEmpty())
				{
					CellDataAt.Output = FString::Printf(TEXT("Success (Executed at %s)"), *FDateTime::Now().ToString());
				}
			}
			else
			{
				CellDataAt.bIsCompleted = false;
			}
			Notebook->MarkPackageDirty();
		}
		Notebook->CompilingCells.Remove(InIndex);
		Notebook->bIsProcessingQueue = false;
		Notebook->bIsCompiling = (Notebook->CompilingCells.Num() > 0);
		Notebook->ProcessNextInQueue();
	};

	FString NotebookFilePath;
	if (!ClingNotebookFile::WriteNotebookFile(this, NotebookFilePath, nullptr))
	{
		CompletionHandler(-1, TEXT("Failed to generate notebook file for compilation."));
		return;
	}

	const FString IncludePath = ClingNotebookFile::NormalizeIncludePath(NotebookFilePath);
	const FString Code = FString::Printf(TEXT("#undef COMPILE\n#define COMPILE %d\n#include \"%s\"\n"), InIndex, *IncludePath);

	if (bRunInGameThread)
	{
		int32 CompilationResult = -1;
		FString OutErrors;
		{
			FScopeLock Lock(&FClingRuntimeModule::Get().GetCppInterOpLock());
			void* StoreInterp = Cpp::GetInterpreter();
			Cpp::ActivateInterpreter(Interp);

			static FString StaticErrors;
			StaticErrors.Reset();

			Cpp::BeginStdStreamCapture(Cpp::kStdErr);
			CompilationResult = Cpp::Process(TCHAR_TO_ANSI(*Code));
			Cpp::EndStdStreamCapture([](const char* Result)
			{
				StaticErrors = UTF8_TO_TCHAR(Result);
			});

			OutErrors = StaticErrors;
			Cpp::ActivateInterpreter(StoreInterp);
		}
		CompletionHandler(CompilationResult, OutErrors);
	}
	else
	{
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, Interp, Code, InIndex, CompletionHandler]()
		{
			int32 CompilationResult = -1;
			FString OutErrors;

			{
				FScopeLock Lock(&FClingRuntimeModule::Get().GetCppInterOpLock());
				void* StoreInterp = Cpp::GetInterpreter();
				Cpp::ActivateInterpreter(Interp);

				static FString StaticErrors;
				StaticErrors.Reset();

				Cpp::BeginStdStreamCapture(Cpp::kStdErr);
				CompilationResult = Cpp::Process(TCHAR_TO_ANSI(*Code));
				Cpp::EndStdStreamCapture([](const char* Result)
				{
					StaticErrors = UTF8_TO_TCHAR(Result);
				});

				OutErrors = StaticErrors;
				Cpp::ActivateInterpreter(StoreInterp);
			}

			AsyncTask(ENamedThreads::GameThread, [CompletionHandler, CompilationResult, OutErrors]()
			{
				CompletionHandler(CompilationResult, OutErrors);
			});
		});
	}
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
	if (Cells[Index].bIsCompiling) return true;

	for (int32 i = Index; i < Cells.Num(); ++i)
	{
		if (Cells[i].bIsCompleted) return true;
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
	struct FEnsureOpenInIDE
	{
		~FEnsureOpenInIDE()
		{
			if (FilePath.IsEmpty())
			{
				return;
			}

			ISourceCodeAccessModule& SourceCodeAccessModule = FModuleManager::LoadModuleChecked<ISourceCodeAccessModule>("SourceCodeAccess");
			ISourceCodeAccessor& SourceCodeAccessor = SourceCodeAccessModule.GetAccessor();
			if (FPaths::FileExists(FilePath))
			{
				SourceCodeAccessor.OpenFileAtLine(FilePath, Line);
			}
			else
			{
				SourceCodeAccessModule.OnOpenFileFailed().Broadcast(FilePath);
			}
		}

		FString FilePath;
		int32 Line = 1;
	};

	FEnsureOpenInIDE EnsureOpenInIDE;

	ClingNotebookFile::WriteNotebookFile(this, EnsureOpenInIDE.FilePath, &EnsureOpenInIDE.Line);
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
			if (Cells[Index].bIsCompleted && FirstCompiledChanged == INDEX_NONE)
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
#endif
