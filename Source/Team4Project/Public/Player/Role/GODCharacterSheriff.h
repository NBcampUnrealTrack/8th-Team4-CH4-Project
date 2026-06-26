// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BaseCharacter.h"
#include "GODCharacterSheriff.generated.h"

/**
 * 보안관 (Sheriff) — 특수 진영
 *
 * 패시브 — 시체 감지: 10m 이내에 마피아가 소멸시킨 시체 존재 시 알림
 * 액티브 A — 수색: 팀원 GA 담당 (결과는 OnSearchResult BP 이벤트로 수신)
 * 액티브 B — 문 따기: GA_UnlockDoor → UnlockDoor 호출
 */
UCLASS()
class TEAM4PROJECT_API AGODCharacterSheriff : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AGODCharacterSheriff();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ── 패시브: 소멸 시체 감지 ──

	// BP에서 오버라이드: 감지된 시체 위치에 UI 마커 등 처리
	UFUNCTION(BlueprintImplementableEvent, Category = "Sheriff")
	void OnHiddenBodyDetected(ABaseCharacter* HiddenBody);

	// ── 액티브 A: 수색 (팀원 GA 담당) ──

	// 팀원 GA가 결과를 Client RPC로 전달할 때 이 BP 이벤트 호출
	UFUNCTION(BlueprintImplementableEvent, Category = "Sheriff")
	void OnSearchResult(bool bHasMafiaAbility, ABaseCharacter* Target);

	// ── 액티브 B: 문 따기 (GA_UnlockDoor 에서 호출) ──

	void UnlockDoor(AActor* DoorActor);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sheriff")
	float BodyDetectionRadius = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sheriff")
	float BodyDetectionInterval = 1.0f;

private:
	FTimerHandle BodyDetectionTimer;

	void CheckForHiddenBodies();
};
