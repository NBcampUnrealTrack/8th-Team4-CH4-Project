#include "Component/FurnanceComponent.h"
#include "Net/UnrealNetwork.h"

UFurnanceComponent::UFurnanceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UFurnanceComponent::BeginPlay()
{
	Super::BeginPlay();
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

	if (!GetOwner()->HasAuthority() || !bIsBurning) return;

	CurrentFuel = FMath::Max(0.f, CurrentFuel - FuelBurnRate * DeltaTime);

	if (CurrentFuel <= 0.f)
	{
		bIsBurning = false;
		OnFurnaceDeactivated.Broadcast();
	}

	OnFuelLevelChanged.Broadcast(CurrentFuel / MaxFuel);
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

	OnFuelLevelChanged.Broadcast(CurrentFuel / MaxFuel);
}

void UFurnanceComponent::OnRep_CurrentFuel()
{
	OnFuelLevelChanged.Broadcast(CurrentFuel / MaxFuel);
}

void UFurnanceComponent::OnRep_bIsBurning()
{
	if (bIsBurning)
		OnFurnaceActivated.Broadcast();
	else
		OnFurnaceDeactivated.Broadcast();
}
