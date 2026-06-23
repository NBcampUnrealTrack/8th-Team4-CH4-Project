#include "Game/GODGameMode.h"
#include "Game/GODGameState.h"
#include "Player/GODPlayerState.h"
#include "Player/BasePlayerController.h"
#include "InteractiveProp/GODTrain.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "EngineUtils.h"

AGODGameMode::AGODGameMode()
{
	GameStateClass = AGODGameState::StaticClass();
	PlayerStateClass = AGODPlayerState::StaticClass();
	bUseSeamlessTravel = true;
}

void AGODGameMode::BeginPlay()
{
	Super::BeginPlay();
}

// ============================================================
// 접속 / 이탈
// ============================================================

void AGODGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AGODGameState* GODGS = GetGameState<AGODGameState>();
	if (!GODGS || GODGS->CurrentPhase != EGamePhase::WaitingForPlayers) return;

	// 현재 접속한 플레이어 수 확인
	int32 Connected = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (It->Get()) Connected++;
	}

	if (Connected >= MaxPlayers)
	{
		StartCountdown();
	}
}

void AGODGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	AGODGameState* GODGS = GetGameState<AGODGameState>();
	if (!GODGS) return;

	// 카운트다운 중 이탈 → 인원 부족이므로 카운트다운 취소
	if (GODGS->CurrentPhase == EGamePhase::Countdown)
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
		GODGS->CurrentPhase = EGamePhase::WaitingForPlayers;
		GODGS->LobbyCountdown = 0;
		GODGS->OnRep_GamePhase();
	}
}

// ============================================================
// 카운트다운
// ============================================================

void AGODGameMode::StartCountdown()
{
	AGODGameState* GODGS = GetGameState<AGODGameState>();
	if (!GODGS) return;

	GODGS->CurrentPhase = EGamePhase::Countdown;
	GODGS->LobbyCountdown = CountdownDuration;
	GODGS->OnRep_GamePhase(); // 서버 측 즉시 브로드캐스트 (출발 경적/연출 트리거)

	GetWorld()->GetTimerManager().SetTimer(
		CountdownTimerHandle, this, &AGODGameMode::UpdateCountdown, 1.0f, /*bLoop=*/true);
}

void AGODGameMode::UpdateCountdown()
{
	AGODGameState* GODGS = GetGameState<AGODGameState>();
	if (!GODGS) return;

	GODGS->LobbyCountdown--;

	if (GODGS->LobbyCountdown <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
		StartGame();
	}
}

// ============================================================
// 게임 진행
// ============================================================

void AGODGameMode::StartGame()
{
	AGODGameState* GODGS = GetGameState<AGODGameState>();
	if (!GODGS) return;

	GODGS->CurrentPhase = EGamePhase::Playing;
	GODGS->RemainingTime = TotalMatchTime;
	GODGS->bGunsUnlocked = false;
	GODGS->LobbyCountdown = 0;
	TimeElapsed = 0;

	AssignRoles();

	GetWorld()->GetTimerManager().SetTimer(
		GameTimerHandle, this, &AGODGameMode::UpdateGameTimer, 1.0f, /*bLoop=*/true);
}

void AGODGameMode::AssignRoles()
{
	TArray<APlayerController*> PlayerArray;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get()) PlayerArray.Add(PC);
	}

	TArray<EMainRole> RolePool = {
		EMainRole::Sheriff, EMainRole::Outlaw, EMainRole::Mafia,
		EMainRole::Citizen, EMainRole::Citizen
	};

	for (int32 i = RolePool.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		RolePool.Swap(i, j);
	}

	for (int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		if (AGODPlayerState* PS = PlayerArray[i]->GetPlayerState<AGODPlayerState>())
		{
			if (i < RolePool.Num())
			{
				PS->MainRole = RolePool[i];
				PS->CitizenClass = (PS->MainRole == EMainRole::Citizen)
					? static_cast<ECitizenClass>(FMath::RandRange(1, 4))
					: ECitizenClass::None;
				PS->AmmoCount = 1;
				PS->bIsAlive = true;
			}
		}
	}
}

