// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Component/VoipTalkerComponent.h"
#include "Player/GODPlayerState.h"
#include "Player/BaseCharacter.h"
#include "Player/VoiceChannelSubsystem.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	// PlayerState 가 가리키는 플레이어가 사망 상태인지(생존 플래그 OR 캐릭터 래그돌).
	bool IsPlayerStateDead(const APlayerState* PS)
	{
		if (!PS)
		{
			return false;
		}

		// 게임 로직상의 생존 플래그(관전 전환/승패 판정과 동일 소스).
		if (const AGODPlayerState* GodPS = Cast<AGODPlayerState>(PS))
		{
			if (!GodPS->bIsAlive)
			{
				return true;
			}
		}

		// 캐릭터 사망(래그돌) 상태. 총격 사망은 현재 이쪽만 갱신될 수 있어 함께 확인.
		if (const ABaseCharacter* Char = Cast<ABaseCharacter>(PS->GetPawn()))
		{
			if (Char->IsDead())
			{
				return true;
			}
		}

		return false;
	}
}

void UVoipTalkerComponent::OnRegister()
{
	Super::OnRegister();

	// 발화가 시작되기 훨씬 전(빙의 시점)부터 Settings 를 최신 상태로 유지하기 위해 미리 구독한다.
	// 엔진은 발화 세션 시작 시(OnTalkingBegin 직전) Settings.AttenuationSettings/ComponentToAttachTo 를
	// 읽어 synth 에 적용하므로, 이걸 OnTalkingBegin 에서 채우면 항상 한 발 늦어 감쇠가 안 걸린다.
	if (APlayerState* OwnerPS = GetOwner<APlayerState>())
	{
		OwnerPS->OnPawnSet.AddUniqueDynamic(this, &UVoipTalkerComponent::HandleOwnerPawnSet);
	}

	RefreshVoiceSettings();
}

void UVoipTalkerComponent::OnUnregister()
{
	if (APlayerState* OwnerPS = GetOwner<APlayerState>())
	{
		OwnerPS->OnPawnSet.RemoveDynamic(this, &UVoipTalkerComponent::HandleOwnerPawnSet);
	}

	Super::OnUnregister();
}

void UVoipTalkerComponent::OnTalkingBegin(UAudioComponent* AudioComponent)
{
	if (!IsValid(AudioComponent))
	{
		Super::OnTalkingBegin(AudioComponent);
		return;
	}

	PlayingAudioComponent = AudioComponent;

	ApplyVoicePolicy();

	// 사망/생존과 무관하게 발화자 Pawn 이 파괴(seamless travel 포함)되면 즉시 teardown 하도록 구독.
	// TALKING 인 채로 travel 이 일어나면 OnTalkingEnd 가 파괴 "후"에야 불려 늦기 때문.
	APlayerState* OwnerPS = GetOwner<APlayerState>();
	if (APawn* OwnerPawn = OwnerPS ? OwnerPS->GetPawn() : nullptr)
	{
		BindOwnerPawnEndPlay(OwnerPawn);
	}

	// 발화 중 생사 상태가 바뀌면(사망/부활/청자 사망) 서브시스템이 ApplyVoicePolicy를
	// 다시 불러준다 — 폴링 없이 이벤트 시점에만 재적용되는 구조.
	if (UVoiceChannelSubsystem* Voice = UVoiceChannelSubsystem::Get(GetWorld()))
	{
		Voice->RegisterActiveTalker(this);
	}

	Super::OnTalkingBegin(AudioComponent);
}

void UVoipTalkerComponent::HandleOwnerPawnSet(APlayerState* /*Player*/, APawn* /*NewPawn*/, APawn* /*OldPawn*/)
{
	// 빙의(또는 빙의 해제) 시점마다 Settings 를 갱신 → 다음 발화가 올바른 감쇠/부착으로 시작된다.
	// 발화 중이면 ApplyVoicePolicy 가 현재 재생에도 즉시 반영한다.
	ApplyVoicePolicy();
}

void UVoipTalkerComponent::RefreshVoiceSettings()
{
	APlayerState* OwnerPS = GetOwner<APlayerState>();
	APawn* OwnerPawn = OwnerPS ? OwnerPS->GetPawn() : nullptr;
	USceneComponent* PawnRoot = OwnerPawn ? OwnerPawn->GetRootComponent() : nullptr;

	// 전체 2D 보이스: 감쇠 없음(거리 무관).
	// 사망 발화자만 래그돌을 따라다니지 않도록 미부착으로 둔다(생존자는 부착해도 2D라 무해).
	Settings.AttenuationSettings = nullptr;
	Settings.ComponentToAttachTo = IsSpeakerDead() ? nullptr : PawnRoot;
}

