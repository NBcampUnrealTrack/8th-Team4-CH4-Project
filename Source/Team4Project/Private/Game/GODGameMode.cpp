#include "Game/GODGameMode.h"
#include "Game/GODGameState.h"
#include "Player/GODPlayerState.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"
#include "Player/BasePlayerController.h"
#include "InteractiveProp/GODTrain.h"
#include "InteractiveProp/PressureValve.h"
#include "Component/PressureComponent.h"
#include "InteractiveProp/GearSlot.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "UI/HUD/GODHUD.h"
#include "Game/PlayerGameInstance.h"
#include "Kismet/GameplayStatics.h"


AGODGameMode::AGODGameMode()
{
	GameStateClass   = AGODGameState::StaticClass();
	PlayerStateClass = AGODPlayerState::StaticClass();
	HUDClass         = AGODHUD::StaticClass();
	bUseSeamlessTravel = true;
}

void AGODGameMode::BeginPlay()
{
	Super::BeginPlay();
}

// ============================================================
// 접속 / 이탈
// ============================================================

void AGODGameMode::PreLogin(const FString& Options, const FString& Address,
	const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty()) return;

	// 비밀번호 방 검증 (서버 권위). 클라이언트는 접속 URL에 ?SessionPassword= 로 전달한다.
	// ErrorMessage를 채우면 엔진이 접속을 거부하고 클라에 사유를 보낸다.
	if (const UPlayerGameInstance* GI = GetGameInstance<UPlayerGameInstance>())
	{
		const FString& HostPassword = GI->GetHostSessionPassword();
		if (!HostPassword.IsEmpty())
		{
			const FString Supplied = UGameplayStatics::ParseOption(Options, TEXT("SessionPassword"));
			if (Supplied != HostPassword)
			{
				ErrorMessage = TEXT("비밀번호가 올바르지 않습니다.");
			}
		}
	}
}

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
	for (TActorIterator<AGearSlot> It(GetWorld()); It; ++It)
	{
		if (!(*It)->bIsAssembled)
			(*It)->ForceReassemble();
	}

	AssignRoles();

	// 압력 폭발 시 마피아 승리 처리 연결 (중복 방지) + 열차 출발
	if (AGODTrain* Train = FindTrainActor())
	{
		if (Train->Pressure)
		{
			Train->Pressure->OnPressureExplode.RemoveDynamic(this, &AGODGameMode::HandlePressureExplosion);
			Train->Pressure->OnPressureExplode.AddDynamic(this, &AGODGameMode::HandlePressureExplosion);
		}

		// 게임 시작과 함께 열차 주행 시작 (서버 권위). 이 전까지는 제자리 대기.
		Train->StartRunning();
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

	// 접속 인원수에 맞춰 역할 풀을 동적으로 구성 (5~8인 지원).
	//  - 5~7인: 보안관 1 · 무법자 1 · 마피아 1 + 나머지 시민(2~4명)
	//  - 8인:   마피아를 2명으로 늘려 밸런스 유지 → 시민 4명 (시민 직업 4종과 정확히 일치)
	TArray<EMainRole> RolePool = {
		EMainRole::Sheriff, EMainRole::Outlaw, EMainRole::Mafia
	};
	if (PlayerArray.Num() >= 8)
	{
		RolePool.Add(EMainRole::Mafia);
	}
	while (RolePool.Num() < PlayerArray.Num())
	{
		RolePool.Add(EMainRole::Citizen);
	}

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
		// 시민이 직업 종류(4종)보다 많아지는 예외 상황엔 직업을 순환 배정 (배열 초과 방지)
		PS->CitizenClass = (PS->MainRole == EMainRole::Citizen)
			? CitizenPool[CitizenIndex++ % CitizenPool.Num()]
			: ECitizenClass::None;
		PS->AmmoCount = 1;
		PS->SetIsAlive(true);

		// 로비 캐릭터를 그대로 유지하고 태그만 부여 (열차 위 위치 보존).
		FGameplayTag AssignedTag = GetTagForRole(PS);
		if (ABaseCharacter* ExistingPawn = Cast<ABaseCharacter>(PC->GetPawn()))
		{
			ExistingPawn->SetCharacterTag(AssignedTag);
		}

		// 역할 배정 디버그 출력
		FString PlayerName = PS->GetPlayerName();
		FString RoleStr;
		switch (PS->MainRole)
		{
			case EMainRole::Mafia:   RoleStr = TEXT("Mafia");   break;
			case EMainRole::Sheriff: RoleStr = TEXT("Sheriff"); break;
			case EMainRole::Outlaw:  RoleStr = TEXT("Outlaw");  break;
			case EMainRole::Citizen:
				switch (PS->CitizenClass)
				{
					case ECitizenClass::Mechanic: RoleStr = TEXT("Mechanic"); break;
					case ECitizenClass::Watchman: RoleStr = TEXT("Watchman"); break;
					case ECitizenClass::Stoker:   RoleStr = TEXT("Stoker");   break;
					case ECitizenClass::Porter:   RoleStr = TEXT("Porter");   break;
					default:                      RoleStr = TEXT("Citizen");  break;
				}
				break;
			default: RoleStr = TEXT("Unknown"); break;
		}
		FString PawnOk = PC->GetPawn() ? TEXT("OK") : TEXT("NO PAWN");
		UE_LOG(LogTemp, Warning, TEXT("[AssignRoles] %s → %s (Tag:%s, Pawn:%s)"),
			*PlayerName, *RoleStr, *AssignedTag.ToString(), *PawnOk);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				i, 10.f, FColor::Cyan,
				FString::Printf(TEXT("[역할] %s → %s"), *PlayerName, *RoleStr));
		}
	}

	SetupWatchmanTracking();
}

