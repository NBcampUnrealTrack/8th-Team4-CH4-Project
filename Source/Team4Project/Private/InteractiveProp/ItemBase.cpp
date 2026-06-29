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
	
	ABaseCharacter* BaseChar = Cast<ABaseCharacter>(Holder);
	if (BaseChar)
	{
		BaseChar->ClearEquipSlot();
	}

	bIsHeld = true;
	HolderCharacter = Holder;
	
	RefreshHeldAttachment();
	
	if (BaseChar)
	{
		BaseChar->SetCurrentHeldItem(this);

		// 무게 부여 → 속도 감소(짐꾼 면제). 떨굴 때 동일량 차감.
		if (WeightAmount != 0.f)
		{
			BaseChar->AddWeight(WeightAmount);
		}

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

		// 줍기 때 더한 무게를 되돌린다.
		if (WeightAmount != 0.f)
		{
			BaseChar->AddWeight(-WeightAmount);
		}
	}

	bIsHeld = false;
	HolderCharacter = nullptr;

	// 서버에서 즉시 분리(클라는 OnRep 에서 동일하게 분리).
	RefreshHeldAttachment();
}

void AItemBase::OnRep_bIsHeld()
{
	RefreshHeldAttachment();
}

void AItemBase::OnRep_HolderCharacter()
{
	RefreshHeldAttachment();
}

void AItemBase::RefreshHeldAttachment()
{
	// bIsHeld 와 HolderCharacter 는 따로 복제되어 도착 순서가 보장되지 않는다.
	// 둘 다 유효할 때만 부착하고, 그 외에는 분리한다. (서버/클라 공통)
	if (bIsHeld && HolderCharacter)
	{
		SetPhysicsForHeld(true);

		if (USkeletalMeshComponent* SkelMesh = HolderCharacter->GetMesh())
		{
			AttachToComponent(SkelMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				AttachSocketName);
		}
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetPhysicsForHeld(false);
	}
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
