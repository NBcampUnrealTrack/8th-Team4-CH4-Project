// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BaseCharacter.h"
#include "GODCharacterWatchman.generated.h"

class USpotLightComponent;
class UMaterialInterface;

USTRUCT()
struct FFootprintRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	float Timestamp = 0.f;
};

/**
 * 순찰자 (Watchman) — 엔지니어 진영
 *
 * 랜턴: GA_ToggleLantern → ToggleLantern 호출 (토글)
 * 발자국 추적: GA_FootprintVision → ActivateFootprintVision 호출
 *   - 서버에서 30초 기록 관리, 활성화 시 소유 클라이언트에 전송 → 데칼 생성
 *   - GameMode가 SetTrackedPlayers 로 추적 대상 2명 지정
 */
UCLASS()
class TEAM4PROJECT_API AGODCharacterWatchman : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AGODCharacterWatchman();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ── 랜턴 (GA_ToggleLantern 에서 호출, 서버 전용) ──

	void ToggleLantern();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Watchman")
	bool IsLanternOn() const { return bLanternOn; }

	// ── 발자국 추적 ──

	// GameMode에서 호출: 추적 대상 및 색상 지정 (서버 전용)
	UFUNCTION(BlueprintCallable, Category = "Watchman")
	void SetTrackedPlayers(ABaseCharacter* InPlayer1, ABaseCharacter* InPlayer2,
	                       FLinearColor InColor1, FLinearColor InColor2);

	// GA_FootprintVision 에서 호출 (서버 전용)
	void ActivateFootprintVision();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Watchman")
	TObjectPtr<USpotLightComponent> LanternLight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Watchman")
	TObjectPtr<UMaterialInterface> FootprintDecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Watchman")
	FVector DecalSize = FVector(10.f, 8.f, 2.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Watchman")
	float DecalLifespan = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Watchman")
	float FootprintRecordDuration = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Watchman")
	float RecordInterval = 0.5f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_LanternOn)
	bool bLanternOn = false;

	UFUNCTION()
	void OnRep_LanternOn();

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
};
