#include "InteractiveProp/CoalPile.h"
#include "InteractiveProp/CoalItem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

ACoalPile::ACoalPile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	PileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PileMesh"));
	PileMesh->SetupAttachment(SceneRoot);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(SceneRoot);
	InteractionBox->SetBoxExtent(FVector(80.f, 80.f, 60.f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	InteractionBox->SetGenerateOverlapEvents(true);
}

void ACoalPile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoalPile, CurrentCoalCount);
}

void ACoalPile::Interact_Implementation(ACharacter* Interactor)
{
	
	if (!HasAuthority() || !Interactor) return;

	if (CurrentCoalCount <= 0) return;

	if (!CoalItemClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CoalPile] CoalItemClass 미지정: %s"), *GetName());
		return;
	}

	// 쿨다운 체크 (연속 배출 방지)
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastDispenseTime < DispenseCooldown) return;
	LastDispenseTime = Now;

	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector SpawnLoc = GetActorTransform().TransformPosition(DispenseOffset);
	ACoalItem* Coal = GetWorld()->SpawnActor<ACoalItem>(
		CoalItemClass, SpawnLoc, FRotator::ZeroRotator, Params);
	if (!Coal) return;

	SetCoalCount(CurrentCoalCount - 1);
}

FText ACoalPile::GetInteractPrompt_Implementation() const
{
	if (CurrentCoalCount <= 0)
	{
		return FText::FromString(TEXT("석탄 없음"));
	}
	return FText::FromString(FString::Printf(TEXT("석탄 꺼내기 (%d개)"), CurrentCoalCount));
}

void ACoalPile::AddCoal(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0) return;
	SetCoalCount(CurrentCoalCount + Amount);
}

void ACoalPile::SetCoalCount(int32 NewCount)
{
	NewCount = FMath::Clamp(NewCount, 0, MaxCoalCount);
	if (CurrentCoalCount == NewCount) return;

	CurrentCoalCount = NewCount;

	
	OnCoalCountChanged(CurrentCoalCount, MaxCoalCount);
}

void ACoalPile::OnRep_CurrentCoalCount()
{
	OnCoalCountChanged(CurrentCoalCount, MaxCoalCount);
}

