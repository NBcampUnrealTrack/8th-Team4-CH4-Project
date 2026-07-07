#include "InteractiveProp/GODTrain.h"
#include "InteractiveProp/GODTrainTrack.h"
#include "Component/FurnanceComponent.h"
#include "Component/PressureComponent.h"
#include "Game/GODGameState.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/GameSoundStatics.h"
#include "Sound/GameSoundTypes.h"

// 기차 칸(피벗) 개수. BP에서 각 피벗(Car_0~) 밑에 메시를 붙인다.
static constexpr int32 GTrainNumCars = 6;

AGODTrain::AGODTrain()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	// 위치는 표준 이동 복제 대신, 복제된 DistanceAlongSpline로 각 머신이 스플라인을
	// 평가해 "부드럽게" 재구성한다 (일반 액터 이동 복제는 클라에서 보간이 없어 끊긴다).
	// based movement는 발판 기준 "상대 좌표"로 복제되므로, 클라 열차 위치가 서버와
	// 미세하게 달라도 라이더는 발판 위 정확한 상대 위치에 배치된다(라이더 안전).
	SetReplicateMovement(false);
	// 열차는 핵심 게임플레이 액터이므로 항상 관련(멀리 있어도 TrainSpeed/거리값 복제 유지).
	bAlwaysRelevant = true;

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

	// 에디터에서 둔 각 피벗의 상대 트랜스폼(칸 간격)을 캐시해 흔들림의 베이스로 쓴다.
	CarPivotBaseTransforms.Reset(CarPivots.Num());
	for (const TObjectPtr<USceneComponent>& Pivot : CarPivots)
	{
		CarPivotBaseTransforms.Add(Pivot ? Pivot->GetRelativeTransform() : FTransform::Identity);
	}

	if (HasAuthority())
	{
		TrainSpeed = MinSpeed;
		DistanceToDestination = TotalDistance;
		DistanceAlongSpline = 0.f;
		LocalDistanceAlongSpline = 0.f;
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

	// BeginPlay 시점에 스플라인 0번 지점으로 즉시 스냅.
	// StartRunning() 이후 첫 Tick에서 UpdateTransformAlongSpline(0) 이 호출될 때
	// 열차가 에디터 배치 위치에서 스플라인 0번 지점으로 순간이동하는 현상을 방지한다.
	if (Track && Track->Spline)
	{
		UpdateTransformAlongSpline(LocalDistanceAlongSpline);
	}
}

void AGODTrain::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터에서 열차를 배치/이동/편집할 때마다 스플라인 0번 지점으로 스냅한다.
	// 런타임 시작 시 가는 위치(스플라인 0번)와 에디터 미리보기를 일치시켜,
	// 시작 순간 순간이동처럼 보이는 현상을 없앤다.
	// Track이 아직 지정되지 않았으면 아무것도 하지 않는다(어느 트랙 기준인지 모름).
	if (Track && Track->Spline)
	{
		UpdateTransformAlongSpline(0.f);
	}
}

void AGODTrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 서버: 압력 컴포넌트에 주행/화로 상태 전달.
	// 주행 중에만 압력이 오르고(출발 전에는 0 유지), 석탄 연소 중이면 상승률이 배가된다.
	if (HasAuthority() && Pressure)
	{
		Pressure->bTrainRunning = bIsRunning && !bIsDerailed;
		Pressure->bFurnaceActive = Furnace && Furnace->bIsBurning;
	}

	// 출발 전에는 제자리 대기. 게임 시작 시 서버가 StartRunning()을 호출하면 주행한다.
	// (bIsRunning은 복제되므로 클라도 동시에 움직이기 시작한다.)
	if (!bIsRunning)
	{
		return;
	}

	const float SplineLen = (Track && Track->Spline) ? Track->Spline->GetSplineLength() : 0.f;

	// ----- 서버: 게임 로직 + 권위적 거리 적분 -----
	if (HasAuthority())
	{
		if (!bIsDerailed)
		{
			// 연료량에 따른 목표 속도로 부드럽게 수렴
			// (연료↑ → 가속, 연료 0이어도 MinSpeed 유지)
			// 고압 경고 중이면 그 위에 배수 페널티를 곱한다.
			float TargetSpeed = ComputeTargetSpeed();
			if (bHighPressure)
			{
				TargetSpeed *= HighPressureSpeedMultiplier;
			}
			TrainSpeed = FMath::FInterpTo(TrainSpeed, TargetSpeed, DeltaTime, SpeedInterpRate);

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
	// ----- 클라이언트: 로컬 적분 후 복제값으로 부드럽게 보정 (끊김 없는 이동) -----
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

	// ----- 모든 머신: 칸별 흔들림 연출 (비주얼 전용, 로컬 계산) -----
	UpdateShake(DeltaTime);
}

void AGODTrain::UpdateShake(float DeltaTime)
{
	if (!bEnableShake)
	{
		return;
	}

	ShakeTime += DeltaTime;

	// 속도 비례 세기 (정차 시 0, ShakeFullSpeed에서 1). 급정거/가속해도 부드럽게.
	float Intensity = FMath::Clamp(TrainSpeed / FMath::Max(1.f, ShakeFullSpeed), 0.f, 1.f);
	if (bIsDerailed)
	{
		Intensity = 0.f; // 탈선 연출은 별도 처리
	}

	// 진폭이 0이면 흔들림 없이 베이스(에디터에서 둔 간격)로 복귀
	const bool bZero = (Intensity <= KINDA_SMALL_NUMBER);

	// BeginPlay 캐시가 아직 없으면(엣지 케이스) 건드리지 않는다.
	if (CarPivotBaseTransforms.Num() != CarPivots.Num())
	{
		return;
	}

	for (int32 i = 0; i < CarPivots.Num(); ++i)
	{
		USceneComponent* Pivot = CarPivots[i];
		if (!Pivot)
		{
			continue;
		}

		const FTransform& Base = CarPivotBaseTransforms[i];

		if (bZero)
		{
			// 흔들림 오프셋 없이 베이스 상대 트랜스폼 그대로.
			Pivot->SetRelativeLocation(Base.GetLocation());
			Pivot->SetRelativeRotation(Base.Rotator());
			continue;
		}

		// 칸 인덱스마다 위상을 어긋나게 해서 칸별로 따로 덜컹이게 한다.
		const float Phase = i * ShakePhasePerCar;
		const float T = ShakeTime * ShakeFrequency + Phase;

		// 주파수를 살짝 어긋나게 겹쳐 반복감 제거 (기차 특유의 불규칙 롤링)
		const float Bounce = FMath::Sin(T * 2.13f) * 0.6f + FMath::Sin(T * 5.7f) * 0.4f; // 상하
		const float Sway   = FMath::Sin(T * 1.70f) * 0.7f + FMath::Sin(T * 3.9f) * 0.3f; // 좌우
		const float RollN  = FMath::Sin(T * 1.90f);
		const float PitchN = FMath::Sin(T * 2.60f);

		const FVector LocOffset(
			0.f,
			Sway   * ShakeLocAmplitude.Y * Intensity,
			Bounce * ShakeLocAmplitude.Z * Intensity);

		const FRotator RotOffset(
			PitchN * ShakeRotAmplitude.X * Intensity, // Pitch
			0.f,                                       // Yaw는 진행방향이라 건드리지 않음
			RollN  * ShakeRotAmplitude.Y * Intensity); // Roll

		// 베이스(칸 간격) 위에 흔들림을 더한다.
		Pivot->SetRelativeLocation(Base.GetLocation() + LocOffset);
		Pivot->SetRelativeRotation(Base.Rotator() + RotOffset);
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
	SetActorLocationAndRotation(RailT.GetLocation(), R, true);

	TrainVelocity = R.Vector() * TrainSpeed;
}

void AGODTrain::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGODTrain, TrainSpeed);
	DOREPLIFETIME(AGODTrain, bIsDerailed);
	DOREPLIFETIME(AGODTrain, bIsRunning);
	DOREPLIFETIME(AGODTrain, DistanceAlongSpline);
}

