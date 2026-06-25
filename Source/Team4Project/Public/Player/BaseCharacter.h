// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"
#include "BaseCharacter.generated.h"

class UBaseAbilitySystemComponent;
class UBaseAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class ABaseWeapon;
class UInteractComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterDied,
	ABaseCharacter*, DeadCharacter,
	AActor*, Killer);

UCLASS()
class TEAM4PROJECT_API ABaseCharacter : public ACharacter, public IAbilitySystemInterface
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
	// 무기
	// ============================================================

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	ABaseWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	// 장착한 무기 설정 (서버에서 EquipGun 어빌리티가 호출)
	void SetCurrentWeapon(ABaseWeapon* InWeapon) { CurrentWeapon = InWeapon; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UInteractComponent> InteractComponent;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY()
	TObjectPtr<UBaseAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UBaseAttributeSet> AttributeSet;

	// 이 캐릭터의 직업 태그 (예: Character.Crew.Mafia). DT_CharacterRow 조회 키.
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	FGameplayTag CharacterTag;

	//UPROPERTY(EditDefaultsOnly, Category = "Stats")
	//TObjectPtr<UCharacterStatsConfig> StatsConfig;

	void InitializeAbilityActorInfo();
	void ServerInitGAS();

	// DT_CharacterRow 의 한 Row(태그)에 담긴 속성 GE/이펙트/어빌리티를 모두 부여 (서버 전용)
	void ApplyCharacterDataRow(const FGameplayTag& RowTag);

	bool bGASInitialized = false;

	// 현재 장착 무기 (서버에서 스폰 후 설정, 클라에 복제)
	UPROPERTY(Replicated)
	TObjectPtr<ABaseWeapon> CurrentWeapon;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HandleDeath();

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_IsDead();
};
