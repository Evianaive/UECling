#pragma once

#include "Containers/UnrealString.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#include "Kismet/KismetSystemLibrary.h"

void TestTIterator(int32& TestIn, int32& TestResult, FString& StringResult, int32& OutActorsCount, UWorld* World){
	TestResult = TestIn*TestIn;
	StringResult = FString::Printf(TEXT("Re=%i"),TestResult);

	UClass* ActorClass = AActor::StaticClass();
	TArray<AActor*> OutActors;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		OutActors.Add(Actor);
	}
	OutActorsCount = OutActors.Num();
}

