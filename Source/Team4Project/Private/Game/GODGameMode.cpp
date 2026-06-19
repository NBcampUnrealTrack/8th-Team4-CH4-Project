#include "Game/GODGameMode.h"
#include "Game/GODGameState.h"
#include "Player/GODPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

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

void AGODGameMode::StartSeamlessTravelToGame()
{
	// TODO: 대기실 전원 레디 시 호출. 맵 경로 지정
	//GetWorld()->ServerTravel("/Game/Maps/GODTrainMap?listen");
}

void AGODGameMode::StartGame()
{
	AGODGameState* GODGS = GetGameState<AGODGameState>();
	if (GODGS)
	{
		GODGS->CurrentPhase = EGamePhase::Playing;
		GODGS->RemainingTime = TotalMatchTime;
		GODGS->bGunsUnlocked = false;
		TimeElapsed = 0;

		AssignRoles();

		GetWorld()->GetTimerManager().SetTimer(GameTimerHandle, this, &AGODGameMode::UpdateGameTimer, 1.0f, true);
	}
}

void AGODGameMode::AssignRoles()
{
	TArray<APlayerController*> PlayerArray;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get()) PlayerArray.Add(PC);
	}

	// 반드시 5명 기준 세팅
	TArray<EMainRole> RolePool = { EMainRole::Sheriff, EMainRole::Outlaw, EMainRole::Mafia, EMainRole::Citizen, EMainRole::Citizen };

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

				if (PS->MainRole == EMainRole::Citizen)
				{
					int32 RandomClass = FMath::RandRange(1, 4);
					PS->CitizenClass = static_cast<ECitizenClass>(RandomClass);
				}
				else
				{
					PS->CitizenClass = ECitizenClass::None;
				}

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
		// TODO: 총기 잠금 해제 UI/사운드 알림 트리거
	}

	// TODO: 열차 이동 거리 차감 로직 연동
	//GODGS->DistanceToDestination -= (Speed * GetWorld()->GetDeltaSeconds());

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

	// TODO: Victim 캐릭터 래그돌 처리 및 관전 모드 전환 호출

	if (KillerPS)
	{
		KillerPS->AmmoCount--;

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
	int32 AliveOutlaws = 0;
	int32 AliveMafias = 0;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (AGODPlayerState* PS = PC->GetPlayerState<AGODPlayerState>())
			{
				if (PS->bIsAlive)
				{
					if (PS->MainRole == EMainRole::Sheriff) AliveSheriffs++;
					else if (PS->MainRole == EMainRole::Outlaw) AliveOutlaws++;
					else if (PS->MainRole == EMainRole::Mafia) AliveMafias++;
				}
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
	EndGame(EGamePhase::MafiaWon);
}

void AGODGameMode::EndGame(EGamePhase WinningPhase)
{
	AGODGameState* GODGS = GetGameState<AGODGameState>();
	if (GODGS)
	{
		GODGS->CurrentPhase = WinningPhase;
	}

	GetWorld()->GetTimerManager().ClearTimer(GameTimerHandle);

	// TODO: 게임 종료 UI 출력 및 일정 시간 후 로비로 복귀 로직
}