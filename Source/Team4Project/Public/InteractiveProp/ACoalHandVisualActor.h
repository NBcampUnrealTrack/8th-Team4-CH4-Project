// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACoalHandVisualActor.generated.h"

UCLASS()
class TEAM4PROJECT_API AACoalHandVisualActor : public AActor
{
	GENERATED_BODY()
	
public:
	AACoalHandVisualActor();
	virtual void BeginPlay() override;
	void Tick(float DeltaTime);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual")
	float VisualScale = 0.2f;

protected:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual")
	UMaterialInterface* CoalMaterial;
};

