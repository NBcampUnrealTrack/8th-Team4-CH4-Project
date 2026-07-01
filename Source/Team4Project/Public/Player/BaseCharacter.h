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
#include "BaseCharacter.generated.h"

class UBaseAbilitySystemComponent;
class UBaseAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class ABaseWeapon;
class AItemBase;
class UInteractComponent;
class UNiagaraSystem;
class UWidgetComponent;
class USpotLightComponent;
class UMaterialInterface;

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterDied,
	ABaseCharacter*, DeadCharacter,
	AActor*, Killer);

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
	ABaseCharacter();

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

	// Speed 속성(디버프/날씨/버프 GE 반영분) × 무게배수(짐꾼 면제)로 MaxWalkSpeed 재계산.
	// Speed/Weight 속성 변경 시 자동 호출된다.
	UFUNCTION(BlueprintCallable, Category = "Stats|Speed")
	void RecalculateMoveSpeed();

	// Weight 속성을 Delta 만큼 증감 (음수 방지). 서버 전용 — 아이템 줍기/떨구기에서 호출.
	// Weight 가 바뀌면 RecalculateMoveSpeed 가 자동 호출되어 속도에 반영된다.
	UFUNCTION(BlueprintCallable, Category = "Stats|Speed")
	void AddWeight(float Delta);

	// 일정 시간 동안 메시(+무기/아이템)를 숨겼다가 복구 (투명화). 서버에서 호출.
	void SetInvisibleForDuration(float Duration);

	// 일정 시간 동안 시체(메시)를 숨겼다가 복구 (마피아 시체 은폐). 서버에서 호출.
	void HideCorpseForDuration(float Duration);

	// 이 캐릭터 위치에 나이아가라 이펙트를 재생(전 클라). 마피아 감별 표식 등에 사용.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayNiagaraAtSelf(UNiagaraSystem* System);

	// ============================================================
	// 무기
	// ============================================================

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	ABaseWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	// 장착한 무기 설정 (서버에서 EquipGun 어빌리티가 호출)
	void SetCurrentWeapon(ABaseWeapon* InWeapon) { CurrentWeapon = InWeapon; }

	// 현재 손에 든 물리 아이템(석탄/기어 등). 무기는 CurrentWeapon 으로 별도 관리.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
	AItemBase* GetCurrentHeldItem() const { return CurrentHeldItem; }
	
	void SetCurrentHeldItem(AItemBase* InItem) { CurrentHeldItem = InItem; }
	
	void ClearEquipSlot();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UInteractComponent> InteractComponent;

	// IInteractable — 죽은 캐릭터: 탄약 빼앗기 / 살아있는 캐릭터: 수색
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;
	// 캐릭터에 붙는 위젯(이름표/표식 등). BP 에서 위젯 클래스·위치 지정.
	// 투명화/시체 은폐로 메시가 숨겨질 때 이 위젯도 함께 숨겨진다.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
	TObjectPtr<UWidgetComponent> WidgetComponent;
	
	
	// Coal
	FORCEINLINE bool IsCoalEquipped() const { return bIsCoalEquipped; }
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

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HideBody(ABaseCharacter* DeadCharacter);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ShowBody(ABaseCharacter* DeadCharacter);

	// ============================================================
	// 역할별 능력 — Sheriff
	// ============================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Sheriff")
	void OnHiddenBodyDetected(ABaseCharacter* HiddenBody);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sheriff")
	void OnSearchResult(bool bHasMafiaAbility, ABaseCharacter* Target);

	void UnlockDoor(AActor* DoorActor);

	// ============================================================
	// 역할별 능력 — Outlaw
	// ============================================================

	void StartFakeDeath();
	void StopFakeDeath();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Outlaw")
	bool IsFakeDead() const { return bIsFakeDead; }

	void StealAmmo(ABaseCharacter* DeadCharacter);

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
	
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Stats")
	FGameplayTag CharacterTag;

	void InitializeAbilityActorInfo();
	void ServerInitGAS();

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

	// 현재 장착 무기 (서버에서 스폰 후 설정, 클라에 복제)
	UPROPERTY(Replicated)
	TObjectPtr<ABaseWeapon> CurrentWeapon;

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

	// 시체를 지정 발판에 부착(kinematic 고정) → 발판을 따라 함께 이동.
	void AttachCorpseToBase(UPrimitiveComponent* Base);

	FTimerHandle CorpseAttachTimer;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_IsDead();

	// 투명화/시체 은폐 공용: 메시 숨김 상태(서버에서 토글, 클라에 복제).
	UPROPERTY(ReplicatedUsing = OnRep_MeshHidden)
	bool bMeshHidden = false;

	UFUNCTION()
	void OnRep_MeshHidden();

	// bMeshHidden 값을 실제 메시/무기/아이템 가시성에 반영.
	// 서브클래스(마피아)가 bIsInvisible 등 추가 조건을 합성할 수 있도록 virtual.
	virtual void ApplyMeshVisibility();

	// 타이머 콜백: 숨김 해제.
	void RevealMesh();

	FTimerHandle InvisibleTimerHandle;
	FTimerHandle CorpseHideTimerHandle;
	
	//손쪽 소켓
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SocketforCoal")
	TSubclassOf<AACoalHandVisualActor> CoalHandClass;
	
	UPROPERTY()
	AACoalHandVisualActor* LeftCoalActor;
	
	UPROPERTY()
	AACoalHandVisualActor* RightCoalActor;
	
	UPROPERTY(ReplicatedUsing=OnRep_CoalEquipped)
	bool bIsCoalEquipped;
	
	
	UFUNCTION()
	void OnRep_CoalEquipped();
	void UpdateCoalVisual();
	
	
	void SpawnCoalHands();

	void DestroyCoalHands();

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

	// ── Sheriff 설정 ──
	UPROPERTY(EditDefaultsOnly, Category = "Sheriff")
	float BodyDetectionRadius = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Sheriff")
	float BodyDetectionInterval = 1.0f;

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

	// ── Outlaw 내부 상태 ──
	UPROPERTY(ReplicatedUsing = OnRep_IsFakeDead)
	bool bIsFakeDead = false;

	UFUNCTION() void OnRep_IsFakeDead();
	void ApplyFakeDeathPhysics(bool bActivate);
	FVector DefaultMeshRelativeLocation = FVector::ZeroVector;
	FRotator DefaultMeshRelativeRotation = FRotator::ZeroRotator;

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

	// ── Sheriff 내부 상태 ──
	FTimerHandle BodyDetectionTimer;
	void CheckForHiddenBodies();

#pragma region Input
protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);
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
#pragma endregion
};
