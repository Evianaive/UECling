#include "Asset/ClingScriptBlueprintFactory.h"

#include "ClingScriptBlueprint.h"
#include "GameFramework/Actor.h"
#include "Kismet2/KismetEditorUtilities.h"

UClingScriptBlueprintFactory::UClingScriptBlueprintFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UClingScriptBlueprint::StaticClass();
	ParentClass = AActor::StaticClass();
}

UObject* UClingScriptBlueprintFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UBlueprint* Created = FKismetEditorUtilities::CreateBlueprint(
		ParentClass ? ParentClass.Get() : AActor::StaticClass(),
		InParent,
		InName,
		BPTYPE_Normal,
		UClingScriptBlueprint::StaticClass(),
		UClingScriptBlueprintGeneratedClass::StaticClass(),
		TEXT("UClingScriptBlueprintFactory"));

	if (UClingScriptBlueprint* ClingBP = Cast<UClingScriptBlueprint>(Created))
	{
		ClingBP->SourceCode = TEXT("");
		return ClingBP;
	}

	return Created;
}
