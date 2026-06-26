// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterOutlaw.h"
#include "Game/BaseGameplayTags.h"
#include "Player/GODPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

AGODCharacterOutlaw::AGODCharacterOutlaw()
{
	CharacterTag = Character::Special::Outlaw;
}

void AGODCharacterOutlaw::BeginPlay()
{
	Super::BeginPlay();

	if (GetMesh())
	{
		DefaultMeshRelativeLocation = GetMesh()->GetRelativeLocation();
		DefaultMeshRelativeRotation = GetMesh()->GetRelativeRotation();
	}
}

void AGODCharacterOutlaw::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGODCharacterOutlaw, bIsFakeDead);
}

// ── 가짜 시체 ──

void AGODCharacterOutlaw::StartFakeDeath()
{
	if (!HasAuthority() || bIsFakeDead || bIsDead) return;
	bIsFakeDead = true;
	ApplyFakeDeathPhysics(true);
}

void AGODCharacterOutlaw::StopFakeDeath()
{
	if (!HasAuthority() || !bIsFakeDead) return;
	bIsFakeDead = false;
	ApplyFakeDeathPhysics(false);
}

void AGODCharacterOutlaw::OnRep_IsFakeDead()
{
	ApplyFakeDeathPhysics(bIsFakeDead);
}

void AGODCharacterOutlaw::ApplyFakeDeathPhysics(bool bActivate)
{
	if (bActivate)
	{
		if (GetCharacterMovement()) GetCharacterMovement()->DisableMovement();
		if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (GetMesh())
		{
			GetMesh()->SetSimulatePhysics(true);
			GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		}
		if (APlayerController* PC = Cast<APlayerController>(GetController())) DisableInput(PC);
	}
	else
	{
		if (GetMesh())
		{
			GetMesh()->SetSimulatePhysics(false);
			GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
			GetMesh()->AttachToComponent(GetCapsuleComponent(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			GetMesh()->SetRelativeLocationAndRotation(
				DefaultMeshRelativeLocation, DefaultMeshRelativeRotation);
		}
		if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (GetCharacterMovement()) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		if (APlayerController* PC = Cast<APlayerController>(GetController())) EnableInput(PC);
	}
}

// ── 도굴꾼 ──

void AGODCharacterOutlaw::StealAmmo(ABaseCharacter* DeadCharacter)
{
	if (!HasAuthority() || !IsValid(DeadCharacter) || !DeadCharacter->IsDead()) return;

	AGODPlayerState* OutlawPS = GetPlayerState<AGODPlayerState>();
	AGODPlayerState* DeadPS   = DeadCharacter->GetPlayerState<AGODPlayerState>();
	if (!OutlawPS || !DeadPS) return;

	if (DeadPS->AmmoCount > 0)
	{
		OutlawPS->AmmoCount += DeadPS->AmmoCount;
		DeadPS->AmmoCount = 0;
	}
}
