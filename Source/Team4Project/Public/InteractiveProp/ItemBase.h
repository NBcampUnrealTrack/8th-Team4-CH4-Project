#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interface/Interactable.h"
#include "ItemBase.generated.h"

class UStaticMeshComponent;

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
};
