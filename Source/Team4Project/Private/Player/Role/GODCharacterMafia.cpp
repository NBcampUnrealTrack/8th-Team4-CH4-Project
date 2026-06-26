// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterMafia.h"
#include "InteractiveProp/DoorBase.h"
#include "InteractiveProp/ItemBase.h"
#include "Player/Weapon/BaseWeapon.h"
#include "Components/WidgetComponent.h"
#include "Game/BaseGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

AGODCharacterMafia::AGODCharacterMafia()
{
	CharacterTag = Character::Special::Mafia;
}

void AGODCharacterMafia::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGODCharacterMafia, bIsInVent);
	DOREPLIFETIME(AGODCharacterMafia, bIsInvisible);
}

// ── 환풍구 이동 ──

void AGODCharacterMafia::EnterVent(AActor* VentActor)
{
	if (!HasAuthority() || bIsInVent) return;
	bIsInVent = true;
	ApplyVentMovement(true);
}

void AGODCharacterMafia::ExitVent()
{
	if (!HasAuthority() || !bIsInVent) return;
	bIsInVent = false;
	ApplyVentMovement(false);
}

void AGODCharacterMafia::OnRep_IsInVent()
{
	ApplyVentMovement(bIsInVent);
}

void AGODCharacterMafia::ApplyVentMovement(bool bEntering)
{
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
		CMC->MaxWalkSpeed = bEntering ? 150.f : 600.f;

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		Capsule->SetCapsuleHalfHeight(bEntering ? 24.f : 88.f);
}

// ── 마스터키 ──

void AGODCharacterMafia::UseMasterKey(AActor* DoorActor)
{
	if (!HasAuthority() || !IsValid(DoorActor)) return;

	if (ADoorBase* Door = Cast<ADoorBase>(DoorActor))
	{
		Door->SetLocked(true);
	}
}

// ── 절단기 ──

void AGODCharacterMafia::UseWireCutter(AActor* GearActor)
{
	if (!HasAuthority() || !IsValid(GearActor)) return;

	// 다른 플레이어가 들고 있는 경우 먼저 놓게 해서 CurrentHeldItem dangling 방지
	if (AItemBase* Item = Cast<AItemBase>(GearActor))
	{
		if (Item->bIsHeld)
		{
			Item->Server_Drop();
		}
	}

	GearActor->Destroy();
}

// ── 시체 은폐 (팀원 GA 호출용) ──

void AGODCharacterMafia::Multicast_HideBody_Implementation(ABaseCharacter* DeadCharacter)
{
	if (!IsValid(DeadCharacter)) return;
	if (DeadCharacter->GetMesh())
		DeadCharacter->GetMesh()->SetVisibility(false);
	// 보안관 패시브 감지용 태그 (서버에서만 의미 있음)
	if (HasAuthority())
		DeadCharacter->Tags.AddUnique(TEXT("Body.Hidden"));
}

void AGODCharacterMafia::Multicast_ShowBody_Implementation(ABaseCharacter* DeadCharacter)
{
	if (!IsValid(DeadCharacter)) return;
	if (DeadCharacter->GetMesh())
		DeadCharacter->GetMesh()->SetVisibility(true);
	if (HasAuthority())
		DeadCharacter->Tags.Remove(TEXT("Body.Hidden"));
}

// ── 투명화 ──

void AGODCharacterMafia::SetInvisible(bool bNewInvisible)
{
	if (!HasAuthority()) return;
	bIsInvisible = bNewInvisible;
	UpdateMeshVisibilityForInvisibility();
}

void AGODCharacterMafia::OnRep_IsInvisible()
{
	UpdateMeshVisibilityForInvisibility();
}

void AGODCharacterMafia::UpdateMeshVisibilityForInvisibility()
{
	ApplyMeshVisibility();
}

void AGODCharacterMafia::ApplyMeshVisibility()
{
	// 투명화 중이고 자신의 화면이 아닌 경우: 메시/무기/아이템/위젯 모두 강제 숨김.
	// bMeshHidden 상태는 투명화 해제 후 RevealMesh()가 처리.
	if (bIsInvisible && !IsLocallyControlled())
	{
		if (GetMesh()) GetMesh()->SetVisibility(false);
		if (CurrentWeapon) CurrentWeapon->SetActorHiddenInGame(true);
		if (CurrentHeldItem) CurrentHeldItem->SetActorHiddenInGame(true);
		if (WidgetComponent) WidgetComponent->SetVisibility(false, true);
		return;
	}
	// 투명화가 아닐 때는 bMeshHidden 기반 베이스 로직을 그대로 사용
	Super::ApplyMeshVisibility();
}
