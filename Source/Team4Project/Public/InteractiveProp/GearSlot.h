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
class UPressureComponent;
class UDataTable;
class UWidgetComponent;

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

	// 기어 리스폰 카운트다운을 보여주는 3D 위젯(슬롯 위 공중, World Space). 리스폰 대기
	// 중일 때만 보인다. Widget Class(WBP)는 에디터 디테일에서 지정한다(CoalFeeder::FuelWidget 패턴).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* RespawnTimerWidget;

	// 레벨에 미리 배치된 조립 상태 기어. 에디터에서 드래그로 연결.
	// 비워두면 BeginPlay에서 이 액터의 자식(Child Actor Component로 PickupGear를
	// 붙인 경우 등)에서 자동 탐색해 채운다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Gear")
	APickupGear* MountedGear;

	// 실패(QTE 타임아웃) 시 ForceExplode를 호출할 PressureComponent를 찾기 위한 열차 참조.
	// 비워두면 BeginPlay/QTE 실패 시점에 부모 액터(Child Actor Component로 GODTrain 밑에
	// 붙인 경우 등)에서 자동 탐색한다 (CoalFeeder::GetFurnace()와 동일한 패턴).
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

	// 파손/조립 사운드 DT (캐릭터 사운드 DT 공용 — Gear.Break / Gear.Repair 행 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
	TObjectPtr<UDataTable> SoundTable;

	// 기어가 슬롯에서 떨어져 나간(파손) 뒤 누가 집어가면, 이 시간(초) 뒤에도 수리되지
	// 않았으면 원위치로 강제 복귀시킨다 — 기어를 들고 사라져 슬롯을 영구히 수리 불가로
	// 만드는 것을 방지. QTE 는 여전히 필요하므로 bIsAssembled 는 그대로 false 로 남는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear|Respawn")
	float GearRespawnDelay = 60.f;

	// 리스폰 카운트다운 진행 중 여부 (3D 위젯 표시 트리거).
	UPROPERTY(ReplicatedUsing = OnRep_RespawnPending, BlueprintReadOnly, Category = "Gear|Respawn")
	bool bRespawnPending = false;

	// 리스폰 카운트다운 시작 시각 (서버 월드시간). 클라는 이 값 기준으로 잔여시간을 계산한다
	// (QTEStartServerTime 과 동일한 패턴).
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Gear|Respawn")
	float RespawnStartServerTime = 0.f;

	UFUNCTION()
	void OnRep_RespawnPending();

	// 기어가 슬롯에서 (처음으로) 집혔을 때 PickupGear 가 호출한다 (서버 전용).
	void OnGearTaken();

	// 슬롯 위치에서 DT 사운드 재생 (전 클라). 기어 파손은 주변 플레이어에게 들려야
	// 사보타주를 알아챌 수 있으므로 능력음(로컬)과 달리 월드 사운드로 낸다.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlaySlotSound(FName RowName);

	// 마피아 와이어커터에 의해 호출 (서버 전용). 실제로 파손시켰으면 true.
	bool BreakGear();

	// 라운드 재시작 시 강제 복구 (서버 전용)
	void ForceReassemble();

	// BaseCharacter가 방향키 입력을 relay할 때 호출 (서버 전용)
	void SubmitQTEInput(EQTEDirection Dir);

	// IInteractable
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;

	// QTE 플레이어 사망/이탈 시: 실패(강제 폭발) 없이 중단 (서버 전용)
	void AbortQTE();

private:
	void StartQTE(ABaseCharacter* Player);
	void OnQTETimeout();

	// bAnnounce=false 는 라운드 초기화(ForceReassemble)용. 게임 시작 직후 수리 방송이 뜨는 것을 막는다.
	void CompleteRepair(bool bAnnounce = true);

	// QTEPlayer 를 비우고 사망 델리게이트를 언바인딩한 뒤 반환 (모든 종료 경로 공용).
	ABaseCharacter* ReleaseQTEPlayer();

	UFUNCTION()
	void OnQTEPlayerDied(ABaseCharacter* DeadCharacter, AActor* Killer);

	UPressureComponent* GetPressureComponent() const;

	// MountedGear가 비어있을 때 자식(Child Actor Component 등)에서 자동 탐색
	APickupGear* ResolveMountedGear() const;

	// 리스폰 타이머 만료 콜백 (서버 전용).
	void RespawnGear();

	FTransform OriginalGearTransform;
	FTimerHandle QTETimeoutHandle;
	FTimerHandle RespawnTimerHandle;

	UPROPERTY()
	TObjectPtr<ABaseCharacter> QTEPlayer;
};
