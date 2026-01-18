#include "ClingNotebookAssetTypeActions.h"
#include "ClingNotebook.h"
#include "Asset/ClingNotebookEditor.h"

FText FClingNotebookAssetTypeActions::GetName() const
{
	return INVTEXT("Cling Notebook");
}

FColor FClingNotebookAssetTypeActions::GetTypeColor() const
{
	return FColor::Cyan;
}

UClass* FClingNotebookAssetTypeActions::GetSupportedClass() const
{
	return UClingNotebook::StaticClass();
}

uint32 FClingNotebookAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FClingNotebookAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UClingNotebook* Notebook = Cast<UClingNotebook>(*ObjIt))
		{
			TSharedRef<FClingNotebookEditor> NewEditor(new FClingNotebookEditor());
			NewEditor->InitEditor(Mode, EditWithinLevelEditor, Notebook);
		}
	}
}
