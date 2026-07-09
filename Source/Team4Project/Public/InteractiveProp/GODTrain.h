#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GODTrain.generated.h"

class UStaticMeshComponent;
class UFurnanceComponent;
class UPressureComponent;
class AGODTrainTrack;
class UAudioComponent;
class UDataTable;
class UNiagaraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrainArrived);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrainDerailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrainStarted);

UCLASS()
class TEAM4PROJECT_API AGODTrain : public AActor
{
	GENERATED_BODY()

public:
	AGODTrain();

protected:
	virtual void BeginPlay() override;

	// 에디터에서 배치/이동/편집할 때마다 스플라인 0번 지점으로 스냅 →
	// 에디터 미리보기가 런타임 시작 위치(스플라인 0번)와 일치한다.
	virtual void OnConstruction(const FTransform& Transform) override;

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

	// 열차 주행 여부. 게임 시작 전에는 false로 제자리 대기, StartRunning()으로 출발한다.
	UPROPERTY(ReplicatedUsing = OnRep_bIsRunning, BlueprintReadOnly, Category = "Train")
	bool bIsRunning = false;

	// ============================================================
	// 블루프린트 이벤트
	// ============================================================
	UPROPERTY(BlueprintAssignable, Category = "Train|Events")
	FOnTrainArrived OnTrainArrived;

	UPROPERTY(BlueprintAssignable, Category = "Train|Events")
	FOnTrainDerailed OnTrainDerailed;

	// 출발(주행 시작) 시 브로드캐스트 — 경적/출발 연출 등에 바인딩
	UPROPERTY(BlueprintAssignable, Category = "Train|Events")
	FOnTrainStarted OnTrainStarted;

	// ============================================================
	// 서버 API
	// ============================================================
	UFUNCTION(BlueprintCallable, Category = "Train")
	void SetTrainSpeed(float NewSpeed);

	// 서버 전용: 열차 주행 시작 (게임 시작 시 GameMode가 호출). 이후 스플라인을 따라 이동.
	UFUNCTION(BlueprintCallable, Category = "Train")
	void StartRunning();

	// 서버 전용: 열차 주행 정지 (다시 제자리 대기).
	UFUNCTION(BlueprintCallable, Category = "Train")
	void StopRunning();

	UFUNCTION(BlueprintCallable, Category = "Train")
	void ApplyTrackSwitchImpulse(float ImpulseStrength = 800.f, float ImpulseRadius = 2000.f);

	UFUNCTION(BlueprintCallable, Category = "Train")
	void TriggerDerailment();

	// ============================================================
	// 에디터 설정
	// ============================================================
	// 제한 시간 10분(GODGameMode::TotalMatchTime) 기준 도착 시간:
	// 만석(400) 5분 / 고연료(350) 5분43초 / 중간연료(300) 6분40초 / 저연료(250) 8분 /
	// 연료 없이(200) 계속 기어가면 정확히 10분 → 아슬아슬하게 마피아 승
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float TotalDistance = 120000.f;

	// 연료가 없어도 유지되는 최소 속도 (기차는 절대 멈추지 않음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Speed")
	float MinSpeed = 200.f;

	// 연료 가득(Full) 상태 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Speed")
	float MaxSpeed = 400.f;

	// 연료 상태별 목표 속도. Empty=MinSpeed, Full=MaxSpeed, 중간 단계는 아래 값.
	// Tick에서 SpeedInterpRate로 부드럽게 수렴하므로 단계가 튀지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Speed")
	float LowFuelSpeed = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Speed")
	float MediumFuelSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Speed")
	float HighFuelSpeed = 350.f;

	// 현재 속도가 목표 속도로 수렴하는 빠르기 (가/감속 부드러움)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Speed")
	float SpeedInterpRate = 2.f;

	// 압력 폭발 시 속도 페널티
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float ExplosionSpeedPenalty = 200.f;

	// 압력 80% 이상(고압 경고) 시 적용할 속도 배수. 목표 속도에 곱해진다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float HighPressureSpeedMultiplier = 0.5f;

	// 기어가 하나라도 파손됐을 때 적용할 속도 배수. 고압 배수와 곱연산으로 겹친다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float GearBrokenSpeedMultiplier = 0.5f;

	// 모든 기어가 파손되면 열차를 정지시킨다. 기어를 하나라도 수리하면 다시 출발한다.
	// (압력 폭발로 인한 탈선과 달리 게임이 끝나지 않는다)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	bool bHaltWhenAllGearsBroken = true;

	// 연료 부족 방송 임계값 (0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float FuelLowAnnounceThreshold = 0.2f;

	// 남은 거리가 전체의 이 비율 이하로 떨어지면 "목적지 임박" 방송 (0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float NearDestinationRatio = 0.25f;

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

	// ============================================================
	// 흔들림 연출 (순수 비주얼, 각 머신 로컬 계산 · 복제 X)
	// ============================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Shake")
	bool bEnableShake = true;

	// 흔들림 최대 위치 진폭 (cm). X는 진행방향이라 보통 0, Y=좌우, Z=상하
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Shake")
	FVector ShakeLocAmplitude = FVector(0.f, 2.f, 1.5f);

