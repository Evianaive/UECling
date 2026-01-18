#include "ClingNotebookFactory.h"
#include "ClingNotebook.h"

UClingNotebookFactory::UClingNotebookFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UClingNotebook::StaticClass();
}

UObject* UClingNotebookFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UClingNotebook>(InParent, InClass, InName, Flags);
}
