// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PlayerGameInstance.h"

#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

#include "UI/MenuSystem/MainMenu.h"
#include "UI/MenuSystem/MenuWidget.h"
#include "Game/GODGameState.h"
#include "Game/GODGameUserSettings.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Internationalization/Internationalization.h"
#include "Misc/Base64.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"

#include "Blueprint/UserWidget.h"

const static FName SESSION_NAME = TEXT("GameSession");
const static FName SERVER_NAME_SETTINGS_KEY = TEXT("ServerName");

// 유령 세션 필터링용 메타데이터 키.
// 호스트가 강제 종료되면 스팀 로비가 백엔드에 수 분간 남는데, 재접속 후 새 방을 만들면
// 다른 플레이어 검색에 옛 방(유령)과 새 방이 같이 잡힌다. 세션에 호스트 고유 ID와
// 생성 시각을 심어두고, 검색 측에서 같은 호스트의 최신 세션만 남기는 데 쓴다.
const static FName HOST_ID_SETTINGS_KEY = TEXT("HostId");
const static FName CREATED_AT_SETTINGS_KEY = TEXT("CreatedAt");

// 비밀번호 방: 목록 자물쇠 표시용 플래그 + 클라 측 선검증용 해시(원문은 광고하지 않음).
// 최종 검증은 서버 GameMode::PreLogin에서 원문 비교로 한다 (조작 클라 차단).
const static FName HAS_PASSWORD_SETTINGS_KEY = TEXT("HasPassword");
const static FName PW_HASH_SETTINGS_KEY = TEXT("PwHash");

namespace
{
	// 로컬 플레이어의 고유 ID(스팀 SteamID). 실패 시 머신 로그인 ID 폴백.
	FString GetLocalHostIdString(UWorld* World)
	{
		if (IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World))
		{
			if (FUniqueNetIdPtr Id = Identity->GetUniquePlayerId(0))
			{
				return Id->ToString();
			}
		}
		return FPlatformMisc::GetLoginId();
	}

	const TCHAR* GRoomNameB64Prefix = TEXT("b64:");

	FString EncodeRoomName(const FString& In)
	{
		FTCHARToUTF8 Utf8(*In);
		const FString Encoded = FBase64::Encode(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		return FString(GRoomNameB64Prefix) + Encoded;
	}

	FString DecodeRoomName(const FString& In)
	{
		if (!In.StartsWith(GRoomNameB64Prefix))
		{
			return In;
		}
		const FString Payload = In.RightChop(FCString::Strlen(GRoomNameB64Prefix));
		TArray<uint8> Bytes;
		if (!FBase64::Decode(Payload, Bytes))
		{
			return In;
		}
		Bytes.Add(0);
		return FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Bytes.GetData())));
	}
}


UPlayerGameInstance::UPlayerGameInstance(const FObjectInitializer& ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<UUserWidget> MenuBPClass(TEXT("/Game/GameSystem/UI/MenuSystem/WBP_MainMenu"));
	if (!ensure(MenuBPClass.Class != nullptr))
		return;
    
	MenuClass = MenuBPClass.Class;
	
	ConstructorHelpers::FClassFinder<UUserWidget> InGameMenuBPClass(TEXT("/Game/GameSystem/UI/MenuSystem/WBP_InGameMenu"));
	if (!ensure(InGameMenuBPClass.Class != nullptr))
		return;

	InGameMenuClass = InGameMenuBPClass.Class;
	
	 ConstructorHelpers::FClassFinder<UUserWidget> InLobbyMenuClass(TEXT("/Game/GameSystem/UI/MenuSystem/WBP_InGameMenu"));
	 if (!ensure(InLobbyMenuClass.Class != nullptr))
	 	return;
	
	LobbyMenuClass = InLobbyMenuClass.Class;
}

void UPlayerGameInstance::Init()
{
	Super::Init();
	
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Found Subsysytem %s"), *Subsystem->GetSubsystemName().ToString());
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			CreateSessionCompleteDelegateHandle = SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UPlayerGameInstance::OnCreateSessionComplete);
			DestroySessionCompleteDelegateHandle = SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UPlayerGameInstance::OnDestorySessionComplete);
			FindSessionsCompleteDelegateHandle = SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UPlayerGameInstance::OnFindSessionComplete);
			JoinSessionCompleteDelegateHandle = SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UPlayerGameInstance::OnJoinSessionComplete);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Found No Subsysytem"));
	}

	if (GEngine != nullptr)
	{
		NetworkFailureDelegateHandle = GEngine->OnNetworkFailure().AddUObject(this, &UPlayerGameInstance::OnNetworkFailure);
	}

	ApplyLanguage();
	ApplyAudioSettings();
	if (UGODGameUserSettings* Settings = UGODGameUserSettings::Get())
	{
		Settings->ApplyGamma();
	}
}

