#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "CoalPile.generated.h"

UCLASS()
class TEAM4PROJECT_API ACoalPile : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	ACoalPile();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> PileMesh;

	// 플레이어가 인터랙션 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UBoxComponent> InteractionBox;

	// 꺼낼 때 스폰할 석탄 클래스 (BP_CoalItem 을 디테일에서 지정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoalPile")
	TSubclassOf<class ACoalItem> CoalItemClass;

	// 최대 저장 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoalPile")
	int32 MaxCoalCount = 10;
	
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_CurrentCoalCount, BlueprintReadOnly, Category = "CoalPile")
	int32 CurrentCoalCount = 10;

	// 연속 꺼내기 방지 쿨다운 (초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoalPile")
	float DispenseCooldown = 1.f;

	// 석탄이 배출될 위치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoalPile")
	FVector DispenseOffset = FVector(100.f, 0.f, 50.f);

	// 더미에 석탄 보충 
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CoalPile")
	void AddCoal(int32 Amount = 1);

	
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "CoalPile")
	void OnCoalCountChanged(int32 NewCount, int32 MaxCount);

private:
	UFUNCTION() void OnRep_CurrentCoalCount();

	void SetCoalCount(int32 NewCount);

	// 서버 전용 쿨다운 타임스탬프
	float LastDispenseTime = -1000.f;
};
