// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BaseCharacter.h"
#include "GODCharacterMechanic.generated.h"

// 모든 역할별 능력은 ABaseCharacter로 이전됨.
// 기존 BP 호환을 위해 빈 서브클래스만 유지.
UCLASS()
class TEAM4PROJECT_API AGODCharacterMechanic : public ABaseCharacter
{
	GENERATED_BODY()
public:
	AGODCharacterMechanic();
};
