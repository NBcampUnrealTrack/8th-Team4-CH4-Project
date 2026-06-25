#include "Game/GODGameState.h"
#include "Net/UnrealNetwork.h"

AGODGameState::AGODGameState()
{
	CurrentPhase = EGamePhase::WaitingForPlayers;
	RemainingTime = 600;
	DistanceToDestination = 10000.0f;
	bGunsUnlocked = false;
	LobbyCountdown = 0;
}

void AGODGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGODGameState, CurrentPhase);
	DOREPLIFETIME(AGODGameState, RemainingTime);
	DOREPLIFETIME(AGODGameState, DistanceToDestination);
	DOREPLIFETIME(AGODGameState, bGunsUnlocked);
	DOREPLIFETIME(AGODGameState, LobbyCountdown);
	DOREPLIFETIME(AGODGameState, ChatHistory);
}

void AGODGameState::OnRep_GamePhase()
{
	OnGamePhaseChanged.Broadcast(CurrentPhase);
}

void AGODGameState::AddChatMessage(const FChatMessage& Msg)
{
	ChatHistory.Add(Msg);

	// 일단 임의로 50개 정도만 둘게용
	if (ChatHistory.Num() > 50)
		ChatHistory.RemoveAt(0);
	
	OnChatMessageReceived.Broadcast(Msg);
	// 추가
	UE_LOG(LogTemp, Warning, TEXT("[Chat] %s: %s"), *Msg.SenderName, *Msg.Message);

}

void AGODGameState::OnRep_ChatHistory()
{
	if (ChatHistory.Num() > 0)
	{
		OnChatMessageReceived.Broadcast(ChatHistory.Last());
	}
}
