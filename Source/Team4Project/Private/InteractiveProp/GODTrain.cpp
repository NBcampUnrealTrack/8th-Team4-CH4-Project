#include "InteractiveProp/GODTrain.h"
#include "InteractiveProp/GODTrainTrack.h"
#include "Component/FurnanceComponent.h"
#include "Component/PressureComponent.h"
#include "Game/GODGameState.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/KismetSystemLibrary.h"

// 기차 칸(피벗) 개수. BP에서 각 피벗(Car_0~) 밑에 메시를 붙인다.
static constexpr int32 GTrainNumCars = 6;

AGODTrain::AGODTrain()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	// 엔진 기본 Transform 복제는 끈다. 위치는 복제된 DistanceAlongSpline로
	// 각 머신이 스플라인을 평가해 재구성한다 (대역폭 최소화).
	SetReplicateMovement(false);

	TrainMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrainMesh"));
	RootComponent = TrainMesh;
	// 캐릭터가 "움직이는 발판"으로 인식하려면 바닥 메시가 Movable이어야 한다.
	TrainMesh->SetMobility(EComponentMobility::Movable);

	// 기관실 + 칸 피벗 SceneComponent를 미리 생성한다. BP에서 각 피벗 밑에 메시를 붙이면
	// 피벗이 스플라인 따라 움직일 때 자식 메시가 자동으로 따라간다.
	// 간격은 에디터에서 둔 피벗 위치에서 BeginPlay 때 자동으로 가져온다(별도 보정 없음).
	EngineRoom = CreateDefaultSubobject<USceneComponent>(TEXT("EngineRoom"));
	EngineRoom->SetupAttachment(RootComponent);
	EngineRoom->SetMobility(EComponentMobility::Movable);
	CarPivots.Add(EngineRoom);

	for (int32 i = 0; i < GTrainNumCars; ++i)
	{
		const FName PivotName(*FString::Printf(TEXT("Car_%d"), i));
		USceneComponent* Pivot = CreateDefaultSubobject<USceneComponent>(PivotName);
		Pivot->SetupAttachment(RootComponent);
		Pivot->SetMobility(EComponentMobility::Movable);
		CarPivots.Add(Pivot);
	}

	Furnace = CreateDefaultSubobject<UFurnanceComponent>(TEXT("Furnace"));
	Pressure = CreateDefaultSubobject<UPressureComponent>(TEXT("Pressure"));
}

