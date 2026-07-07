// Fill out your copyright notice in the Description page of Project Settings.

#include "VFX/GameVFXStatics.h"
#include "VFX/GameVFXTypes.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/DataTable.h"

const FGameVFXRow* UGameVFXStatics::FindRow(const UDataTable* VFXTable, FName RowName)
{
      if (!VFXTable || RowName.IsNone())
      {
              return nullptr;
      }
      // 행이 없으면 경고 로그 — DT 에 행을 추가해야 한다는 신호.
      return VFXTable->FindRow<FGameVFXRow>(RowName, TEXT("GameVFX"));
}

UNiagaraSystem* UGameVFXStatics::PickSystem(const FGameVFXRow& Row)
{
      TArray<UNiagaraSystem*> ValidSystems;
      for (UNiagaraSystem* System : Row.Systems)
      {
              if (System)
              {
                      ValidSystems.Add(System);
              }
      }
      if (ValidSystems.Num() == 0)
      {
              return nullptr;
      }
      return ValidSystems[FMath::RandHelper(ValidSystems.Num())];
}

UNiagaraSystem* UGameVFXStatics::GetVFXFromTable(const UDataTable* VFXTable, FName RowName)
{
      const FGameVFXRow* Row = FindRow(VFXTable, RowName);
      return Row ? PickSystem(*Row) : nullptr;
}

UNiagaraComponent* UGameVFXStatics::SpawnVFXAtLocationFromTable(const UObject* WorldContextObject, const UDataTable* VFXTable, FName RowName, FVector Location, FRotator Rotation)
{
	const FGameVFXRow* Row = FindRow(VFXTable, RowName);
      UNiagaraSystem* System = Row ? PickSystem(*Row) : nullptr;
      if (!System)
      {
              return nullptr;
      }
      return UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContextObject, System, Location, Rotation, Row->Scale);
}

UNiagaraComponent* UGameVFXStatics::SpawnVFXAttachedFromTable(const UDataTable* VFXTable, FName RowName, USceneComponent* AttachToComponent, FName SocketName)
{
      const FGameVFXRow* Row = FindRow(VFXTable, RowName);
	  UNiagaraSystem* System = Row ? PickSystem(*Row) : nullptr;
      if (!System || !AttachToComponent)
      {
              return nullptr;
      }
	return UNiagaraFunctionLibrary::SpawnSystemAttached(System, AttachToComponent, SocketName,
	   FVector::ZeroVector, FRotator::ZeroRotator, Row->Scale,
	   EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::AutoRelease);
}