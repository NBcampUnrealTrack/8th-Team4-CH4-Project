// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"
#include "Interface/Interactable.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "InteractiveProp/ACoalHandVisualActor.h"
#include "Game/MinigameTypes.h"
#include "BaseCharacter.generated.h"

class UBaseAbilitySystemComponent;
class UBaseAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class AItemBase;
class UInteractComponent;
class UNiagaraSystem;
class UWidgetComponent;
class USpotLightComponent;
class UMaterialInterface;
class AGearSlot;
class APressureValve;

class UDamageType;

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

class UCustomMovementComponent;
class UDataTable;
class UAnimMontage;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterDied,
	ABaseCharacter*, DeadCharacter,
	AActor*, Killer);

// 직업 태그가 배정/변경될 때 발화 (서버 + 소유 클라). HUD 역할 아이콘 갱신용 —
// 게임 시작 시 리스폰 없이 SetCharacterTag 만 호출되므로 HUD 는 이 델리게이트로 갱신해야 한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterTagChanged, const FGameplayTag&, NewTag);

// 선택 가능한 스킨 하나 (동물 스켈레탈 메시 + 전용 ABP)
USTRUCT(BlueprintType)
struct FCharacterSkinData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	TObjectPtr<USkeletalMesh> Mesh = nullptr;

	// 이 스킨 스켈레톤 전용 애님 블루프린트 (ABP_Dog 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	TSubclassOf<UAnimInstance> AnimClass = nullptr;

	// 동물별 모델 크기 차이 보정용 메시 스케일
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	FVector MeshScale = FVector(1.f, 1.f, 1.f);

	// 밀치기 몽타주. 동물마다 스켈레톤이 달라 하나로 공유할 수 없으므로 스킨별로 지정한다.
	// 비워 두면 캐릭터 기본값(PushMontage / StumbleMontage)으로 폴백한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	TObjectPtr<UAnimMontage> PushMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	TObjectPtr<UAnimMontage> StumbleMontage = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	TArray<TObjectPtr<UTexture2D>> VariantTextures;

	// 동물 스킨별 등반 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	TObjectPtr<UAnimMontage> IdleToClimbMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	TObjectPtr<UAnimMontage> ClimbToTopMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	TObjectPtr<UAnimMontage> ClimbDownLedgeMontage = nullptr;

	//문 열기 / 닫기 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	TObjectPtr<UAnimMontage> DoorOpenMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	TObjectPtr<UAnimMontage> DoorCloseMontage = nullptr;

	//게임 종료 연출용 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin|MatchEnd")
	TObjectPtr<UAnimMontage> VictoryMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin|MatchEnd")
	TObjectPtr<UAnimMontage> DefeatMontage = nullptr;
};

// 순찰자 발자국 기록 (기록 시각 + 위치)
USTRUCT()
struct FFootprintRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	float Timestamp = 0.f;
};

UCLASS()
class TEAM4PROJECT_API ABaseCharacter : public ACharacter, public IAbilitySystemInterface, public IInteractable
{
	GENERATED_BODY()

public:
	ABaseCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 컨트롤러 점유 시 ASC ActorInfo 초기화 + (서버) 어빌리티/속성 부여
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// ============================================================
	// 사망 처리 (공유)
	// ============================================================
	
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCharacterDied OnCharacterDied;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	bool IsDead() const { return bIsDead; }

	void Die(AActor* Killer);

	// 부활/재시작 시 사망 플래그를 리셋한다(서버 전용, 복제됨).
	// 보이스 사망 판정이 PlayerState::bIsAlive OR Character::IsDead() 라서,
	// 재시작으로 bIsAlive 만 살아나고 이 플래그가 남으면 보이스가 계속 '사망(2D/음소거)'으로 고착된다.
	void ResetDeathState();

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	// ============================================================
	// 직업 / 어빌리티 보조
	// ============================================================

	// 이 캐릭터의 직업 태그가 마피아(Character.Crew.Mafia)인지.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Role")
	bool IsMafia() const;

