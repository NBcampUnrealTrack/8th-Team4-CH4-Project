#pragma once

#include "CoreMinimal.h"
#include "InteractiveProp/ItemBase.h"
#include "PickupGear.generated.h"

class AGearSlot;

UCLASS()
class TEAM4PROJECT_API APickupGear : public AItemBase
{
	GENERATED_BODY()

public:
	APickupGear();

	// 무게 (kg) — 짐꾼 외 이동속도 감소 계산용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
	float WeightKg = 50.f;

	// 금속 탐지기 트리거 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
	bool bIsMetal = true;

	// 집을 때 오염도 증가량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
	float SootOnPickup = 0.2f;

	// 이 기어가 장착되어 있던 슬롯 (AGearSlot::BeginPlay 에서 설정). 슬롯에서 떨어져 나간
	// 뒤 누가 집어가면 리스폰 카운트다운을 시작시키기 위해 역참조가 필요하다.
	UPROPERTY()
	TObjectPtr<AGearSlot> OwningGearSlot;

protected:
	virtual void Server_PickUp_Implementation(ACharacter* Holder) override;
};