void UPlayerGameInstance::ApplySoundClassOverride(USoundMix* Mix, USoundClass* Class, float Volume)
{
	if (Mix == nullptr || Class == nullptr)
	{
		return;
	}
	UGameplayStatics::SetSoundMixClassOverride(this, Mix, Class, Volume, 1.f, 0.f, true);
}

void UPlayerGameInstance::ApplyAudioSettings()
{
	UGODGameUserSettings* Settings = UGODGameUserSettings::Get();
	if (Settings == nullptr)
	{
		return;
	}

	USoundMix* Mix = MasterSoundMix.LoadSynchronous();
	if (Mix == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Audio] MasterSoundMix 미지정 — BP_PlayerGameInstance 에 SoundMix/SoundClass 를 지정하세요."));
		return;
	}

	const float MasterVol = Settings->bMuted ? 0.f : FMath::Clamp(Settings->MasterVolume, 0.f, 1.f);

	ApplySoundClassOverride(Mix, MasterSoundClass.LoadSynchronous(), MasterVol);
	ApplySoundClassOverride(Mix, BGMSoundClass.LoadSynchronous(), FMath::Clamp(Settings->BGMVolume, 0.f, 1.f));
	ApplySoundClassOverride(Mix, SFXSoundClass.LoadSynchronous(), FMath::Clamp(Settings->SFXVolume, 0.f, 1.f));
	ApplySoundClassOverride(Mix, UISoundClass.LoadSynchronous(), FMath::Clamp(Settings->UIVolume, 0.f, 1.f));

	UGameplayStatics::PushSoundMixModifier(this, Mix);
}

void UPlayerGameInstance::ApplyLanguage()
{
	UGODGameUserSettings* Settings = UGODGameUserSettings::Get();
	if (Settings == nullptr || Settings->LanguageCulture.IsEmpty())
	{
		return;
	}
	FInternationalization::Get().SetCurrentCulture(Settings->LanguageCulture);
}

void UPlayerGameInstance::Shutdown()
{
	// 정상 종료(창 닫기, PIE 종료, 메뉴에서 나가기) 시 세션을 파괴해
	// 스팀 로비가 유령으로 남지 않게 한다. DestroySession의 스팀 LeaveLobby 요청은
	// 즉시 전송되므로 완료 콜백을 기다릴 필요 없다.
	// (강제 종료/크래시는 이 경로를 못 타므로 검색 측 유령 필터링이 2차 방어선)
	DesiredServerName = TEXT(""); // 종료 중 OnDestroy 콜백이 CreateSession을 다시 부르지 않게
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(SESSION_NAME) != nullptr)
	{
		SessionInterface->DestroySession(SESSION_NAME);
	}

	// 등록했던 델리게이트를 해제하고, 캐싱한 세션 인터페이스 참조를 놓아준다.
	// (놓아주지 않으면 PIE 종료 시 OnlineSubsystemNull이 SessionInterface.IsUnique() ensure로 터진다.)
	if (SessionInterface.IsValid())
	{
		if (CreateSessionCompleteDelegateHandle.IsValid())
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		if (DestroySessionCompleteDelegateHandle.IsValid())
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		if (FindSessionsCompleteDelegateHandle.IsValid())
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		if (JoinSessionCompleteDelegateHandle.IsValid())
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}
	SessionInterface.Reset();

	if (GEngine != nullptr && NetworkFailureDelegateHandle.IsValid())
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureDelegateHandle);
	}

	Super::Shutdown();
}

void UPlayerGameInstance::LoadMenuWidget()
{
	if (!ensure(MenuClass != nullptr))
		return;

	Menu = CreateWidget<UMainMenu>(this, MenuClass);
	if (!ensure(Menu != nullptr))
		return;

	Menu->Setup();

	Menu->SetMenuInterface(this);
}

