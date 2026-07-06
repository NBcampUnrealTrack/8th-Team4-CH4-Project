// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BasePlayerController.h"

#include "OnlineSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableText.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Net/UnrealNetwork.h"
#include "Player/VoicePlayerState.h"
#include "Player/GODPlayerState.h"
#include "Engine/Engine.h"
#include "Game/GODGameState.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Game/ChatTypes.h"
#include "HAL/FileManager.h"
#include "Sound/GameSoundStatics.h"
#include "Sound/GameSoundTypes.h"

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
	
	static ConstructorHelpers::FClassFinder<UUserWidget> ChatBoxFinder(
	  TEXT("/Game/ChatUI/WBP_ChatBox"));
	if (ChatBoxFinder.Succeeded())
		ChatBoxWidgetClass = ChatBoxFinder.Class;

	static ConstructorHelpers::FClassFinder<UUserWidget> ChatEntryFinder(
		TEXT("/Game/ChatUI/WBP_ChatEntry"));
	if (ChatEntryFinder.Succeeded())
		ChatEntryWidgetClass = ChatEntryFinder.Class;
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
	
	// 채팅 위젯 미리 생성
	if (IsLocalController() && ChatBoxWidgetClass)
	{
		ChatBoxWidget = CreateWidget(this, ChatBoxWidgetClass);
		ChatBoxWidget->AddToViewport();
		ChatBoxWidget->SetVisibility(ESlateVisibility::HitTestInvisible); // 변경: Collapsed → HitTestInvisible

		if (AGODGameState* GS = GetWorld()->GetGameState<AGODGameState>())
		{
			GS->OnChatMessageReceived.AddDynamic(
				this, &ABasePlayerController::HandleChatMessage);
		}

		if (UEditableText* Input = GetChatInputWidget())
		{
			Input->OnTextCommitted.AddDynamic(
				this, &ABasePlayerController::OnChatTextCommitted);
			Input->SetVisibility(ESlateVisibility::Collapsed); // 추가: 시작 시 입력창은 숨김
		}

		// 추가: 시작할 때 히스토리 한 번 채우기 (한 프레임 뒤)
		GetWorldTimerManager().SetTimerForNextTick([this]()
		{
			if (AGODGameState* GS = GetWorld()->GetGameState<AGODGameState>())
			{
				// 백필은 과거 메시지라 수신음을 내지 않는다.
				bChatBackfillInProgress = true;
				for (const FChatMessage& Msg : GS->ChatHistory)
					HandleChatMessage(Msg);
				bChatBackfillInProgress = false;
			}
		});
	}
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
	InputComponent->BindKey(EKeys::T, IE_Pressed, this, &ABasePlayerController::OnChatToggle);

	// 왼쪽 Alt 홀드: 커서 표시 + HUD 클릭 모드 (떼면 다시 마우스 시야 전환)
	InputComponent->BindKey(EKeys::LeftAlt, IE_Pressed, this, &ABasePlayerController::OnHUDCursorPressed);
	InputComponent->BindKey(EKeys::LeftAlt, IE_Released, this, &ABasePlayerController::OnHUDCursorReleased);
}

// ============================================================
// HUD 커서 모드
// ============================================================

void ABasePlayerController::SetHUDCursorMode(bool bEnable)
{
	// 가짜 시체 등 게임플레이가 강제로 켜는 지속형 커서 (Alt 를 뗐다고 꺼지지 않는다).
	bCursorForced = bEnable;
	ApplyHUDCursorMode();
}

void ABasePlayerController::OnHUDCursorPressed()
{
	bCursorHeldByKey = true;
	ApplyHUDCursorMode();
}

void ABasePlayerController::OnHUDCursorReleased()
{
	bCursorHeldByKey = false;
	ApplyHUDCursorMode();
}