	// 흔들림 최대 회전 진폭 (deg). X=Pitch(끄덕), Y=Roll(좌우 기울기)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Shake")
	FVector2D ShakeRotAmplitude = FVector2D(0.4f, 0.8f);

	// 흔들림 기본 주파수 느낌 (클수록 빠르게 덜컹)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Shake")
	float ShakeFrequency = 6.f;

	// 이 속도에서 진폭이 100%가 된다. 정차에 가까울수록 흔들림이 잦아든다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Shake")
	float ShakeFullSpeed = 1200.f;

	// 칸마다 흔들림 위상을 얼마나 어긋나게 할지 (rad). 클수록 칸별로 따로 덜컹인다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Shake")
	float ShakePhasePerCar = 1.3f;

	// 기관실 피벗. BP에서 이 밑에 메시를 붙인다. 스플라인 따라 이동(선두 기준).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> EngineRoom;

	// C++가 생성한 칸 피벗들 (EngineRoom 다음 Car_0 ~ Car_N).
	// ★ 네가 에디터에서 각 피벗을 옮겨둔 위치(상대 배치) 그대로를 간격으로 사용한다.
	// 각 피벗 밑에 메시를 붙이면, 피벗이 스플라인 따라 움직일 때 자식 메시가 자동으로 따라간다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<USceneComponent>> CarPivots;

	// 열차 위치에서 게임 사운드 DT 행 재생 (전 클라 — 폭발 등 서버 이벤트용)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayTrainSound(FName RowName);
	
	
	// 열차 나이아가라 이펙트 추가 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> VFXSpawnPoint;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|VFX")
	TObjectPtr<UDataTable> VFXTable;

private:
	UFUNCTION() void OnRep_bIsDerailed();
	UFUNCTION() void OnRep_bIsRunning();

	// 게임 사운드 DT — GameState BP 에 지정된 것을 읽어온다 (열차에 별도 지정 불필요).
	const UDataTable* GetGameSoundTable() const;

	// 주행/탈선 상태를 운행 루프음에 반영 (각 머신 로컬).
	void UpdateRunningAudio();

	// 운행 루프음 (Train.Running 행, TrainMesh 부착)
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> RunningAudio;

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

	// 기어가 하나라도 풀렸는지. Tick 앞부분에서 한 번만 계산해 압력/속도 양쪽에 쓴다.
	bool bAnyGearBroken = false;

	// ── 알림 방송 에지 감지 (서버 전용) ──
	// 연료가 한 번이라도 임계값 위로 올라온 뒤에만 부족 경고 (시작 직후 0 초기값 오경보 방지).
	bool bFuelInitialized = false;
	bool bFuelLowAnnounced = false;
	bool bFuelEmptyAnnounced = false;
	bool bNearDestinationAnnounced = false;

	// 매 Tick 연료/거리를 보고 경계를 넘는 순간에만 방송한다.
	void CheckAnnouncementEdges();

	// 레벨의 기어 슬롯 캐시 (서버 전용). 매 Tick TActorIterator 를 돌지 않기 위함.
	UPROPERTY()
	TArray<TObjectPtr<class AGearSlot>> GearSlots;

	// 캐시된 슬롯 중 분해 상태인 개수. 유효한 슬롯 수는 OutValidCount 로 함께 돌려준다.
	int32 CountBrokenGears(int32& OutValidCount) const;

	// 이번 정지가 기어 전멸 때문인지. 압력 폭발로 인한 탈선은 되돌릴 수 없으므로 구분해 둔다.
	bool bDerailedByGears = false;

	// 기어 전멸 → 정지 / 기어 수리 → 최저 속도로 재출발 (둘 다 서버 전용)
	void HaltForBrokenGears();
	void ResumeFromGearHalt();

	void SyncDistanceToGameState();

	// 선두 거리값(LeadDistance) 기준으로 기차(부모)를 스플라인 위에 통째로 배치 (모든 머신에서 호출)
	void UpdateTransformAlongSpline(float LeadDistance);

	// 칸별(피벗별) 절차적 흔들림을 로컬 오프셋으로 적용 (순수 비주얼)
	void UpdateShake(float DeltaTime);

	// 흔들림 누적 시간
	float ShakeTime = 0.f;

	// BeginPlay 시점의 각 피벗 상대 트랜스폼(= 에디터에서 둔 칸 간격).
	// 흔들림은 이 베이스 위에 더해서 적용한다(간격을 덮어쓰지 않도록).
	TArray<FTransform> CarPivotBaseTransforms;

	// 클라이언트 표시용 로컬 거리(보간 대상). 복제값(DistanceAlongSpline)으로 부드럽게 수렴시킨다.
	float LocalDistanceAlongSpline = 0.f;

	// 현재 기차의 대표 월드 속도(선두 접선 × 속력). 점프 impart 폴백용.
	FVector TrainVelocity = FVector::ZeroVector;
	
	void UpdateSmokeVFX();

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SmokeVFX;
};
