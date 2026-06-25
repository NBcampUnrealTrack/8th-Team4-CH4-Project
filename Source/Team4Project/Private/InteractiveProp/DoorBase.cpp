#include "InteractiveProp/DoorBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

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
	OnRep_IsLocked();
}

void ADoorBase::OnRep_IsOpen()
{
	if (!bRotationsInitialized) return;

	TargetRotation = bIsOpen ? OpenRotation : ClosedRotation;
	bIsAnimating = true;
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
	ToggleDoor();
}

FText ADoorBase::GetInteractPrompt_Implementation() const
{
	if (bIsLocked) return FText::FromString(TEXT("잠김"));
	return bIsOpen ? FText::FromString(TEXT("닫기")) : FText::FromString(TEXT("열기"));
}