void ABasePlayerController::ApplyHUDCursorMode()
{
	// 관전/채팅은 자체적으로 입력 모드를 관리하므로 덮어쓰지 않는다.
	if (!IsLocalController() || bIsSpectating || bChatOpen) return;

	if (bCursorForced || bCursorHeldByKey)
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
	}
	else
	{
		bShowMouseCursor = false;
		FInputModeGameOnly Mode;
		Mode.SetConsumeCaptureMouseDown(false);
		SetInputMode(Mode);
	}
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

		// 살아있는 플레이어만 관전 대상 (죽은 시체는 제외)
		if (const AGODPlayerState* GPS = Cast<AGODPlayerState>(PS))
		{
			if (!GPS->bIsAlive) continue;
		}

		APawn* OtherPawn = PS->GetPawn();
		if (!OtherPawn) continue;

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


// ============================================================
// 채팅 시스템
// ============================================================

void ABasePlayerController::OpenChat()
{
	if (bIsSpectating || bChatOpen || !ChatBoxWidget) return;
	bChatOpen = true;

	if (UEditableText* Input = GetChatInputWidget())
	{
		Input->SetVisibility(ESlateVisibility::Visible);

		GetWorldTimerManager().SetTimerForNextTick([this, Input]()
		{
			TSharedPtr<SWidget> SlateWidget = Input->GetCachedWidget();
			if (SlateWidget.IsValid())
			{
				FSlateApplication::Get().SetKeyboardFocus(SlateWidget, EFocusCause::SetDirectly);
			}
		});
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	SetInputMode(InputMode);
	bShowMouseCursor = false;
}

void ABasePlayerController::OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter) return;

	const FString Message = Text.ToString();
	if (!Message.IsEmpty())
		SubmitChat(Message);
	else
		CloseChat();
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

void ABasePlayerController::HandleChatMessage(const FChatMessage& Msg)
{
	UScrollBox* ScrollBox = GetChatScrollBox();
	if (!ScrollBox || !ChatEntryWidgetClass) return;

	UUserWidget* Entry = CreateWidget(this, ChatEntryWidgetClass);
	if (!Entry) return;

	
	if (UTextBlock* Text = Cast<UTextBlock>(
			Entry->GetWidgetFromName(TEXT("Text_Line"))))
	{
		Text->SetText(FText::FromString(
			FString::Printf(TEXT("[%s]: %s"), *Msg.SenderName, *Msg.Message)));
	}

	ScrollBox->AddChild(Entry);
	ScrollBox->ScrollToEnd();

	// 수신음 — 접속 시 히스토리 백필 중에는 생략.
	if (!bChatBackfillInProgress)
	{
		UGameSoundStatics::PlaySound2DFromTable(this, UISoundTable, SoundRows::UIChatReceive);
	}
}

void ABasePlayerController::CloseChat()
{
	if (!bChatOpen) return;
	bChatOpen = false;

	if (UEditableText* Input = GetChatInputWidget())
	{
		Input->SetText(FText::GetEmpty());
		Input->SetVisibility(ESlateVisibility::Collapsed); // 변경: 입력창만 숨김
	}

	// 삭제: ChatBoxWidget->SetVisibility(Collapsed) 하지 않음 (로그 유지)

	GetWorldTimerManager().SetTimerForNextTick([this]()
	{
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		SetInputMode(InputMode);
	});
}

UScrollBox* ABasePlayerController::GetChatScrollBox() const
{
	if (!ChatBoxWidget) return nullptr;
	return Cast<UScrollBox>(
		ChatBoxWidget->GetWidgetFromName(TEXT("ScrollBox_ChatLog")));
}

UEditableText* ABasePlayerController::GetChatInputWidget() const
{
	if (!ChatBoxWidget) return nullptr;
	return Cast<UEditableText>(
		ChatBoxWidget->GetWidgetFromName(TEXT("EditableText_Input")));
}
void ABasePlayerController::OnChatToggle()
{
	UE_LOG(LogTemp, Warning, TEXT("OnChatToggle called, bChatOpen=%d"), bChatOpen);
	if (bChatOpen)
		CloseChat();
	else
		OpenChat();
}