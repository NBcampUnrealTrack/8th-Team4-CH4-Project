// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PlayerGameInstance.h"

#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

#include "UI/MenuSystem/MainMenu.h"
#include "UI/MenuSystem/MenuWidget.h"
#include "Game/GODGameState.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "Blueprint/UserWidget.h"

const static FName SESSION_NAME = TEXT("GameSession");
const static FName SERVER_NAME_SETTINGS_KEY = TEXT("ServerName");


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
	
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Found Subsysytem %s"), *Subsystem->GetSubsystemName().ToString());
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UPlayerGameInstance::OnCreateSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UPlayerGameInstance::OnDestorySessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UPlayerGameInstance::OnFindSessionComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UPlayerGameInstance::OnJoinSessionComplete);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Found No Subsysytem"));
	}

	if (GEngine != nullptr)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UPlayerGameInstance::OnNetworkFailure);
	}
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

void UPlayerGameInstance::Host(FString ServerName)
{
	DesiredServerName = ServerName; 
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
	if (Success && SessionSearch.IsValid() && Menu != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Finished Find Session"));

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
				Data.Name = ServerName;
			}
			else
			{
				Data.Name = "Could not find name.";
			}

			ServerNames.Add(Data);
		}

		Menu->SetServerList(ServerNames);
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

		SessionSettings.NumPublicConnections = 5;
		SessionSettings.bShouldAdvertise = true;
		SessionSettings.bUsesPresence = true;
       
		// 스팀 매칭을 사용할 때 필수적으로 켜주어야 하는 옵션들입니다.
		SessionSettings.bUseLobbiesIfAvailable = true; 
		SessionSettings.bAllowJoinInProgress = true;
		
		SessionSettings.Set(SERVER_NAME_SETTINGS_KEY, DesiredServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		SessionInterface->CreateSession(0, SESSION_NAME, SessionSettings);
	}
}

void UPlayerGameInstance::MoveToTitleMap()
{
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!ensure(PlayerController != nullptr))
		return;

	PlayerController->ClientTravel("/Game/Level/TitleMap", ETravelType::TRAVEL_Absolute);
}

void UPlayerGameInstance::Join(uint32 Index)
{
	if (!SessionInterface.IsValid())
		return;
	if (!SessionSearch.IsValid())
		return;

	if (Index >= (uint32)SessionSearch->SearchResults.Num())
		return;

	if (Menu != nullptr)
	{
		Menu->Teardown();
	}

	// 스팀 JoinSession 은 bUsesPresence == bUseLobbiesIfAvailable 를 요구한다.
	// 검색 결과는 bUseLobbiesIfAvailable 가 기본 false 라 호스트(둘 다 true)와 불일치 → 맞춰준다.
	// (안 맞추면 JoinSession 이 거부돼 GetResolvedConnectString 실패 → "Could not get connect string")
	FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[Index];
	Result.Session.SessionSettings.bUsesPresence = true;
	Result.Session.SessionSettings.bUseLobbiesIfAvailable = true;

	SessionInterface->JoinSession(0, SESSION_NAME, Result);
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