void UVoipTalkerComponent::ApplyVoicePolicy()
{
	// 다음 발화 세션을 위해 Settings 를 항상 먼저 최신화한다(엔진이 그 시점에 이 값을 읽는다).
	RefreshVoiceSettings();

	UAudioComponent* AudioComponent = PlayingAudioComponent.Get();
	if (!IsValid(AudioComponent))
	{
		return; // 재생 중이 아니면 Settings 갱신만으로 충분(다음 발화에 반영).
	}

	// 전체 2D 보이스: 공간화/감쇠는 항상 끈다(거리 무관).
	// "들리냐 / 안 들리냐 / 얼마나 크게"는 오직 볼륨 하나로 결정 — ComputeListenerVolume 참고.
	// (매 호출 볼륨을 다시 계산하므로, 음소거됐던 컴포넌트가 재사용돼도 자동 복구된다.)
	AudioComponent->SetVolumeMultiplier(ComputeListenerVolume());
	AudioComponent->bAllowSpatialization = false;
	AudioComponent->SetAttenuationSettings(nullptr);

	// 사망 발화자: 죽은 캐릭터(래그돌)를 따라다니지 않도록 재생 컴포넌트를 떼어낸다.
	if (IsSpeakerDead() && AudioComponent->GetAttachParent())
	{
		AudioComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
}

float UVoipTalkerComponent::ComputeListenerVolume() const
{
	// 오디빌리티 정책은 여기 한 곳에 집중한다.
	//  - 사망 발화자 → 죽은 청자에게만 들림. 산 청자에겐 음소거(0).
	//  - 생존 발화자 → 청자 생사와 무관하게 전원에게 들림.
	// ※ 단방향 격리(산 사람→안 들림 / 죽은 사람→들림)는 엔진 양방향 Mute 로 표현할 수 없어,
	//   각 청자 머신에서 로컬 청자의 생사를 보고 재생 여부를 직접 결정한다.
	if (IsSpeakerDead() && !IsLocalListenerDead())
	{
		return 0.f;
	}

	return VoiceVolume;
}

bool UVoipTalkerComponent::IsSpeakerDead() const
{
	return IsPlayerStateDead(GetOwner<APlayerState>());
}

bool UVoipTalkerComponent::IsLocalListenerDead() const
{
	// 이 머신의 로컬 플레이어 컨트롤러(클라=자기 자신, 리슨서버=호스트).
	const APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
	return LocalPC ? IsPlayerStateDead(LocalPC->PlayerState) : false;
}

void UVoipTalkerComponent::OnTalkingEnd()
{
	TeardownVoiceAudio();
	Super::OnTalkingEnd();
}

void UVoipTalkerComponent::TeardownVoiceAudio()
{
	// 발화 종료 → 채널 등록 해제 (정책 재적용 대상에서 제외)
	if (UVoiceChannelSubsystem* Voice = UVoiceChannelSubsystem::Get(GetWorld()))
	{
		Voice->UnregisterActiveTalker(this);
	}
	// OnPawnSet 구독은 OnRegister/OnUnregister 가 컴포넌트 수명 동안 유지한다(여기서 해제하지 않음).
	// 정상 발화 종료 시엔 Pawn 이 살아있어 Settings.ComponentToAttachTo 를 그대로 두어야
	// 다음 발화가 곧바로 Pawn 에 부착돼 감쇠가 유지된다. (부착 대상 해제는 Pawn 파괴 시에만)

	// 재생 컴포넌트를 죽는 월드에서 완전히 빼낸다.
	// synth 는 /Engine/Transient 전역이라 월드가 죽어도 살아남으므로, 단순 detach 만으론
	// 죽은 월드의 오디오 디바이스에 "등록된 채" 남아 오디오 렌더 스레드가 역참조하다 크래시한다.
	// → Stop(렌더 중단) + Detach(부모 끊기) + Unregister(오디오 디바이스에서 제거)까지 한다.
	if (UAudioComponent* AC = PlayingAudioComponent.Get())
	{
		AC->Stop();
		AC->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		if (AC->IsRegistered())
		{
			AC->UnregisterComponent(); // 다음 발화 시 엔진 ApplyVoiceSettings 가 새 월드로 재등록한다.
		}
	}
	PlayingAudioComponent.Reset();

	UnbindOwnerPawnEndPlay();
}

void UVoipTalkerComponent::BindOwnerPawnEndPlay(APawn* OwnerPawn)
{
	if (BoundPawn.Get() == OwnerPawn)
	{
		return;
	}

	UnbindOwnerPawnEndPlay();

	if (IsValid(OwnerPawn))
	{
		OwnerPawn->OnEndPlay.AddDynamic(this, &UVoipTalkerComponent::HandleOwnerPawnEndPlay);
		BoundPawn = OwnerPawn;
	}
}

void UVoipTalkerComponent::UnbindOwnerPawnEndPlay()
{
	if (APawn* Prev = BoundPawn.Get())
	{
		Prev->OnEndPlay.RemoveDynamic(this, &UVoipTalkerComponent::HandleOwnerPawnEndPlay);
	}
	BoundPawn.Reset();
}

void UVoipTalkerComponent::HandleOwnerPawnEndPlay(AActor* /*Actor*/, EEndPlayReason::Type /*EndPlayReason*/)
{
	// 발화자 Pawn 파괴(travel 포함) → 다음 재생이 죽은 컴포넌트에 attach 되지 않도록 부착 대상 해제.
	// (재빙의 시 OnPawnSet → RefreshVoiceSettings 가 새 Pawn 으로 다시 채운다.)
	Settings.ComponentToAttachTo = nullptr;

	TeardownVoiceAudio();
}