void AGODGameMode::UpdateGameTimer()
{
	AGODGameState* GODGS = GetGameState<AGODGameState>();
	if (!GODGS) return;

	GODGS->RemainingTime--;
	TimeElapsed++;

	if (TimeElapsed >= 180 && !GODGS->bGunsUnlocked)
	{
		GODGS->bGunsUnlocked = true;
	}

	if (GODGS->RemainingTime <= 0)
	{
		EndGame(EGamePhase::MafiaWon);
	}
	else if (GODGS->DistanceToDestination <= 0.0f)
	{
		EndGame(EGamePhase::CitizensWon);
	}
}

void AGODGameMode::HandlePlayerDeath(AGODPlayerState* KillerPS, AGODPlayerState* VictimPS)
{
	if (!VictimPS || !VictimPS->bIsAlive) return;

	VictimPS->bIsAlive = false;

	// 피해자 관전 모드 전환
	if (ABasePlayerController* VictimPC = Cast<ABasePlayerController>(VictimPS->GetOwner()))
	{
		VictimPC->Client_StartSpectating();
	}

	// TODO: 피해자 캐릭터 래그돌 활성화 — 캐릭터 클래스 구현 후 연결
	// ACharacter* VictimChar = Cast<ACharacter>(VictimPS->GetPawn());
	// if (VictimChar) { VictimChar->EnableRagdoll(); }

	if (KillerPS)
	{
		KillerPS->AmmoCount--;

		// 특수 직군이 다른 특수 직군 사살 시 탄약 1발 리필
		if (KillerPS->MainRole != EMainRole::Citizen && VictimPS->MainRole != EMainRole::Citizen)
		{
			KillerPS->AmmoCount = 1;
		}
	}

	CheckWinConditions();
}

void AGODGameMode::CheckWinConditions()
{
	int32 AliveSheriffs = 0;
	int32 AliveOutlaws  = 0;
	int32 AliveMafias   = 0;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (AGODPlayerState* PS = PC->GetPlayerState<AGODPlayerState>())
			{
				if (!PS->bIsAlive) continue;
				if (PS->MainRole == EMainRole::Sheriff)      AliveSheriffs++;
				else if (PS->MainRole == EMainRole::Outlaw)  AliveOutlaws++;
				else if (PS->MainRole == EMainRole::Mafia)   AliveMafias++;
			}
		}
	}

	if (AliveSheriffs == 0 && AliveMafias == 0 && AliveOutlaws > 0)
	{
		EndGame(EGamePhase::OutlawWon);
	}
	else if (AliveOutlaws == 0 && AliveMafias == 0)
	{
		EndGame(EGamePhase::CitizensWon);
	}
}

void AGODGameMode::TriggerDerailment()
{
	if (AGODTrain* Train = FindTrainActor())
	{
		Train->TriggerDerailment();
	}
	EndGame(EGamePhase::MafiaWon);
}

void AGODGameMode::EndGame(EGamePhase WinningPhase)
{
	AGODGameState* GODGS = GetGameState<AGODGameState>();
	if (!GODGS || GODGS->CurrentPhase != EGamePhase::Playing) return;

	GODGS->CurrentPhase = WinningPhase;
	GODGS->OnRep_GamePhase();

	GetWorld()->GetTimerManager().ClearTimer(GameTimerHandle);

	GetWorld()->GetTimerManager().SetTimer(
		MenuReturnTimerHandle, this, &AGODGameMode::ReturnToMainMenu, EndGameDelay, /*bLoop=*/false);
}

void AGODGameMode::ReturnToMainMenu()
{
	GetWorld()->ServerTravel(MainMenuMapPath);
}

AGODTrain* AGODGameMode::FindTrainActor() const
{
	for (TActorIterator<AGODTrain> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}
