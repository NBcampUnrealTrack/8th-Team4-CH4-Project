// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/GODGameUserSettings.h"
#include "Engine/Engine.h"

UGODGameUserSettings::UGODGameUserSettings()
{
	// 생성자 기본값 — SetToDefaults() 에서도 동일하게 맞춘다.
	MasterVolume = 1.f;
	BGMVolume = 0.8f;
	SFXVolume = 1.f;
	UIVolume = 1.f;
	bMuted = false;
	VoiceOutputVolume = 1.f;
	VoiceInputVolume = 1.f;

	MouseSensitivity = 1.f;
	bInvertYAxis = false;

	Gamma = 2.2f;
	FieldOfView = 90.f;
	bColorblindMode = false;
	ColorblindType = EGODColorblindType::None;
	bSubtitlesEnabled = true;
	SubtitleScale = 1.f;
	LanguageCulture = FString();
}

UGODGameUserSettings* UGODGameUserSettings::Get()
{
	if (GEngine)
	{
		return Cast<UGODGameUserSettings>(GEngine->GetGameUserSettings());
	}
	return nullptr;
}

void UGODGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	ResetAudioToDefaults();
	ResetControlsToDefaults();

	// 영상 추가 필드 (그래픽 프리셋/해상도는 Super 가 처리).
	Gamma = 2.2f;
	FieldOfView = 90.f;
	bColorblindMode = false;
	ColorblindType = EGODColorblindType::None;
	bSubtitlesEnabled = true;
	SubtitleScale = 1.f;
	LanguageCulture = FString();
}

void UGODGameUserSettings::ResetAudioToDefaults()
{
	MasterVolume = 1.f;
	BGMVolume = 0.8f;
	SFXVolume = 1.f;
	UIVolume = 1.f;
	bMuted = false;
	VoiceOutputVolume = 1.f;
	VoiceInputVolume = 1.f;
}

void UGODGameUserSettings::ResetControlsToDefaults()
{
	MouseSensitivity = 1.f;
	bInvertYAxis = false;
}

void UGODGameUserSettings::ResetGraphicsToDefaults()
{
	// 화질 프리셋을 High(2) 로, VSync 켜고, 프레임 무제한. 해상도는 현재 데스크톱 유지.
	SetVSyncEnabled(true);
	SetFrameRateLimit(0.f);
	SetOverallScalabilityLevel(2);

	Gamma = 2.2f;
	FieldOfView = 90.f;
	ApplyGamma();
}

void UGODGameUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();

	// 감마는 엔진 전역이라 여기서 직접 반영. (오디오/언어/FOV 는 값 소스만 여기 두고
	// 적용은 GameInstance / 설정 위젯 / 캐릭터가 담당 — World 컨텍스트가 필요하기 때문.)
	ApplyGamma();
}

void UGODGameUserSettings::ApplyGamma()
{
	if (GEngine)
	{
		// 1.0~4.0 정도 범위. 2.2 가 표준.
		GEngine->DisplayGamma = FMath::Clamp(Gamma, 1.0f, 4.0f);
	}
}
