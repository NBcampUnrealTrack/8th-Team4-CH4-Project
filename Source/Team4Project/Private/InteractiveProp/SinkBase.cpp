#include "InteractiveProp/SinkBase.h"
#include "Player/GODPlayerState.h"
#include "Player/BaseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

// Sets default values
ASinkBase::ASinkBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	BasinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasinMesh"));
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
	
	BasinMesh->SetupAttachment(SceneRoot);

	
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(60.f, 60.f, 60.f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

}

void ASinkBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASinkBase, bIsInUse);
}

void ASinkBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || !bIsInUse) return;

	ACharacter* User = WashingUser.Get();
	if (!CanKeepWashing(User))
	{
		StopWashing();
		return;
	}

	AGODPlayerState* PS = User->GetPlayerState<AGODPlayerState>();
	if (!PS)
	{
		StopWashing();
		return;
	}

	PS->SootLevel = FMath::Clamp(PS->SootLevel - CleanPerSecond * DeltaTime, 0.f, 1.f);

	// 다 씻었으면 자동 종료
	if (PS->SootLevel <= 0.f)
	{
		StopWashing();
	}
}

void ASinkBase::Interact_Implementation(ACharacter* Interactor)
{
	if (!HasAuthority() || !Interactor) return;

	if (bIsInUse)
	{
		// 본인만 중단 가능. 다른 사람이 사용 중이면 무시.
		if (WashingUser.Get() == Interactor)
		{
			StopWashing();
		}
		return;
	}

	StartWashing(Interactor);
}

FText ASinkBase::GetInteractPrompt_Implementation() const
{
	return bIsInUse ? FText::FromString(TEXT("씻기 중단")) : FText::FromString(TEXT("손 씻기"));
}

void ASinkBase::StartWashing(ACharacter* User)
{
	if (!CanKeepWashing(User)) return;

	// 오염도가 이미 0 이면 시작할 필요 없음
	const AGODPlayerState* PS = User->GetPlayerState<AGODPlayerState>();
	if (!PS || PS->SootLevel <= 0.f) return;

	WashingUser = User;
	SetInUse(true);
}

void ASinkBase::StopWashing()
{
	WashingUser.Reset();
	SetInUse(false);
}

bool ASinkBase::CanKeepWashing(const ACharacter* User) const
{
	if (!IsValid(User)) return false;

	// 범위를 벗어나면 자동 중단
	if (InteractionBox && !InteractionBox->IsOverlappingActor(User)) return false;

	if (const ABaseCharacter* Char = Cast<ABaseCharacter>(User))
	{
		// 사망 시 중단. 석탄을 든 손으로는 씻을 수 없다.
		if (Char->IsDead() || Char->IsCoalEquipped()) return false;
	}

	return true;
}

void ASinkBase::SetInUse(bool bNewInUse)
{
	if (bIsInUse == bNewInUse) return;

	bIsInUse = bNewInUse;

	// OnRep 은 서버(리슨 호스트)에서 호출되지 않으므로 연출 훅을 직접 호출
	OnWashingStateChanged(bIsInUse);
}

void ASinkBase::OnRep_IsInUse()
{
	OnWashingStateChanged(bIsInUse);
}

