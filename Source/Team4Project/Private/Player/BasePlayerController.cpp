// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BasePlayerController.h"

#include "OnlineSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Net/UnrealNetwork.h"
#include "Player/VoicePlayerState.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

// [임시 디버그] Shipping 에서도 보이는 유일한 채널 = 파일. Saved/NameDebug.log 에 누적. 확인 후 제거.
namespace
{
	void NameDbg(const FString& Line)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Line);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Cyan, Line);
		}
		const FString Path = FPaths::ProjectSavedDir() / TEXT("NameDebug.log");
		const FString Stamp = FDateTime::Now().ToString(TEXT("%H:%M:%S"));
		FFileHelper::SaveStringToFile(
			FString::Printf(TEXT("[%s] %s\r\n"), *Stamp, *Line),
			*Path, FFileHelper::EEncodingOptions::ForceUTF8,
			&IFileManager::Get(), EFileWrite::FILEWRITE_Append);
	}
}


ABasePlayerController::ABasePlayerController()
{
	bReplicates = true;
}

void ABasePlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	TrySendNickname();
}

void ABasePlayerController::TrySendNickname()
{
	if (!IsLocalController() || bNicknameSent) return;

	FString Nick;
	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		// NULL 서브시스템(PIE 등)은 진짜 닉네임이 없고 머신/계정 기본 이름(DESKTOP-...)을 돌려준다.
		// 이 경우 전송하지 않고 서버 폴백("Player{N}")을 유지한다. 실제 플랫폼(Steam/EOS)만 닉네임 사용.
		if (OSS->GetSubsystemName() != NULL_SUBSYSTEM)
		{
			if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
			{
				Nick = Identity->GetPlayerNickname(0);
			}
		}
	}

	if (Nick.IsEmpty()) return;

	bNicknameSent = true;
	Server_SetNickname(Nick);
}

void ABasePlayerController::Server_SetNickname_Implementation(const FString& Nick)
{
	if (Nick.IsEmpty()) return;

	if (AVoicePlayerState* PS = GetPlayerState<AVoicePlayerState>())
	{
		PS->SetPlayerNameString(Nick);
	}
}

void ABasePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	StartLocalVoice(InPawn);
}

void ABasePlayerController::OnUnPossess()
{
	if (IsLocalController())
	{
		ClearAudioListenerOverride();
	}

	Super::OnUnPossess();
}

void ABasePlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	// 원격 클라이언트 경로.
	StartLocalVoice(P);
}

void ABasePlayerController::StartLocalVoice(APawn* P)
{
	if (!IsLocalController() || !IsValid(P)) return;
	
	if (USceneComponent* PawnRoot = P->GetRootComponent())
	{
		SetAudioListenerOverride(PawnRoot, FVector::ZeroVector, FRotator::ZeroRotator);
	}

	// 오픈 마이크: 소유 클라가 상시 음성 전송 시작(VOIP).
	if (!bVoiceStarted)
	{
		bVoiceStarted = true;
		StartTalking();
	}
}

void ABasePlayerController::StopLocalVoice()
{
	if (!IsLocalController()) return;
	
	StopTalking();
	
	if (AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (AVoicePlayerState* BPS = Cast<AVoicePlayerState>(PS))
			{
				BPS->StopVoicePlayback();
			}
		}
	}
	
	ClearAudioListenerOverride();
	
	bVoiceStarted = false;
}

void ABasePlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	StopLocalVoice();

	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);
}

void ABasePlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ABasePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (!PostLoadMapHandle.IsValid())
	{
		PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
			this, &ABasePlayerController::HandlePostLoadMap);
	}
	
	TrySendNickname();
}

void ABasePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void ABasePlayerController::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!IsLocalController()) return;

	if (LoadedWorld && LoadedWorld != GetWorld()) return;
}


void ABasePlayerController::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();
	
}

void ABasePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ABasePlayerController::OnSpectateNextClicked);
	InputComponent->BindKey(EKeys::T, IE_Pressed, this, &ABasePlayerController::OpenChat);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ABasePlayerController::CloseChat);
	
}

// ============================================================
// 관전 시스템
// ============================================================

void ABasePlayerController::Client_StartSpectating_Implementation()
{
	bIsSpectating = true;

	bShowMouseCursor = true;
	SetInputMode(FInputModeGameAndUI());

	Server_SpectateNext();
}

void ABasePlayerController::Server_StartSpectating_Implementation()
{
	bIsSpectating = true;
	Server_SpectateNext();
}

void ABasePlayerController::Server_SpectateNext_Implementation()
{
	APawn* Target = FindNextSpectateTarget();
	if (!Target) return;

	CurrentSpectateTarget = Target;
	SetViewTargetWithBlend(Target, 0.3f);
}

APawn* ABasePlayerController::FindNextSpectateTarget() const
{
	AGameStateBase* GS = GetWorld()->GetGameState();
	if (!GS) return nullptr;

	TArray<APawn*> AlivePlayers;
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (!PS || PS == GetPlayerState<APlayerState>()) continue;

		APawn* OtherPawn = PS->GetPawn();
		if (!OtherPawn) continue;

		if (ACharacter* OtherCharacter = Cast<ACharacter>(OtherPawn))
		{
			//if (OtherCharacter->IsDead()) continue;
		}

		AlivePlayers.Add(OtherPawn);
	}

	if (AlivePlayers.Num() == 0) return nullptr;

	int32 CurrentIndex = -1;
	if (CurrentSpectateTarget.IsValid())
	{
		CurrentIndex = AlivePlayers.IndexOfByKey(CurrentSpectateTarget.Get());
	}

	int32 NextIndex = (CurrentIndex + 1) % AlivePlayers.Num();
	return AlivePlayers[NextIndex];
}

void ABasePlayerController::OnSpectateNextClicked()
{
	if (!bIsSpectating) return;

	Server_SpectateNext();
}


//채팅 관련
void ABasePlayerController::OpenChat()
{
	if (bIsSpectating || bChatOpen) return;
	bChatOpen = true;
	
	// UI 없이 테스트용 — T 누르면 고정 메시지 바로 전송
	SubmitChat(TEXT("안녕하세요 테스트 메시지입니다"));
}

void ABasePlayerController::CloseChat()
{
	if (!bChatOpen) return;
	bChatOpen = false;
	OnChatClosed();
}

void ABasePlayerController::SubmitChat(const FString& Message)
{
	if (Message.IsEmpty()) return;

	if (AGODPlayerState* PS = GetPlayerState<AGODPlayerState>())
	{
		PS->Server_SendChat(Message);
	}
	CloseChat();
}