#include "Player/GODPlayerState.h"
#include "Player/VoiceChannelSubsystem.h"
#include "Player/BaseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Game/ChatTypes.h"
#include "Game/GODGameState.h"
#include "Quest/QuestStation.h"


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
	// 역할은 소유 클라에만 복제 — 소셜 디덕션이라 다른 플레이어에게 역할이 노출되면 안 된다.
	// (ABaseCharacter::CharacterTag 의 COND_OwnerOnly 와 동일한 정책)
	DOREPLIFETIME_CONDITION(AGODPlayerState, MainRole, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGODPlayerState, CitizenClass, COND_OwnerOnly);
	DOREPLIFETIME(AGODPlayerState, bIsAlive);
	DOREPLIFETIME(AGODPlayerState, AmmoCount);
	DOREPLIFETIME(AGODPlayerState, SootLevel);

	DOREPLIFETIME_CONDITION(AGODPlayerState, AssignedQuests, COND_OwnerOnly);
}

void AGODPlayerState::OnRep_AssignedQuests()
{
	OnAssignedQuestsChanged.Broadcast();
}

bool AGODPlayerState::TryMarkQuestCompleted(AQuestStation* Station)
{
	if (!HasAuthority() || !Station) return false;

	for (FAssignedQuest& Quest : AssignedQuests)
	{
		if (Quest.Station != Station) continue;
		if (Quest.bCompleted) return false; // 중복 완료 방지

		Quest.bCompleted = true;
		OnRep_AssignedQuests(); // 리슨 호스트는 OnRep 이 안 오므로 직접 호출
		return true;
	}

	// 배정되지 않은 스테이션
	return false;
}

bool AGODPlayerState::AreAllQuestsCompleted() const
{
	if (AssignedQuests.Num() == 0) return false;

	for (const FAssignedQuest& Quest : AssignedQuests)
	{
		if (!Quest.bCompleted) return false;
	}
	return true;
}

int32 AGODPlayerState::GetCompletedQuestCount() const
{
	int32 Count = 0;
	for (const FAssignedQuest& Quest : AssignedQuests)
	{
		if (Quest.bCompleted) ++Count;
	}
	return Count;
}

void AGODPlayerState::SetIsAlive(bool bNewAlive)
{
	if (!HasAuthority() || bIsAlive == bNewAlive)
	{
		return;
	}

	bIsAlive = bNewAlive;

	// 부활(재시작) 시 캐릭터의 사망 플래그도 함께 리셋한다.
	// 보이스 사망 판정 = bIsAlive OR Character::IsDead() 이므로, 한쪽만 살아나면
	// 보이스가 계속 '사망(2D/음소거)'으로 고착돼 재시작 후 감쇠가 안 돌아온다.
	if (bNewAlive)
	{
		if (ABaseCharacter* Char = Cast<ABaseCharacter>(GetPawn()))
		{
			Char->ResetDeathState();
		}
	}

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
	
	//관전자 채팅용
	Msg.bIsSpectatorChat = !bIsAlive;

	GS->AddChatMessage(Msg);
}

bool AGODPlayerState::Server_SendChat_Validate(const FString& Message)
{
	return !Message.IsEmpty() && Message.Len() <= MaxChatLength;
}
