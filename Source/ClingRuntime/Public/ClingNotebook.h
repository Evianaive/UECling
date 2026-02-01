#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#if WITH_EDITORONLY_DATA
#include "StructUtils/PropertyBag.h"
#endif

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

#if WITH_EDITORONLY_DATA
UENUM()
enum class EClingNotebookSymbolKind : uint8
{
	Function,
	Variable
};

USTRUCT()
struct FClingNotebookTypeDesc
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category="Cling")
	FString OriginalType;

	UPROPERTY(VisibleAnywhere, Category="Cling")
	EPropertyBagPropertyType ValueType = EPropertyBagPropertyType::None;

	UPROPERTY(VisibleAnywhere, Category="Cling")
	FPropertyBagContainerTypes ContainerTypes;

	UPROPERTY(VisibleAnywhere, Category="Cling")
	TObjectPtr<const UObject> ValueTypeObject = nullptr;

	UPROPERTY(VisibleAnywhere, Category="Cling")
	bool bSupported = false;

	UPROPERTY(VisibleAnywhere, Category="Cling")
	bool bIsVoid = false;
};

USTRUCT()
struct FClingNotebookParamDesc
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category="Cling")
	FName Name;

	UPROPERTY(VisibleAnywhere, Category="Cling")
	FClingNotebookTypeDesc Type;
};

USTRUCT()
struct FClingNotebookSymbolDesc
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category="Cling")
	EClingNotebookSymbolKind Kind = EClingNotebookSymbolKind::Function;

	UPROPERTY(VisibleAnywhere, Category="Cling")
	FName Name;

	UPROPERTY(VisibleAnywhere, Category="Cling")
	FClingNotebookTypeDesc ReturnType;

	UPROPERTY(VisibleAnywhere, Category="Cling")
	TArray<FClingNotebookParamDesc> Params;

	UPROPERTY(VisibleAnywhere, Category="Cling")
	FClingNotebookTypeDesc ValueType;
};
#endif

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
	bool bIsInitializingInterpreter = false;

	void* GetInterpreter();
	void EnsureInterpreterAsync(TFunction<void(void*)> OnReady);
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
	bool TryGetSectionSymbolsFromFile(int32 SectionIndex, TArray<FClingNotebookSymbolDesc>& OutSymbols) const;
#endif

private:
	void* Interpreter = nullptr;
};