FGameplayTag AGODGameMode::GetTagForRole(AGODPlayerState* PS) const
{
	if (!PS) return FGameplayTag();

	switch (PS->MainRole)
	{
		case EMainRole::Mafia:   return Character::Special::Mafia.GetTag();
		case EMainRole::Sheriff: return Character::Special::Sheriff.GetTag();
		case EMainRole::Outlaw:  return Character::Special::Outlaw.GetTag();
		case EMainRole::Citizen:
			switch (PS->CitizenClass)
			{
				case ECitizenClass::Mechanic: return Character::Crew::Mechanic.GetTag();
				case ECitizenClass::Watchman: return Character::Crew::Watchman.GetTag();
				case ECitizenClass::Stoker:   return Character::Crew::Stoker.GetTag();
				case ECitizenClass::Porter:   return Character::Crew::Porter.GetTag();
				default: return FGameplayTag();
			}
		default: return FGameplayTag();
	}
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
	ABaseCharacter* Watchman = nullptr;
	TArray<ABaseCharacter*> SpecialChars;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;

		AGODPlayerState* PS = PC->GetPlayerState<AGODPlayerState>();
		if (!PS) continue;

		ABaseCharacter* Char = Cast<ABaseCharacter>(PC->GetPawn());
		if (!Char) continue;

		if (PS->MainRole == EMainRole::Citizen && PS->CitizenClass == ECitizenClass::Watchman)
		{
			Watchman = Char;
		}
		else if (PS->MainRole == EMainRole::Mafia || PS->MainRole == EMainRole::Outlaw)
		{
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

AActor* AGODGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AGODGameMode::HandlePlayerDeath(AGODPlayerState* KillerPS, AGODPlayerState* VictimPS)
{
	if (!VictimPS || !VictimPS->bIsAlive) return;

	VictimPS->SetIsAlive(false);

	// 캐릭터 사망 처리 (래그돌 + bIsDead 설정)
	if (AController* VictimPC = Cast<AController>(VictimPS->GetOwner()))
	{
		if (ABaseCharacter* VictimChar = Cast<ABaseCharacter>(VictimPC->GetPawn()))
		{
			AController* KillerPC = KillerPS ? Cast<AController>(KillerPS->GetOwner()) : nullptr;
			VictimChar->Die(KillerPC ? KillerPC->GetPawn() : nullptr);
		}
	}

	// 관전 모드 전환은 Die() 내부에서 처리한다(총격 등 모든 사망 경로 공통).

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
