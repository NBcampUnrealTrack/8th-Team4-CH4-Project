#include "InteractiveProp/GearSlot.h"
#include "InteractiveProp/PickupGear.h"
#include "Player/BaseCharacter.h"
#include "Component/PressureComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
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

	if (MountedGear)
	{
		OriginalGearTransform = MountedGear->GetActorTransform();
		MountedGear->Tags.AddUnique(FName(TEXT("Gear.Mounted")));
	}
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

	if (TrainActor)
	{
		if (UPressureComponent* Pressure = TrainActor->FindComponentByClass<UPressureComponent>())
		{
			Pressure->ForceExplode();
		}
	}
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
