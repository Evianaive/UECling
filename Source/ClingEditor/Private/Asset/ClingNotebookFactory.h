#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ClingNotebookFactory.generated.h"

UCLASS()
class UClingNotebookFactory : public UFactory
{
	GENERATED_BODY()

public:
	UClingNotebookFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