void UPlayerGameInstance::InGameLoadMenu()
{
	// 이미 열려 있으면 닫기 (토글)
	if (InGameMenuInstance && InGameMenuInstance->IsInViewport())
	{
		InGameMenuInstance->Teardown();
		InGameMenuInstance = nullptr;
		return;
	}

	// 상호작용 중이면 메뉴 생성 차단
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			
		}
	}

	if (!ensure(InGameMenuClass != nullptr))
		return;

	InGameMenuInstance = CreateWidget<UMenuWidget>(this, InGameMenuClass);
	if (!ensure(InGameMenuInstance != nullptr))
		return;

	InGameMenuInstance->SetIsFocusable(true);
	InGameMenuInstance->AddToViewport();
	InGameMenuInstance->SetMenuInterface(this);

	APlayerController* PC = World->GetFirstPlayerController();
	if (PC)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InGameMenuInstance->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void UPlayerGameInstance::LobbyLoadMenu()
{
	if (!ensure(LobbyMenuClass != nullptr))
		return;

	UMenuWidget* _Menu = CreateWidget<UMenuWidget>(this, LobbyMenuClass);
	if (!ensure(_Menu != nullptr))
		return;

	_Menu->InSetup();

	_Menu->SetMenuInterface(this);
}

void UPlayerGameInstance::Host(FString ServerName, FString Password)
{
	DesiredServerName = ServerName;
	DesiredPassword = Password;
	if (SessionInterface.IsValid())
	{
		auto ExistingSession = SessionInterface->GetNamedSession(SESSION_NAME);
		if (ExistingSession != nullptr)
		{
			// 만약 이전에 파괴되지 않은 유령 세션이 남아있다면 파괴를 요청합니다.
			SessionInterface->DestroySession(SESSION_NAME);
		}
		else
		{
			CreateSession();
		}
	}
}

void UPlayerGameInstance::OnCreateSessionComplete(FName Sessionname, bool Success)
{
	if (!Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not create session"));
		return;
	}

	if (Menu != nullptr)
	{
		Menu->Teardown();
	}

	UEngine* Engine = GetEngine();
	if (!ensure(Engine != nullptr))
		return;

	Engine->AddOnScreenDebugMessage(0, 5.f, FColor::Green, TEXT("Hosting"));

	UWorld* World = GetWorld();
	if (!ensure(World != nullptr))
		return;

	World->ServerTravel("/Game/Level/LobbyMap?listen");
}

void UPlayerGameInstance::OnDestorySessionComplete(FName Sessionname, bool Success)
{
	if (!Success) return;
	
	if (!DesiredServerName.IsEmpty())
	{
		CreateSession();
	}
	else
	{
		MoveToTitleMap();
	}
}

void UPlayerGameInstance::OnNetworkFailure(UWorld* world, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	LoadMainMenu();
}

