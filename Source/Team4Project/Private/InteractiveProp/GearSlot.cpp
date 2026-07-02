#include "InteractiveProp/GearSlot.h"
#include "InteractiveProp/PickupGear.h"
#include "Player/BaseCharacter.h"
#include "Component/PressureComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

AGearSlot::AGearSlot()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SlotMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlotMesh"));
	RootComponent = SlotMesh;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(60.f, 60.f, 60.f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AGearSlot::BeginPlay()
{
	Super::BeginPlay();

	if (!MountedGear)
	{
		MountedGear = ResolveMountedGear();
	}

	if (MountedGear)
	{
		OriginalGearTransform = MountedGear->GetActorTransform();
		MountedGear->Tags.AddUnique(FName(TEXT("Gear.Mounted")));
	}
}

APickupGear* AGearSlot::ResolveMountedGear() const
{
	// 1) Child Actor Component로 붙인 PickupGear 탐색
	TArray<UChildActorComponent*> ChildActorComps;
	GetComponents<UChildActorComponent>(ChildActorComps);
	for (UChildActorComponent* Comp : ChildActorComps)
	{
		if (APickupGear* Gear = Cast<APickupGear>(Comp->GetChildActor()))
		{
			return Gear;
		}
	}

	// 2) 일반 액터 부착(AttachToActor)으로 붙인 경우 대비
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	for (AActor* Actor : AttachedActors)
	{
		if (APickupGear* Gear = Cast<APickupGear>(Actor))
		{
			return Gear;
		}
	}

	return nullptr;
}

void AGearSlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGearSlot, bIsAssembled);
	DOREPLIFETIME(AGearSlot, bQTEActive);
	DOREPLIFETIME(AGearSlot, QTESequence);
	DOREPLIFETIME(AGearSlot, QTEProgressIndex);
	DOREPLIFETIME(AGearSlot, QTEStartServerTime);
}

void AGearSlot::BreakGear()
{
	if (!HasAuthority() || !bIsAssembled || !IsValid(MountedGear)) return;

	MountedGear->Tags.Remove(FName(TEXT("Gear.Mounted")));

	if (MountedGear->Mesh)
	{
		MountedGear->Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MountedGear->Mesh->SetSimulatePhysics(true);

		FVector Dir = (MountedGear->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			Dir = GetActorForwardVector();
		}

		MountedGear->Mesh->AddImpulse(Dir * EjectImpulseStrength + FVector::UpVector * 150.f, NAME_None, true);
	}

	bIsAssembled = false;
	bQTEActive = false;
	QTEProgressIndex = 0;
	QTEPlayer = nullptr;
}

void AGearSlot::ForceReassemble()
{
	if (!HasAuthority() || bIsAssembled) return;
	CompleteRepair();
}

void AGearSlot::CompleteRepair()
{
	if (!IsValid(MountedGear)) return;

	GetWorldTimerManager().ClearTimer(QTETimeoutHandle);

	if (MountedGear->bIsHeld)
	{
		MountedGear->Server_Drop();
	}

	MountedGear->SetActorTransform(OriginalGearTransform);

	if (MountedGear->Mesh)
	{
		MountedGear->Mesh->SetSimulatePhysics(false);
		MountedGear->Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	MountedGear->Tags.AddUnique(FName(TEXT("Gear.Mounted")));

	bIsAssembled = true;
	bQTEActive = false;
	QTEProgressIndex = 0;
	QTEPlayer = nullptr;
}

void AGearSlot::StartQTE(ABaseCharacter* Player)
{
	bQTEActive = true;
	QTEProgressIndex = 0;
	QTEStartServerTime = GetWorld()->GetTimeSeconds();

	QTESequence.Empty(QTESequenceLength);
	for (int32 i = 0; i < QTESequenceLength; ++i)
	{
		QTESequence.Add(static_cast<uint8>(FMath::RandRange(0, 3)));
	}

	QTEPlayer = Player;

	GetWorldTimerManager().SetTimer(QTETimeoutHandle, this, &AGearSlot::OnQTETimeout, QTETimeLimit, false);

	Player->Client_StartGearQTE(this);
}

void AGearSlot::SubmitQTEInput(EQTEDirection Dir)
{
	if (!HasAuthority() || !bQTEActive) return;

	if (QTESequence.IsValidIndex(QTEProgressIndex) && QTESequence[QTEProgressIndex] == static_cast<uint8>(Dir))
	{
		++QTEProgressIndex;

		if (QTEProgressIndex >= QTESequence.Num())
		{
			ABaseCharacter* SuccessPlayer = QTEPlayer;
			CompleteRepair();
			if (SuccessPlayer)
			{
				SuccessPlayer->Client_EndGearQTE(true);
			}
		}
	}
	else
	{
		QTEProgressIndex = 0;
	}
}

void AGearSlot::OnQTETimeout()
{
	if (!bQTEActive) return;

	bQTEActive = false;
	QTEProgressIndex = 0;

	ABaseCharacter* FailedPlayer = QTEPlayer;
	QTEPlayer = nullptr;

	if (FailedPlayer)
	{
		FailedPlayer->Client_EndGearQTE(false);
	}

	if (UPressureComponent* Pressure = GetPressureComponent())
	{
		Pressure->ForceExplode();
	}
}

UPressureComponent* AGearSlot::GetPressureComponent() const
{
	// 1) 디테일에서 명시적으로 지정한 TrainActor 우선
	if (TrainActor)
	{
		if (UPressureComponent* Pressure = TrainActor->FindComponentByClass<UPressureComponent>())
		{
			return Pressure;
		}
	}

	// 2) 미지정이면 부모 액터에서 자동 탐색 (CoalFeeder::GetFurnace()와 동일한 패턴)
	AActor* Parent = GetAttachParentActor();
	if (!Parent)
	{
		Parent = GetParentActor();
	}
	for (; Parent; Parent = Parent->GetAttachParentActor())
	{
		if (UPressureComponent* Pressure = Parent->FindComponentByClass<UPressureComponent>())
		{
			return Pressure;
		}
	}

	return nullptr;
}

void AGearSlot::Interact_Implementation(ACharacter* Interactor)
{
	if (bIsAssembled || bQTEActive) return;

	ABaseCharacter* BaseChar = Cast<ABaseCharacter>(Interactor);
	if (!BaseChar || BaseChar->GetCurrentHeldItem() != MountedGear) return;

	if (BaseChar->CanInstantFixGear())
	{
		CompleteRepair();
		return;
	}

	StartQTE(BaseChar);
}

FText AGearSlot::GetInteractPrompt_Implementation() const
{
	if (bIsAssembled || bQTEActive) return FText::GetEmpty();

	return FText::FromString(TEXT("기어 조립"));
}
