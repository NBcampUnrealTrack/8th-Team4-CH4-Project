#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "SinkBase.generated.h"

UCLASS()
class TEAM4PROJECT_API ASinkBase : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	ASinkBase();
	
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USceneComponent> SceneRoot;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> BasinMesh;

	// 플레이어가 인터랙션할 수 있는 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<class UBoxComponent> InteractionBox;
	
	// 사용 중 여부 
	UPROPERTY(ReplicatedUsing = OnRep_IsInUse, BlueprintReadOnly, Category = "WashBasin")
	bool bIsInUse = false;

	// 초당 오염도 감소량 (SootLevel 0~1 기준. 0.25 = 최대 오염에서 4초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WashBasin")
	float CleanPerSecond = 0.25f;

	
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;

protected:
	
	UFUNCTION(BlueprintImplementableEvent, Category = "WashBasin")
	void OnWashingStateChanged(bool bNowInUse);

private:
	UFUNCTION() void OnRep_IsInUse();

	void StartWashing(ACharacter* User);
	void StopWashing();

	
	bool CanKeepWashing(const ACharacter* User) const;

	void SetInUse(bool bNewInUse);
	
	TWeakObjectPtr<ACharacter> WashingUser;
};
