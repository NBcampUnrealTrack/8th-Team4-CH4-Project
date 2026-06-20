// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Component/VoipTalkerComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundAttenuation.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

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

	// 공간화 감쇠. Settings 는 엔진 ApplyVoiceSettings 가 "다음" 재생에 적용하고,
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

		// 캐릭터가 파괴(seamless travel 포함)되는 즉시 teardown 하도록 구독.
		// TALKING 인 채로 travel 이 일어나면 OnTalkingEnd 가 파괴 "후"에야 불려 늦기 때문.
		BindOwnerPawnEndPlay(OwnerPawn);
	}

	Super::OnTalkingBegin(AudioComponent);
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
