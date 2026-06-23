#include "InteractiveProp/GODTrain.h"
#include "Game/GODGameState.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

AGODTrain::AGODTrain()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	TrainMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrainMesh"));
	RootComponent = TrainMesh;
}

void AGODTrain::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		TrainSpeed = DefaultSpeed;
		DistanceToDestination = TotalDistance;
		bIsDerailed = false;
		SyncDistanceToGameState();
	}
}

void AGODTrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || bIsDerailed) return;

	DistanceToDestination = FMath::Max(0.f, DistanceToDestination - TrainSpeed * DeltaTime);
	SyncDistanceToGameState();
}

void AGODTrain::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGODTrain, TrainSpeed);
	DOREPLIFETIME(AGODTrain, bIsDerailed);
}

void AGODTrain::SetTrainSpeed(float NewSpeed)
{
	if (!HasAuthority()) return;
	TrainSpeed = FMath::Max(0.f, NewSpeed);
}

void AGODTrain::ApplyTrackSwitchImpulse(float ImpulseStrength, float ImpulseRadius)
{
	if (!HasAuthority()) return;

	const FVector ImpulseDir = GetActorRightVector() * ImpulseStrength;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	TArray<UPrimitiveComponent*> OutComponents;
	UKismetSystemLibrary::SphereOverlapComponents(
		this, GetActorLocation(), ImpulseRadius,
		ObjectTypes, nullptr, IgnoreActors, OutComponents);

	for (UPrimitiveComponent* Comp : OutComponents)
	{
		if (Comp && Comp->IsSimulatingPhysics())
		{
			Comp->AddImpulse(ImpulseDir, NAME_None, /*bVelChange=*/true);
		}
	}
}

void AGODTrain::TriggerDerailment()
{
	if (!HasAuthority()) return;

	bIsDerailed = true;
	TrainSpeed = 0.f;
	OnRep_bIsDerailed(); // 서버 측 즉시 통지
}

void AGODTrain::SyncDistanceToGameState()
{
	if (AGODGameState* GS = GetWorld()->GetGameState<AGODGameState>())
	{
		GS->DistanceToDestination = DistanceToDestination;
	}
}

void AGODTrain::OnRep_bIsDerailed()
{
	if (bIsDerailed)
	{
		OnTrainDerailed.Broadcast();
	}
}
