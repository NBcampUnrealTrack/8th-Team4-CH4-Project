#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractComponent.generated.h"

class USphereComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TEAM4PROJECT_API UInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractComponent();

	virtual void BeginPlay() override;

	// f키 입력 시 호출
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	// HUD 힌트 텍스트
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	FText GetCurrentPrompt() const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool HasInteractable() const { return GetClosestInteractable() != nullptr; }

	// 범위 안의 IInteractable 중 가장 가까운 것 반환
	AActor* GetClosestInteractable() const;

	// 범위 안의 IInteractable 중 특정 클래스에 해당하는 가장 가까운 것 반환 (GA 직접 타겟팅용)
	AActor* GetClosestInteractableOfClass(TSubclassOf<AActor> ActorClass) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractRadius = 200.f;

	// 연속 발동 방지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractCooldown = 0.5f;

private:
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
