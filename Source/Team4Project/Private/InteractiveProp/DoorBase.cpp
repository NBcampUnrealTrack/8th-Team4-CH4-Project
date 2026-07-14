#include "InteractiveProp/DoorBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Sound/GameSoundStatics.h"
#include "Sound/GameSoundTypes.h"
#include "Player/BaseCharacter.h"

ADoorBase::ADoorBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrame"));
	RootComponent = DoorFrame;

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(DoorFrame);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(DoorFrame);
	InteractionVolume->SetBoxExtent(FVector(50.f, 100.f, 100.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	
	DoorFrame->SetGenerateOverlapEvents(false);
	DoorMesh->SetGenerateOverlapEvents(false);
}

void ADoorBase::BeginPlay()
{
	Super::BeginPlay();

	ClosedRotation = DoorMesh->GetRelativeRotation();
	OpenRotation = ClosedRotation + FRotator(0.f, OpenAngleDegrees, 0.f);
	TargetRotation = ClosedRotation;
	bRotationsInitialized = true;

	if (HasAuthority())
	{
		bIsOpen = bStartOpen;
		bIsLocked = bStartLocked;
	}

	// 초기 상태가 열린 경우 즉시 위치 적용 (애니메이션 없이)
	if (bIsOpen)
	{
		DoorMesh->SetRelativeRotation(OpenRotation);
		TargetRotation = OpenRotation;
	}

	bLastOpenStateForSound = bIsOpen;
}

void ADoorBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsAnimating) return;

	FRotator Current = DoorMesh->GetRelativeRotation();
	FRotator NewRot = FMath::RInterpConstantTo(Current, TargetRotation, DeltaTime, OpenSpeed);
	DoorMesh->SetRelativeRotation(NewRot);

	if (NewRot.Equals(TargetRotation, 0.1f))
	{
		DoorMesh->SetRelativeRotation(TargetRotation);
		bIsAnimating = false;
	}
}

void ADoorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADoorBase, bIsOpen);
	DOREPLIFETIME(ADoorBase, bIsLocked);
}

void ADoorBase::ToggleDoor()
{
	if (!HasAuthority() || bIsLocked) return;

	bIsOpen = !bIsOpen;
	OnRep_IsOpen(); // 서버 측 즉시 애니메이션 시작
}

void ADoorBase::SetLocked(bool bLock)
{
	if (!HasAuthority()) return;

	bIsLocked = bLock;

	// Actor Tag 동기화 (GA_UnlockDoor의 ActorHasTag 체크와 일치시키기 위해)
	if (bLock)
		Tags.AddUnique(TEXT("Door.Locked"));
	else
		Tags.Remove(TEXT("Door.Locked"));

	OnRep_IsLocked();
}

void ADoorBase::OnRep_IsOpen()
{
	if (!bRotationsInitialized) return;

	TargetRotation = bIsOpen ? OpenRotation : ClosedRotation;
	bIsAnimating = true;
	ApplyDoorCollision();

	// 개폐 사운드 — 서버(ToggleDoor 의 수동 호출)와 클라(복제) 양쪽에서 재생.
	// 접속 직후 초기 복제로 상태만 맞춰질 때(GameTime 0 부근)는 소리를 내지 않는다.
	if (bIsOpen != bLastOpenStateForSound)
	{
		bLastOpenStateForSound = bIsOpen;
		if (GetGameTimeSinceCreation() > 1.f)
		{
			UGameSoundStatics::PlaySoundAtLocationFromTable(this, SoundTable,
				bIsOpen ? SoundRows::DoorOpen : SoundRows::DoorClose, GetActorLocation());
		}
	}
}

void ADoorBase::OnRep_IsLocked()
{
	// 잠기면 강제로 닫음
	if (bIsLocked && bIsOpen)
	{
		bIsOpen = false;
		OnRep_IsOpen();
	}
}

void ADoorBase::Interact_Implementation(ACharacter* Interactor)
{
	if (!HasAuthority() || !Interactor) return;

	// 역할별 GA 이벤트 전송 (ActivationRequiredTags가 역할 판정)
	// MasterKey(마피아): 잠금 / UnlockDoor(보안관): 잠금 해제
	FGameplayEventData EventData;
	EventData.OptionalObject = this;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Interactor, FGameplayTag::RequestGameplayTag(TEXT("Ability.Trigger.MasterKey")), EventData);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Interactor, FGameplayTag::RequestGameplayTag(TEXT("Ability.Trigger.UnlockDoor")), EventData);


	if (bIsLocked) return;

	// 토글하기 전에 문이 원래 '열려있었는지' 상태를 먼저 기억
	bool bWasOpen = bIsOpen;

	// 일반 개폐 (잠긴 상태면 ToggleDoor 내부에서 무시됨)
	ToggleDoor();

	// 나랑 상호작용한 Interactor가 BaseCharacter인지 확인
	if (ABaseCharacter* BaseChar = Cast<ABaseCharacter>(Interactor))
	{
		// 방금 닫혀있었으면(!bWasOpen) -> 여는 애니메이션(true)을 재생하라고 명령
		// 방금 열려있었으면(bWasOpen) -> 닫는 애니메이션(false)을 재생하라고 명령
		BaseChar->Multicast_PlayDoorMontage(!bWasOpen);
	}
}

FText ADoorBase::GetInteractPrompt_Implementation() const
{
	if (bIsLocked) return FText::FromString(TEXT("잠김"));
	return bIsOpen ? FText::FromString(TEXT("닫기")) : FText::FromString(TEXT("열기"));
}

void ADoorBase::ApplyDoorCollision()
{
	const ECollisionEnabled::Type NewCollision = bIsOpen
			 ? ECollisionEnabled::NoCollision
			 : ECollisionEnabled::QueryAndPhysics;
	DoorFrame->SetCollisionEnabled(NewCollision);
	DoorMesh->SetCollisionEnabled(NewCollision);
}
