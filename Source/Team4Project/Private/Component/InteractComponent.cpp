#include "Component/InteractComponent.h"
#include "Interface/Interactable.h"
#include "Components/SphereComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Character.h"
#include "Player/BaseCharacter.h"
#include "InteractiveProp/ItemBase.h"
#include "TimerManager.h"

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

	// 대상 변화 감지 타이머. 최근접 판정은 거리 기반이라 순수 이벤트로는 못 잡고,
	// 낮은 주기 폴링 후 "바뀌었을 때만" 브로드캐스트한다 (UI 쪽 틱 바인딩 제거 목적).
	if (TargetCheckInterval > 0.f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			TargetCheckTimer, this, &UInteractComponent::CheckTargetChanged,
			TargetCheckInterval, /*bLoop=*/true);
	}
}

void UInteractComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TargetCheckTimer);
	}
	// 종료 시 남아있는 하이라이트 제거 (포커스 중 폰 파괴/레벨 전환 대비)
	SetActorHighlight(LastTarget.Get(), false);
	Super::EndPlay(EndPlayReason);
}

void UInteractComponent::CheckTargetChanged()
{
	// 프롬프트 UI는 로컬 플레이어에게만 의미 있음 (서버/타 클라 사본은 스킵)
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	AActor* Target = GetClosestInteractable();
	const FText Prompt = Target
		? IInteractable::Execute_GetInteractPrompt(Target)
		: FText::GetEmpty();

	// 대상이 바뀌었거나, 같은 대상이라도 문구가 바뀐 경우(문 잠김↔열림 등)만 알림
	if (Target != LastTarget.Get() || !Prompt.EqualTo(LastPrompt))
	{
		// 포커스 대상이 실제로 바뀐 경우에만 하이라이트를 이전 대상→새 대상으로 옮긴다.
		// (같은 대상에서 프롬프트 문구만 바뀐 경우는 오버레이 유지)
		if (Target != LastTarget.Get())
		{
			SetActorHighlight(LastTarget.Get(), false);
			SetActorHighlight(Target, true);
		}

		LastTarget = Target;
		LastPrompt = Prompt;
		OnInteractTargetChanged.Broadcast(Target, Prompt);
	}
}

void UInteractComponent::SetActorHighlight(AActor* Actor, bool bEnable)
{
	if (!IsValid(Actor)) return;

	// 액터의 모든 메시를 CustomDepth 스텐실로 마킹/해제 → 툰 PP가 값 2를 하이라이트로 그린다.
	// (Nanite 호환. 캐릭터 실루엣은 값 1을 쓰므로 서로 안 겹침)
	TArray<UMeshComponent*> Meshes;
	Actor->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (bEnable)
		{
			Mesh->SetCustomDepthStencilValue(HighlightStencilValue);
		}
		Mesh->SetRenderCustomDepth(bEnable);
	}
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
