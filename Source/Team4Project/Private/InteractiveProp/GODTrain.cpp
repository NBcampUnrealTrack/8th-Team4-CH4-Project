#include "InteractiveProp/GODTrain.h"
#include "Component/FurnanceComponent.h"
#include "Component/PressureComponent.h"
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

	Furnace = CreateDefaultSubobject<UFurnanceComponent>(TEXT("Furnace"));
	Pressure = CreateDefaultSubobject<UPressureComponent>(TEXT("Pressure"));
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

		if (Furnace)
		{
			Furnace->OnFurnaceActivated.AddDynamic(this, &AGODTrain::OnFurnaceActivated);
			Furnace->OnFurnaceDeactivated.AddDynamic(this, &AGODTrain::OnFurnaceDeactivated);
		}

		if (Pressure)
		{
			Pressure->OnPressureExplode.AddDynamic(this, &AGODTrain::OnPressureExploded);
		}
	}
}

void AGODTrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || bIsDerailed) return;

	// 연료 없으면 점진적 감속
	if (Furnace && !Furnace->bIsBurning && TrainSpeed > 0.f)
	{
		TrainSpeed = FMath::Max(0.f, TrainSpeed - Deceleration * DeltaTime);
	}

	// 압력 컴포넌트에 연료 상태 전달
	if (Pressure && Furnace)
	{
		Pressure->bFurnaceActive = Furnace->bIsBurning;
	}

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
	OnRep_bIsDerailed();
}

void AGODTrain::OnFurnaceActivated()
{
	// 연료 공급 재개 시 기본 속도로 복귀
	TrainSpeed = DefaultSpeed;
}

void AGODTrain::OnFurnaceDeactivated()
{
	// 감속은 Tick에서 점진적으로 처리
}

void AGODTrain::OnPressureExploded()
{
	// 폭발 시 즉시 속도 패널티
	TrainSpeed = FMath::Max(0.f, TrainSpeed - ExplosionSpeedPenalty);
	// 이펙트/데미지는 BP에서 OnPressureExplode 델리게이트에 바인딩
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
