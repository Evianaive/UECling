#include "ClingNotebookEditor.h"
#include "ClingNotebook.h"
#include "Repl/SNumericNotebook.h"
#include "Widgets/Docking/SDockTab.h"

const FName ClingNotebookTabId(TEXT("ClingNotebook_Notebook"));

void FClingNotebookEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& EditWithinLevelEditor, UClingNotebook* InNotebook)
{
	NotebookAsset = InNotebook;

	TSharedRef<FTabManager::FLayout> StandaloneLayout = FTabManager::NewLayout("Standalone_ClingNotebook_Layout")
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->AddTab(ClingNotebookTabId, ETabState::OpenedTab)
		)
	);

	FAssetEditorToolkit::InitAssetEditor(Mode, EditWithinLevelEditor, TEXT("ClingNotebookEditor"), StandaloneLayout, true, true, InNotebook);
}

FName FClingNotebookEditor::GetToolkitFName() const
{
	return FName("ClingNotebookEditor");
}

FText FClingNotebookEditor::GetBaseToolkitName() const
{
	return INVTEXT("Cling Notebook Editor");
}

FString FClingNotebookEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("ClingNotebook");
}

FLinearColor FClingNotebookEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

void FClingNotebookEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(INVTEXT("Cling Notebook Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(ClingNotebookTabId, FOnSpawnTab::CreateSP(this, &FClingNotebookEditor::SpawnTab_Notebook))
		.SetDisplayName(INVTEXT("Notebook"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FClingNotebookEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(ClingNotebookTabId);
}

TSharedRef<SDockTab> FClingNotebookEditor::SpawnTab_Notebook(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	[
		SNew(SNumericNotebook)
		.NotebookAsset(NotebookAsset)
	];
}
