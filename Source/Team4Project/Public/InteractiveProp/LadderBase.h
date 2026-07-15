// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "LadderBase.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

UCLASS()
class TEAM4PROJECT_API ALadderBase : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:
	ALadderBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* LadderMesh;

	// 플레이어가 다가갔을 때 F키(상호작용)가 뜨게 할 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* ClimbWallVolume;

	// IInteractable 인터페이스
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;

};