void AGODTrain::SetTrainSpeed(float NewSpeed)
{
	if (!HasAuthority()) return;
	TrainSpeed = FMath::Max(0.f, NewSpeed);
}

void AGODTrain::StartRunning()
{
	if (!HasAuthority() || bIsRunning) return;

	bIsRunning = true;
	TrainSpeed = MinSpeed;      // 출발은 최소 속도부터
	OnRep_bIsRunning();         // 리슨 서버(호스트) 즉시 반영
}

void AGODTrain::StopRunning()
{
	if (!HasAuthority() || !bIsRunning) return;

	bIsRunning = false;
	TrainSpeed = 0.f;
	OnRep_bIsRunning();
}

void AGODTrain::OnRep_bIsRunning()
{
	if (bIsRunning)
	{
		OnTrainStarted.Broadcast();
	}

	UpdateRunningAudio();
}

const UDataTable* AGODTrain::GetGameSoundTable() const
{
	const AGODGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGODGameState>() : nullptr;
	return GS ? GS->GameSoundTable.Get() : nullptr;
}

void AGODTrain::UpdateRunningAudio()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const bool bShouldPlay = bIsRunning && !bIsDerailed;

	if (bShouldPlay && !RunningAudio)
	{
		RunningAudio = UGameSoundStatics::SpawnSoundAttachedFromTable(
			GetGameSoundTable(), SoundRows::TrainRunning, TrainMesh);
	}
	else if (!bShouldPlay && RunningAudio)
	{
		RunningAudio->FadeOut(1.f, 0.f);
		RunningAudio = nullptr;
	}
}

void AGODTrain::Multicast_PlayTrainSound_Implementation(FName RowName)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	UGameSoundStatics::PlaySoundAtLocationFromTable(this, GetGameSoundTable(), RowName, GetActorLocation());
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
	if (!Furnace)
	{
		return MinSpeed;
	}

	// 연료 잔량 구간(상태)에 따라 목표 속도가 단계적으로 달라진다.
	// (실제 속도는 Tick에서 SpeedInterpRate로 부드럽게 수렴)
	switch (Furnace->GetFuelState())
	{
	case EFuelState::Empty:  return MinSpeed;
	case EFuelState::Low:    return LowFuelSpeed;
	case EFuelState::Medium: return MediumFuelSpeed;
	case EFuelState::High:   return HighFuelSpeed;
	case EFuelState::Full:   return MaxSpeed;
	default:                 return MinSpeed;
	}
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

	// 폭발음 (서버에서만 바인딩되는 델리게이트라 멀티캐스트로 전 클라 재생)
	Multicast_PlayTrainSound(SoundRows::TrainExplosion);
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

		if (Furnace && Furnace->MaxFuel > 0.f)
		{
			GS->FuelLevel = Furnace->CurrentFuel / Furnace->MaxFuel;
			GS->OnRep_FuelLevel(); // 리슨 서버 클라이언트 즉시 알림
		}
	}
}

void AGODTrain::OnRep_bIsDerailed()
{
	if (bIsDerailed)
	{
		OnTrainDerailed.Broadcast();

		// 탈선음 — OnRep 은 서버(수동 호출)와 클라(복제) 모두에서 실행되므로 각자 로컬 재생.
		if (GetNetMode() != NM_DedicatedServer)
		{
			UGameSoundStatics::PlaySoundAtLocationFromTable(
				this, GetGameSoundTable(), SoundRows::TrainDerail, GetActorLocation());
		}
	}

	UpdateRunningAudio();
}
