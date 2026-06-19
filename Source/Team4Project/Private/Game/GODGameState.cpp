#include "Game/GODGameState.h"
#include "Net/UnrealNetwork.h"

AGODGameState::AGODGameState()
{
	CurrentPhase = EGamePhase::WaitingForPlayers;
	RemainingTime = 600; // 10분
	DistanceToDestination = 10000.0f;
	bGunsUnlocked = false; // 처음에는 발포 불가
}

void AGODGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGODGameState, CurrentPhase);
	DOREPLIFETIME(AGODGameState, RemainingTime);
	DOREPLIFETIME(AGODGameState, DistanceToDestination);
	DOREPLIFETIME(AGODGameState, bGunsUnlocked);
}

void AGODGameState::OnRep_GamePhase()
{
	// TODO: Phase 변경에 따른 승리/패배 UI 연출 로직 연결
}
