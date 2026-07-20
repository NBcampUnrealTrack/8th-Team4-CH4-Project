#include "InteractiveProp/PressureValve.h"
#include "Component/PressureComponent.h"
#include "Game/GODGameState.h"
#include "Player/BaseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

APressureValve::APressureValve()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ValveMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ValveMesh"));
	RootComponent = ValveMesh;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(60.f, 60.f, 60.f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void APressureValve::BeginPlay()
{
	Super::BeginPlay();
}

void APressureValve::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APressureValve, bMinigameActive);
	DOREPLIFETIME(APressureValve, SuccessCount);
	DOREPLIFETIME(APressureValve, MissCount);
	DOREPLIFETIME(APressureValve, RoundStartServerTime);
}

float APressureValve::ComputeNeedlePosition(float Elapsed, float Period)
{
	if (Period <= KINDA_SMALL_NUMBER) return 0.f;

	const float Phase = FMath::Fmod(FMath::Max(Elapsed, 0.f), Period) / Period;
	return Phase < 0.5f ? Phase * 2.f : 2.f - Phase * 2.f;
}

UPressureComponent* APressureValve::GetPressureComponent() const
{
	// 1) 디테일에서 명시적으로 지정한 TrainActor 우선
	if (TrainActor)
	{
		if (UPressureComponent* Pressure = TrainActor->FindComponentByClass<UPressureComponent>())
		{
			return Pressure;
		}
	}

	// 2) 미지정이면 부모 액터에서 자동 탐색 (CoalFeeder::GetFurnace()와 동일한 패턴)
	AActor* Parent = GetAttachParentActor();
	if (!Parent)
	{
		Parent = GetParentActor();
	}
	for (; Parent; Parent = Parent->GetAttachParentActor())
	{
		if (UPressureComponent* Pressure = Parent->FindComponentByClass<UPressureComponent>())
		{
			return Pressure;
		}
	}

	return nullptr;
}

void APressureValve::StartMinigame(ABaseCharacter* Player)
{
	bMinigameActive = true;
	SuccessCount = 0;
	MissCount = 0;
	MinigamePlayer = Player;

	Player->ActivePressureValve = this;

	// 진행 중 사망하면 실패(폭발) 없이 중단하도록 감시.
	Player->OnCharacterDied.AddDynamic(this, &APressureValve::OnMinigamePlayerDied);

	StartNextRound();

	Player->Client_StartPressureMinigame(this);
}

ABaseCharacter* APressureValve::ReleaseMinigamePlayer()
{
	ABaseCharacter* Player = MinigamePlayer;
	MinigamePlayer = nullptr;
	if (Player)
	{
		Player->ActivePressureValve = nullptr;
		Player->OnCharacterDied.RemoveDynamic(this, &APressureValve::OnMinigamePlayerDied);
	}
	return Player;
}

void APressureValve::OnMinigamePlayerDied(ABaseCharacter* /*DeadCharacter*/, AActor* /*Killer*/)
{
	AbortMinigame();
}

void APressureValve::AbortMinigame()
{
	if (!HasAuthority() || !bMinigameActive) return;

	GetWorldTimerManager().ClearTimer(RoundTimeoutHandle);
	bMinigameActive = false;

	ABaseCharacter* Player = ReleaseMinigamePlayer();
	if (IsValid(Player)) Player->Client_EndPressureMinigame(false);
}

void APressureValve::StartNextRound()
{
	RoundStartServerTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(RoundTimeoutHandle, this, &APressureValve::OnRoundTimeout, RoundDuration, false);
}

void APressureValve::SubmitStopInput()
{
	if (!HasAuthority() || !bMinigameActive) return;

	GetWorldTimerManager().ClearTimer(RoundTimeoutHandle);

	const float Elapsed = GetWorld()->GetTimeSeconds() - RoundStartServerTime;
	const float NeedlePos = ComputeNeedlePosition(Elapsed, NeedleOscillationPeriod);
	const bool bHit = NeedlePos >= RedZoneStart && NeedlePos <= RedZoneEnd;

	if (bHit)
	{
		++SuccessCount;
		if (UPressureComponent* Pressure = GetPressureComponent())
		{
			Pressure->ReducePressure(PressureReductionPerSuccess);
		}
	}
	else
	{
		++MissCount;
	}

	EvaluateRoundResult();
}

void APressureValve::OnRoundTimeout()
{
	if (!bMinigameActive) return;

	// 플레이어가 나갔거나(폰 파괴) 죽었으면 실패(폭발) 처리 없이 중단.
	if (!IsValid(MinigamePlayer) || MinigamePlayer->IsDead())
	{
		AbortMinigame();
		return;
	}

	++MissCount;
	EvaluateRoundResult();
}

void APressureValve::EvaluateRoundResult()
{
	if (SuccessCount >= RequiredSuccesses)
	{
		bMinigameActive = false;
		ABaseCharacter* Player = ReleaseMinigamePlayer();
		if (Player) Player->Client_EndPressureMinigame(true);
	}
	else if (MissCount >= MaxMisses)
	{
		bMinigameActive = false;
		ABaseCharacter* Player = ReleaseMinigamePlayer();
		if (Player) Player->Client_EndPressureMinigame(false);

		if (UPressureComponent* Pressure = GetPressureComponent())
		{
			Pressure->ForceExplode();
		}
	}
	else
	{
		StartNextRound();
	}
}

void APressureValve::ForceStop()
{
	if (!HasAuthority()) return;

	Tags.AddUnique(TEXT("Stoker.ForceClose"));

	if (!bMinigameActive) return;

	GetWorldTimerManager().ClearTimer(RoundTimeoutHandle);
	bMinigameActive = false;

	ABaseCharacter* Player = ReleaseMinigamePlayer();
	if (Player) Player->Client_EndPressureMinigame(false);
}

bool APressureValve::IsUsableNow() const
{
	if (bMinigameActive || ActorHasTag(TEXT("Stoker.ForceClose"))) return false;

	// 폭발 직후 쿨타임 동안은 조작 불가 — 마피아가 연속 폭발로 무한 감속하는 것을 막는다.
	if (const UPressureComponent* Pressure = GetPressureComponent())
	{
		if (Pressure->IsValveOnCooldown()) return false;
	}

	const AGODGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGODGameState>() : nullptr;
	return GS && GS->CurrentPhase == EGamePhase::Playing;
}

void APressureValve::Interact_Implementation(ACharacter* Interactor)
{
	if (!IsUsableNow()) return;

	ABaseCharacter* BaseChar = Cast<ABaseCharacter>(Interactor);
	if (!BaseChar) return;

	StartMinigame(BaseChar);
}

FText APressureValve::GetInteractPrompt_Implementation() const
{
	if (!IsUsableNow()) return FText::GetEmpty();
	return FText::FromString(TEXT("밸브 조작"));
}
