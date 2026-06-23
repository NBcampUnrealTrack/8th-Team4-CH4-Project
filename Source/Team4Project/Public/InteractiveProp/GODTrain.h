#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GODTrain.generated.h"

class UStaticMeshComponent;

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

	// ============================================================
	// 복제 상태
	// ============================================================
	/** 현재 운행 속도 (cm/s). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Train")
	float TrainSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Train")
	float DistanceToDestination;

	/** 탈선 여부 */
	UPROPERTY(ReplicatedUsing = OnRep_bIsDerailed, BlueprintReadOnly, Category = "Train")
	bool bIsDerailed;

	// ============================================================
	// 블루프린트 이벤트 (UI/이펙트 연결용)
	// ============================================================
	UPROPERTY(BlueprintAssignable, Category = "Train|Events")
	FOnTrainArrived OnTrainArrived;

	UPROPERTY(BlueprintAssignable, Category = "Train|Events")
	FOnTrainDerailed OnTrainDerailed;

	// ============================================================
	// 서버 API
	// ============================================================
	/** 열차 속도 변경  */
	UFUNCTION(BlueprintCallable, Category = "Train")
	void SetTrainSpeed(float NewSpeed);

	/**
	 * 선로 전환 시 호출. 반경 내 물리 오브젝트에만 측면 충격을 가해
	 * 관성 쏠림(화물/플레이어 넉백) 연출. SphereOverlap 사용으로 경량화.
	 */
	UFUNCTION(BlueprintCallable, Category = "Train")
	void ApplyTrackSwitchImpulse(float ImpulseStrength = 800.f, float ImpulseRadius = 2000.f);

	/** 탈선 처리 — GameMode::TriggerDerailment 가 호출 */
	UFUNCTION(BlueprintCallable, Category = "Train")
	void TriggerDerailment();

	// ============================================================
	// 에디터 설정
	// ============================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float DefaultSpeed = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train|Config")
	float TotalDistance = 10000.f;

private:
	UFUNCTION()
	void OnRep_bIsDerailed();

	/** 서버 Tick 마다 GameState.DistanceToDestination 에 값을 밀어 넣음 */
	void SyncDistanceToGameState();
};
