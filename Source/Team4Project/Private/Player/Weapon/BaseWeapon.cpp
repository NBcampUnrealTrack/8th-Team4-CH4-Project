// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Weapon/BaseWeapon.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/BaseCharacter.h"
#include "Sound/GameSoundStatics.h"
#include "Sound/GameSoundTypes.h"

ABaseWeapon::ABaseWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseWeapon, HolderCharacter);
	DOREPLIFETIME(ABaseWeapon, AttachSocketName);
}

void ABaseWeapon::AttachToCharacter(ACharacter* InCharacter, FName InSocketName)
{
	HolderCharacter = InCharacter;
	AttachSocketName = InSocketName;

	// 서버에서 즉시 부착(클라는 OnRep_HolderCharacter 에서 동일하게 부착).
	ApplyAttachment();
}

void ABaseWeapon::OnRep_HolderCharacter()
{
	ApplyAttachment();
}

void ABaseWeapon::ApplyAttachment()
{
	if (!HolderCharacter) return;

	if (USkeletalMeshComponent* OwnerMesh = HolderCharacter->GetMesh())
	{
		AttachToComponent(OwnerMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName);
	}
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

	// 총성: 소지 캐릭터의 사운드 DT 에서 조회해 총구 위치에 재생.
	if (const ABaseCharacter* Holder = Cast<ABaseCharacter>(HolderCharacter))
	{
		UGameSoundStatics::PlaySoundAtLocationFromTable(
			this, Holder->GetCharacterSoundTable(), SoundRows::GunFire, GetMuzzleLocation());
	}
}
