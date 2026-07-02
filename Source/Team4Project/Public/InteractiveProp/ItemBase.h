#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interface/Interactable.h"
#include "ItemBase.generated.h"

class UStaticMeshComponent;
class UPhysicalMaterial;

UCLASS()
class TEAM4PROJECT_API AItemBase : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AItemBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;
	
	UPROPERTY(ReplicatedUsing = OnRep_bIsHeld, BlueprintReadOnly, Category = "Item")
	bool bIsHeld;
	
	UPROPERTY(ReplicatedUsing = OnRep_HolderCharacter, BlueprintReadOnly, Category = "Item")
	TObjectPtr<ACharacter> HolderCharacter;

	// 부착 소켓 이름 (스켈레톤에 소켓 추가 후 BP에서 재지정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName AttachSocketName = TEXT("RightHand");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FGameplayTag EquipStateTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float WeightAmount = 0.f;

	// ── 떨궜을 때 물리 (굴러감 억제) ──
	// 각 감쇠: 회전(구르기)을 감쇠한다. 둥근 물체가 안 굴러가게 하는 핵심 값.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Physics")
	float DroppedAngularDamping = 0.f;

	// 선형 감쇠: 미끄러짐(이동)을 감쇠한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Physics")
	float DroppedLinearDamping = 0.01f;

	// 떨궜을 때 적용할 물리 머티리얼(마찰). 지정하면 콜리전 복구와 함께 적용된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Physics")
	TObjectPtr<UPhysicalMaterial> DroppedPhysMaterial;

	// ── 안전 드롭 (캐릭터와 겹쳐 튕기지 않도록 내려놓는 위치) ──
	// 캐릭터 앞쪽으로 얼마나 떨어뜨릴지(캡슐 반경보다 커야 겹침이 안 남).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Drop")
	float DropForwardOffset = 60.f;

	// 살짝 위로 띄워서 바닥/발판 관통을 피한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Drop")
	float DropUpOffset = 20.f;

	// 버릴 때 앞으로 던지는 힘(임펄스, kg·cm/s). 질량(WeightAmount)으로 나뉘어 속도가 되므로
	// 무거운 아이템일수록 적게 날아간다(무게 비례). 열차 속도 위에 더해진다. 0이면 그 자리에 놓임.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Drop")
	float DropTossImpulse = 2000.f;
	
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Item")
	void Server_PickUp(ACharacter* Holder);
	
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Item")
	void Server_Drop();

	// IInteractable
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;

protected:
	virtual void Server_PickUp_Implementation(ACharacter* Holder);
	virtual void Server_Drop_Implementation();

private:
	UFUNCTION()
	void OnRep_bIsHeld();

	UFUNCTION()
	void OnRep_HolderCharacter();

	void SetPhysicsForHeld(bool bHeld);

	void RefreshHeldAttachment();

	// 캐릭터와 겹쳐 튕기지 않게 안전한 위치로 내려놓고, 달리는 열차 속도를 상속시킨다. (서버 전용)
	void DropSafely(ACharacter* Dropper);
};
