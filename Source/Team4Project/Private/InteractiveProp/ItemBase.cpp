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

	// 무게를 물리 질량(kg)으로 반영 → 던질 때 임펄스가 질량으로 나뉘어 무게 비례로 동작한다.
	if (Mesh && WeightAmount > 0.f)
	{
		Mesh->SetMassOverrideInKg(NAME_None, WeightAmount, true);
	}
}

void AItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AItemBase, bIsHeld);
	DOREPLIFETIME(AItemBase, HolderCharacter);
}

void AItemBase::Server_PickUp_Implementation(ACharacter* Holder)
{
	if (!Holder || bIsHeld || ActorHasTag(TEXT("Gear.Destroyed")) || ActorHasTag(TEXT("Gear.Mounted"))) return;
	
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

	// 안전 드롭 계산(위치/열차 속도 상속)에 쓰려고 홀더를 미리 캡처. (아래에서 null 로 비움)
	ACharacter* Dropper = HolderCharacter;

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

	// 안전하게 내려놓기: 캐릭터와 겹쳐 튕기는 것 방지 + 달리는 열차 속도 상속.
	DropSafely(Dropper);
}

void AItemBase::DropSafely(ACharacter* Dropper)
{
	if (!Dropper || !Mesh) return;

	// 1) 몸 안에서 튀어나오는 느낌 방지로 앞쪽 살짝 위에 놓는다.
	//    폰 충돌은 이미 무시라 겹쳐도 캐릭터를 밀지 않는다(=돌진/캐릭터 날아감 없음).
	const FVector DropLoc = Dropper->GetActorLocation()
		+ Dropper->GetActorForwardVector() * DropForwardOffset
		+ FVector(0.f, 0.f, DropUpOffset);
	SetActorLocation(DropLoc, false, nullptr, ETeleportType::TeleportPhysics);

	// 2) 속도 세팅: 달리는 열차와 함께 가도록 발판(열차) 속도를 상속하고,
	//    그 위에 앞으로 "톡" 던지는 정도(DropTossSpeed)만 더한다.
	//    캐릭터 GetVelocity()는 based-movement(열차 이동)를 포함하지 않으므로 발판 속도를 따로 더한다.
	FVector InheritVel = Dropper->GetVelocity();
	if (UPrimitiveComponent* Base = Dropper->GetMovementBase())
	{
		if (const AActor* BaseOwner = Base->GetOwner())
		{
			InheritVel += BaseOwner->GetVelocity(); // AGODTrain::GetVelocity() → TrainVelocity
		}
	}
	Mesh->SetPhysicsLinearVelocity(InheritVel);
	Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	// 앞으로 던지는 힘은 임펄스로 준다(bVelChange=false → 질량 반영).
	// 같은 임펄스라도 질량(=WeightAmount)이 클수록 속도가 작아져 무거운 아이템이 덜 날아간다.
	Mesh->AddImpulse(Dropper->GetActorForwardVector() * DropTossImpulse, NAME_None, false);
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

		// 떨군 아이템이 캐릭터(폰)를 밀쳐 날려버리지 않도록 폰 채널과는 충돌하지 않게 한다.
		// 바닥/발판(WorldStatic/Dynamic)과는 그대로 충돌하므로 열차 위에 잘 안착한다.
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

		Mesh->SetSimulatePhysics(true);

		// 떨궜을 때 굴러가지 않도록 감쇠/마찰 적용.
		Mesh->SetLinearDamping(DroppedLinearDamping);
		Mesh->SetAngularDamping(DroppedAngularDamping);
		if (DroppedPhysMaterial)
		{
			Mesh->SetPhysMaterialOverride(DroppedPhysMaterial);
		}
	}
}

void AItemBase::Interact_Implementation(ACharacter* Interactor)
{
	Server_PickUp(Interactor);
}

FText AItemBase::GetInteractPrompt_Implementation() const
{
	if (ActorHasTag(TEXT("Gear.Destroyed")) || ActorHasTag(TEXT("Gear.Mounted")))
		return FText::GetEmpty();
	return FText::FromString(TEXT("집기"));
}
