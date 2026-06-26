// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Weapon/BaseWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"

ABaseWeapon::ABaseWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FVector ABaseWeapon::GetMuzzleLocation() const
{
	if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMesh->GetSocketLocation(MuzzleSocketName);
	}
	return GetActorLocation();
}

void ABaseWeapon::Multicast_PlayFireFX_Implementation(bool bHit, FVector HitLocation, FVector HitNormal)
{
	// 데디케이티드 서버는 렌더링이 없으므로 스킵(리슨서버 호스트는 재생).
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	// 머즐 이펙트: 총구 소켓에 부착해 재생.
	if (MuzzleEffect && WeaponMesh)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleEffect, WeaponMesh, MuzzleSocketName,
			FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, true);
	}

	// 임팩트 이펙트: 명중 시 피격 지점에 법선 방향으로 재생.
	if (bHit && ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, ImpactEffect, HitLocation, HitNormal.Rotation());
	}
}