	// 직업 태그 조회 (게임모드/HUD 등에서 역할 확인용).
	UFUNCTION(BlueprintPure, Category = "Role")
	FGameplayTag GetCharacterTag() const { return CharacterTag; }

	// 직업 태그 설정 (게임모드가 서버에서 역할 배정 시 호출). 소유 클라에 복제된다.
	// GAS 가 이미 초기화된 뒤에 호출하면 이전 역할의 어빌리티/이펙트를 정리하고 새 역할 데이터로우를 재적용한다.
	UFUNCTION(BlueprintCallable, Category = "Role")
	void SetCharacterTag(const FGameplayTag& NewTag);

	// 직업 태그 변경 알림 (서버 SetCharacterTag / 클라 OnRep 양쪽에서 발화).
	UPROPERTY(BlueprintAssignable, Category = "Role")
	FOnCharacterTagChanged OnCharacterTagChanged;

	// Speed 속성(디버프/날씨/버프 GE 반영분) × 무게배수(짐꾼 면제)로 MaxWalkSpeed 재계산.
	// Speed/Weight 속성 변경 시 자동 호출된다.
	UFUNCTION(BlueprintCallable, Category = "Stats|Speed")
	void RecalculateMoveSpeed();

	// Weight 속성을 Delta 만큼 증감 (음수 방지). 서버 전용 — 아이템 줍기/떨구기에서 호출.
	// Weight 가 바뀌면 RecalculateMoveSpeed 가 자동 호출되어 속도에 반영된다.
	UFUNCTION(BlueprintCallable, Category = "Stats|Speed")
	void AddWeight(float Delta);

	// 일정 시간 동안 메시(+아이템)를 숨겼다가 복구 (투명화). 서버에서 호출.
	void SetInvisibleForDuration(float Duration);

	// 이 캐릭터 위치에 나이아가라 이펙트를 재생(전 클라). 마피아 감별 표식 등에 사용.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayNiagaraAtSelf(UNiagaraSystem* System);

	// ============================================================
	// 사운드 — 캐릭터 사운드 DT (발소리/총/문/능력. 행 이름은 SoundRows 참조)
	// ============================================================

	// 캐릭터 사운드 DT. BP 디폴트에서 이것 하나만 지정하면 모든 캐릭터 사운드를 행 이름으로 조회한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<UDataTable> CharacterSoundTable;

	UFUNCTION(BlueprintPure, Category = "Sound")
	UDataTable* GetCharacterSoundTable() const { return CharacterSoundTable; }

	// 이 캐릭터 위치에서 DT 사운드 재생 — 호출한 머신에서만 들림 (애님 노티파이 발소리 등 로컬 연출용).
	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayCharacterSoundLocal(FName RowName);

