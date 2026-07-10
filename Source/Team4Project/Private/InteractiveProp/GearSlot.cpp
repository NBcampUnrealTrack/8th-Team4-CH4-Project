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
#include "Game/GODGameState.h"
#include "Sound/GameSoundStatics.h"
#include "Sound/GameSoundTypes.h"

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
	ReleaseQTEPlayer();

	Multicast_PlaySlotSound(SoundRows::GearBreak);

	if (AGODGameState* GS = GetWorld()->GetGameState<AGODGameState>())
	{
		GS->Announce(NSLOCTEXT("Announce", "GearBroken", "엔진 기어 파손 감지 — 열차 감속"),
			EAnnouncementType::Warning);
	}
}

void AGearSlot::ForceReassemble()
{
	if (!HasAuthority() || bIsAssembled) return;

	// 라운드 초기화 경로. StartGame 은 이미 CurrentPhase 를 Playing 으로 바꾼 뒤에 이 함수를 부르므로
	// Announce 의 페이즈 게이트로는 막히지 않는다. 여기서 명시적으로 방송을 끈다.
	CompleteRepair(/*bAnnounce=*/false);
}

void AGearSlot::CompleteRepair(bool bAnnounce)
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

	Multicast_PlaySlotSound(SoundRows::GearRepair);

	if (bAnnounce)
	{
		if (AGODGameState* GS = GetWorld()->GetGameState<AGODGameState>())
		{
			GS->Announce(NSLOCTEXT("Announce", "GearRepaired", "엔진 기어 수리 완료"),
				EAnnouncementType::Info);
		}
	}

	// QTE 진행 중 완료(정상 성공/라운드 재시작 ForceReassemble 포함)라면 플레이어에게
	// 종료를 알린다. 안 보내면 클라 쪽 bInputLockedByMinigame 이 남아 조작 불능이 된다.
	if (ABaseCharacter* Player = ReleaseQTEPlayer())
	{
		Player->Client_EndGearQTE(true);
	}
}

ABaseCharacter* AGearSlot::ReleaseQTEPlayer()
{
	ABaseCharacter* Player = QTEPlayer;
	QTEPlayer = nullptr;
	if (Player)
	{
		Player->ActiveGearQTESlot = nullptr;
		Player->OnCharacterDied.RemoveDynamic(this, &AGearSlot::OnQTEPlayerDied);
	}
	return Player;
}

void AGearSlot::OnQTEPlayerDied(ABaseCharacter* /*DeadCharacter*/, AActor* /*Killer*/)
{
	AbortQTE();
}

void AGearSlot::AbortQTE()
{
	if (!HasAuthority() || !bQTEActive) return;

	GetWorldTimerManager().ClearTimer(QTETimeoutHandle);
	bQTEActive = false;
	QTEProgressIndex = 0;

	ABaseCharacter* Player = ReleaseQTEPlayer();
	if (IsValid(Player)) Player->Client_EndGearQTE(false);
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

	Player->ActiveGearQTESlot = this;

	// 진행 중 사망하면 실패(폭발) 없이 중단하도록 감시.
	Player->OnCharacterDied.AddDynamic(this, &AGearSlot::OnQTEPlayerDied);

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
			CompleteRepair(); // 내부에서 QTE 종료(성공) 알림까지 처리
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

	// 플레이어가 나갔거나(폰 파괴) 죽었으면 실패(폭발) 처리 없이 중단.
	if (!IsValid(QTEPlayer) || QTEPlayer->IsDead())
	{
		AbortQTE();
		return;
	}

	bQTEActive = false;
	QTEProgressIndex = 0;

	ABaseCharacter* FailedPlayer = ReleaseQTEPlayer();
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

void AGearSlot::Multicast_PlaySlotSound_Implementation(FName RowName)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	UGameSoundStatics::PlaySoundAtLocationFromTable(this, SoundTable, RowName, GetActorLocation());
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
