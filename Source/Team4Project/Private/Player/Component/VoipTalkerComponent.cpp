// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Component/VoipTalkerComponent.h"
#include "Player/GODPlayerState.h"
#include "Player/BaseCharacter.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundAttenuation.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"

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

void UVoipTalkerComponent::OnTalkingBegin(UAudioComponent* AudioComponent)
{
	if (!IsValid(AudioComponent))
	{
		Super::OnTalkingBegin(AudioComponent);
		return;
	}
	
	PlayingAudioComponent = AudioComponent;

	APlayerState* OwnerPS = GetOwner<APlayerState>();
	APawn* OwnerPawn = OwnerPS ? OwnerPS->GetPawn() : nullptr;
	USceneComponent* PawnRoot = OwnerPawn ? OwnerPawn->GetRootComponent() : nullptr;

	if (IsSpeakerDead())
	{
		// 사망자 보이스: 죽은 사람끼리만 들린다.
		// 청자가 살아있으면 음소거(0), 청자도 죽었으면 정상 청취(1, 이전 세션에서 줄었을 수 있어 복구).
		// ※ 단방향 격리(산 사람→안 들림 / 죽은 사람→들림)는 엔진 양방향 Mute 로 표현 불가해
		//   청자 머신에서 재생 여부를 직접 결정한다.
		AudioComponent->SetVolumeMultiplier(IsLocalListenerDead() ? 1.f : 0.f);

		// 2D 처리: 공간화/감쇠 없이, 죽은 캐릭터(래그돌)를 따라다니지 않게 떼어낸다.
		Settings.AttenuationSettings = nullptr;
		Settings.ComponentToAttachTo = nullptr;

		AudioComponent->bAllowSpatialization = false;
		AudioComponent->SetAttenuationSettings(nullptr);
		AudioComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
	else
	{
		// 생존자 보이스: 모두에게 들림(죽은 청자 포함). 3D 공간화 감쇠(근접 보이스).
		// 이전에 사망자 음소거(0)로 쓰였던 컴포넌트가 재사용될 수 있어 볼륨 복구.
		AudioComponent->SetVolumeMultiplier(1.f);

		// Settings 는 엔진 ApplyVoiceSettings 가 "다음" 재생에 적용하고,
		// SetAttenuationSettings 는 "현재" 재생 중인 ActiveSound 에 즉시 반영한다(둘 다 해 둠).
		if (USoundAttenuation* Attenuation = ResolveAttenuation())
		{
			Settings.AttenuationSettings = Attenuation;
			AudioComponent->bAllowSpatialization = true;
			AudioComponent->SetAttenuationSettings(Attenuation);
		}

		// 재생 컴포넌트를 발화자 Pawn 에 부착 → 감쇠가 카메라가 아닌 캐릭터 위치 기준으로 계산되게.
		// 엔진 ApplyVoiceSettings 는 OnTalkingBegin 보다 먼저 실행돼 World origin 에서 재생 중이므로,
		// 여기서 직접 Attach 해 현재 재생의 위치를 즉시 바로잡는다(Settings 는 다음 재생용).
		if (PawnRoot)
		{
			Settings.ComponentToAttachTo = PawnRoot;
			AudioComponent->AttachToComponent(
				PawnRoot,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}

	// 사망/생존과 무관하게 발화자 Pawn 이 파괴(seamless travel 포함)되면 즉시 teardown 하도록 구독.
	// TALKING 인 채로 travel 이 일어나면 OnTalkingEnd 가 파괴 "후"에야 불려 늦기 때문.
	if (OwnerPawn)
	{
		BindOwnerPawnEndPlay(OwnerPawn);
	}

	Super::OnTalkingBegin(AudioComponent);
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

	// 다음 재생이 죽은 컴포넌트에 attach 되지 않도록 Settings 도 비운다(빙의 후 OnTalkingBegin 이 다시 채움).
	Settings.ComponentToAttachTo = nullptr;

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
	TeardownVoiceAudio();
}

USoundAttenuation* UVoipTalkerComponent::ResolveAttenuation()
{
	if (IsValid(VoiceAttenuationOverride))
	{
		return VoiceAttenuationOverride;
	}

	if (!IsValid(RuntimeAttenuation))
	{
		RuntimeAttenuation = NewObject<USoundAttenuation>(this);
		FSoundAttenuationSettings& S = RuntimeAttenuation->Attenuation;
		S.bAttenuate = true;
		S.bSpatialize = true;
		S.AttenuationShape = EAttenuationShape::Sphere;
		S.AttenuationShapeExtents = FVector(InnerRadius, 0.f, 0.f);
		S.FalloffDistance = FalloffDistance;
		S.bEnableOcclusion = true;
	}

	return RuntimeAttenuation;
}
