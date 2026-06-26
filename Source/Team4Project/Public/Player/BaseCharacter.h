// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"
#include "Interface/Interactable.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterDied,
	ABaseCharacter*, DeadCharacter,
	AActor*, Killer);

UCLASS()
class TEAM4PROJECT_API ABaseCharacter : public ACharacter, public IAbilitySystemInterface, public IInteractable
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	virtual void BeginPlay() override;

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

	// 보유 아이템 설정 (서버에서 ItemBase 픽업/드롭이 호출)
	void SetCurrentHeldItem(AItemBase* InItem) { CurrentHeldItem = InItem; }

	// 장착 슬롯을 비운다(서버 전용): 총이면 해제(파괴+태그 제거), 물리 아이템이면 떨군다.
	// 새 아이템/무기를 장착하기 직전에 호출해 "한 번에 하나만" 들도록 강제한다.
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

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY()
	TObjectPtr<UBaseAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UBaseAttributeSet> AttributeSet;

	// 이 캐릭터의 직업 태그 (예: Character.Crew.Mafia). DT_CharacterRow 조회 키.
	// 소유 클라에만 복제(역할 노출 방지). 서버 어빌리티는 서버 값을 그대로 사용.
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Stats")
	FGameplayTag CharacterTag;

	//UPROPERTY(EditDefaultsOnly, Category = "Stats")
	//TObjectPtr<UCharacterStatsConfig> StatsConfig;

	void InitializeAbilityActorInfo();
	void ServerInitGAS();

	// DT_CharacterRow 의 한 Row(태그)에 담긴 속성 GE/이펙트/어빌리티를 모두 부여 (서버 전용).
	// 재호출 시 직전 부여분을 먼저 정리하고 새로 적용한다(역할 변경 지원).
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
};
