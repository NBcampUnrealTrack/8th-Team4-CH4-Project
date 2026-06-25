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

	// 현재 압력 (0 ~ 100)
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPressure, BlueprintReadOnly, Category = "Pressure")
	float CurrentPressure = 50.f;

	// 연료 활성화 시 초당 압력 상승량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	float PressureRiseRate = 3.f;

	// 연료 없을 때 초당 자연 감소량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	float PressureDecayRate = 1.5f;

	// 경고 구간 진입 임계값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	float WarningThreshold = 70.f;

	// 폭발 임계값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	float ExplosionThreshold = 100.f;

	// FurnaceComponent 활성화 상태 (GODTrain이 매 Tick 설정)
	UPROPERTY(BlueprintReadOnly, Category = "Pressure")
	bool bFurnaceActive = false;

	// 폭발 발생 여부 (재폭발 방지용)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Pressure")
	bool bExploded = false;

	// 밸브 조작으로 감압
	UFUNCTION(BlueprintCallable, Category = "Pressure")
	void ReducePressure(float Amount);

	// 폭발 후 압력 초기화 (BP나 GameMode에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Pressure")
	void ResetAfterExplosion();

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
