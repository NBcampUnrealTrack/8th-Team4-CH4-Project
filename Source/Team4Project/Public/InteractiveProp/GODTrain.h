#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GODTrain.generated.h"

class UStaticMeshComponent;
class UFurnanceComponent;
class UPressureComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrainArrived);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrainDerailed);

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

	UPROPERTY(ReplicatedUsing = OnRep_bIsDerailed, BlueprintReadOnly, Category = "Train")
	bool bIsDerailed;

	// ============================================================
	// 블루프린트 이벤트
	// ============================================================
	UPROPERTY(BlueprintAssignable, Category = "Train|Events")
	FOnTrainArrived OnTrainArrived;

	UPROPERTY(BlueprintAssignable, Category = "Train|Events")
	FOnTrainDerailed OnTrainDerailed;

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
	float DefaultSpeed = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float TotalDistance = 10000.f;

	// 연료 소진 시 초당 감속량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float Deceleration = 100.f;

	// 압력 폭발 시 속도 페널티
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float ExplosionSpeedPenalty = 200.f;

private:
	UFUNCTION() void OnRep_bIsDerailed();

	// 화로 이벤트 핸들러
	UFUNCTION() void OnFurnaceActivated();
	UFUNCTION() void OnFurnaceDeactivated();
	UFUNCTION() void OnPressureExploded();

	void SyncDistanceToGameState();
};