	// 전 클라에서 이 캐릭터 위치에 DT 사운드 재생 (서버에서 호출 — 아이템 줍기/버리기, 죽은 척 등).
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "Sound")
	void Multicast_PlayCharacterSound(FName RowName);

	// 소유 클라에서만 재생 (서버에서 호출 — 능력 사용음. 타인에게 들리면 역할이 노출되므로 본인 전용).
	UFUNCTION(Client, Unreliable)
	void Client_PlayCharacterSound(FName RowName);

	// 점프/착지 사운드 훅.
	virtual void OnJumped_Implementation() override;
	virtual void Landed(const FHitResult& Hit) override;

	// ============================================================
	// 스킨
	// ============================================================

	// 메인 메뉴에서 고른 스킨을 서버에 알림 (소유 클라 → 서버). 서버가 SkinIndex 를 복제해 전 클라에 반영.
	UFUNCTION(Server, Reliable)
	void Server_SetSkin(int32 InSkinIndex);

	UFUNCTION(BlueprintPure, Category = "Skin")
	int32 GetSkinIndex() const { return SkinIndex; }

	// ============================================================
	// 퀘스트
	// ============================================================

	// 서버에서 스테이션이 대입한다.
	void SetActiveQuestStation(class AQuestStation* InStation);

	UFUNCTION(BlueprintPure, Category = "Quest")
	class AQuestStation* GetActiveQuestStation() const { return ActiveQuestStation.Get(); }

	UFUNCTION(Server, Reliable)
	void Server_CompleteQuest();

	UFUNCTION(Server, Reliable)
	void Server_CancelQuest();

	UFUNCTION(Client, Reliable)
	void Client_StartQuestMinigame(class AQuestStation* Station);

	UFUNCTION(Client, Reliable)
	void Client_EndQuestMinigame(bool bSuccess);

	// 퀘스트 작업 중 자세
	UPROPERTY(ReplicatedUsing = OnRep_IsWorkingOnQuest, BlueprintReadOnly, Category = "Quest")
	bool bIsWorkingOnQuest = false;

	UFUNCTION()
	void OnRep_IsWorkingOnQuest();

	// 퀘스트 팝업이 열려 있는 동안 이동/시점 입력을 막는다 (로컬).
	bool IsQuestUIOpen() const { return bQuestUIOpen; }

	// ============================================================
	// 밀치기 (좌클릭 — UGA_Push)
	// ============================================================

	// 서버: 이 캐릭터가 밀쳐졌을 때. 비틀거림 몽타주 + 넉백 + 낙사 킬 크레딧 기록.
	void ReceivePush(ABaseCharacter* Pusher, const FVector& LaunchVelocity, float StumbleDuration);

	// 미는 쪽 모션 (명중 여부와 무관하게 재생).
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayPushMontage();

	// 밀린 쪽 모션.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayStumbleMontage();

	// ============================================================
	// 아이템
	// ============================================================

	// 현재 손에 든 물리 아이템(석탄/기어/귀중품 등).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
	AItemBase* GetCurrentHeldItem() const { return CurrentHeldItem; }

	void SetCurrentHeldItem(AItemBase* InItem) { CurrentHeldItem = InItem; }

	void ClearEquipSlot();

	// 손에 든 아이템 버리기 (G키 / BP 공용). 클라에서 호출하면 서버로 relay 된다.
	UFUNCTION(BlueprintCallable, Category = "Item")
	void DropHeldItem();

	UFUNCTION(Server, Reliable)
	void Server_DropHeldItem();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UInteractComponent> InteractComponent;

	// ============================================================
	// 미니게임 (기어 QTE / 압력밸브 바늘) — 입력 relay & HUD 트리거
	// ============================================================

	// 미니게임 진행 중 이동/점프 억제용
	bool bInputLockedByMinigame = false;

	TWeakObjectPtr<class AGearSlot> ActiveGearQTESlot;
	TWeakObjectPtr<class APressureValve> ActivePressureValve;

	UFUNCTION(Server, Reliable)
	void Server_SubmitQTEDirection(EQTEDirection Dir);

	UFUNCTION(Server, Reliable)
	void Server_SubmitValveStop();

	UFUNCTION(Client, Reliable)
	void Client_StartGearQTE(AGearSlot* Slot);

	UFUNCTION(Client, Reliable)
	void Client_EndGearQTE(bool bSuccess);

	UFUNCTION(Client, Reliable)
	void Client_StartPressureMinigame(APressureValve* Valve);

	UFUNCTION(Client, Reliable)
	void Client_EndPressureMinigame(bool bSuccess);

	// IInteractable — 살아있는 캐릭터: 수색 (죽은 캐릭터는 상호작용 없음)
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;
	// 캐릭터에 붙는 위젯(이름표/표식 등). BP 에서 위젯 클래스·위치 지정.
	// 투명화/시체 은폐로 메시가 숨겨질 때 이 위젯도 함께 숨겨진다.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
	TObjectPtr<UWidgetComponent> WidgetComponent;
	
	
	// Coal
	FORCEINLINE bool IsCoalEquipped() const { return CurrentCoal != nullptr; }
	void SetCoalEquipped(bool bEquip);

	// ============================================================
	// 역할별 능력 — Mafia
	// ============================================================

	void EnterVent(AActor* VentActor);
	void ExitVent();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mafia")
	bool IsInVent() const { return bIsInVent; }

	void SetInvisible(bool bNewInvisible);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mafia")
	bool IsCurrentlyInvisible() const { return bIsInvisible; }

	void UseMasterKey(AActor* DoorActor);
	void UseWireCutter(AActor* GearActor);

	// ============================================================
	// 역할별 능력 — Sheriff
	// ============================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Sheriff")
	void OnSearchResult(bool bHasMafiaAbility, ABaseCharacter* Target);

	void UnlockDoor(AActor* DoorActor);

	// ============================================================
	// 역할별 능력 — Mechanic
	// ============================================================

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mechanic")
	bool CanInstantFixGear() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mechanic")
	float GetRepairSpeedMultiplier() const { return RepairSpeedMultiplier; }

	// ============================================================
	// 역할별 능력 — Watchman
	// ============================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Watchman")
	TObjectPtr<USpotLightComponent> LanternLight;

	void ToggleLantern();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Watchman")
	bool IsLanternOn() const { return bLanternOn; }

	void SetTrackedPlayers(ABaseCharacter* P1, ABaseCharacter* P2,
	                       FLinearColor C1, FLinearColor C2);
	void ActivateFootprintVision();

	// ============================================================
	// 역할별 능력 — Stoker
	// ============================================================

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stoker")
	bool IsHeatImmune() const;

	void ForceClosePressureValve(AActor* PressureValveActor);

	// ============================================================
	// 역할별 능력 — Porter
	// ============================================================

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Porter")
	float GetHeavyCarrySpeedPenalty() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Porter")
	float GetMaxCarryWeight() const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY()
	TObjectPtr<UBaseAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UBaseAttributeSet> AttributeSet;
	
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing = OnRep_CharacterTag, Category = "Stats")
	FGameplayTag CharacterTag;

	UFUNCTION()
	void OnRep_CharacterTag();

	// CharacterTag 를 ASC 의 루즈 태그로 동기화. 역할 능력들의 ActivationRequiredTags
	// (Character.Special.Mafia 등)는 ASC "보유 태그"를 검사하므로 이게 없으면 발동이 항상 거부된다.
	// 서버(SetCharacterTag)와 소유 클라(OnRep)에서만 넣는다 — 타 클라에 넣으면 역할이 노출된다.
	void RefreshRoleLooseTag();
	FGameplayTag AppliedRoleLooseTag;

	void InitializeAbilityActorInfo();
	void ServerInitGAS();

	// ============================================================
	// 스킨 (동물 5종)
	// ============================================================

	// 선택 가능한 스킨 목록. BP 에서 지정 — 인덱스가 메인 메뉴 버튼 순서(0:Dog 1:Frog 2:Panda 3:Rabbit 4:Raccoon)와 일치해야 한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skin")
	TArray<FCharacterSkinData> SkinOptions;

	// 현재 적용된 스킨 (서버에서 설정, 전 클라에 복제. INDEX_NONE 이면 BP 기본 메시 유지)
	UPROPERTY(ReplicatedUsing = OnRep_SkinIndex)
	int32 SkinIndex = INDEX_NONE;

	UFUNCTION()
	void OnRep_SkinIndex();

	// SkinIndex 에 해당하는 메시/ABP/스케일을 실제 메시 컴포넌트에 반영.
	void ApplySkin();

	// 소유 클라: GameInstance 에 저장된 선택 스킨을 서버로 1회 전송.
	void SendSkinSelectionToServer();
	bool bSkinSelectionSent = false;
	
	
	// 같은 캐릭터일 경우 다른 색상
	UPROPERTY(ReplicatedUsing = OnRep_SkinVariantIndex)
	int32 SkinVariantIndex = INDEX_NONE;

	UFUNCTION()
	void OnRep_SkinVariantIndex();

	// ============================================================
	// 이동 속도 (무게 / 직업)
	// ============================================================
	// 디버프/날씨/버프 등 "속도 배수"는 전부 Speed 속성을 곱하는 GameplayEffect 로 표현한다.
	// (GE 가 Speed CurrentValue 를 깎고/올리면 아래 다리가 MaxWalkSpeed 에 반영)
	// 무게만 연속값 + 짐꾼 면제 조건이라 C++ 에서 마지막에 곱한다.

	// 무게 1당 이동 속도 감소 비율(짐꾼 제외). 0.01 이면 무게 10에 10% 감소.
	UPROPERTY(EditDefaultsOnly, Category = "Stats|Speed")
	float WeightSpeedPenaltyPerUnit = 0.01f;

	// 무게로 인한 최저 속도 배수(이 밑으로는 더 느려지지 않음).
	UPROPERTY(EditDefaultsOnly, Category = "Stats|Speed")
	float MinWeightSpeedMultiplier = 0.3f;

	// 속성 변경 콜백 중복 바인딩 방지 플래그.
	bool bSpeedBindingsInitialized = false;

	// Speed/Weight 속성 변경에 RecalculateMoveSpeed 를 바인딩.
	void InitSpeedBindings();
	void HandleSpeedAttributeChanged(const struct FOnAttributeChangeData& Data);
	
	void ApplyCharacterDataRow(const FGameplayTag& RowTag);

	// ApplyCharacterDataRow 가 부여한 어빌리티/이펙트를 모두 제거 (서버 전용).
	void ClearCharacterDataRow();

	// ApplyCharacterDataRow 가 부여한 핸들(역할 변경 시 정리용).
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
	TArray<FActiveGameplayEffectHandle> AppliedEffectHandles;

	bool bGASInitialized = false;

	// ── 퀘스트 ──
	TWeakObjectPtr<class AQuestStation> ActiveQuestStation;

	// 팝업이 열려 있는 동안만 true (로컬 전용). 마우스 커서 + 입력 잠금 복원에 쓴다.
	bool bQuestUIOpen = false;

	void SetQuestUIOpen(bool bOpen);

	// ── 밀치기 ──
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Push")
	TObjectPtr<UAnimMontage> PushMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Push")
	TObjectPtr<UAnimMontage> StumbleMontage;

	// 밀린 뒤 이 시간(초) 안에 낙사하면 밀친 사람을 킬러로 인정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push")
	float PushKillCreditWindow = 6.f;

	// 넉백은 서버뿐 아니라 소유 클라에서도 실행해야 이동 예측이 어긋나지 않는다.
	UFUNCTION(Client, Reliable)
	void Client_LaunchFromPush(FVector LaunchVelocity, float StumbleDuration);

	// 현재 스킨의 몽타주를 우선 사용하고, 없으면 캐릭터 기본 몽타주로 폴백한다.
	UAnimMontage* ResolvePushMontage(bool bStumble) const;
	void PlayMontageLocal(UAnimMontage* Montage);

	// 비틀거리는 동안 이동 입력을 잠근다 (밀려 떨어지는 걸 공중조작으로 취소하지 못하게).
	void ApplyLocalStumble(const FVector& LaunchVelocity, float StumbleDuration);
	void EndStumble();

	// 낙사 시 킬 크레딧을 받을 플레이어 (최근에 민 사람, 없으면 nullptr).
	class AGODPlayerState* GetFallKillCredit() const;

	FTimerHandle StumbleTimer;
	FTimerHandle StumbleInputLockTimer;
	TWeakObjectPtr<ABaseCharacter> LastPusher;
	float LastPushedTime = -1000.f;

	// 현재 손에 든 물리 아이템 (서버에서 픽업 시 설정, 클라에 복제)
	UPROPERTY(Replicated)
	TObjectPtr<AItemBase> CurrentHeldItem;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HandleDeath();

	// 발판(열차) 위 사망 시, 시체 부착까지 래그돌 시뮬을 유지할 시간(초).
	// 열차는 순간이동 방식이라 값이 크면 그 사이 열차가 앞서가 시체가 뒤로 밀린다.
	// 0이면 즉시 부착(현재 포즈로 고정)해 열차 위에서 바로 함께 이동. (기본 권장 0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	float CorpseSettleTime = 0.f;

	void AttachRagdollToBase(UPrimitiveComponent* Base, bool bInAir);

	// 시체를 지정 발판에 부착(kinematic 고정) → 발판을 따라 함께 이동.
	void AttachCorpseToBase(UPrimitiveComponent* Base);

	// 공중 사망 시: 래그돌이 바닥에 떨어져 멈출 때까지 기다렸다가 발판에 부착.
	// (즉시 부착하면 시체가 공중에 고정된 채 남는다)
	void WaitForCorpseToLand(UPrimitiveComponent* Base);

	// 공중 사망 시 착지를 기다리는 최대 시간(초). 초과하면 그 자리에서 부착.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	float CorpseFallAttachTimeout = 3.f;

	FTimerHandle CorpseAttachTimer;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_IsDead();

	// 투명화용: 메시 숨김 상태(서버에서 토글, 클라에 복제).
	UPROPERTY(ReplicatedUsing = OnRep_MeshHidden)
	bool bMeshHidden = false;

	UFUNCTION()
	void OnRep_MeshHidden();

	// bMeshHidden 값을 실제 메시/아이템 가시성에 반영.
	// 서브클래스(마피아)가 bIsInvisible 등 추가 조건을 합성할 수 있도록 virtual.
	virtual void ApplyMeshVisibility();

	// 타이머 콜백: 숨김 해제.
	void RevealMesh();

	FTimerHandle InvisibleTimerHandle;
	
	//손쪽 소켓
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SocketforCoal")
	TSubclassOf<AACoalHandVisualActor> CoalHandClass;

	// 석탄을 붙일 손 소켓 (무기의 AttachSocketName 과 동일한 역할).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SocketforCoal")
	FName CoalAttachSocketName = TEXT("Right_HandSocket");

	// 현재 장착 석탄 (서버에서 스폰 후 설정, 클라에 복제) — 무기 CurrentWeapon 과 동일 패턴.
	UPROPERTY(Replicated)
	TObjectPtr<AACoalHandVisualActor> CurrentCoal;

	// ── Watchman 설정 ──
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Watchman")
	TObjectPtr<UMaterialInterface> FootprintDecalMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Watchman")
	FVector DecalSize = FVector(10.f, 8.f, 2.f);

	UPROPERTY(EditDefaultsOnly, Category = "Watchman")
	float DecalLifespan = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Watchman")
	float FootprintRecordDuration = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Watchman")
	float RecordInterval = 0.5f;

	// ── Mechanic 설정 ──
	UPROPERTY(EditDefaultsOnly, Category = "Mechanic")
	float RepairSpeedMultiplier = 1.0f;

	// ── Porter 설정 ──
	UPROPERTY(EditDefaultsOnly, Category = "Porter")
	float MaxCarryWeight = 200.f;

