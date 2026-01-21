#include "ClingScriptBlueprint.h"
#include "ClingScriptBlueprintGeneratedClass.h"

#if WITH_EDITOR
UClass* UClingScriptBlueprint::GetBlueprintClass() const
{
	return UClingScriptBlueprintGeneratedClass::StaticClass();
}

bool UClingScriptBlueprint::ValidateGeneratedClass(const UClass* InClass)
{
	return Cast<UClingScriptBlueprintGeneratedClass>(InClass) != nullptr;
}
#endif
