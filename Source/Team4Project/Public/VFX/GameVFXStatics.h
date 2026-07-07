// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameVFXStatics.generated.h"

class UDataTable;
class UNiagaraSystem;
class UNiagaraComponent;
class USceneComponent;
struct FGameVFXRow;

UCLASS()
class TEAM4PROJECT_API UGameVFXStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	
public:
	// 위치 단발 스폰 (총구 화염, 피격, 파괴 등). 반환된 컴포넌트로 루프 이펙트 Stop 제어 가능.
	UFUNCTION(BlueprintCallable, Category = "VFX", meta = (WorldContext = "WorldContextObject"))
	static UNiagaraComponent* SpawnVFXAtLocationFromTable(const UObject* WorldContextObject, const UDataTable* VFXTable, FName RowName, FVector Location, FRotator Rotation = FRotator::ZeroRotator);

	// 컴포넌트 부착 스폰 (화로 불꽃, 캐릭터 부착 이펙트 등 움직이는 대상).
	UFUNCTION(BlueprintCallable, Category = "VFX")
	static UNiagaraComponent* SpawnVFXAttachedFromTable(const UDataTable* VFXTable, FName RowName, USceneComponent* AttachToComponent, FName SocketName = NAME_None);

	// 행에서 시스템 하나 선택해 반환 (여러 개면 무작위). 스폰을 직접 제어하고 싶을 때 사용.
	UFUNCTION(BlueprintCallable, Category = "VFX")
	static UNiagaraSystem* GetVFXFromTable(const UDataTable* VFXTable, FName RowName);

private:
	static const FGameVFXRow* FindRow(const UDataTable* VFXTable, FName RowName);
	static UNiagaraSystem* PickSystem(const FGameVFXRow& Row);
};
