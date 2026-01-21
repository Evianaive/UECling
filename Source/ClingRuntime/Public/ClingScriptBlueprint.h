#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "ClingScriptBlueprintGeneratedClass.h"
#include "ClingScriptBlueprint.generated.h"

UCLASS(BlueprintType)
class CLINGRUNTIME_API UClingScriptBlueprint : public UBlueprint
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString SourceCode;

#if WITH_EDITOR
	virtual UClass* GetBlueprintClass() const override;

	virtual bool SupportedByDefaultBlueprintFactory() const override
	{
		return false;
	}

	static bool ValidateGeneratedClass(const UClass* InClass);
#endif
};
