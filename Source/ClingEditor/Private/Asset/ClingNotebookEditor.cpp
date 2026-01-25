#include "ClingNotebookEditor.h"
#include "ClingNotebook.h"
#include "Repl/SNumericNotebook.h"
#include "Widgets/Docking/SDockTab.h"

const FName ClingNotebookTabId(TEXT("ClingNotebook_Notebook"));
const FName ClingNotebookDetailsTabId(TEXT("ClingNotebook_Details"));

void FClingNotebookEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& EditWithinLevelEditor, UClingNotebook* InNotebook)
{
	// Todo check if this work?
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseOtherEditors(InNotebook, this);
	NotebookAsset = InNotebook;

	const TSharedRef<FTabManager::FLayout> StandaloneLayout = FTabManager::NewLayout("Standalone_ClingNotebook_Layout_v2")
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Horizontal)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.4f)
			->AddTab(ClingNotebookTabId, ETabState::OpenedTab)
		)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.6f)
			->AddTab(ClingNotebookDetailsTabId, ETabState::OpenedTab)
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

	InTabManager->RegisterTabSpawner(ClingNotebookDetailsTabId, FOnSpawnTab::CreateSP(this, &FClingNotebookEditor::SpawnTab_Details))
		.SetDisplayName(INVTEXT("Cell Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FClingNotebookEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(ClingNotebookTabId);
	InTabManager->UnregisterTabSpawner(ClingNotebookDetailsTabId);
}

TSharedRef<SDockTab> FClingNotebookEditor::SpawnTab_Notebook(const FSpawnTabArgs& Args)
{
	NotebookWidget = SNew(SNumericNotebook)
		.NotebookAsset(NotebookAsset);

	return SNew(SDockTab)
	[
		NotebookWidget.ToSharedRef()
	];
}

TSharedRef<SDockTab> FClingNotebookEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	[
		SNew(SClingNotebookDetailsPanel)
		.NotebookAsset(NotebookAsset)
		.NotebookWidget(NotebookWidget)
	];
}
