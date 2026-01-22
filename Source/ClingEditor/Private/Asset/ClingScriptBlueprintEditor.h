#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

class UClingScriptBlueprint;
class SCppMultiLineEditableTextBox;
class SDockTab;
class FSpawnTabArgs;

class FClingScriptBlueprintEditor : public FAssetEditorToolkit
{
public:
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& EditWithinLevelEditor, UClingScriptBlueprint* InBlueprint);

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;

	void ExtendToolbar();
	void FillToolbar(FToolBarBuilder& ToolbarBuilder);
	void CompileBlueprint();

private:
	TSharedRef<SDockTab> SpawnTab_Source(const FSpawnTabArgs& Args);
	void HandleSourceChanged(const FText& NewText);

	UClingScriptBlueprint* BlueprintAsset = nullptr;
	TSharedPtr<SCppMultiLineEditableTextBox> SourceEditor;
};
