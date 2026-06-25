// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BasePlayerController.h"

#include "OnlineSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Player/VoicePlayerState.h"


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
		// 복제 + 변경 알림 → 모든 클라(위젯 컴포넌트 포함)가 받고 갱신한다.
		// PlayerNameString = 스팀 계정 값 = 영속 저장 키(GetPersistentPlayerID).
		PS->SetPlayerNameString(Nick);
	}
}

void ABasePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 호스트(리슨서버) 로컬 플레이어는 AcknowledgePossession 이 안 불릴 수 있어 여기서도 보이스 시작.
	StartLocalVoice(InPawn);
}

void ABasePlayerController::OnUnPossess()
{
	// seamless travel 로 옛 Pawn 이 파괴되기 직전 호출된다(transition map 진입 시점).
	// 오디오 리스너 오버라이드가 파괴될 Pawn 의 RootComponent 를 계속 가리키면,
	// 오디오 엔진이 GC 전에 garbage 컴포넌트를 역참조해 EXCEPTION_ACCESS_VIOLATION 으로 터진다.
	// 새 Pawn 이 possess 되면 StartLocalVoice 가 다시 걸어주므로 여기서는 안전하게 비운다.
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

	// 오디오 리스너를 카메라(3인칭 SpringArm 400cm 뒤) 대신 로컬 Pawn 에 고정한다.
	// 이게 없으면 근접 보이스 감쇠가 카메라~발화자 3D 거리로 계산돼,
	// 바로 옆 사람도 항상 400cm+ 떨어진 것으로 측정되어 왜곡된다.
	//
	// 중요: seamless travel 시 PlayerController 는 유지되지만 Pawn 은 파괴/재생성된다.
	// 따라서 리스너 고정은 bVoiceStarted 가드와 무관하게 "매 possess 마다" 새 Pawn 으로
	// 다시 걸어야 한다. (안 그러면 새 맵에서 리스너가 파괴된 옛 Pawn 을 가리켜 보이스가 끊긴다.)
	if (USceneComponent* PawnRoot = P->GetRootComponent())
	{
		SetAudioListenerOverride(PawnRoot, FVector::ZeroVector, FRotator::ZeroRotator);
	}

	// 오픈 마이크: 소유 클라가 상시 음성 전송 시작(VOIP).
	// seamless travel 직전 StopLocalVoice 가 bVoiceStarted 를 리셋하므로, 도착 후 여기서 다시 켜진다.
	if (!bVoiceStarted)
	{
		bVoiceStarted = true;
		StartTalking();
	}
}

void ABasePlayerController::StopLocalVoice()
{
	if (!IsLocalController()) return;

	// 1) 나가는 마이크 중단.
	StopTalking();

	// 2) 핵심: 들어오는 음성 재생(VoipListenerSynthComponent)을 죽는 월드에서 떼어낸다.
	//    synth 는 발화자(=각 PlayerState)마다 그 Pawn 에 attach·register 되므로,
	//    travel 전에 "모든" PlayerState 의 재생 컴포넌트를 Stop+Unregister 해야
	//    오디오 렌더 스레드가 죽은 월드를 참조하다 크래시("World None.None")하는 걸 막는다.
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

	// 3) 리스너 오버라이드가 곧 파괴될 Pawn 을 가리키지 않도록 정리.
	ClearAudioListenerOverride();

	// 도착 후 OnPossess → StartLocalVoice 에서 다시 StartTalking 하도록 재무장.
	bVoiceStarted = false;
}

void ABasePlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	// 원격 클라이언트 경로: travel 직전 발화 중단.
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

	// 심리스 트래블 완료 후엔 서버/클라 양쪽에서 broadcast 된다.
	// (PostSeamlessTravel 은 서버 전용이라 클라 UI 재생성을 여기서 보장)
	if (!PostLoadMapHandle.IsValid())
	{
		PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
			this, &ABasePlayerController::HandlePostLoadMap);
	}

	// 호스트(리슨서버)는 OnRep_PlayerState 가 안 불리므로 여기서도 닉네임 전송 시도.
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