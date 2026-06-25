#include "Component/PressureComponent.h"
#include "Net/UnrealNetwork.h"

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
}

void UPressureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner()->HasAuthority() || bExploded) return;

	if (bFurnaceActive)
		CurrentPressure = FMath::Min(CurrentPressure + PressureRiseRate * DeltaTime, ExplosionThreshold);
	else
		CurrentPressure = FMath::Max(CurrentPressure - PressureDecayRate * DeltaTime, 0.f);

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
	bWasWarning = false;
}

void UPressureComponent::OnRep_CurrentPressure()
{
	OnPressureChanged.Broadcast(CurrentPressure / ExplosionThreshold);
}
