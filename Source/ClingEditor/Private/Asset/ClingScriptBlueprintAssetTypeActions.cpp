#include "Asset/ClingScriptBlueprintAssetTypeActions.h"

#include "Asset/ClingScriptBlueprintEditor.h"
#include "ClingScriptBlueprint.h"

FText FClingScriptBlueprintAssetTypeActions::GetName() const
{
	return INVTEXT("Cling Script Blueprint");
}

FColor FClingScriptBlueprintAssetTypeActions::GetTypeColor() const
{
	return FColor(201, 29, 85);
}

UClass* FClingScriptBlueprintAssetTypeActions::GetSupportedClass() const
{
	return UClingScriptBlueprint::StaticClass();
}

uint32 FClingScriptBlueprintAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FClingScriptBlueprintAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UClingScriptBlueprint* ScriptBlueprint = Cast<UClingScriptBlueprint>(*ObjIt))
		{
			TSharedRef<FClingScriptBlueprintEditor> NewEditor(new FClingScriptBlueprintEditor());
			NewEditor->InitEditor(Mode, EditWithinLevelEditor, ScriptBlueprint);
		}
	}
}
