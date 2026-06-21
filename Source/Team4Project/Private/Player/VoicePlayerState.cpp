// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/VoicePlayerState.h"
#include "Player/Component/VoipTalkerComponent.h"
#include "Net/VoiceConfig.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UObjectGlobals.h"

AVoicePlayerState::AVoicePlayerState()
	: PlayerNameString(TEXT("None"))
{
	bReplicates = true;

	VoipTalker = CreateDefaultSubobject<UVoipTalkerComponent>(TEXT("VoipTalker"));

	NetUpdateFrequency = 100.f;
}

void AVoicePlayerState::BeginPlay()
{
	Super::BeginPlay();

	// VoiceTalkerMap 등록 시도. UniqueId 가 아직 안 왔으면 내부에서 조용히 실패하고,
	// OnSetUniqueId 가 유효해지는 시점에 다시 등록한다.
	if (IsValid(VoipTalker))
	{
		VoipTalker->RegisterWithPlayerState(this);
	}

	// seamless travel 후 맵 로드가 끝나는 시점에 재등록한다.
	// 트래블 중엔 (1)새 PlayerState 가 등록 → (2)옛 PlayerState 파괴 시 같은 UniqueId 의
	// VoiceTalkerMap 엔트리를 지워 무음이 되는 레이스가 있다. 맵 로드 완료 후 다시 등록해 이를 이긴다.
	if (!VoicePostLoadMapHandle.IsValid())
	{
		VoicePostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
			this, &AVoicePlayerState::HandlePostLoadMap);
	}
}

void AVoicePlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (VoicePostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(VoicePostLoadMapHandle);
		VoicePostLoadMapHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void AVoicePlayerState::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// 이 PlayerState 가 속한 월드가 로드된 경우에만 재등록.
	if (LoadedWorld && LoadedWorld != GetWorld())
	{
		return;
	}

	ReregisterVoiceTalker();
}

void AVoicePlayerState::StopVoicePlayback()
{
	if (IsValid(VoipTalker))
	{
		VoipTalker->TeardownVoiceAudio();
	}
}

void AVoicePlayerState::ReregisterVoiceTalker()
{
	if (!IsValid(VoipTalker))
	{
		return;
	}

	// 1) 이 플레이어의 기존 VoiceTalkerMap 엔트리를 제거(옛 Talker 의 잔재/덮어쓰기 방지).
	UVOIPStatics::ResetPlayerVoiceTalker(this);

	// 2) 현재 살아있는 Talker 로 다시 등록.
	VoipTalker->RegisterWithPlayerState(this);
}

void AVoicePlayerState::OnSetUniqueId()
{
	Super::OnSetUniqueId();

	if (IsValid(VoipTalker))
	{
		VoipTalker->RegisterWithPlayerState(this);
	}
}

void AVoicePlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PlayerNameString);
}

void AVoicePlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	// seamless travel: 옛 PlayerState → 새 PlayerState 로 닉네임(스팀 계정 = 영속 키) 이전.
	// 이게 없으면 새 맵에서 PlayerNameString 이 비어 GameMode 가 Player{N} 폴백을 다시 박는다.
	if (AVoicePlayerState* NewPS = Cast<AVoicePlayerState>(PlayerState))
	{
		NewPS->PlayerNameString = PlayerNameString;
	}
}

FString AVoicePlayerState::GetPlayerInfoString()
{
	FString PlayerInfoString = PlayerNameString;
	return PlayerInfoString;
}

void AVoicePlayerState::OnRep_PlayerNameString()
{
	OnPlayerNameChanged.Broadcast(PlayerNameString);
}

void AVoicePlayerState::SetPlayerNameString(const FString& InName)
{
	PlayerNameString = InName;
	OnPlayerNameChanged.Broadcast(PlayerNameString); // 호스트 즉시(클라는 OnRep_PlayerNameString)
}

FString AVoicePlayerState::GetPersistentPlayerID() const
{
	// 1순위: 소유 클라가 올린 닉네임(스팀 계정 값). CopyProperties 로 seamless travel 간 유지된다.
	if (!PlayerNameString.IsEmpty() && PlayerNameString != TEXT("None"))
	{
		return PlayerNameString;
	}

	// 폴백: PlayerState UniqueNetId → 이름. (스팀 닉네임이 아직 도착 전이거나 NULL 서브시스템)
	FUniqueNetIdRepl PsUniqueId = GetUniqueId();
	if (PsUniqueId.IsValid())
	{
		return PsUniqueId->ToString();
	}
	return GetPlayerName();
}

