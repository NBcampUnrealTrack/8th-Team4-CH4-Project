#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "CoalFeeder.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UFurnanceComponent;
class UNiagaraSystem;
class USoundBase;

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

	// IInteractable
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;

private:
	UFurnanceComponent* GetFurnace() const;

	// 모든 클라이언트에서 투입 이펙트/소리 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFeedFX();

	// 마지막 투입 시각(쿨다운 판정용, 서버)
	float LastFeedTime = -FLT_MAX;
};