private:
	// ── 역할 타이머 관리 ──
	void ActivateRoleTimers();
	void DeactivateRoleTimers();

	// ── Mafia 내부 상태 ──
	UPROPERTY(ReplicatedUsing = OnRep_IsInVent)
	bool bIsInVent = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsInvisible)
	bool bIsInvisible = false;

	UFUNCTION() void OnRep_IsInVent();
	UFUNCTION() void OnRep_IsInvisible();
	void ApplyVentMovement(bool bEntering);

	// ── Watchman 내부 상태 ──
	UPROPERTY(ReplicatedUsing = OnRep_LanternOn)
	bool bLanternOn = false;

	UFUNCTION() void OnRep_LanternOn();

	TWeakObjectPtr<ABaseCharacter> TrackedPlayer1;
	TWeakObjectPtr<ABaseCharacter> TrackedPlayer2;
	FLinearColor TrackColor1 = FLinearColor(1.f, 0.4f, 0.f);
	FLinearColor TrackColor2 = FLinearColor(0.f, 0.5f, 1.f);

	TArray<FFootprintRecord> Footprints1;
	TArray<FFootprintRecord> Footprints2;

	FTimerHandle FootprintRecordTimer;
	void RecordFootprintPositions();
	void PruneOldRecords(TArray<FFootprintRecord>& Records);

	UFUNCTION(Client, Reliable)
	void Client_ReceiveFootprints(const TArray<FVector>& Positions1, FLinearColor InColor1,
	                              const TArray<FVector>& Positions2, FLinearColor InColor2);
	void SpawnFootprintDecals(const TArray<FVector>& Positions, FLinearColor Color);

	// ── 낙하 처리 (로비=복귀 / 진행 중=사망) ──
