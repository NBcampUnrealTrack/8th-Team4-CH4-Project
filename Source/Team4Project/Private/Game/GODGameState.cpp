#include "Game/GODGameState.h"
#include "Net/UnrealNetwork.h"
#include "Player/GODPlayerState.h"
#include "Sound/GameSoundStatics.h"
#include "Sound/GameSoundTypes.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"

AGODGameState::AGODGameState()
{
	CurrentPhase = EGamePhase::WaitingForPlayers;
	RemainingTime = 600;
	DistanceToDestination = 10000.0f;
	bGunsUnlocked = false;
	LobbyCountdown = 0;
}

void AGODGameState::BeginPlay()
{
	Super::BeginPlay();

	// 데디케이티드 서버는 들을 사람이 없으므로 감시 타이머 불필요.
	if (IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	LastSoundPhase = CurrentPhase;
	GetWorldTimerManager().SetTimer(SoundMonitorTimer, this, &AGODGameState::SoundMonitorTick, 0.25f, true);
}

void AGODGameState::SoundMonitorTick()
{
	// 페이즈 전환 사운드 (출발 / 승리 / 패배)
	if (CurrentPhase != LastSoundPhase)
	{
		PlayPhaseSound(CurrentPhase);
		LastSoundPhase = CurrentPhase;
	}

	// 경고음은 게임 진행 중에만
	if (CurrentPhase != EGamePhase::Playing)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	if (PressureLevel >= PressureWarningThreshold &&
		Now - LastPressureWarningTime >= GetPressureWarningInterval())
	{
		LastPressureWarningTime = Now;
		UGameSoundStatics::PlaySound2DFromTable(this, GameSoundTable, SoundRows::WarningPressure);
	}

	if (FuelLevel > FuelLowThreshold)
	{
		bFuelInitialized = true;
	}
	else if (bFuelInitialized && Now - LastFuelWarningTime >= FuelWarningInterval)
	{
		LastFuelWarningTime = Now;
		UGameSoundStatics::PlaySound2DFromTable(this, GameSoundTable, SoundRows::WarningFuelLow);
	}
}

float AGODGameState::GetPressureWarningInterval() const
{
	// 경고 임계값(80)에서 Max, 폭발 임계값(100)에서 Min 으로 선형 보간.
	const float Span = PressureCriticalLevel - PressureWarningThreshold;
	if (Span <= KINDA_SMALL_NUMBER)
	{
		return PressureWarningIntervalMin;
	}

	const float Alpha = FMath::Clamp((PressureLevel - PressureWarningThreshold) / Span, 0.f, 1.f);
	return FMath::Lerp(PressureWarningIntervalMax, PressureWarningIntervalMin, Alpha);
}

void AGODGameState::PlayPhaseSound(EGamePhase NewPhase)
{
	switch (NewPhase)
	{
	case EGamePhase::Playing:
		UGameSoundStatics::PlaySound2DFromTable(this, GameSoundTable, SoundRows::GameStart);
		break;

	case EGamePhase::CitizensWon:
	case EGamePhase::OutlawWon:
	case EGamePhase::MafiaWon:
		UGameSoundStatics::PlaySound2DFromTable(this, GameSoundTable,
			IsLocalPlayerWinner(NewPhase) ? SoundRows::GameVictory : SoundRows::GameDefeat);
		break;

	default:
		break;
	}
}

bool AGODGameState::IsLocalPlayerWinner(EGamePhase WinPhase) const
{
	const UGameInstance* GI = GetGameInstance();
	const APlayerController* PC = GI ? GI->GetFirstLocalPlayerController() : nullptr;
	const AGODPlayerState* PS = PC ? PC->GetPlayerState<AGODPlayerState>() : nullptr;
	if (!PS)
	{
		return false;
	}

	switch (WinPhase)
	{
	case EGamePhase::CitizensWon:
		return PS->MainRole == EMainRole::Citizen || PS->MainRole == EMainRole::Sheriff;
	case EGamePhase::MafiaWon:
		return PS->MainRole == EMainRole::Mafia;
	case EGamePhase::OutlawWon:
		return PS->MainRole == EMainRole::Outlaw;
	default:
		return false;
	}
}

void AGODGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGODGameState, CurrentPhase);
	DOREPLIFETIME(AGODGameState, RemainingTime);
	DOREPLIFETIME(AGODGameState, DistanceToDestination);
	DOREPLIFETIME(AGODGameState, PressureLevel);
	DOREPLIFETIME(AGODGameState, FuelLevel);
	DOREPLIFETIME(AGODGameState, bGunsUnlocked);
	DOREPLIFETIME(AGODGameState, LobbyCountdown);
	DOREPLIFETIME(AGODGameState, ChatHistory);
}

void AGODGameState::OnRep_GamePhase()
{
	OnGamePhaseChanged.Broadcast(CurrentPhase);
}

void AGODGameState::OnRep_RemainingTime()
{
	OnRemainingTimeChanged.Broadcast(RemainingTime);
}

void AGODGameState::OnRep_DistanceToDestination()
{
	OnDistanceToDestinationChanged.Broadcast(DistanceToDestination);
}

void AGODGameState::OnRep_PressureLevel()
{
	OnPressureLevelChanged.Broadcast(PressureLevel);
}

void AGODGameState::OnRep_FuelLevel()
{
	OnFuelLevelChanged.Broadcast(FuelLevel);
}

void AGODGameState::OnRep_bGunsUnlocked()
{
	if (bGunsUnlocked)
	{
		OnGunsUnlocked.Broadcast();
	}
}

void AGODGameState::AddChatMessage(const FChatMessage& Msg)
{
	ChatHistory.Add(Msg);

	// 일단 임의로 50개 정도만 둘게용
	if (ChatHistory.Num() > 50)
		ChatHistory.RemoveAt(0);
	
	OnChatMessageReceived.Broadcast(Msg);
	// 추가
	UE_LOG(LogTemp, Warning, TEXT("[Chat] %s: %s"), *Msg.SenderName, *Msg.Message);

}

void AGODGameState::OnRep_ChatHistory()
{
	if (ChatHistory.Num() > 0)
	{
		OnChatMessageReceived.Broadcast(ChatHistory.Last());
	}
}
