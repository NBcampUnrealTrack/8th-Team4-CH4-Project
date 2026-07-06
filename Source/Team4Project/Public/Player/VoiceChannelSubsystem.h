#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VoiceChannelSubsystem.generated.h"

class UVoipTalkerComponent;

/**
 * 보이스 채널 정책 중앙 관리자 (이벤트 구동, 폴링/틱 없음).
 *
 * 발화 중인 Talker 들을 등록해 두고, 누군가의 생사 상태가 바뀌는 "이벤트 시점"에만
 * 각 Talker 의 ApplyVoicePolicy() 를 재적용한다.
 *
 * 이벤트 소스 (RefreshVoicePolicies 호출 지점):
 *  - AGODPlayerState::SetIsAlive (서버/리슨 호스트) · OnRep_bIsAlive (클라)
 *  - ABaseCharacter::OnRep_IsDead (래그돌 사망 복제 도착)
 *
 * 정책 자체(누가 누구에게 들리는가)는 VoipTalkerComponent::ApplyVoicePolicy 참고.
 */
UCLASS()
class TEAM4PROJECT_API UVoiceChannelSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UVoiceChannelSubsystem* Get(const UWorld* World)
	{
		return World ? World->GetSubsystem<UVoiceChannelSubsystem>() : nullptr;
	}

	// 발화 시작 시 Talker 가 자신을 등록 (OnTalkingBegin)
	void RegisterActiveTalker(UVoipTalkerComponent* Talker);

	// 발화 종료/teardown 시 등록 해제
	void UnregisterActiveTalker(UVoipTalkerComponent* Talker);

	// 생사 상태 변화 시 호출 → 현재 발화 중인 모든 Talker 의 정책 재적용
	void RefreshVoicePolicies();

private:
	// 현재 발화 중인 Talker 들. 파괴된 항목은 Refresh 때 정리된다.
	TArray<TWeakObjectPtr<UVoipTalkerComponent>> ActiveTalkers;
};
