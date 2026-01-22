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

	void* GetInterpreter();
	void RestartInterpreter();

private:
	void* Interpreter = nullptr;
};