public:
	// 월드 KillZ 아래로 떨어졌을 때: 로비 페이즈면 파괴하지 않고 열차(스타트 지점)로 복귀.
	virtual void FellOutOfWorld(const UDamageType& dmgType) override;

protected:
	// 로비 페이즈에서 스타트 지점보다 이만큼 낮아지면 열차로 복귀시킨다 (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rescue")
	float LobbyFallRescueDepth = 2000.f;

	// 진행 중에 열차보다 이만큼 낮아지면 "떨어진" 것으로 판정한다 (cm).
	// 사망 시스템 제거(2026-07-13) 후에는 사망 대신 FallRespawnDelay 뒤 열차로 복귀한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rescue")
	float FallDeathDepth = 2000.f;

	// 낙하 판정 후 열차로 복귀하기까지의 대기 시간 (초).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rescue")
	float FallRespawnDelay = 2.f;

	// 회의실(MeetingRoom)이 없을 때 열차 액터 기준 복귀 오프셋 (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rescue")
	FVector TrainRespawnOffset = FVector(0.f, 0.f, 300.f);

private:
	FTimerHandle FallRescueTimer;

	// 낙하 판정 기준이 되는 액터 캐시 (서버 전용). 매초 TActorIterator 를 돌지 않기 위함.
	// 열차와 스타트 지점은 레벨 배치 액터라 런타임에 교체되지 않는다.
	UPROPERTY()
	TObjectPtr<class AGODTrain> CachedTrain;

	UPROPERTY()
	TObjectPtr<class APlayerStart> CachedPlayerStart;

	// 위 두 캐시를 채운다. BeginPlay 에서 1회 (서버 전용).
	void CacheFallReferenceActors();

	// 타이머 콜백(서버, 1초 주기): 로비면 복귀, 진행 중이면 사망.
	void CheckFallRescueOrDeath();

	// 스타트 지점(PlayerStart)으로 텔레포트. 성공 여부 반환. (서버 전용)
	bool RescueToStart();

	// 주행 중 낙하 복귀: 회의실 좌석(열차 자식) 우선, 없으면 열차 기준 오프셋. (서버 전용)
	// 주의: 로비 복귀(RescueToStart)의 PlayerStart 는 출발 지점 고정이라 주행 중에는 못 쓴다.
	bool RescueToTrain();
	FTimerHandle TrainRescueTimer;

	// 낙하 판정의 기준 높이. 주행 중 열차는 스플라인을 따라 고도가 바뀌므로 열차를 우선 사용한다.
	bool GetFallReferenceZ(float& OutZ) const;

#pragma region Input

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);

	void HandleGroundMovementInput(const FInputActionValue& Value);
	void HandleClimbMovementInput(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	void OnClimbActionStarted(const FInputActionValue& Value);

	// 미니게임 중에는 점프 억제 (스페이스바가 밸브 정지와 겹치므로)
	void HandleJumpInput();

	void OnQTEUpPressed();
	void OnQTEDownPressed();
	void OnQTELeftPressed();
	void OnQTERightPressed();
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ClimbAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UCustomMovementComponent* CustomMovementComponent;

public:
	FORCEINLINE UCustomMovementComponent* GetCustomMovementComponent() const { return CustomMovementComponent; }
#pragma endregion

	//문 열고 닫는 모션
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDoorMontage(bool bIsOpening);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayMatchEndMontage(bool bIsVictory);
};
