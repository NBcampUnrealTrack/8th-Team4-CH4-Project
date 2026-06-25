#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FurnanceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFuelLevelChanged, float, FuelPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFurnaceStateChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TEAM4PROJECT_API UFurnanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFurnanceComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 현재 연료량 (0 ~ MaxFuel), 서버에서만 변경됨
	UPROPERTY(ReplicatedUsing = OnRep_CurrentFuel, BlueprintReadOnly, Category = "Furnace")
	float CurrentFuel = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace")
	float MaxFuel = 100.f;

	// 초당 연료 소모량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace")
	float FuelBurnRate = 2.5f;

	// 연료 > 0 이면 true (bIsActive는 UActorComponent 예약어라 bIsBurning 사용)
	UPROPERTY(ReplicatedUsing = OnRep_bIsBurning, BlueprintReadOnly, Category = "Furnace")
	bool bIsBurning = false;

	// 석탄 투입 (서버에서 호출, 화로 BoxCollision Overlap 이벤트에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Furnace")
	void AddCoal(float Amount);

	// 연료량 변화 이벤트 (0.0 ~ 1.0 퍼센트)
	UPROPERTY(BlueprintAssignable, Category = "Furnace|Events")
	FOnFuelLevelChanged OnFuelLevelChanged;

	// 연료 생김 (꺼진 상태 → 켜진 상태)
	UPROPERTY(BlueprintAssignable, Category = "Furnace|Events")
	FOnFurnaceStateChanged OnFurnaceActivated;

	// 연료 소진 (켜진 상태 → 꺼진 상태)
	UPROPERTY(BlueprintAssignable, Category = "Furnace|Events")
	FOnFurnaceStateChanged OnFurnaceDeactivated;

private:
	UFUNCTION() void OnRep_CurrentFuel();
	UFUNCTION() void OnRep_bIsBurning();
};
