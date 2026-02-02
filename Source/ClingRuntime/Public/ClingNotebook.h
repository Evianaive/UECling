#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Async/Future.h"
#if WITH_EDITORONLY_DATA
#include "StructUtils/PropertyBag.h"
#endif

#include "ClingNotebook.generated.h"

/**
 * Result of interpreter initialization
 */
struct FClingInterpreterResult
{
	void* Interpreter = nullptr;
	bool bSuccess = false;
	FString ErrorMessage;

	FClingInterpreterResult() = default;
	explicit FClingInterpreterResult(void* InInterpreter)
		: Interpreter(InInterpreter), bSuccess(InInterpreter != nullptr) {}
	FClingInterpreterResult(void* InInterpreter, const FString& InError)
		: Interpreter(InInterpreter), bSuccess(InInterpreter != nullptr), ErrorMessage(InError) {}
};

/**
 * Compilation state of a cell
 */
UENUM(BlueprintType)
enum class EClingCellCompilationState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Compiling UMETA(DisplayName = "Compiling"),
	Completed UMETA(DisplayName = "Completed")
};

/**
 * Result of cell compilation
 */
USTRUCT(BlueprintType)
struct FClingCellCompilationResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Cling")
	FString ErrorOutput;

	UPROPERTY(VisibleAnywhere, Category = "Cling")
	bool bSuccess = false;
};

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
	EClingCellCompilationState CompilationState = EClingCellCompilationState::Idle;

	UPROPERTY(Transient)
	FClingCellCompilationResult LastCompilationResult;
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

	/** PCH Profile to use for this notebook's interpreter */
	UPROPERTY(EditAnywhere, Category = "Cling")
	FName PCHProfile{ TEXT("Default") };

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCellSelectionChanged, int32);
	FOnCellSelectionChanged OnCellSelectionChanged;

	void SetSelectedCell(int32 Index);

	// Compilation state
	bool bIsCompiling = false;
	bool bIsInitializingInterpreter = false;  // Exposed for UI
	TSet<int32> CompilingCells;
	TArray<int32> ExecutionQueue;
	bool bIsProcessingQueue = false;

	// Interpreter management
	void* GetInterpreter();
	TFuture<FClingInterpreterResult> GetInterpreterAsync();
	void RestartInterpreter();

private:
	// Async operations using TPromise/TFuture
	TFuture<FClingInterpreterResult> StartInterpreterAsync();
	TFuture<FClingCellCompilationResult> CompileCellAsync(void* Interp, int32 CellIndex);
	void OnCellCompilationComplete(int32 CellIndex, const FClingCellCompilationResult& Result);

	// Compilation execution (extracted common logic)
	FClingCellCompilationResult ExecuteCellCompilation(void* Interp, const FString& Code);

	// Shared promise for interpreter initialization (allows multiple waiters)
	TSharedPtr<TPromise<FClingInterpreterResult>> InterpreterPromise;

public:
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
