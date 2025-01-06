// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"
#include "UObject/Object.h"
#include "K2Node_ExecuteCppScript.generated.h"

// UENUM()
// enum ECppScriptNodeState
// {
// 	CodeEditingInIDE,
// 	CodeEditingInline	
// };

USTRUCT()
struct FCppScriptCompiledResult
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, Transient, Category="Preview")
	FGuid FunctionGuid;
	UPROPERTY(VisibleAnywhere, Transient, Category="Preview")
	bool bGuidCompiled;
	UPROPERTY(VisibleAnywhere, Transient, Category="Preview")
	int64 FunctionPtrAddress{0};
	UPROPERTY(VisibleAnywhere, Transient, Category="Preview")
	FString CodePreview;
};
/**
 * 
 */
UCLASS()
class CLINGKISMET_API UK2Node_ExecuteCppScript : public UK2Node_CallFunction
{
	GENERATED_BODY()
	UK2Node_ExecuteCppScript();
	
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
	virtual void PreDuplicate(FObjectDuplicationParameters& DupParams) override;
	virtual void PreloadRequiredAssets() override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface.
	virtual void AllocateDefaultPins() override;
	virtual void PostReconstructNode() override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PinTypeChanged(UEdGraphPin* Pin) override;
	virtual FText GetPinDisplayName(const UEdGraphPin* Pin) const override;
	//~ End UEdGraphNode Interface.

	//~ Begin UK2Node Interface.
	virtual void EarlyValidation(class FCompilerResultsLog& MessageLog) const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph) const override;
	virtual bool IsActionFilteredOut(const class FBlueprintActionFilter& Filter) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual int32 GetNodeRefreshPriority() const override { return EBaseNodeRefreshPriority::Low_UsesDependentWildcard; }
	virtual FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const override;
	//~ End UK2Node Interface.

private:
	/** Synchronize the type of the given argument pin with the type its connected to, or reset it to a wildcard pin if there's no connection */
	void SynchronizeArgumentPinType(UEdGraphPin* Pin);
	void SynchronizeArgumentPinTypeImpl(UEdGraphPin* Pin);

	/** Try and find a named argument pin - this is the same as FindPin except it searches the array from the end as that's where the argument pins will be */
	UEdGraphPin* FindArgumentPin(const FName PinName, EEdGraphPinDirection PinDirection = EGPD_MAX);
	UEdGraphPin* FindArgumentPinChecked(const FName PinName, EEdGraphPinDirection PinDirection = EGPD_MAX);

private:
	UEdGraphPin* FunctionPtrAddressPin{nullptr};
public:
	FString GenerateCode(bool UpdateGuid = false);
	FString GetLambdaName() const;
	FString GetTempFileName() const;
	FString GetFileFolderPath() const;
	FString GetTempFilePath() const;
	bool HasTempFile(bool bCheckFile = false) const;
	bool IsOpenedInIDE() const;
	bool IsCurrentGuidCompiled() const;

	// UFUNCTION(CallInEditor)
	void OpenInIDE();
	// UFUNCTION(CallInEditor)
	void BackFromIDE();
	// Todo add function to get enum selection
	UPROPERTY(EditAnywhere, Category="Function")
	FName FunctionName;
	UPROPERTY(EditAnywhere, Category="Code", Transient)
	bool bEditInIDE{false};
	UPROPERTY(EditAnywhere, Category="Code", meta=(MultiLine, EditCondition="!bFileOpenedInIDE"))
	FString Includes;
	UPROPERTY(EditAnywhere, Category="Code", meta=(MultiLine, EditCondition="!bFileOpenedInIDE"))
	FString Snippet;
	/** User-defined input pins */
	UPROPERTY(EditAnywhere, Category="Arguments")
	TArray<FName> Inputs;
	/** User-defined output pins */
	UPROPERTY(EditAnywhere, Category="Arguments")
	TArray<FName> Outputs;
	UPROPERTY(VisibleAnywhere, Category="Preview")
	FGuid FunctionGuid;
	UPROPERTY(VisibleAnywhere)
	bool bFileOpenedInIDE;
	UPROPERTY(EditAnywhere, Category="Preview")
	FCppScriptCompiledResult Result;
	UPROPERTY()
	int64 ResultPtr;
	void CreateTempFunctionPtrPin();
	UEdGraphPin* GetFunctionPtrPin() const;
};
