// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "GODGameUserSettings.generated.h"

// 색약 보정 타입 (색약 모드 켰을 때 적용할 색맹 유형).
UENUM(BlueprintType)
enum class EGODColorblindType : uint8
{
	None        UMETA(DisplayName = "None"),
	Deuteranope UMETA(DisplayName = "Deuteranope"),  // 녹색맹
	Protanope   UMETA(DisplayName = "Protanope"),    // 적색맹
	Tritanope   UMETA(DisplayName = "Tritanope")     // 청색맹
};

UCLASS()
class TEAM4PROJECT_API UGODGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UGODGameUserSettings();

	// GEngine 에서 이 타입으로 캐스팅해 가져오는 헬퍼 (없으면 nullptr).
	UFUNCTION(BlueprintCallable, Category = "Settings")
	static UGODGameUserSettings* Get();

	// ============================================================
	// 오디오 (0~1). 적용은 UPlayerGameInstance::ApplyAudioSettings().
	// ============================================================
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	float MasterVolume;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	float BGMVolume;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	float SFXVolume;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	float UIVolume;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	bool bMuted;

	// 보이스챗 수신(스피커)/송신(마이크) 볼륨 (0~1).
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	float VoiceOutputVolume;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	float VoiceInputVolume;

	// ============================================================
	// 조작
	// ============================================================
	// 마우스 감도 배율 (기본 1.0). ABaseCharacter::Look() 에서 곱해진다.
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Controls")
	float MouseSensitivity;

	// 마우스 Y축 반전.
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Controls")
	bool bInvertYAxis;

	// ============================================================
	// 영상 추가 (밝기/FOV/색약/자막/언어)
	// ============================================================
	// 화면 밝기(감마). UGameUserSettings 에 이미 SetFrameRateLimit 등은 있으나 감마는 없어 직접 보관.
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Video")
	float Gamma;

	// 시야각(도). 1인칭 카메라에 적용.
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Video")
	float FieldOfView;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Video")
	bool bColorblindMode;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Video")
	EGODColorblindType ColorblindType;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Video")
	bool bSubtitlesEnabled;

	// 자막 크기 배율.
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Video")
	float SubtitleScale;

	// 언어(컬처 코드, 예: "ko", "en"). 빈 문자열이면 시스템 기본.
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Video")
	FString LanguageCulture;

	// ============================================================
	// 적용/기본값
	// ============================================================
	// 커스텀 필드까지 포함해 전부 기본값으로. (베이스 그래픽 기본값도 리셋)
	virtual void SetToDefaults() override;

	// 비-해상도 설정 적용 시 감마/FOV/오디오/언어 훅도 함께 태운다.
	virtual void ApplyNonResolutionSettings() override;

	// 카테고리별 초기화 — 해당 그룹 필드만 기본값으로 되돌린다 (다른 값은 유지).
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ResetAudioToDefaults();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ResetGraphicsToDefaults();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ResetControlsToDefaults();

	// 감마를 실제 엔진에 반영 (GEngine->DisplayGamma).
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void ApplyGamma();
};
