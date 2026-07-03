#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "CoalFeeder.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UFurnanceComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UPointLightComponent;
class USoundBase;
class UWidgetComponent;

// 석탄 투입구. 플레이어가 석탄을 든 채로 상호작용키를 누르면 트레인 화로에 연료를 넣는다.
// (PressureValve 와 동일한 패턴: 메시 + 상호작용 박스 + TrainActor 참조)
UCLASS()
class TEAM4PROJECT_API ACoalFeeder : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ACoalFeeder();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* FeederMesh;

	// 플레이어가 인터랙션할 수 있는 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionBox;

	// 연료 상태를 보여줄 3D 위젯. BP에서 Widget Class(WBP)를 지정한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* FuelWidget;

	// 화로 불 이펙트. 나이아가라 에셋(예: NS_Fire)은 BP 디테일에서 지정하고,
	// 위치를 화구(불이 보일 자리)에 맞게 옮겨둔다. 연료가 있을 때만 재생되며
	// 연료량에 따라 크기가 변한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* FireEffect;

	// 화로 불빛. FireEffect에 붙어 함께 켜지고 꺼지며, 타는 동안 일렁인다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* FireLight;

	// 에디터에서 GODTrain Actor를 드래그해서 연결
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Coal")
	AActor* TrainActor;

	// 기차에 자동 부착 (TrainActor를 따라 움직임). 배치한 위치를 유지한 채 따라간다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coal")
	bool bAttachToTrain = true;

	// 연속 투입 방지 쿨다운(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coal")
	float FeedCooldown = 1.0f;

	// 투입 이펙트(연기 등). BP에서 나이아가라 지정.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coal|FX")
	UNiagaraSystem* FeedEffect = nullptr;

	// 투입 소리. BP에서 사운드 지정.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coal|FX")
	USoundBase* FeedSound = nullptr;

	// 이펙트/소리 재생 위치 오프셋 (투입구 로컬 기준)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coal|FX")
	FVector EffectOffset = FVector(0.f, 0.f, 50.f);

	// 연료 가득일 때 불빛 세기 (연료가 줄면 어두워짐)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coal|FX")
	float FireLightIntensity = 5000.f;

	// 연료량 0~100%에 대응하는 불 이펙트 스케일 (X=바닥 직전, Y=가득)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coal|FX")
	FVector2D FireScaleRange = FVector2D(0.5f, 1.2f);

	// IInteractable
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Coal|UI")
	void OnFuelWidgetUpdate(float CurrentFuel, float MaxFuel, float FuelPercent);

	// 현재 화로 상태로 위젯을 즉시 한 번 갱신한다(초기 표시/수동 갱신용).
	UFUNCTION(BlueprintCallable, Category = "Coal|UI")
	void RefreshFuelWidget();

private:
	UFurnanceComponent* GetFurnace() const;

	// 화로 델리게이트를 이 액터에 바인딩 시도. 성공하면 true.
	bool TryBindFurnace();

	// 복제 타이밍 대비 재시도(타이머 콜백). 성공하면 타이머 정리.
	void RetryBindFurnace();

	// 화로 연료 변화 콜백(퍼센트 수신). 위젯 갱신 이벤트로 포워딩.
	UFUNCTION() void HandleFuelLevelChanged(float FuelPercent);

	// 화로 점화/소진 콜백 → 불 이펙트 on/off
	UFUNCTION() void HandleFurnaceActivated();
	UFUNCTION() void HandleFurnaceDeactivated();

	// 현재 화로 상태(bIsBurning + 연료량)에 맞춰 불 이펙트/불빛을 갱신.
	// 델리게이트(점화/소진/연료 변화)에서만 호출되는 이벤트 구동 — 틱 없음.
	// 불빛 일렁임은 FireLight의 Light Function 머테리얼(GPU)이 담당한다.
	void UpdateFireFX();

	// 바인딩된 화로 캐시(재탐색 방지 + 초기 갱신용)
	UPROPERTY() UFurnanceComponent* BoundFurnace = nullptr;

	// 클라이언트에서 기차/화로 복제 타이밍이 늦을 때 재시도용 타이머
	FTimerHandle FurnaceBindTimer;

	// 모든 클라이언트에서 투입 이펙트/소리 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFeedFX();

	// 마지막 투입 시각(쿨다운 판정용, 서버)
	float LastFeedTime = -FLT_MAX;
};
