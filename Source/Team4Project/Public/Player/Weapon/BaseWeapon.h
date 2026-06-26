// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseWeapon.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;

UCLASS()
class TEAM4PROJECT_API ABaseWeapon : public AActor
{
	GENERATED_BODY()

public:
	ABaseWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FVector GetMuzzleLocation() const;

	// 발사 이펙트 재생(머즐 + 명중 시 임팩트). 서버에서 호출 → 모든 클라이언트에서 재생.
	// bHit 가 false 면 HitLocation/HitNormal 은 무시되고 머즐만 재생한다.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireFX(bool bHit, FVector HitLocation, FVector HitNormal);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	// 총구 화염 등 머즐 이펙트 (Muzzle 소켓에 부착 재생)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|FX")
	TObjectPtr<UNiagaraSystem> MuzzleEffect;

	// 피격 지점 임팩트 이펙트 (명중 위치/법선에 재생)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|FX")
	TObjectPtr<UNiagaraSystem> ImpactEffect;
};