void AGODTrain::BeginPlay()
{
	Super::BeginPlay();

	LocalDistanceAlongSpline = DistanceAlongSpline;

	if (HasAuthority())
	{
		TrainSpeed = MinSpeed;
		DistanceToDestination = TotalDistance;
		DistanceAlongSpline = 0.f;
		bIsDerailed = false;
		bIsOverheated = false;
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

	const float SplineLen = (Track && Track->Spline) ? Track->Spline->GetSplineLength() : 0.f;

	// ----- 서버: 게임 로직 + 권위적 거리 적분 -----
	if (HasAuthority())
	{
		if (!bIsDerailed)
		{
			// 연료 비율에 따른 목표 속도로 부드럽게 수렴
			// (연료↑ → 가속, 적정선 초과 → 과열로 감속, 연료 0이어도 MinSpeed 유지)
			// 고압 경고 중이면 그 위에 배수 페널티를 곱한다.
			float TargetSpeed = ComputeTargetSpeed();
			if (bHighPressure)
			{
				TargetSpeed *= HighPressureSpeedMultiplier;
			}
			TrainSpeed = FMath::FInterpTo(TrainSpeed, TargetSpeed, DeltaTime, SpeedInterpRate);

			// 과열 상태 갱신 (적정 연료 비율 초과 시)
			if (Furnace && Furnace->MaxFuel > 0.f)
			{
				const float FuelRatio = Furnace->CurrentFuel / Furnace->MaxFuel;
				const bool bNowOverheated = FuelRatio > OptimalFuelRatio;
				if (bNowOverheated != bIsOverheated)
				{
					bIsOverheated = bNowOverheated;
					OnRep_bIsOverheated();
				}
			}

			// 압력 컴포넌트에 연료 상태 전달 (석탄 연소 + 주행 중일 때만 압력 상승)
			if (Pressure && Furnace)
			{
				Pressure->bFurnaceActive = Furnace->bIsBurning && TrainSpeed > 0.f;
			}

			// 게임플레이용 목적지 카운트다운
			DistanceToDestination = FMath::Max(0.f, DistanceToDestination - TrainSpeed * DeltaTime);
			SyncDistanceToGameState();

			// 트랙 위 거리 누적 (닫힌 루프이므로 길이로 wrap)
			if (SplineLen > 0.f)
			{
				DistanceAlongSpline = FMath::Fmod(DistanceAlongSpline + TrainSpeed * DeltaTime, SplineLen);
			}
		}
		LocalDistanceAlongSpline = DistanceAlongSpline;
	}
	// ----- 클라이언트: 로컬 적분 후 복제값으로 부드럽게 보정 -----
	else if (SplineLen > 0.f)
	{
		LocalDistanceAlongSpline = FMath::Fmod(LocalDistanceAlongSpline + TrainSpeed * DeltaTime, SplineLen);

		// 닫힌 루프에서의 최단 부호 거리차 (-L/2, L/2]
		float Delta = FMath::Fmod(DistanceAlongSpline - LocalDistanceAlongSpline + SplineLen * 1.5f, SplineLen) - SplineLen * 0.5f;

		// 큰 차이는 스냅, 작은 차이는 점진 수렴
		if (FMath::Abs(Delta) > SplineLen * 0.25f)
		{
			LocalDistanceAlongSpline = DistanceAlongSpline;
		}
		else
		{
			LocalDistanceAlongSpline = FMath::Fmod(LocalDistanceAlongSpline + Delta * FMath::Clamp(10.f * DeltaTime, 0.f, 1.f) + SplineLen, SplineLen);
		}
	}

	// ----- 모든 머신: 거리값으로 위치 갱신 -----
	if (SplineLen > 0.f)
	{
		UpdateTransformAlongSpline(LocalDistanceAlongSpline);
	}
}

void AGODTrain::UpdateTransformAlongSpline(float LeadDistance)
{
	if (!Track || !Track->Spline) return;

	USplineComponent* Spline = Track->Spline;
	const float Len = Spline->GetSplineLength();
	if (Len <= 0.f) return;

	float D = FMath::Fmod(LeadDistance, Len);
	if (D < 0.f) D += Len;

	const FTransform RailT = Spline->GetTransformAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
	FRotator R = RailT.Rotator();
	if (bYawOnly) { R.Pitch = 0.f; R.Roll = 0.f; }

	// 부모(TrainMesh 루트)를 통째로 이동 → 자식 칸들이 조립된 그대로 붙어서 따라온다(틈 없음).
	// 크기·간격 등 네가 설정한 값은 하나도 바꾸지 않는다.
	SetActorLocationAndRotation(RailT.GetLocation(), R);

	TrainVelocity = R.Vector() * TrainSpeed;
}

void AGODTrain::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGODTrain, TrainSpeed);
	DOREPLIFETIME(AGODTrain, bIsDerailed);
	DOREPLIFETIME(AGODTrain, bIsOverheated);
	DOREPLIFETIME(AGODTrain, DistanceAlongSpline);
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

float AGODTrain::ComputeTargetSpeed() const
{
	// 화로가 없으면 최소 속도로만 주행
	if (!Furnace || Furnace->MaxFuel <= 0.f)
	{
		return MinSpeed;
	}

	const float FuelRatio = FMath::Clamp(Furnace->CurrentFuel / Furnace->MaxFuel, 0.f, 1.f);

	if (FuelRatio <= OptimalFuelRatio)
	{
		// 0 ~ 적정선: MinSpeed → MaxSpeed (석탄 넣을수록 가속)
		const float Alpha = (OptimalFuelRatio > 0.f) ? (FuelRatio / OptimalFuelRatio) : 1.f;
		return FMath::Lerp(MinSpeed, MaxSpeed, Alpha);
	}

	// 적정선 ~ 가득: MaxSpeed → OverheatedSpeed (과투입 → 과열로 감속)
	const float Alpha = (OptimalFuelRatio < 1.f) ? ((FuelRatio - OptimalFuelRatio) / (1.f - OptimalFuelRatio)) : 1.f;
	return FMath::Lerp(MaxSpeed, OverheatedSpeed, Alpha);
}

void AGODTrain::OnFurnaceActivated()
{
	// 속도는 ComputeTargetSpeed로 매 틱 계산되므로 여기선 이펙트 훅으로만 사용
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

void AGODTrain::OnPressureWarningStarted()
{
	// 고압 진입: 목표 속도가 배수만큼 줄어든다(Tick에서 부드럽게 감속).
	bHighPressure = true;
}

void AGODTrain::OnPressureWarningEnded()
{
	// 고압 해제: 다시 정상 목표 속도로 복귀.
	bHighPressure = false;
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

void AGODTrain::OnRep_bIsOverheated()
{
	OnTrainOverheatChanged.Broadcast(bIsOverheated);
}
