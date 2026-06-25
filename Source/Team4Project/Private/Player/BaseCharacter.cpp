// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BaseCharacter.h"
#include "Player/Component/BaseAbilitySystemComponent.h"
#include "Player/Component/BaseAttributeSet.h"
#include "GameplayEffect.h"
#include "Components/CapsuleComponent.h"
#include "Game/BaseDataSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UBaseAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("AttributeSet"));
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// ============================================================
// GAS 초기화
// ============================================================

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitializeAbilityActorInfo();
	ServerInitGAS();
}

void ABaseCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	
	InitializeAbilityActorInfo();
}

void ABaseCharacter::InitializeAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABaseCharacter::ServerInitGAS()
{
	if (!HasAuthority() || !AbilitySystemComponent || bGASInitialized) return;
	bGASInitialized = true;
	
	ApplyCharacterDataRow(CharacterTag);
}

void ABaseCharacter::ApplyCharacterDataRow(const FGameplayTag& RowTag)
{
	if (!HasAuthority() || !AbilitySystemComponent || !RowTag.IsValid()) return;

	UBaseDataSubsystem* DataSubsystem = GetGameInstance()->GetSubsystem<UBaseDataSubsystem>();
	if (!DataSubsystem) return;

	const FCharacterAttributeRow* Row = DataSubsystem->GetCharacterAttributeRow(RowTag);
	if (!Row) return;
	
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;
	if (Row->DefaultAttributeGE)
	{
		EffectsToApply.Add(Row->DefaultAttributeGE);
	}
	EffectsToApply.Append(Row->GrantedEffects);

	for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectsToApply)
	{
		if (!EffectClass) continue;

		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	// 공통 어빌리티 부여 (예: 총기 발사)
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Row->CommonAbilities)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
		}
	}

	// 직업 전용 어빌리티 부여 (예: E 특수스킬)
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Row->JobAbilities)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
		}
	}
}

// ============================================================
// 리플리케이션
// ============================================================

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseCharacter, bIsDead);
	DOREPLIFETIME(ABaseCharacter, CurrentWeapon);
}

// ============================================================
// 사망 처리
// ============================================================

void ABaseCharacter::Die(AActor* Killer)
{
	if (!HasAuthority() || bIsDead) return;

	bIsDead = true;
	
	OnCharacterDied.Broadcast(this, Killer);
	
	Multicast_HandleDeath();
}

void ABaseCharacter::Multicast_HandleDeath_Implementation()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	if (GetMesh())
	{
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	}
	
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
}

void ABaseCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		Multicast_HandleDeath_Implementation();
	}
}

