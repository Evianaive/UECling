#include "Asset/ClingScriptBlueprintFactory.h"

#include "ClingScriptBlueprint.h"
#include "GameFramework/Actor.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"
#include "ClingScriptBlueprintGeneratedClass.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"

UClingScriptBlueprintFactory::UClingScriptBlueprintFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UClingScriptBlueprint::StaticClass();
	ParentClass = AActor::StaticClass();
}

bool UClingScriptBlueprintFactory::ConfigureProperties()
{
	ParentClass = nullptr;

	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;
	Options.DisplayMode = EClassViewerDisplayMode::ListView;
	Options.bIsBlueprintBaseOnly = true;
	
	// Filter for C++ classes and Cling blueprint classes
	class FClingParentClassFilter : public IClassViewerFilter
	{
	public:
		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
		{
			if (InClass->IsNative())
			{
				return true;
			}

			if (InClass->IsChildOf(UClingScriptBlueprintGeneratedClass::StaticClass()))
			{
				return true;
			}

			return false;
		}

		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
		{
			if (InUnloadedClassData->IsChildOf(UClingScriptBlueprintGeneratedClass::StaticClass()))
			{
				return true;
			}
			return false;
		}
	};

	Options.ClassFilters.Add(MakeShareable(new FClingParentClassFilter));

	UClass* ChosenClass = nullptr;
	const FText TitleText = INVTEXT("Pick Parent Class for Cling Script Blueprint");
	if (SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UObject::StaticClass()))
	{
		ParentClass = ChosenClass;
	}

	return ParentClass != nullptr;
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
