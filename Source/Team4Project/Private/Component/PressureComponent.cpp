#include "Component/PressureComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

UPressureComponent::UPressureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UPressureComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPressureComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPressureComponent, CurrentPressure);
	DOREPLIFETIME(UPressureComponent, bExploded);
	DOREPLIFETIME(UPressureComponent, bValveOnCooldown);
	DOREPLIFETIME(UPressureComponent, ValveCooldownEndTime);
}

void UPressureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner()->HasAuthority() || bExploded || bFrozen) return;

	if (bTrainRunning)
	{
		// 운행 중에는 기본 상승, 석탄 연소로 가속 중이면 배수 적용.
		const float RiseRate = PressureRiseRate * (bFurnaceActive ? FurnaceRiseMultiplier : 1.f);
		CurrentPressure = FMath::Min(CurrentPressure + RiseRate * DeltaTime, ExplosionThreshold);
	}
	else
	{
		CurrentPressure = FMath::Max(CurrentPressure - PressureDecayRate * DeltaTime, 0.f);
	}

	OnPressureChanged.Broadcast(CurrentPressure / ExplosionThreshold);

	// 경고 구간 진입/해제
	if (!bWasWarning && CurrentPressure >= WarningThreshold)
	{
		bWasWarning = true;
		OnPressureWarning.Broadcast();
	}
	else if (bWasWarning && CurrentPressure < WarningThreshold)
	{
		bWasWarning = false;
		OnPressureRecovered.Broadcast();
	}

	// 폭발
	if (CurrentPressure >= ExplosionThreshold && !bExploded)
	{
		bExploded = true;
		StartValveCooldown();
		OnPressureExplode.Broadcast();
	}
}

void UPressureComponent::ReducePressure(float Amount)
{
	if (!GetOwner()->HasAuthority()) return;
	CurrentPressure = FMath::Max(0.f, CurrentPressure - Amount);
}

void UPressureComponent::ResetAfterExplosion()
{
	if (!GetOwner()->HasAuthority()) return;
	bExploded = false;
	CurrentPressure = 30.f;
	// bWasWarning 은 건드리지 않는다 — 직전까지 경고 구간이었으므로, 다음 Tick 에서
	// (30 < WarningThreshold) 조건으로 OnPressureRecovered 가 정상 발화해야
	// 열차의 고압 감속(bHighPressure)이 풀린다. 여기서 false 로 지우면 해제 이벤트가
	// 영영 나가지 않아 감속 배수가 게임 끝까지 남는다.
}

void UPressureComponent::ResetForNewGame()
{
	if (!GetOwner()->HasAuthority()) return;

	bExploded = false;
	CurrentPressure = 0.f;
	bWasWarning = false;

	// 새 게임 시작 시 폭발 쿨타임도 초기화.
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ValveCooldownTimer);
	}
	bValveOnCooldown = false;
	ValveCooldownEndTime = 0.f;

	OnPressureChanged.Broadcast(0.f);
}

void UPressureComponent::ForceExplode()
{
	if (!GetOwner()->HasAuthority() || bExploded) return;

	bExploded = true;
	CurrentPressure = ExplosionThreshold;
	StartValveCooldown();
	OnPressureExplode.Broadcast();
}

void UPressureComponent::StartValveCooldown()
{
	if (!GetOwner()->HasAuthority()) return;

	// 쿨타임 0 이하면(에디터에서 꺼둔 경우) 아예 걸지 않는다.
	if (ValveCooldownAfterExplosion <= 0.f)
	{
		bValveOnCooldown = false;
		ValveCooldownEndTime = 0.f;
		return;
	}

	bValveOnCooldown = true;

	// 남은 시간 표시는 서버/클라 공통으로 GameState 동기 시각(GetServerWorldTimeSeconds)을 쓴다.
	const AGameStateBase* GS = GetWorld()->GetGameState();
	const float Now = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	ValveCooldownEndTime = Now + ValveCooldownAfterExplosion;

	GetWorld()->GetTimerManager().SetTimer(
		ValveCooldownTimer, this, &UPressureComponent::EndValveCooldown,
		ValveCooldownAfterExplosion, /*bLoop=*/false);
}

void UPressureComponent::EndValveCooldown()
{
	bValveOnCooldown = false;
	ValveCooldownEndTime = 0.f;
}

float UPressureComponent::GetValveCooldownRemaining() const
{
	if (!bValveOnCooldown) return 0.f;

	const UWorld* World = GetWorld();
	if (!World) return 0.f;

	const AGameStateBase* GS = World->GetGameState();
	const float Now = GS ? GS->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	return FMath::Max(0.f, ValveCooldownEndTime - Now);
}

void UPressureComponent::OnRep_CurrentPressure()
{
	OnPressureChanged.Broadcast(CurrentPressure / ExplosionThreshold);
}