void UPlayerGameInstance::RefreshServerList()
{
	if (!SessionInterface.IsValid()) return; 
    
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	if (SessionSearch.IsValid())
	{
		// NULL 서브시스템일 때는 LAN 쿼리를 true로 켜줍니다.
		if (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL")
		{
			SessionSearch->bIsLanQuery = true;
		}
		else
		{
			SessionSearch->bIsLanQuery = false;
			SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
		}
       
		SessionSearch->MaxSearchResults = 100;
		UE_LOG(LogTemp, Warning, TEXT("Starting Find Session"));
		SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	}
}

void UPlayerGameInstance::OnFindSessionComplete(bool Success)
{
	// 빠른 방찾기 플래그는 결과와 무관하게 이번 검색에서 소비한다
	const bool bQuickJoinRequested = bQuickJoinPending;
	bQuickJoinPending = false;

	if (Success && SessionSearch.IsValid() && Menu != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Finished Find Session"));

		// ---- 유령 세션 제거 ----
		// 같은 호스트(HostId)의 세션이 여러 개 잡히면(강제 종료로 남은 옛 로비 + 새 로비)
		// 생성 시각(CreatedAt)이 가장 최근인 것만 남긴다.
		// Join(Index)이 SearchResults 배열 인덱스를 그대로 쓰므로, 표시 목록이 아니라
		// SearchResults 자체에서 제거해 인덱스 일관성을 유지한다.
		{
			TArray<FOnlineSessionSearchResult>& Results = SessionSearch->SearchResults;

			// 1) 호스트별 최신 세션 인덱스 수집
			TMap<FString, int32> LatestIndexByHost;   // HostId → 최신 결과 인덱스
			TArray<int64> CreatedAtByIndex;
			CreatedAtByIndex.SetNumZeroed(Results.Num());

			for (int32 i = 0; i < Results.Num(); ++i)
			{
				FString HostId;
				if (!Results[i].Session.SessionSettings.Get(HOST_ID_SETTINGS_KEY, HostId))
				{
					continue; // 메타데이터 없는 세션(구버전 빌드)은 건드리지 않음
				}

				FString CreatedAtStr;
				if (Results[i].Session.SessionSettings.Get(CREATED_AT_SETTINGS_KEY, CreatedAtStr))
				{
					CreatedAtByIndex[i] = FCString::Atoi64(*CreatedAtStr);
				}

				if (const int32* Existing = LatestIndexByHost.Find(HostId))
				{
					if (CreatedAtByIndex[i] > CreatedAtByIndex[*Existing])
					{
						LatestIndexByHost[HostId] = i;
					}
				}
				else
				{
					LatestIndexByHost.Add(HostId, i);
				}
			}

			// 2) 최신이 아닌 중복(유령) 제거 — 역순 순회라 앞쪽 인덱스는 유효하게 유지된다
			for (int32 i = Results.Num() - 1; i >= 0; --i)
			{
				FString HostId;
				if (!Results[i].Session.SessionSettings.Get(HOST_ID_SETTINGS_KEY, HostId))
				{
					continue;
				}
				if (LatestIndexByHost[HostId] != i)
				{
					UE_LOG(LogTemp, Warning, TEXT("유령 세션 제거: %s (호스트 %s의 더 최신 세션 존재)"),
						*Results[i].GetSessionIdStr(), *Results[i].Session.OwningUserName);
					Results.RemoveAt(i);
				}
			}
		}

		TArray<FServerData> ServerNames;


		for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
		{
			UE_LOG(LogTemp, Warning, TEXT("Found session names: %s"), *SearchResult.GetSessionIdStr());
			FServerData Data;
			Data.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
			Data.CurrentPlayers = Data.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;
			Data.HostUsername = SearchResult.Session.OwningUserName;

			FString ServerName;
			if (SearchResult.Session.SessionSettings.Get(SERVER_NAME_SETTINGS_KEY, ServerName))
			{
				Data.Name = DecodeRoomName(ServerName);
			}
			else
			{
				Data.Name = "Could not find name.";
			}

			// 비밀번호 방 여부 (목록 자물쇠 표시용) + 해시 (참여 팝업 클라 선검증용)
			SearchResult.Session.SessionSettings.Get(HAS_PASSWORD_SETTINGS_KEY, Data.bHasPassword);
			SearchResult.Session.SessionSettings.Get(PW_HASH_SETTINGS_KEY, Data.PasswordHash);

			ServerNames.Add(Data);
		}

		Menu->SetServerList(ServerNames);

		// ---- 빠른 방찾기: 자리 있고 비밀번호 없는 방 중 최적을 자동 조인 ----
		// 점수 = 현재인원 × QuickJoinPlayersWeight − 핑(ms) × QuickJoinPingWeight
		// (기본 100:1 — 사람 많은 방 우선, 핑 100ms 차이 = 인원 1명 차이로 환산)
		if (bQuickJoinRequested)
		{
			const TArray<FOnlineSessionSearchResult>& Results = SessionSearch->SearchResults;
			int32 BestIndex = INDEX_NONE;
			float BestScore = -FLT_MAX;

			for (int32 i = 0; i < Results.Num(); ++i)
			{
				if (Results[i].Session.NumOpenPublicConnections <= 0)
				{
					continue; // 만석
				}

				bool bHasPassword = false;
				Results[i].Session.SessionSettings.Get(HAS_PASSWORD_SETTINGS_KEY, bHasPassword);
				if (bHasPassword)
				{
					continue; // 비밀번호 방은 빠른 참여 대상에서 제외
				}

				const int32 Players = Results[i].Session.SessionSettings.NumPublicConnections
					- Results[i].Session.NumOpenPublicConnections;
				const float Score = Players * QuickJoinPlayersWeight
					- Results[i].PingInMs * QuickJoinPingWeight;

				if (Score > BestScore)
				{
					BestScore = Score;
					BestIndex = i;
				}
			}

			if (BestIndex != INDEX_NONE)
			{
				UE_LOG(LogTemp, Warning, TEXT("빠른 방찾기: %d번 방 자동 참여 (점수 %.0f)"), BestIndex, BestScore);
				Join(static_cast<uint32>(BestIndex), TEXT(""));
				return;
			}

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Yellow,
					TEXT("참여 가능한 방이 없습니다."));
			}
		}
	}
}

