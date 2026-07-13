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
	DistanceToDestination = 120000.0f; // 열차 BeginPlay에서 GODTrain::TotalDistance 로 덮어씀. 동일값 유지.
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
	// 페이즈 전환 사운드 (출발 / 승리 / 패배 / 소집)
	if (CurrentPhase != LastSoundPhase)
	{
		PlayPhaseSound(CurrentPhase, LastSoundPhase);
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

void AGODGameState::PlayPhaseSound(EGamePhase NewPhase, EGamePhase OldPhase)
{
	switch (NewPhase)
	{
	case EGamePhase::Playing:
		// 회의 종료 복귀(Meeting→Playing)에는 출발 경적을 다시 울리지 않는다.
		if (OldPhase != EGamePhase::Meeting)
		{
			UGameSoundStatics::PlaySound2DFromTable(this, GameSoundTable, SoundRows::GameStart);
		}
		break;

	case EGamePhase::Meeting:
		UGameSoundStatics::PlaySound2DFromTable(this, GameSoundTable, SoundRows::MeetingCall);
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
		// 무법자는 전향 전(초반 5분)까지만 시민 사이드.
		return PS->MainRole == EMainRole::Citizen || PS->MainRole == EMainRole::Sheriff
			|| (PS->MainRole == EMainRole::Outlaw && !PS->bTurnedToMafia);
	case EGamePhase::MafiaWon:
		// 전향한 무법자는 마피아 사이드로 함께 승리.
		return PS->MainRole == EMainRole::Mafia
			|| (PS->MainRole == EMainRole::Outlaw && PS->bTurnedToMafia);
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
	DOREPLIFETIME(AGODGameState, QuestSpeedMultiplier);
	DOREPLIFETIME(AGODGameState, QuestCompletedCitizens);
	DOREPLIFETIME(AGODGameState, QuestTotalCitizens);
	DOREPLIFETIME(AGODGameState, LobbyCountdown);
	DOREPLIFETIME(AGODGameState, MeetingRemainingTime);
	DOREPLIFETIME(AGODGameState, bMeetingBellReady);
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

void AGODGameState::OnRep_MeetingRemainingTime()
{
	OnMeetingTimeChanged.Broadcast(MeetingRemainingTime);
}

void AGODGameState::OnRep_DistanceToDestination()
{
	OnDistanceToDestinationChanged.Broadcast(DistanceToDestination);
}

void AGODGameState::OnRep_QuestProgress()
{
	OnQuestProgressChanged.Broadcast(QuestSpeedMultiplier, QuestCompletedCitizens, QuestTotalCitizens);
}

void AGODGameState::OnRep_PressureLevel()
{
	OnPressureLevelChanged.Broadcast(PressureLevel);
}

void AGODGameState::OnRep_FuelLevel()
{
	OnFuelLevelChanged.Broadcast(FuelLevel);
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

void AGODGameState::Announce(const FText& Message, EAnnouncementType Type)
{
	// 라운드 초기화(ForceReassemble 등)나 로비 중의 상태 변화가 방송되지 않도록 페이즈로 막는다.
	// 긴급 회의 중에는 소집/종료 알림이 나가야 하므로 Meeting 도 허용한다.
	if (!HasAuthority() ||
		(CurrentPhase != EGamePhase::Playing && CurrentPhase != EGamePhase::Meeting))
	{
		return;
	}

	Multicast_Announce(Message, Type);
}

void AGODGameState::Multicast_Announce_Implementation(const FText& Message, EAnnouncementType Type)
{
	// 데디케이티드 서버는 표시할 HUD 가 없다.
	if (IsNetMode(NM_DedicatedServer)) return;

	OnAnnouncement.Broadcast(Message, Type);
}

void AGODGameState::OnRep_ChatHistory()
{
	if (ChatHistory.Num() > 0)
	{
		OnChatMessageReceived.Broadcast(ChatHistory.Last());
	}
}
