#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FurnanceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFuelLevelChanged, float, FuelPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFurnaceStateChanged);

// 연료 잔량 구간(상태). 속도 결정 + UI 표시에 사용.
UENUM(BlueprintType)
enum class EFuelState : uint8
{
	Empty  UMETA(DisplayName = "Empty"),   // 0
	Low    UMETA(DisplayName = "Low"),      // ~ LowFuelRatio
	Medium UMETA(DisplayName = "Medium"),   // ~ MediumFuelRatio
	High   UMETA(DisplayName = "High"),     // ~ HighFuelRatio
	Full   UMETA(DisplayName = "Full")      // 그 이상
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TEAM4PROJECT_API UFurnanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFurnanceComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 현재 연료량 (0 ~ MaxFuel), 서버에서만 변경됨. UI 갱신은 타이머로 스로틀한다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Furnace")
	float CurrentFuel = 0.f;

	// 넣을 수 있는 최대 연료량 (0 ~ MaxFuel). 에디터/BP에서 자유롭게 조절.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace")
	float MaxFuel = 150.f;

	// 초당 연료 소모량. 2.5 → 1/3 로 하향 — 석탄 투입 후 게이지가 너무 빨리 닳던 문제.
	// BP(열차) 에서 이 값을 오버라이드해 뒀다면 BP 값이 우선이므로 에디터에서도 확인할 것.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace")
	float FuelBurnRate = 0.833333f;

	// 연료 UI(OnFuelLevelChanged) 갱신 간격(초). 매 Tick 대신 이 주기로만 쏜다.
	// 0 이하면 타이머를 끄고 투입 시점 등에만 갱신.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace")
	float UIUpdateInterval = 0.2f;

	// 연료 > 0 이면 true (bIsActive는 UActorComponent 예약어라 bIsBurning 사용)
	UPROPERTY(ReplicatedUsing = OnRep_bIsBurning, BlueprintReadOnly, Category = "Furnace")
	bool bIsBurning = false;

	// 긴급 회의 등으로 연료 연소 정지 (서버 전용). PressureComponent.bFrozen 과 세트로 토글.
	UPROPERTY(BlueprintReadOnly, Category = "Furnace")
	bool bFrozen = false;

	// 연료 상태 구분 임계값 (0~1 비율). 이 값 미만이면 해당 상태의 아래 단계.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace|State")
	float LowFuelRatio = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace|State")
	float MediumFuelRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace|State")
	float HighFuelRatio = 0.9f;

	// 현재 연료 잔량 구간(상태)을 반환. 속도/UI가 이걸로 분기한다.
	UFUNCTION(BlueprintPure, Category = "Furnace")
	EFuelState GetFuelState() const;

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
	UFUNCTION() void OnRep_bIsBurning();

	// 타이머/투입 시점에 호출. 값이 실제로 바뀐 경우에만 OnFuelLevelChanged를 쏜다.
	void BroadcastFuelLevel();

	// UI 갱신 타이머 핸들
	FTimerHandle UIUpdateTimer;

	// 마지막으로 브로드캐스트한 연료량(중복 갱신 방지용). -1로 초기화해 첫 호출은 항상 전송.
	float LastBroadcastFuel = -1.f;
};
