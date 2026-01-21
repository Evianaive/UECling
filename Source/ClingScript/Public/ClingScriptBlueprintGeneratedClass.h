#pragma once

#include "CoreMinimal.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "ClingScriptBlueprintGeneratedClass.generated.h"

UCLASS()
class CLINGSCRIPT_API UClingScriptBlueprintGeneratedClass : public UBlueprintGeneratedClass
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString SourceCode;

	FORCEINLINE static UClingScriptBlueprintGeneratedClass* GetClingScriptGeneratedClass(UClass* InClass)
	{
		UClingScriptBlueprintGeneratedClass* ScriptClass = nullptr;
		for (UClass* MyClass = InClass; MyClass && !ScriptClass; MyClass = MyClass->GetSuperClass())
		{
			ScriptClass = Cast<UClingScriptBlueprintGeneratedClass>(MyClass);
		}
		return ScriptClass;
	}
};
