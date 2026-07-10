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

	UFUNCTION() void OnRep_CurrentPressure();
};
