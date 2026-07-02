#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "Game/MinigameTypes.h"
#include "GearSlot.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class APickupGear;
class ABaseCharacter;

// 메인 기어가 장착되는 고정 축(소켓). 와이어커터로 깨지면 MountedGear가 물리적으로 튕겨나가고,
// 기어를 들고 온 플레이어가 F로 화살표 QTE를 통과하면 재조립된다.
UCLASS()
class TEAM4PROJECT_API AGearSlot : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AGearSlot();

protected:
	virtual void BeginPlay() override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* SlotMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionBox;

	// 레벨에 미리 배치된 조립 상태 기어. 에디터에서 드래그로 연결.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Gear")
	APickupGear* MountedGear;

	// 실패(QTE 타임아웃) 시 ForceExplode를 호출할 PressureComponent를 찾기 위한 열차 참조.
	// PressureValve::TrainActor 와 동일한 패턴.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Gear")
	AActor* TrainActor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Gear")
	bool bIsAssembled = true;

	// QTE 진행 중 여부
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Gear")
	bool bQTEActive = false;

	// 0(Up)~3(Right) = EQTEDirection 값. 랜덤 생성된 정답 순서.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Gear")
	TArray<uint8> QTESequence;

	// 현재까지 맞춘 개수 (다음에 맞춰야 할 인덱스)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Gear")
	int32 QTEProgressIndex = 0;

	// QTE 시작 시각 (서버 월드시간). 클라이언트가 잔여시간 계산용으로 사용.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Gear")
	float QTEStartServerTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
	int32 QTESequenceLength = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
	float QTETimeLimit = 15.f;

	// 이탈 시 부여할 속도(cm/s). AddImpulse(bVelChange=true) 기준이라 질량 무관.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
	float EjectImpulseStrength = 400.f;

	// 마피아 와이어커터에 의해 호출 (서버 전용)
	void BreakGear();

	// 라운드 재시작 시 강제 복구 (서버 전용)
	void ForceReassemble();

	// BaseCharacter가 방향키 입력을 relay할 때 호출 (서버 전용)
	void SubmitQTEInput(EQTEDirection Dir);

	// IInteractable
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;

private:
	void StartQTE(ABaseCharacter* Player);
	void OnQTETimeout();
	void CompleteRepair();

	FTransform OriginalGearTransform;
	FTimerHandle QTETimeoutHandle;

	UPROPERTY()
	TObjectPtr<ABaseCharacter> QTEPlayer;
};
