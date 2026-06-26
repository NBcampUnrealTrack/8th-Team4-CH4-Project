// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BaseCharacter.h"
#include "GODCharacterMafia.generated.h"

/**
 * 마피아 (Mafia) — 특수 진영
 *
 * 패시브 — 시체 소멸: 팀원 GA가 처리 (Multicast_HideBody/ShowBody 호출)
 * 환풍구 이동: GA_EnterVent → EnterVent / ExitVent 호출
 * 마스터키 (1분 쿨타임): GA_MasterKey → UseMasterKey 호출
 * 절단기 (2분 쿨타임): GA_WireCutter → UseWireCutter 호출
 * 투명화 (3분 쿨타임, 10초): 팀원 GA가 bIsInvisible 설정
 */
UCLASS()
class TEAM4PROJECT_API AGODCharacterMafia : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AGODCharacterMafia();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ── 환풍구 이동 (GA_EnterVent 에서 호출) ──

	void EnterVent(AActor* VentActor);
	void ExitVent();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mafia")
	bool IsInVent() const { return bIsInVent; }

	// ── 마스터키 (GA_MasterKey 에서 호출) ──

	void UseMasterKey(AActor* DoorActor);

	// ── 절단기 (GA_WireCutter 에서 호출) ──

	void UseWireCutter(AActor* GearActor);

	// ── 투명화 상태 (팀원 GA 에서 호출) ──

	// 팀원 GA가 서버에서 직접 호출. OnRep이 클라이언트에 전파.
	void SetInvisible(bool bNewInvisible);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mafia")
	bool IsCurrentlyInvisible() const { return bIsInvisible; }

	// ── 시체 은폐 (팀원 GA 에서 호출) ──

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HideBody(ABaseCharacter* DeadCharacter);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ShowBody(ABaseCharacter* DeadCharacter);

private:
	// ── 환풍구 ──

	UPROPERTY(ReplicatedUsing = OnRep_IsInVent)
	bool bIsInVent = false;

	UFUNCTION()
	void OnRep_IsInVent();

	void ApplyVentMovement(bool bEntering);

	// ── 투명화 ──

	UPROPERTY(ReplicatedUsing = OnRep_IsInvisible)
	bool bIsInvisible = false;

	UFUNCTION()
	void OnRep_IsInvisible();

	void UpdateMeshVisibilityForInvisibility();
};
