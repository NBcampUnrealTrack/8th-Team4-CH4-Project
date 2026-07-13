// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/CharacterAnimInstance.h"
#include "Player/BaseCharacter.h"
#include "Component/CustomMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ClimbingSystemCharacter = Cast<ABaseCharacter>(TryGetPawnOwner());
	if (ClimbingSystemCharacter)
	{
		CustomMovementComponent = ClimbingSystemCharacter->GetCustomMovementComponent();

	}
}

void UCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!ClimbingSystemCharacter || !CustomMovementComponent)
	{
		return;
	}
	GetGroundSpeed();
	GetAirSpeed();
	GetShouldMove();
	GetIsFalling();
	GetIsClimbing();
	GetClimbVelocity();
	GetIsDead();
	GetIsWorkingOnQuest();
}

void UCharacterAnimInstance::GetGroundSpeed()
{
	GroundSpeed = UKismetMathLibrary::VSizeXY(ClimbingSystemCharacter->GetVelocity());
}

void UCharacterAnimInstance::GetAirSpeed()
{
	AirSpeed = ClimbingSystemCharacter->GetVelocity().Z;
}

void UCharacterAnimInstance::GetShouldMove()
{
	bShouldMove = (CustomMovementComponent->GetCurrentAcceleration().Size() > 0 &&
		GroundSpeed > 5.f &&
		!bIsFalling);
}

void UCharacterAnimInstance::GetIsFalling()
{
	bIsFalling = CustomMovementComponent->IsFalling();
}

void UCharacterAnimInstance::GetIsClimbing()
{
	bIsClimbing = CustomMovementComponent->IsClimbing();
}

void UCharacterAnimInstance::GetClimbVelocity()
{
	ClimbVelocity = CustomMovementComponent->GetUnrotatedClimbVelocity();
}

void UCharacterAnimInstance::GetIsDead()
{
	bIsDead = ClimbingSystemCharacter->IsDead();
}

void UCharacterAnimInstance::GetIsWorkingOnQuest()
{
	bIsWorkingOnQuest = ClimbingSystemCharacter->bIsWorkingOnQuest;
}
