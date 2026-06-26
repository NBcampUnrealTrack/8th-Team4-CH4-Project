#include "InteractiveProp/PressureValve.h"
#include "Component/PressureComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

APressureValve::APressureValve()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	ValveMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ValveMesh"));
	RootComponent = ValveMesh;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(60.f, 60.f, 60.f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void APressureValve::BeginPlay()
{
	Super::BeginPlay();
}

void APressureValve::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || !bIsTurning) return;

	if (UPressureComponent* PC = GetPressureComponent())
	{
		PC->ReducePressure(ReducePerSecond * DeltaTime);
	}

	// 밸브 시각 회전 (초당 180도)
	ValveRotation = FMath::Fmod(ValveRotation + 180.f * DeltaTime, 360.f);
}

void APressureValve::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APressureValve, ValveRotation);
	DOREPLIFETIME(APressureValve, bIsTurning);
}

void APressureValve::Server_StartTurning_Implementation(AController* Operator)
{
	// 화부가 강제 차단한 밸브는 재조작 불가
	if (bIsTurning || ActorHasTag(TEXT("Stoker.ForceClose"))) return;
	bIsTurning = true;
}

void APressureValve::Server_StopTurning_Implementation()
{
	bIsTurning = false;
}

void APressureValve::OnRep_ValveRotation()
{
	// ValveRotation으로 메시 Roll 회전 (축은 BP에서 필요 시 조정)
	if (ValveMesh)
	{
		FRotator Current = ValveMesh->GetRelativeRotation();
		Current.Roll = ValveRotation;
		ValveMesh->SetRelativeRotation(Current);
	}
}

UPressureComponent* APressureValve::GetPressureComponent() const
{
	if (!TrainActor) return nullptr;
	return TrainActor->FindComponentByClass<UPressureComponent>();
}

void APressureValve::Interact_Implementation(ACharacter* Interactor)
{
	if (bIsTurning)
		Server_StopTurning();
	else
		Server_StartTurning(Interactor ? Interactor->GetController() : nullptr);
}

FText APressureValve::GetInteractPrompt_Implementation() const
{
	return bIsTurning ? FText::FromString(TEXT("밸브 정지")) : FText::FromString(TEXT("밸브 조작"));
}
