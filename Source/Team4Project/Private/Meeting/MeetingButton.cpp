#include "Meeting/MeetingButton.h"
#include "Game/GODGameMode.h"
#include "Game/GODGameState.h"
#include "Player/BaseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

AMeetingButton::AMeetingButton()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
	RootComponent = ButtonMesh;

	// QuestStation 과 동일한 셋업 — 캐릭터 InteractSphere 와 오버랩되려면 이 조합이어야 한다.
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	InteractionBox->SetGenerateOverlapEvents(true);
}

void AMeetingButton::Interact_Implementation(ACharacter* Interactor)
{
	if (!HasAuthority()) return;

	ABaseCharacter* BaseChar = Cast<ABaseCharacter>(Interactor);
	if (!BaseChar || BaseChar->IsDead()) return;

	// 조건 판정과 실행은 전부 서버 GameMode. 결정: 누른 사람은 비공개 — 이름을 넘기지 않는다.
	if (AGODGameMode* GM = GetWorld()->GetAuthGameMode<AGODGameMode>())
	{
		GM->TryStartMeeting();
	}
}

FText AMeetingButton::GetInteractPrompt_Implementation() const
{
	const AGODGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGODGameState>() : nullptr;
	if (!GS || !GS->bMeetingBellReady) return FText::GetEmpty();

	return NSLOCTEXT("Meeting", "BellPrompt", "긴급 소집");
}
