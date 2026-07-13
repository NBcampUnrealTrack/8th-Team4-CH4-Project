#include "Component/FurnanceComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

UFurnanceComponent::UFurnanceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UFurnanceComponent::BeginPlay()
{
	Super::BeginPlay();

	// UI 갱신을 매 Tick이 아니라 일정 간격으로만 쏜다(소수점 떨림 방지).
	// 모든 머신에서 돌며 CurrentFuel(복제됨)을 읽어 위젯을 갱신한다.
	if (UIUpdateInterval > 0.f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			UIUpdateTimer, this, &UFurnanceComponent::BroadcastFuelLevel,
			UIUpdateInterval, /*bLoop=*/true);
	}
}

void UFurnanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(UIUpdateTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UFurnanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFurnanceComponent, CurrentFuel);
	DOREPLIFETIME(UFurnanceComponent, bIsBurning);
}

void UFurnanceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner()->HasAuthority() || !bIsBurning || bFrozen) return;

	// 연료 소모는 매 Tick(부드러운 속도 반영). UI 브로드캐스트는 타이머가 담당.
	CurrentFuel = FMath::Max(0.f, CurrentFuel - FuelBurnRate * DeltaTime);

	if (CurrentFuel <= 0.f)
	{
		bIsBurning = false;
		OnFurnaceDeactivated.Broadcast();
		BroadcastFuelLevel(); // 소진은 즉시 반영
	}
}

EFuelState UFurnanceComponent::GetFuelState() const
{
	if (MaxFuel <= 0.f || CurrentFuel <= 0.f)
	{
		return EFuelState::Empty;
	}

	const float Ratio = CurrentFuel / MaxFuel;
	if (Ratio < LowFuelRatio)    return EFuelState::Low;
	if (Ratio < MediumFuelRatio) return EFuelState::Medium;
	if (Ratio < HighFuelRatio)   return EFuelState::High;
	return EFuelState::Full;
}

void UFurnanceComponent::AddCoal(float Amount)
{
	if (!GetOwner()->HasAuthority()) return;

	const bool bWasActive = bIsBurning;
	CurrentFuel = FMath::Clamp(CurrentFuel + Amount, 0.f, MaxFuel);
	bIsBurning = (CurrentFuel > 0.f);

	if (!bWasActive && bIsBurning)
	{
		OnFurnaceActivated.Broadcast();
	}

	// 석탄 투입은 이산 이벤트라 즉시 반영(게이지가 바로 튀어오름).
	BroadcastFuelLevel();
}

void UFurnanceComponent::BroadcastFuelLevel()
{
	// 값이 실제로 바뀌었을 때만 위젯 갱신(정지/유휴 시 불필요한 갱신 방지).
	if (FMath::IsNearlyEqual(CurrentFuel, LastBroadcastFuel, 0.01f))
	{
		return;
	}
	LastBroadcastFuel = CurrentFuel;

	const float Percent = (MaxFuel > 0.f) ? FMath::Clamp(CurrentFuel / MaxFuel, 0.f, 1.f) : 0.f;
	OnFuelLevelChanged.Broadcast(Percent);
}

void UFurnanceComponent::OnRep_bIsBurning()
{
	if (bIsBurning)
		OnFurnaceActivated.Broadcast();
	else
		OnFurnaceDeactivated.Broadcast();
}
