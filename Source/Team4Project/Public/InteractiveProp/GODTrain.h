#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GODTrain.generated.h"

class UStaticMeshComponent;
class UFurnanceComponent;
class UPressureComponent;
class AGODTrainTrack;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrainArrived);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrainDerailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrainOverheatChanged, bool, bOverheated);

UCLASS()
class TEAM4PROJECT_API AGODTrain : public AActor
{
	GENERATED_BODY()

public:
	AGODTrain();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 위에 탄 캐릭터가 점프로 발판을 떠날 때 CMC가 실어줄 "발판 속도"를 제공한다.
	// 기차는 kinematic이라 컴포넌트 속도가 0이므로, CMC는 이 Owner 속도로 폴백한다.
	virtual FVector GetVelocity() const override { return TrainVelocity; }

	// ============================================================
	// 컴포넌트
	// ============================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* TrainMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFurnanceComponent* Furnace;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPressureComponent* Pressure;

	// ============================================================
	// 복제 상태
	// ============================================================
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Train")
	float TrainSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Train")
	float DistanceToDestination;

	// 트랙(스플라인) 위에서의 누적 이동 거리. 이 스칼라만 복제하고
	// 각 머신이 동일 스플라인을 평가해 위치를 재구성한다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Train")
	float DistanceAlongSpline = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_bIsDerailed, BlueprintReadOnly, Category = "Train")
	bool bIsDerailed;

	// 연료 과투입으로 과열된 상태 (적정선 초과). UI/이펙트용
	UPROPERTY(ReplicatedUsing = OnRep_bIsOverheated, BlueprintReadOnly, Category = "Train")
	bool bIsOverheated = false;

	// ============================================================
	// 블루프린트 이벤트
	// ============================================================
	UPROPERTY(BlueprintAssignable, Category = "Train|Events")
	FOnTrainArrived OnTrainArrived;

	UPROPERTY(BlueprintAssignable, Category = "Train|Events")
	FOnTrainDerailed OnTrainDerailed;

	// 과열 상태 진입/해제 시 브로드캐스트 (이펙트/경고음 등)
	UPROPERTY(BlueprintAssignable, Category = "Train|Events")
	FOnTrainOverheatChanged OnTrainOverheatChanged;

	// ============================================================
	// 서버 API
	// ============================================================
	UFUNCTION(BlueprintCallable, Category = "Train")
	void SetTrainSpeed(float NewSpeed);

	UFUNCTION(BlueprintCallable, Category = "Train")
	void ApplyTrackSwitchImpulse(float ImpulseStrength = 800.f, float ImpulseRadius = 2000.f);

	UFUNCTION(BlueprintCallable, Category = "Train")
	void TriggerDerailment();

	// ============================================================
	// 에디터 설정
	// ============================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float TotalDistance = 10000.f;

	// 연료가 없어도 유지되는 최소 속도 (기차는 절대 멈추지 않음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Speed")
	float MinSpeed = 200.f;

	// 적정 연료 구간에서 도달하는 최고 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Speed")
	float MaxSpeed = 1500.f;

	// 과열(연료 과투입) 시 떨어지는 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Speed")
	float OverheatedSpeed = 400.f;

	// MaxSpeed에 도달하는 연료 비율(0~1). 이보다 더 채우면 과열 구간으로 진입.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Speed", meta = (ClampMin = "0.05", ClampMax = "0.95"))
	float OptimalFuelRatio = 0.6f;

	// 현재 속도가 목표 속도로 수렴하는 빠르기 (가/감속 부드러움)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Speed")
	float SpeedInterpRate = 2.f;

	// 압력 폭발 시 속도 페널티
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float ExplosionSpeedPenalty = 200.f;

	// 압력 80% 이상(고압 경고) 시 적용할 속도 배수. 목표 속도에 곱해진다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float HighPressureSpeedMultiplier = 0.5f;

	// ============================================================
	// 트랙 이동
	// ============================================================
	// 기차가 따라 달릴 루프 트랙. 레벨에서 배치한 인스턴스를 지정한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Train|Track")
	AGODTrainTrack* Track;

	// 위치를 yaw(좌우 방향)만 적용할지 여부.
	// 스플라인 자체의 roll/pitch까지 따라가게 하려면 false.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Track")
	bool bYawOnly = true;

	// 기관실 피벗. BP에서 이 밑에 메시를 붙인다. 스플라인 따라 이동(선두 기준).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> EngineRoom;

	// C++가 생성한 칸 피벗들 (EngineRoom 다음 Car_0 ~ Car_N).
	// ★ 네가 에디터에서 각 피벗을 옮겨둔 위치(상대 배치) 그대로를 간격으로 사용한다.
	// 각 피벗 밑에 메시를 붙이면, 피벗이 스플라인 따라 움직일 때 자식 메시가 자동으로 따라간다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<USceneComponent>> CarPivots;

private:
	UFUNCTION() void OnRep_bIsDerailed();
	UFUNCTION() void OnRep_bIsOverheated();

	// 현재 화로 연료 비율로부터 목표 속도를 계산
	float ComputeTargetSpeed() const;

	// 화로 이벤트 핸들러
	UFUNCTION() void OnFurnaceActivated();
	UFUNCTION() void OnFurnaceDeactivated();
	UFUNCTION() void OnPressureExploded();

	// 압력 경고 핸들러 (80% 진입/해제) — 고압 상태 플래그를 토글한다.
	UFUNCTION() void OnPressureWarningStarted();
	UFUNCTION() void OnPressureWarningEnded();

	// 고압 경고 중인지 여부. 목표 속도에 HighPressureSpeedMultiplier 를 적용한다.
	bool bHighPressure = false;

	void SyncDistanceToGameState();

	// 선두 거리값(LeadDistance) 기준으로 기차(부모)를 스플라인 위에 통째로 배치 (모든 머신에서 호출)
	void UpdateTransformAlongSpline(float LeadDistance);

	// 클라이언트 표시용 로컬 거리(보간 대상). 복제값으로 부드럽게 수렴시킨다.
	float LocalDistanceAlongSpline = 0.f;

	// 현재 기차의 대표 월드 속도(선두 접선 × 속력). 점프 impart 폴백용.
	FVector TrainVelocity = FVector::ZeroVector;
};