void UPlayerGameInstance::CreateSession()
{
	if (SessionInterface.IsValid())
	{
		FOnlineSessionSettings SessionSettings;
		SessionSettings.bIsLANMatch = false;
		if (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL")
		{
			SessionSettings.bIsLANMatch = true;
		}
		else
		{
			SessionSettings.bIsLANMatch = false;
		}

		SessionSettings.NumPublicConnections = FMath::Max(1, MaxSessionPlayers);
		SessionSettings.bShouldAdvertise = true;
		SessionSettings.bUsesPresence = true;
       
		// 스팀 매칭을 사용할 때 필수적으로 켜주어야 하는 옵션들입니다.
		SessionSettings.bUseLobbiesIfAvailable = true; 
		SessionSettings.bAllowJoinInProgress = true;
		
		// 한글 보존을 위해 Base64(ASCII)로 인코딩해 광고 (읽는 측에서 DecodeRoomName).
		SessionSettings.Set(SERVER_NAME_SETTINGS_KEY, EncodeRoomName(DesiredServerName), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		// 유령 세션 필터링용: 호스트 고유 ID + 생성 시각(Unix time).
		// 검색 측(OnFindSessionComplete)에서 같은 호스트의 세션이 여럿이면 최신 것만 남긴다.
		SessionSettings.Set(HOST_ID_SETTINGS_KEY, GetLocalHostIdString(GetWorld()),
			EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		SessionSettings.Set(CREATED_AT_SETTINGS_KEY,
			FString::Printf(TEXT("%lld"), FDateTime::UtcNow().ToUnixTimestamp()),
			EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		// 비밀번호 방: 플래그(목록 자물쇠) + 해시(클라 선검증)만 광고. 원문은 호스트에만 보관.
		HostSessionPassword = DesiredPassword;
		const bool bHasPassword = !HostSessionPassword.IsEmpty();
		SessionSettings.Set(HAS_PASSWORD_SETTINGS_KEY, bHasPassword,
			EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		SessionSettings.Set(PW_HASH_SETTINGS_KEY,
			bHasPassword ? static_cast<int32>(FCrc::StrCrc32(*HostSessionPassword)) : 0,
			EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		SessionInterface->CreateSession(0, SESSION_NAME, SessionSettings);
	}
}

void UPlayerGameInstance::MoveToTitleMap()
{
	// DestroySession 콜백은 비동기라 월드가 이미 정리되는 중(PIE 종료 등)에 도착할 수 있다.
	// 그 시점엔 이동할 대상도, 로컬 컨트롤러도 없으므로 조용히 빠져나간다.
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown) return;

	if (APlayerController* PlayerController = GetFirstLocalPlayerController())
	{
		PlayerController->ClientTravel("/Game/Level/TitleMap", ETravelType::TRAVEL_Absolute);
		return;
	}

	// 레벨 전환 도중이라 컨트롤러가 아직 없는 경우의 폴백.
	UGameplayStatics::OpenLevel(World, FName("TitleMap"));
}

void UPlayerGameInstance::Join(uint32 Index, FString Password)
{
	if (!SessionInterface.IsValid())
		return;
	if (!SessionSearch.IsValid())
		return;

	if (Index >= (uint32)SessionSearch->SearchResults.Num())
		return;

	FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[Index];

	// 비밀번호 방이면 클라 측 선검증 (즉각적인 UX용 — 최종 검증은 서버 PreLogin).
	// 틀리면 메뉴를 유지한 채 안내만 하고 중단한다.
	bool bHasPassword = false;
	Result.Session.SessionSettings.Get(HAS_PASSWORD_SETTINGS_KEY, bHasPassword);
	if (bHasPassword)
	{
		int32 ExpectedHash = 0;
		Result.Session.SessionSettings.Get(PW_HASH_SETTINGS_KEY, ExpectedHash);
		if (Password.IsEmpty() ||
			static_cast<int32>(FCrc::StrCrc32(*Password)) != ExpectedHash)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Red,
					TEXT("비밀번호가 올바르지 않습니다."));
			}
			return;
		}
	}
	PendingJoinPassword = bHasPassword ? Password : TEXT("");

	if (Menu != nullptr)
	{
		Menu->Teardown();
	}

	// 스팀 JoinSession 은 bUsesPresence == bUseLobbiesIfAvailable 를 요구한다.
	// 검색 결과는 bUseLobbiesIfAvailable 가 기본 false 라 호스트(둘 다 true)와 불일치 → 맞춰준다.
	// (안 맞추면 JoinSession 이 거부돼 GetResolvedConnectString 실패 → "Could not get connect string")
	Result.Session.SessionSettings.bUsesPresence = true;
	Result.Session.SessionSettings.bUseLobbiesIfAvailable = true;

	SessionInterface->JoinSession(0, SESSION_NAME, Result);
}

void UPlayerGameInstance::QuickJoin()
{
	// 검색을 돌리고, 완료 시(OnFindSessionComplete) 최적 방을 자동 조인한다.
	bQuickJoinPending = true;
	RefreshServerList();
}

void UPlayerGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (!SessionInterface.IsValid())
		return;

	// 조인 결과부터 확인 — 실패면 주소 조회 시도 안 함.
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSession failed: result %d"), (int32)Result);
		return;
	}

	FString Address;
	if (!SessionInterface->GetResolvedConnectString(SessionName, Address)) {
		UE_LOG(LogTemp, Warning, TEXT("Could not get connect string."));
		return;
	}

	// 비밀번호 방: 접속 URL 옵션으로 전달 → 서버 GameMode::PreLogin에서 최종 검증
	if (!PendingJoinPassword.IsEmpty())
	{
		Address += FString::Printf(TEXT("?SessionPassword=%s"), *PendingJoinPassword);
	}

	UEngine* Engine = GetEngine();
	if (!ensure(Engine != nullptr))
		return;

	Engine->AddOnScreenDebugMessage(0, 5.f, FColor::Green, FString::Printf(TEXT("Joining %s"), *Address));

	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!PlayerController)
		return;

	PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
}

