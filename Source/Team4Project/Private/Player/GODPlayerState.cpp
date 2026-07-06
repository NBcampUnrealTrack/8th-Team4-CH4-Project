#include "Player/GODPlayerState.h"
#include "Player/VoiceChannelSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Game/ChatTypes.h"
#include "Game/GODGameState.h"


AGODPlayerState::AGODPlayerState()
{
	bIsReady = false;
	bIsAlive = true;
	AmmoCount = 1; // 모든 플레이어는 1발로 시작
	SootLevel = 0.0f;
	MainRole = EMainRole::Citizen;
	CitizenClass = ECitizenClass::None;
}

void AGODPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGODPlayerState, bIsReady);
	DOREPLIFETIME(AGODPlayerState, MainRole);
	DOREPLIFETIME(AGODPlayerState, CitizenClass);
	DOREPLIFETIME(AGODPlayerState, bIsAlive);
	DOREPLIFETIME(AGODPlayerState, AmmoCount);
	DOREPLIFETIME(AGODPlayerState, SootLevel);
	
}

void AGODPlayerState::SetIsAlive(bool bNewAlive)
{
	if (!HasAuthority() || bIsAlive == bNewAlive)
	{
		return;
	}

	bIsAlive = bNewAlive;
	OnRep_bIsAlive(); // 리슨 서버(호스트)는 OnRep이 안 오므로 직접 호출
}

void AGODPlayerState::OnRep_bIsAlive()
{
	// 생사 변화 → 발화 중인 보이스들의 격리 정책(죽은 사람끼리만 등)을 즉시 재적용
	if (UVoiceChannelSubsystem* Voice = UVoiceChannelSubsystem::Get(GetWorld()))
	{
		Voice->RefreshVoicePolicies();
	}
}

void AGODPlayerState::Server_SendChat_Implementation(const FString& Message)
{
	AGODGameState* GS = GetWorld()->GetGameState<AGODGameState>();
	if (!GS) return;

	UE_LOG(LogTemp, Warning, TEXT("[Chat RPC] 수신됨: %s"), *Message);
	// 게임 시작하기 전에 채팅 허용하려면 조건 제거하면 됩니당
	FChatMessage Msg;
	Msg.SenderName  = GetPlayerName(); 
	Msg.Message     = Message.Left(MaxChatLength);
	Msg.Timestamp   = GetWorld()->GetTimeSeconds();

	GS->AddChatMessage(Msg);
}

bool AGODPlayerState::Server_SendChat_Validate(const FString& Message)
{
	return !Message.IsEmpty() && Message.Len() <= MaxChatLength;
}
