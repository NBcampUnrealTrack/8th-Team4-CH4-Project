// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BaseCharacter.h"
#include "GODCharacterOutlaw.generated.h"

// 무법자 = 이중스파이
// 초반 5분은 위장 시민 직업으로 활동, 5분 뒤 전향해 슈퍼 넉백으로 시민을 방해한다.
// 모든 역할별 능력은 ABaseCharacter로 이전됨 — 기존 BP 호환용 빈 서브클래스.
UCLASS()
class TEAM4PROJECT_API AGODCharacterOutlaw : public ABaseCharacter
{
	GENERATED_BODY()
public:
	AGODCharacterOutlaw(const FObjectInitializer& ObjectInitializer);
};
