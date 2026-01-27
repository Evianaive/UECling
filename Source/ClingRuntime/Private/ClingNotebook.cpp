#include "ClingNotebook.h"
#include "ClingRuntime.h"
#include "CppInterOp/CppInterOp.h"
#include "Modules/ModuleManager.h"
#include "Async/Async.h"

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
	FString Code = Cell.Content;
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
