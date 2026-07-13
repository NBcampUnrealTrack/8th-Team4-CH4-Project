#include "Quest/QuestStation.h"
#include "Player/BaseCharacter.h"
#include "Game/GODGameState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

AQuestStation::AQuestStation()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	RootComponent = StationMesh;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	InteractionBox->SetGenerateOverlapEvents(true);
}

void AQuestStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AQuestStation, QuestPlayer);
}

bool AQuestStation::GetQuestRow(FQuestRow& OutRow) const
{
	if (!QuestTable || QuestId.IsNone()) return false;

	if (const FQuestRow* Row = QuestTable->FindRow<FQuestRow>(QuestId, TEXT("AQuestStation"), /*bWarnIfMissing=*/false))
	{
		OutRow = *Row;
		return true;
	}
	return false;
}

FText AQuestStation::GetDisplayName() const
{
	FQuestRow Row;
	return GetQuestRow(Row) ? Row.DisplayName : FText::GetEmpty();
}

FText AQuestStation::GetLocationHint() const
{
	FQuestRow Row;
	return GetQuestRow(Row) ? Row.LocationHint : FText::GetEmpty();
}

float AQuestStation::GetMinDuration() const
{
	FQuestRow Row;
	return GetQuestRow(Row) ? Row.MinDuration : 0.f;
}

TSubclassOf<UQuestMinigameWidget> AQuestStation::GetWidgetClass() const
{
	FQuestRow Row;
	return GetQuestRow(Row) ? Row.WidgetClass : nullptr;
}

bool AQuestStation::IsUsableNow() const
{
	if (IsBusy()) return false;

	const AGODGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGODGameState>() : nullptr;
	return GS && GS->CurrentPhase == EGamePhase::Playing;
}

void AQuestStation::Interact_Implementation(ACharacter* Interactor)
{
	if (!HasAuthority() || !IsUsableNow()) return;

	ABaseCharacter* BaseChar = Cast<ABaseCharacter>(Interactor);
	if (!BaseChar || BaseChar->IsDead()) return;

	StartQuest(BaseChar);
}

FText AQuestStation::GetInteractPrompt_Implementation() const
{
	if (!IsUsableNow()) return FText::GetEmpty();
	return GetDisplayName();
}

void AQuestStation::StartQuest(ABaseCharacter* Player)
{
	if (!HasAuthority() || !Player) return;

	QuestPlayer = Player;
	StartServerTime = GetWorld()->GetTimeSeconds();

	// 서버 쪽에도 활성 스테이션을 기록한다.
	Player->SetActiveQuestStation(this);

	Player->OnCharacterDied.AddDynamic(this, &AQuestStation::OnQuestPlayerDied);

	Player->Client_StartQuestMinigame(this);
}

ABaseCharacter* AQuestStation::ReleaseQuestPlayer()
{
	ABaseCharacter* Player = QuestPlayer;
	QuestPlayer = nullptr;

	if (Player)
	{
		Player->SetActiveQuestStation(nullptr);
		Player->OnCharacterDied.RemoveDynamic(this, &AQuestStation::OnQuestPlayerDied);
	}
	return Player;
}

void AQuestStation::OnQuestPlayerDied(ABaseCharacter* /*DeadCharacter*/, AActor* /*Killer*/)
{
	AbortQuest();
}

void AQuestStation::AbortQuest()
{
	if (!HasAuthority() || !IsBusy()) return;

	ABaseCharacter* Player = ReleaseQuestPlayer();
	if (IsValid(Player))
	{
		Player->Client_EndQuestMinigame(false);
	}
}

bool AQuestStation::HasMinDurationElapsed() const
{
	return (GetWorld()->GetTimeSeconds() - StartServerTime) >= GetMinDuration();
}