void UPlayerGameInstance::LoadMainMenu()
{
	DesiredServerName = TEXT("");
	DesiredPassword = TEXT("");
	HostSessionPassword = TEXT("");
	PendingJoinPassword = TEXT("");

	if (SessionInterface.IsValid())
	{
		SessionInterface->DestroySession(SESSION_NAME);
	}
	else
	{
		MoveToTitleMap();
	}
}

// ============================================================
// 플레이어 데이터 영속화
// ============================================================

void UPlayerGameInstance::SavePlayerData(const FString& PlayerID, const FPlayerPersistentData& Data)
{
	PlayerDataMap.Add(PlayerID, Data);
}

bool UPlayerGameInstance::LoadPlayerData(const FString& PlayerID, FPlayerPersistentData& OutData) const
{
	const FPlayerPersistentData* Found = PlayerDataMap.Find(PlayerID);
	if (Found)
	{
		OutData = *Found;
		return true;
	}
	return false;
}


// ============================================================
// ServerTravel 간 매치 상태 보존
// ============================================================

void UPlayerGameInstance::SaveMatchState(EMatchState InMatchState, int32 InStage, float InDifficulty)
{
	PendingMatchState = InMatchState;
	PendingStage = InStage;
	PendingDifficulty = InDifficulty;
	bHasPendingMatchState = true;

	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] SaveMatchState: %d, Stage=%d, Diff=%.1f"),
		(int32)InMatchState, InStage, InDifficulty);
}

void UPlayerGameInstance::RestoreMatchState(EMatchState& OutMatchState, int32& OutStage, float& OutDifficulty)
{
	OutMatchState = PendingMatchState;
	OutStage = PendingStage;
	OutDifficulty = PendingDifficulty;

	// 한 번 복원하면 소비
	bHasPendingMatchState = false;

	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] RestoreMatchState: %d, Stage=%d, Diff=%.1f"),
		(int32)OutMatchState, OutStage, OutDifficulty);
}