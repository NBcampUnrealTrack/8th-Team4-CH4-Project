// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h" 
#include "GameplayTagContainer.h"
#include "BaseGameDataTypes.generated.h"

class UGameplayAbility;

//
USTRUCT(BlueprintType)
struct FCharacterAttributeRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag CharacterTag;

	// 직업별 기본 속성 세팅 GE (MaxAmmo 등 직업마다 다른 값). 기본 장탄수 1.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<class UGameplayEffect> DefaultAttributeGE;

	// 모든 직업 공통 부여 어빌리티 (예: 총기 발사)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<class UGameplayAbility>> CommonAbilities;

	// 이 직업 전용 부여 어빌리티 (예: E 특수스킬)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<class UGameplayAbility>> JobAbilities;

	// 부여할 추가 이펙트 (DefaultAttributeGE 외에 더 적용할 GE)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<class UGameplayEffect>> GrantedEffects;
};

UENUM(BlueprintType)
enum class ETeamSide : uint8
{
	None        UMETA(DisplayName = "None"),
	Player      UMETA(DisplayName = "Player"),
	Enemy       UMETA(DisplayName = "Enemy")
};

USTRUCT(BlueprintType)
struct FItemSpawnRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 아이템 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemName;
	// 어떤 아이템 클래스를 스폰할지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ItemClass;
	// 이 아이템의 스폰 확률
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnChance = 0.0f;
};
