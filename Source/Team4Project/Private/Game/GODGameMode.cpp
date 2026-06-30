#include "Game/GODGameMode.h"
#include "Game/GODGameState.h"
#include "Player/GODPlayerState.h"
#include "Player/BaseCharacter.h"
#include "Player/Role/GODCharacterWatchman.h"
#include "Player/BasePlayerController.h"
#include "InteractiveProp/GODTrain.h"
#include "InteractiveProp/PressureValve.h"
#include "Component/PressureComponent.h"
#include "InteractiveProp/PickupGear.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
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
	GODGS->PressureLevel = 0.f;
	TimeElapsed = 0;
	GODGS->OnRep_RemainingTime(); // 리슨 서버 클라이언트 즉시 알림

	// 재시작 대비: 이전 라운드 상태 초기화
	for (TActorIterator<APressureValve> It(GetWorld()); It; ++It)
	{
		(*It)->Tags.Remove(FName(TEXT("Stoker.ForceClose")));
	}
	for (TActorIterator<APickupGear> It(GetWorld()); It; ++It)
	{
		if ((*It)->ActorHasTag(TEXT("Gear.Destroyed")))
			(*It)->Destroy();
	}

	AssignRoles();

	// 압력 폭발 시 마피아 승리 처리 연결 (중복 방지)
	if (AGODTrain* Train = FindTrainActor())
	{
		if (Train->Pressure)
		{
			Train->Pressure->OnPressureExplode.RemoveDynamic(this, &AGODGameMode::HandlePressureExplosion);
			Train->Pressure->OnPressureExplode.AddDynamic(this, &AGODGameMode::HandlePressureExplosion);
		}
	}

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

	// 시민 직업 풀: 셔플 후 순서대로 배정해 중복 방지
	TArray<ECitizenClass> CitizenPool = {
		ECitizenClass::Mechanic, ECitizenClass::Watchman,
		ECitizenClass::Stoker,   ECitizenClass::Porter
	};
	for (int32 i = CitizenPool.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		CitizenPool.Swap(i, j);
	}
	int32 CitizenIndex = 0;

	for (int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		APlayerController* PC = PlayerArray[i];
		AGODPlayerState* PS = PC ? PC->GetPlayerState<AGODPlayerState>() : nullptr;
		if (!PS || i >= RolePool.Num()) continue;

		PS->MainRole     = RolePool[i];
		PS->CitizenClass = (PS->MainRole == EMainRole::Citizen)
			? CitizenPool[CitizenIndex++]
			: ECitizenClass::None;
		PS->AmmoCount = 1;
		PS->bIsAlive  = true;

		RespawnPlayerAsRole(PC, GetClassForRole(PS));
	}

	SetupWatchmanTracking();
}

TSubclassOf<ABaseCharacter> AGODGameMode::GetClassForRole(AGODPlayerState* PS) const
{
	if (!PS) return nullptr;

	switch (PS->MainRole)
	{
		case EMainRole::Mafia:   return MafiaClass;
		case EMainRole::Sheriff: return SheriffClass;
		case EMainRole::Outlaw:  return OutlawClass;
		case EMainRole::Citizen:
			switch (PS->CitizenClass)
			{
				case ECitizenClass::Mechanic: return MechanicClass;
				case ECitizenClass::Watchman: return WatchmanClass;
				case ECitizenClass::Stoker:   return StokerClass;
				case ECitizenClass::Porter:   return PorterClass;
				default: return nullptr;
			}
		default: return nullptr;
	}
}

void AGODGameMode::RespawnPlayerAsRole(APlayerController* PC, TSubclassOf<ABaseCharacter> CharClass)
{
	if (!PC || !CharClass) return;

	// PlayerStart 먼저 탐색: 없으면 기존 폰을 유지하고 중단(원점 스폰 방지)
	AActor* StartSpot = FindPlayerStart(PC);
	if (!StartSpot)
	{
		UE_LOG(LogTemp, Warning, TEXT("RespawnPlayerAsRole: PlayerStart 없음 (%s)"), *PC->GetName());
		return;
	}

	if (APawn* OldPawn = PC->GetPawn())
	{
		PC->UnPossess();
		OldPawn->Destroy();
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (ABaseCharacter* NewPawn = GetWorld()->SpawnActor<ABaseCharacter>(
			CharClass, StartSpot->GetActorLocation(), StartSpot->GetActorRotation(), Params))
	{
		PC->Possess(NewPawn);
	}
}

void AGODGameMode::SetupWatchmanTracking()
{
	AGODCharacterWatchman* Watchman = nullptr;
	TArray<ABaseCharacter*> SpecialChars;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;

		AGODPlayerState* PS = PC->GetPlayerState<AGODPlayerState>();
		if (!PS) continue;

		if (PS->MainRole == EMainRole::Citizen && PS->CitizenClass == ECitizenClass::Watchman)
		{
			Watchman = Cast<AGODCharacterWatchman>(PC->GetPawn());
		}
		else if (PS->MainRole == EMainRole::Mafia || PS->MainRole == EMainRole::Outlaw)
		{
			if (ABaseCharacter* Char = Cast<ABaseCharacter>(PC->GetPawn()))
				SpecialChars.Add(Char);
		}
	}

	if (!Watchman || SpecialChars.Num() == 0) return;

	ABaseCharacter* P2 = SpecialChars.Num() >= 2 ? SpecialChars[1] : nullptr;
	Watchman->SetTrackedPlayers(SpecialChars[0], P2, FLinearColor::Red, FLinearColor::Blue);
}

void AGODGameMode::UpdateGameTimer()
{
	AGODGameState* GODGS = GetGameState<AGODGameState>();
	if (!GODGS) return;

	GODGS->RemainingTime--;
	GODGS->OnRep_RemainingTime(); // 리슨 서버 클라이언트 즉시 알림
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

	// 캐릭터 사망 처리 (래그돌 + bIsDead 설정)
	if (AController* VictimPC = Cast<AController>(VictimPS->GetOwner()))
	{
		if (ABaseCharacter* VictimChar = Cast<ABaseCharacter>(VictimPC->GetPawn()))
		{
			AController* KillerPC = KillerPS ? Cast<AController>(KillerPS->GetOwner()) : nullptr;
			VictimChar->Die(KillerPC ? KillerPC->GetPawn() : nullptr);
		}
	}

	// 피해자 관전 모드 전환
	if (ABasePlayerController* VictimPC = Cast<ABasePlayerController>(VictimPS->GetOwner()))
	{
		VictimPC->Client_StartSpectating();
	}

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

	// 모든 특수직 전멸(동시 사망 포함) → 시민 승리를 가장 먼저 처리
	if (AliveSheriffs == 0 && AliveMafias == 0 && AliveOutlaws == 0)
	{
		EndGame(EGamePhase::CitizensWon);
	}
	else if (AliveSheriffs == 0 && AliveMafias == 0 && AliveOutlaws > 0)
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

void AGODGameMode::HandlePressureExplosion()
{
	// 압력 100% 달성 → 열차 탈선 + 마피아 승리
	TriggerDerailment();
}

AGODTrain* AGODGameMode::FindTrainActor() const
{
	for (TActorIterator<AGODTrain> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}
