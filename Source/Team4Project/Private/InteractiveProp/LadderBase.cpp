// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveProp/LadderBase.h"
#include "Player/BaseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Component/CustomMovementComponent.h"
#include "GameFramework/PlayerController.h"

// Sets default values
ALadderBase::ALadderBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	LadderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LadderMesh"));
	RootComponent = LadderMesh;

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(LadderMesh);
	InteractionVolume->SetBoxExtent(FVector(20.f, 30.f, 50.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	ClimbWallVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ClimbWallVolume"));
	ClimbWallVolume->SetupAttachment(LadderMesh);
	ClimbWallVolume->SetBoxExtent(FVector(7.f, 27.f, 50.f));
	ClimbWallVolume->SetCollisionProfileName(TEXT("Climbable"));
	// 콜리전 커스텀 세팅
	ClimbWallVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClimbWallVolume->SetCollisionResponseToAllChannels(ECR_Ignore); // 캐릭터 캡슐은 무시(통과)

}

void ALadderBase::Interact_Implementation(ACharacter* Interactor)
{
	if (!HasAuthority() || !Interactor) return;

	if (ABaseCharacter* BaseChar = Cast<ABaseCharacter>(Interactor))
	{
		BaseChar->Client_ForceStartClimbing();
	}
}

FText ALadderBase::GetInteractPrompt_Implementation() const
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ABaseCharacter* LocalChar = Cast<ABaseCharacter>(PC->GetPawn()))
		{
			if (UCustomMovementComponent* MoveComp = LocalChar->GetCustomMovementComponent())
			{
				if (MoveComp->IsClimbing())
				{
					// 클라이밍 중이라면 "내리기" 텍스트 반환
					return FText::FromString(TEXT("내리기"));
				}
			}
		}
	}
	// 클라이밍 중이 아니거나 그 외의 경우는 모두 "오르기" 반환
	return FText::FromString(TEXT("오르기"));
}

