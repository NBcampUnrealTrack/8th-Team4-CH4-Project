#include "Component/InteractComponent.h"
#include "Interface/Interactable.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Player/BaseCharacter.h"
#include "InteractiveProp/ItemBase.h"

UInteractComponent::UInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInteractComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// 오너 액터에 SphereComponent를 동적으로 추가하고 루트에 부착
	InteractSphere = NewObject<USphereComponent>(Owner, TEXT("InteractSphere"));
	InteractSphere->SetSphereRadius(InteractRadius);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap); // 문, 밸브 등
	InteractSphere->SetCollisionResponseToChannel(ECC_WorldStatic,  ECR_Overlap); // 고정 오브젝트
	InteractSphere->SetCollisionResponseToChannel(ECC_PhysicsBody,  ECR_Overlap); // 물리 아이템 + 래그돌 시체
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn,         ECR_Overlap); // 생존 캐릭터 (수색/탄약빼앗기)
	InteractSphere->SetGenerateOverlapEvents(true);
	InteractSphere->RegisterComponent();
	InteractSphere->AttachToComponent(
		Owner->GetRootComponent(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	InteractSphere->OnComponentBeginOverlap.AddDynamic(this, &UInteractComponent::OnOverlapBegin);
	InteractSphere->OnComponentEndOverlap.AddDynamic(this, &UInteractComponent::OnOverlapEnd);
}

void UInteractComponent::TryInteract()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastInteractTime < InteractCooldown) return;
	LastInteractTime = Now;
	Server_TryInteract();
}

void UInteractComponent::Server_TryInteract_Implementation()
{
	AActor* Target = GetClosestInteractable();
	if (!Target) return;

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	IInteractable::Execute_Interact(Target, OwnerChar);
}

FText UInteractComponent::GetCurrentPrompt() const
{
	AActor* Target = GetClosestInteractable();
	if (!Target) return FText::GetEmpty();
	return IInteractable::Execute_GetInteractPrompt(Target);
}

void UInteractComponent::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != GetOwner() && OtherActor->Implements<UInteractable>())
	{
		NearbyInteractables.AddUnique(OtherActor);
	}
}

void UInteractComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	NearbyInteractables.Remove(OtherActor);
}

AActor* UInteractComponent::GetHeldItemActor() const
{
	if (const ABaseCharacter* BaseChar = Cast<ABaseCharacter>(GetOwner()))
	{
		return BaseChar->GetCurrentHeldItem();
	}
	return nullptr;
}

AActor* UInteractComponent::GetClosestInteractable() const
{
	AActor* Closest = nullptr;
	float BestDistSq = FLT_MAX;
	const FVector OwnerLoc = GetOwner()->GetActorLocation();
	const AActor* HeldItem = GetHeldItemActor();

	for (AActor* Actor : NearbyInteractables)
	{
		if (!IsValid(Actor) || Actor == HeldItem) continue;
		const float DistSq = FVector::DistSquared(OwnerLoc, Actor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Closest = Actor;
		}
	}
	return Closest;
}

AActor* UInteractComponent::GetClosestInteractableOfClass(TSubclassOf<AActor> ActorClass) const
{
	if (!ActorClass) return nullptr;

	AActor* Closest = nullptr;
	float BestDistSq = FLT_MAX;
	const FVector OwnerLoc = GetOwner()->GetActorLocation();
	const AActor* HeldItem = GetHeldItemActor();

	for (AActor* Actor : NearbyInteractables)
	{
		if (!IsValid(Actor) || Actor == HeldItem || !Actor->IsA(ActorClass)) continue;
		const float DistSq = FVector::DistSquared(OwnerLoc, Actor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Closest = Actor;
		}
	}
	return Closest;
}
