// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameVFXTypes.generated.h"


class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FGameVFXRow : public FTableRowBase
{
	GENERATED_BODY()

	// 스폰할 나이아가라 시스템. 2개 이상 넣으면 스폰때마다 무작위로 하나 선택 (변주용).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TArray<TObjectPtr<UNiagaraSystem>> Systems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	FVector Scale = FVector::OneVector;
};

class TEAM4PROJECT_API GameVFXTypes
{
public:
	GameVFXTypes();
	~GameVFXTypes();
};


namespace VFXRows
{
	// 상호작용 소품 (월드 3D — 전 클라)
	inline const FName GearBreak(TEXT("Gear.Break"));
	inline const FName GearRepair(TEXT("Gear.Repair"));
	inline const FName CoalDispense(TEXT("Coal.Dispense"));
	inline const FName SinkWashing(TEXT("Sink.Washing")); // 루프

	// ── 게임 전체 VFX DT (DT_GameVFX) — GODGameState(BP) 에 지정 ──
	inline const FName TrainExplosion(TEXT("Train.Explosion"));
	inline const FName TrainDerail(TEXT("Train.Derail"));
	inline const FName FurnaceBurning(TEXT("Furnace.Burning")); // 루프
}