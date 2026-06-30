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
			Pressure->OnPressureWarning.AddDynamic(this, &AGODTrain::OnPressureWarningStarted);
			Pressure->OnPressureRecovered.AddDynamic(this, &AGODTrain::OnPressureWarningEnded);
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
	else if (Furnace && Furnace->bIsBurning)
	{
		// 압력 경고(80% 이상) 중에는 절반 속도로 제한
		TrainSpeed = DefaultSpeed * (bHighPressure ? HighPressureSpeedMultiplier : 1.0f);
	}

	// 석탄 투입 + 열차 이동 중일 때만 압력 상승
	if (Pressure && Furnace)
	{
		Pressure->bFurnaceActive = Furnace->bIsBurning && TrainSpeed > 0.f;
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
	// 압력 경고 중이면 절반 속도, 정상이면 기본 속도
	TrainSpeed = DefaultSpeed * (bHighPressure ? HighPressureSpeedMultiplier : 1.0f);
}

void AGODTrain::OnFurnaceDeactivated()
{
	// 감속은 Tick에서 점진적으로 처리
}

void AGODTrain::OnPressureExploded()
{
	// 폭발 시 즉시 속도 패널티 — GameMode가 TriggerDerailment()로 완전 정지시킴
	TrainSpeed = FMath::Max(0.f, TrainSpeed - ExplosionSpeedPenalty);
	// 이펙트/데미지는 BP에서 OnPressureExplode 델리게이트에 바인딩
}

void AGODTrain::OnPressureWarningStarted()
{
	bHighPressure = true;
	if (Furnace && Furnace->bIsBurning)
		TrainSpeed = DefaultSpeed * HighPressureSpeedMultiplier;
}

void AGODTrain::OnPressureWarningEnded()
{
	bHighPressure = false;
	if (Furnace && Furnace->bIsBurning)
		TrainSpeed = DefaultSpeed;
}

void AGODTrain::SyncDistanceToGameState()
{
	if (AGODGameState* GS = GetWorld()->GetGameState<AGODGameState>())
	{
		GS->DistanceToDestination = DistanceToDestination;
		GS->OnRep_DistanceToDestination(); // 리슨 서버 클라이언트 즉시 알림

		if (Pressure)
		{
			GS->PressureLevel = Pressure->CurrentPressure;
			GS->OnRep_PressureLevel(); // 리슨 서버 클라이언트 즉시 알림
		}
	}
}

void AGODTrain::OnRep_bIsDerailed()
{
	if (bIsDerailed)
	{
		OnTrainDerailed.Broadcast();
	}
}
