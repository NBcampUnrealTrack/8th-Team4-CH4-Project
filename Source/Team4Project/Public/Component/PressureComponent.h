#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PressureComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPressureEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPressureChanged, float, PressurePercent);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TEAM4PROJECT_API UPressureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPressureComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 현재 압력 (0 ~ 100). 게임 시작 전에는 0에서 대기, 운행 시작 후 오른다.
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPressure, BlueprintReadOnly, Category = "Pressure")
	float CurrentPressure = 0.f;

	// 주행 중 초당 압력 상승량 (기본). 석탄 연소 중이면 FurnaceRiseMultiplier 배로 오른다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	float PressureRiseRate = 0.333333f;

	// 석탄 연소(가속) 중 압력 상승 배수.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	float FurnaceRiseMultiplier = 2.f;

	// 연료 없을 때 초당 자연 감소량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	float PressureDecayRate = 1.5f;

	// 속도 저하 경고 임계값 (80% 이상 시 열차 감속)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	float WarningThreshold = 80.f;

	// 폭발 임계값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	float ExplosionThreshold = 100.f;

	// FurnaceComponent 활성화 상태 (GODTrain이 매 Tick 설정) — 압력 상승률 2배 조건.
	UPROPERTY(BlueprintReadOnly, Category = "Pressure")
	bool bFurnaceActive = false;

	// 열차 주행 여부 (GODTrain이 매 Tick 설정). 주행 중에만 압력이 오른다.
	UPROPERTY(BlueprintReadOnly, Category = "Pressure")
	bool bTrainRunning = false;

	// 폭발 발생 여부 (재폭발 방지용)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Pressure")
	bool bExploded = false;

	// 폭발 후 밸브 조작 금지 쿨타임(초). 마피아가 밸브를 연속으로 터뜨려 무한 감속하는 것을 막는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	float ValveCooldownAfterExplosion = 60.f;

	// 폭발 직후 쿨타임 진행 중 여부 (복제). 이 동안 밸브 미니게임을 시작할 수 없다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Pressure")
	bool bValveOnCooldown = false;

	// 쿨타임이 끝나는 서버 동기 시각(GameState 서버 월드시간 기준, 복제). 남은 시간 표시용.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Pressure")
	float ValveCooldownEndTime = 0.f;

	// 지금 밸브가 쿨타임(폭발 직후)이라 조작 불가인가. 밸브가 IsUsableNow 에서 참조.
	UFUNCTION(BlueprintPure, Category = "Pressure")
	bool IsValveOnCooldown() const { return bValveOnCooldown; }

	// 쿨타임 남은 시간(초). 쿨타임이 아니면 0. 서버/클라 모두 GameState 동기 시각으로 계산.
	UFUNCTION(BlueprintPure, Category = "Pressure")
	float GetValveCooldownRemaining() const;

	// 긴급 회의 등으로 압력 시뮬레이션 전체 정지 (서버 전용).
	// bTrainRunning=false 만으로는 감소(PressureDecayRate)가 계속되므로 별도 플래그가 필요하다.
	UPROPERTY(BlueprintReadOnly, Category = "Pressure")
	bool bFrozen = false;

	// 밸브 조작으로 감압
	UFUNCTION(BlueprintCallable, Category = "Pressure")
	void ReducePressure(float Amount);

	// 폭발 후 압력 초기화 (BP나 GameMode에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Pressure")
	void ResetAfterExplosion();

	// 게임 시작 시 완전 초기화.
	UFUNCTION(BlueprintCallable, Category = "Pressure")
	void ResetForNewGame();

	// 미니게임 실패 등으로 즉시 폭발을 강제 (기존 Tick 폭발과 동일한 파이프라인 사용)
	UFUNCTION(BlueprintCallable, Category = "Pressure")
	void ForceExplode();

	// 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Pressure|Events")
	FOnPressureChanged OnPressureChanged;

	// 경고 구간 진입
	UPROPERTY(BlueprintAssignable, Category = "Pressure|Events")
	FOnPressureEvent OnPressureWarning;

	// 경고 구간 해제 (정상 복귀)
	UPROPERTY(BlueprintAssignable, Category = "Pressure|Events")
	FOnPressureEvent OnPressureRecovered;

	// 폭발 발생
	UPROPERTY(BlueprintAssignable, Category = "Pressure|Events")
	FOnPressureEvent OnPressureExplode;

private:
	bool bWasWarning = false;

	// 폭발 시 호출 — 밸브 조작 금지 쿨타임 시작 (서버 전용).
	void StartValveCooldown();
	void EndValveCooldown();

	FTimerHandle ValveCooldownTimer;

	UFUNCTION() void OnRep_CurrentPressure();
};
