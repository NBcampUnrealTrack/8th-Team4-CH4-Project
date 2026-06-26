#include "InteractiveProp/ItemBase.h"
#include "Player/BaseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

AItemBase::AItemBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
}

void AItemBase::BeginPlay()
{
	Super::BeginPlay();
}

void AItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AItemBase, bIsHeld);
	DOREPLIFETIME(AItemBase, HolderCharacter);
}

void AItemBase::Server_PickUp_Implementation(ACharacter* Holder)
{
	if (!Holder || bIsHeld || ActorHasTag(TEXT("Gear.Destroyed"))) return;

	// 단일 슬롯: 새 아이템을 들기 전에 기존 장착(총/다른 아이템)을 먼저 비운다.
	ABaseCharacter* BaseChar = Cast<ABaseCharacter>(Holder);
	if (BaseChar)
	{
		BaseChar->ClearEquipSlot();
	}

	bIsHeld = true;
	HolderCharacter = Holder;
	SetPhysicsForHeld(true);

	if (USkeletalMeshComponent* SkelMesh = Holder->GetMesh())
	{
		AttachToComponent(SkelMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName);
	}

	// 장착 슬롯 등록 + 상태 태그 부여.
	if (BaseChar)
	{
		BaseChar->SetCurrentHeldItem(this);

		if (EquipStateTag.IsValid())
		{
			if (UAbilitySystemComponent* ASC = BaseChar->GetAbilitySystemComponent())
			{
				ASC->AddLooseGameplayTag(EquipStateTag);
				ASC->AddReplicatedLooseGameplayTag(EquipStateTag);
			}
		}
	}
}

void AItemBase::Server_Drop_Implementation()
{
	if (!bIsHeld) return;

	// 장착 상태 태그 / 슬롯 정리 (들고 있던 캐릭터 기준).
	if (ABaseCharacter* BaseChar = Cast<ABaseCharacter>(HolderCharacter))
	{
		if (EquipStateTag.IsValid())
		{
			if (UAbilitySystemComponent* ASC = BaseChar->GetAbilitySystemComponent())
			{
				ASC->RemoveLooseGameplayTag(EquipStateTag);
				ASC->RemoveReplicatedLooseGameplayTag(EquipStateTag);
			}
		}

		if (BaseChar->GetCurrentHeldItem() == this)
		{
			BaseChar->SetCurrentHeldItem(nullptr);
		}
	}

	bIsHeld = false;
	HolderCharacter = nullptr;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetPhysicsForHeld(false);
}

void AItemBase::OnRep_bIsHeld()
{
	SetPhysicsForHeld(bIsHeld);
}

void AItemBase::SetPhysicsForHeld(bool bHeld)
{
	if (bHeld)
	{
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetSimulatePhysics(true);
	}
}

void AItemBase::Interact_Implementation(ACharacter* Interactor)
{
	Server_PickUp(Interactor);
}

FText AItemBase::GetInteractPrompt_Implementation() const
{
	if (ActorHasTag(TEXT("Gear.Destroyed")))
		return FText::GetEmpty();
	return FText::FromString(TEXT("집기"));
}
