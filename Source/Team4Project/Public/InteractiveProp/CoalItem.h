#pragma once

#include "CoreMinimal.h"
#include "InteractiveProp/ItemBase.h"
#include "CoalItem.generated.h"

UCLASS()
class TEAM4PROJECT_API ACoalItem : public AItemBase
{
	GENERATED_BODY()

public:
	ACoalItem();

	// 화로 투입 시 추가 연료량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coal")
	float FuelValue = 25.f;

	// 집을 때 오염도 증가량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coal")
	float SootOnPickup = 0.3f;

protected:
	virtual void Server_PickUp_Implementation(ACharacter* Holder) override;
};
