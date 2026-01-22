#include "Asset/ClingScriptBlueprintEditor.h"

#include "ClingScriptBlueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "SlateWidgets/CppMultiLineEditableTextBox.h"
#include "Widgets/Docking/SDockTab.h"

const FName ClingScriptBlueprintSourceTabId(TEXT("ClingScriptBlueprint_Source"));

void FClingScriptBlueprintEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& EditWithinLevelEditor, UClingScriptBlueprint* InBlueprint)
{
	BlueprintAsset = InBlueprint;

	TSharedRef<FTabManager::FLayout> StandaloneLayout = FTabManager::NewLayout("Standalone_ClingScriptBlueprint_Layout")
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->AddTab(ClingScriptBlueprintSourceTabId, ETabState::OpenedTab)
		)
	);

	FAssetEditorToolkit::InitAssetEditor(Mode, EditWithinLevelEditor, TEXT("ClingScriptBlueprintEditor"), StandaloneLayout, true, true, InBlueprint);

	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

void FClingScriptBlueprintEditor::ExtendToolbar()
{
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FClingScriptBlueprintEditor::FillToolbar)
	);
	AddToolbarExtender(ToolbarExtender);
}

void FClingScriptBlueprintEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection("Compile");
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FClingScriptBlueprintEditor::CompileBlueprint)),
			NAME_None,
			INVTEXT("Compile"),
			INVTEXT("Compile the Cling script"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Recompile")
		);
	}
	ToolbarBuilder.EndSection();
}

void FClingScriptBlueprintEditor::CompileBlueprint()
{
	if (BlueprintAsset)
	{
		FKismetEditorUtilities::CompileBlueprint(BlueprintAsset);
	}
}

FName FClingScriptBlueprintEditor::GetToolkitFName() const
{
	return FName("ClingScriptBlueprintEditor");
}

FText FClingScriptBlueprintEditor::GetBaseToolkitName() const
{
	return INVTEXT("Cling Script Blueprint Editor");
}

FString FClingScriptBlueprintEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("ClingScriptBlueprint");
}

FLinearColor FClingScriptBlueprintEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

void FClingScriptBlueprintEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(INVTEXT("Cling Script Blueprint Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(ClingScriptBlueprintSourceTabId, FOnSpawnTab::CreateSP(this, &FClingScriptBlueprintEditor::SpawnTab_Source))
		.SetDisplayName(INVTEXT("Source"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FClingScriptBlueprintEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(ClingScriptBlueprintSourceTabId);
}

TSharedRef<SDockTab> FClingScriptBlueprintEditor::SpawnTab_Source(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	[
		SAssignNew(SourceEditor, SCppMultiLineEditableTextBox)
		.Text(BlueprintAsset ? FText::FromString(BlueprintAsset->SourceCode) : FText::GetEmpty())
		.OnTextChanged(this, &FClingScriptBlueprintEditor::HandleSourceChanged)
	];
}

void FClingScriptBlueprintEditor::HandleSourceChanged(const FText& NewText)
{
	if (!BlueprintAsset)
	{
		return;
	}

	BlueprintAsset->Modify();
	BlueprintAsset->SourceCode = NewText.ToString();
	BlueprintAsset->MarkPackageDirty();
}
