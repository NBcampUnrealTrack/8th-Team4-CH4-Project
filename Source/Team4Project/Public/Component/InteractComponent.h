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

	UFUNCTION(Server, Reliable)
	void Server_TryInteract();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	AActor* GetClosestInteractable() const;
};
