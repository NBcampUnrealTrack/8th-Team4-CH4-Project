#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractComponent.generated.h"

class USphereComponent;

// 최근접 인터랙션 대상이 바뀌거나 프롬프트 문구가 바뀔 때 알림.
// NewTarget == nullptr 이면 대상 없음 (UI 숨김).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractTargetChanged, AActor*, NewTarget, FText, Prompt);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TEAM4PROJECT_API UInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// f키 입력 시 호출
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	// HUD 힌트 텍스트
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	FText GetCurrentPrompt() const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool HasInteractable() const { return GetClosestInteractable() != nullptr; }

	// 범위 안의 IInteractable 중 가장 가까운 것 반환.
	// (월드 마커 UI가 대상 위치를 얻을 때도 사용 — BlueprintPure)
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetClosestInteractable() const;

	// 대상/프롬프트가 바뀔 때만 발화. 프롬프트 UI는 여기 바인딩하면
	// 매 프레임 폴링(UMG 틱 바인딩) 없이 갱신된다. 로컬 플레이어에서만 돈다.
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractTargetChanged OnInteractTargetChanged;

	// 대상 탐지 주기(초). 반응성과 비용의 트레이드오프.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float TargetCheckInterval = 0.15f;

	// 범위 안의 IInteractable 중 특정 클래스에 해당하는 가장 가까운 것 반환 (GA 직접 타겟팅용)
	AActor* GetClosestInteractableOfClass(TSubclassOf<AActor> ActorClass) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractRadius = 200.f;

	// 연속 발동 방지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractCooldown = 0.5f;

private:
	// 타이머 콜백: 최근접 대상/프롬프트를 재계산해 바뀌었을 때만 브로드캐스트
	void CheckTargetChanged();

	FTimerHandle TargetCheckTimer;
	TWeakObjectPtr<AActor> LastTarget;
	FText LastPrompt;

	float LastInteractTime = -FLT_MAX;
	UPROPERTY()
	USphereComponent* InteractSphere;

	// 범위 안의 IInteractable 액터 목록
	UPROPERTY()
	TArray<AActor*> NearbyInteractables;

	// 현재 손에 든 아이템(있다면). 손에 든 아이템은 자기 자신과 가까이 있어
	// 다른 프롭보다 우선 선택될 수 있으므로 후보에서 제외한다.
	AActor* GetHeldItemActor() const;

	UFUNCTION(Server, Reliable)
	void Server_TryInteract();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
