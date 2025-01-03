﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyTestInterpreter.generated.h"

UCLASS()
class CLINGRUNTIME_API AMyTestInterpreter : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMyTestInterpreter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(CallInEditor)
	void Process();
	UFUNCTION(CallInEditor)
	void TestCallByUE();
	UFUNCTION(CallInEditor)
	void TestTArrayStruct();
	
	UPROPERTY(EditAnywhere,meta=(MultiLine))
	FString ProcessString;
};
