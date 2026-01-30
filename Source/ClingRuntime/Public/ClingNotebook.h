#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ClingNotebook.generated.h"

/**
 * Notebook cell data for persistent storage
 */
USTRUCT(BlueprintType)
struct FClingNotebookCellData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Cling")
	FString Content;

	UPROPERTY(EditAnywhere, Category = "Cling")
	FString Output;

	UPROPERTY(EditAnywhere, Category = "Cling")
	bool bHasOutput = false;

	UPROPERTY(EditAnywhere, Category = "Cling")
	bool bIsExpanded = true;

	UPROPERTY(EditAnywhere, Category = "Cling")
	bool bExecuteInGameThread = false;

	UPROPERTY(Transient)
	bool bIsCompleted = false;

	UPROPERTY(Transient)
	bool bIsCompiling = false;
};

/**
 * Cling Notebook Asset
 */
UCLASS(BlueprintType)
class CLINGRUNTIME_API UClingNotebook : public UObject
{
	GENERATED_BODY()

public:
	virtual void FinishDestroy() override;

	UPROPERTY(EditAnywhere, Category = "Cling")
	TArray<FClingNotebookCellData> Cells;

	UPROPERTY(Transient)
	int32 SelectedCellIndex = -1;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCellSelectionChanged, int32);
	FOnCellSelectionChanged OnCellSelectionChanged;

	void SetSelectedCell(int32 Index);

	// Compilation state
	bool bIsCompiling = false;
	TSet<int32> CompilingCells;
	TArray<int32> ExecutionQueue;
	bool bIsProcessingQueue = false;

	void* GetInterpreter();
	void RestartInterpreter();

	// Cell Management
	void AddNewCell(int32 InIndex = -1);
	void DeleteCell(int32 InIndex);

	// Execution Logic
	void RunCell(int32 InIndex);
	void RunToHere(int32 InIndex);
	void UndoToHere(int32 InIndex);
	void ProcessNextInQueue();

	// State Queries
	FClingNotebookCellData* GetSelectedCellData();
	bool IsCellReadOnly(int32 Index) const;
	bool IsCellDeletable(int32 Index) const;
	bool IsCellAddableBelow(int32 Index) const;

#if WITH_EDITOR
	void OpenInIDE();
	void BackFromIDE();
	bool TryGetSectionContentFromFile(int32 SectionIndex, FString& OutContent) const;
#endif

private:
	void* Interpreter = nullptr;
};
