#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ClingScriptBlueprintFactory.generated.h"

UCLASS()
class UClingScriptBlueprintFactory : public UFactory
{
	GENERATED_BODY()

public:
	UClingScriptBlueprintFactory();

	UPROPERTY(EditAnywhere, Category = "Cling")
	TSubclassOf<class UObject> ParentClass;

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
