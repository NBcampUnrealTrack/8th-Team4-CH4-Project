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
}

void AGODGameState::OnRep_GamePhase()
{
	OnGamePhaseChanged.Broadcast(CurrentPhase);
}
