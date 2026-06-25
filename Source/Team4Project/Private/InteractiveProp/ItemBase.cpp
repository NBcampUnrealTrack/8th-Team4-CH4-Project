#include "InteractiveProp/ItemBase.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
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
	if (!Holder || bIsHeld) return;

	bIsHeld = true;
	HolderCharacter = Holder;
	SetPhysicsForHeld(true);

	if (USkeletalMeshComponent* SkelMesh = Holder->GetMesh())
	{
		AttachToComponent(SkelMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName);
	}
}

void AItemBase::Server_Drop_Implementation()
{
	if (!bIsHeld) return;

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
	return FText::FromString(TEXT("집기"));
}
