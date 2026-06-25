#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

	// 집기 상태 - OnRep에서 물리/콜리전 토글
	UPROPERTY(ReplicatedUsing = OnRep_bIsHeld, BlueprintReadOnly, Category = "Item")
	bool bIsHeld;

	// 현재 들고 있는 캐릭터
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
	ACharacter* HolderCharacter;

	// 부착 소켓 이름 (스켈레톤에 소켓 추가 후 BP에서 재지정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName AttachSocketName = TEXT("RightHand");

	// 집기 - 클라이언트가 호출 → 서버에서 실행
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Item")
	void Server_PickUp(ACharacter* Holder);

	// 놓기
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

	void SetPhysicsForHeld(bool bHeld);
};
