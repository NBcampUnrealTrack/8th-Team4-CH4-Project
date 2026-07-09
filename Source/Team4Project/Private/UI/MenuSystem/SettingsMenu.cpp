// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MenuSystem/SettingsMenu.h"

#include "Components/Slider.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ScrollBox.h"

#include "Game/GODGameUserSettings.h"
#include "Game/PlayerGameInstance.h"
#include "Sound/GameSoundTypes.h"

#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"

namespace
{
	// 콤보 표시 문자열 (한글).
	const TCHAR* GWindowModeLabels[] = { TEXT("전체화면"), TEXT("테두리 없는 창모드"), TEXT("창모드") };
	// 인덱스 = EWindowMode::Type (0=Fullscreen, 1=WindowedFullscreen, 2=Windowed)

	const TCHAR* GQualityLabels[] = { TEXT("낮음"), TEXT("중간"), TEXT("높음"), TEXT("최고") };
	const TCHAR* GFrameLimitLabels[] = { TEXT("30"), TEXT("60"), TEXT("120"), TEXT("무제한") };
	const float  GFrameLimitValues[] = { 30.f, 60.f, 120.f, 0.f };
	const TCHAR* GColorblindLabels[] = { TEXT("없음"), TEXT("녹색맹"), TEXT("적색맹"), TEXT("청색맹") };

	// 언어: 표시명 → 컬처 코드.
	struct FLangOption { const TCHAR* Label; const TCHAR* Culture; };
	const FLangOption GLanguages[] = {
		{ TEXT("시스템 기본"), TEXT("") },
		{ TEXT("한국어"),      TEXT("ko") },
		{ TEXT("English"),     TEXT("en") },
	};

	FString FrameLimitToLabel(float Limit)
	{
		for (int32 i = 0; i < 4; ++i)
		{
			if (FMath::IsNearlyEqual(GFrameLimitValues[i], Limit))
			{
				return GFrameLimitLabels[i];
			}
		}
		return GFrameLimitLabels[3]; // 무제한
	}
}

UGODGameUserSettings* USettingsMenu::GetSettings() const
{
	return UGODGameUserSettings::Get();
}

UPlayerGameInstance* USettingsMenu::GetGI() const
{
	return GetGameInstance() ? Cast<UPlayerGameInstance>(GetGameInstance()) : nullptr;
}

bool USettingsMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	BindCallbacks();

	// 버튼 클릭/호버 사운드 (null 은 무시됨).
	BindButtonSounds(AudioTabButton);
	BindButtonSounds(GraphicsTabButton);
	BindButtonSounds(ControlsTabButton);
	BindButtonSounds(ApplyButton);
	BindButtonSounds(CancelButton);
	BindButtonSounds(ResetButton);
	BindButtonSounds(ResetAllButton);
	BindButtonSounds(CloseButton);

	return true;
}

void USettingsMenu::BindCallbacks()
{
	// 탭
	if (AudioTabButton)    AudioTabButton->OnClicked.AddDynamic(this, &USettingsMenu::ShowAudioTab);
	if (GraphicsTabButton) GraphicsTabButton->OnClicked.AddDynamic(this, &USettingsMenu::ShowGraphicsTab);
	if (ControlsTabButton) ControlsTabButton->OnClicked.AddDynamic(this, &USettingsMenu::ShowControlsTab);

	// 오디오
	if (MasterVolumeSlider) MasterVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnMasterVolumeChanged);
	if (BGMVolumeSlider)    BGMVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnBGMVolumeChanged);
	if (SFXVolumeSlider)    SFXVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnSFXVolumeChanged);
	if (UIVolumeSlider)     UIVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnUIVolumeChanged);
	if (VoiceOutputSlider)  VoiceOutputSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnVoiceOutputChanged);
	if (VoiceInputSlider)   VoiceInputSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnVoiceInputChanged);
	if (MuteToggle)         MuteToggle->OnCheckStateChanged.AddDynamic(this, &USettingsMenu::OnMuteChanged);

	// 그래픽
	if (ResolutionCombo)    ResolutionCombo->OnSelectionChanged.AddDynamic(this, &USettingsMenu::OnResolutionSelected);
	if (WindowModeCombo)    WindowModeCombo->OnSelectionChanged.AddDynamic(this, &USettingsMenu::OnWindowModeSelected);
	if (QualityPresetCombo) QualityPresetCombo->OnSelectionChanged.AddDynamic(this, &USettingsMenu::OnQualityPresetSelected);
	if (FrameLimitCombo)    FrameLimitCombo->OnSelectionChanged.AddDynamic(this, &USettingsMenu::OnFrameLimitSelected);
	if (LanguageCombo)      LanguageCombo->OnSelectionChanged.AddDynamic(this, &USettingsMenu::OnLanguageSelected);
	if (ColorblindCombo)    ColorblindCombo->OnSelectionChanged.AddDynamic(this, &USettingsMenu::OnColorblindSelected);
	if (VSyncToggle)        VSyncToggle->OnCheckStateChanged.AddDynamic(this, &USettingsMenu::OnVSyncChanged);
	if (SubtitleToggle)     SubtitleToggle->OnCheckStateChanged.AddDynamic(this, &USettingsMenu::OnSubtitleChanged);

	if (ShadowQualitySlider)  ShadowQualitySlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnShadowQualityChanged);
	if (TextureQualitySlider) TextureQualitySlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnTextureQualityChanged);
	if (EffectsQualitySlider) EffectsQualitySlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnEffectsQualityChanged);
	if (ViewDistanceSlider)   ViewDistanceSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnViewDistanceChanged);
	if (AAQualitySlider)      AAQualitySlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnAAQualityChanged);
	if (PostProcessSlider)    PostProcessSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnPostProcessChanged);
	if (FoliageSlider)        FoliageSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnFoliageChanged);
	if (ShadingSlider)        ShadingSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnShadingChanged);
	if (GammaSlider)          GammaSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnGammaChanged);
	if (FOVSlider)            FOVSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnFOVChanged);
	if (SubtitleScaleSlider)  SubtitleScaleSlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnSubtitleScaleChanged);

	// 조작
	if (MouseSensitivitySlider) MouseSensitivitySlider->OnValueChanged.AddDynamic(this, &USettingsMenu::OnMouseSensitivityChanged);
	if (InvertYToggle)          InvertYToggle->OnCheckStateChanged.AddDynamic(this, &USettingsMenu::OnInvertYChanged);

	// 하단 바
	if (ApplyButton)    ApplyButton->OnClicked.AddDynamic(this, &USettingsMenu::OnApplyClicked);
	if (CancelButton)   CancelButton->OnClicked.AddDynamic(this, &USettingsMenu::OnCancelClicked);
	if (ResetButton)    ResetButton->OnClicked.AddDynamic(this, &USettingsMenu::OnResetClicked);
	if (ResetAllButton) ResetAllButton->OnClicked.AddDynamic(this, &USettingsMenu::OnResetAllClicked);
	if (CloseButton)    CloseButton->OnClicked.AddDynamic(this, &USettingsMenu::OnCloseClicked);

	// 미저장 팝업
	if (UnsavedSaveButton)    UnsavedSaveButton->OnClicked.AddDynamic(this, &USettingsMenu::OnUnsavedSave);
	if (UnsavedDiscardButton) UnsavedDiscardButton->OnClicked.AddDynamic(this, &USettingsMenu::OnUnsavedDiscard);
	if (UnsavedCancelButton)  UnsavedCancelButton->OnClicked.AddDynamic(this, &USettingsMenu::OnUnsavedCancel);

	// 해상도 되돌림 팝업
	if (RevertKeepButton) RevertKeepButton->OnClicked.AddDynamic(this, &USettingsMenu::OnRevertKeep);
	if (RevertBackButton) RevertBackButton->OnClicked.AddDynamic(this, &USettingsMenu::OnRevertBack);
}

void USettingsMenu::NativeConstruct()
{
	Super::NativeConstruct();
	OpenSettings();
}

void USettingsMenu::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RevertTimerHandle);
	}
	Super::NativeDestruct();
}

void USettingsMenu::OpenSettings()
{
	// [진단] Get() 이 null 이면 설정 시스템 전체가 무력화된다 (슬라이더 0, 적용/초기화 무반응).
	// 대개 DefaultEngine.ini 의 GameUserSettingsClass 가 아직 로드 안 된 상태 → 에디터 완전 재시작 필요.
	if (!GetSettings())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Settings] UGODGameUserSettings::Get() == null! DefaultEngine.ini 의 GameUserSettingsClass 가 "
			     "로드되지 않았습니다. 에디터를 완전히 종료 후 재시작하세요."));
	}

	SetIsFocusable(true);
	PopulateStaticCombos();
	PopulateResolutionCombo();
	PopulateFromSettings();
	RefreshKeybindList();

	if (UnsavedChangesPopup)    UnsavedChangesPopup->SetVisibility(ESlateVisibility::Collapsed);
	if (ResolutionRevertPopup)  ResolutionRevertPopup->SetVisibility(ESlateVisibility::Collapsed);
	if (DuplicateKeyWarningText) DuplicateKeyWarningText->SetVisibility(ESlateVisibility::Collapsed);

	// 되돌림 기준 = 현재 적용된 해상도/모드 (메뉴 열 때 캐시).
	if (UGODGameUserSettings* S = GetSettings())
	{
		PrevResolution = S->GetScreenResolution();
		PrevWindowMode = (int32)S->GetFullscreenMode();
	}

	bDirty = false;
	SetTab(ESettingsTab::Audio);
	PlaySlideIn();
	SetKeyboardFocus();
}

// ============================================================
// 콤보 채우기
// ============================================================
void USettingsMenu::PopulateStaticCombos()
{
	if (WindowModeCombo)
	{
		WindowModeCombo->ClearOptions();
		for (const TCHAR* Label : GWindowModeLabels) WindowModeCombo->AddOption(Label);
	}
	if (QualityPresetCombo)
	{
		QualityPresetCombo->ClearOptions();
		for (const TCHAR* Label : GQualityLabels) QualityPresetCombo->AddOption(Label);
		QualityPresetCombo->AddOption(TEXT("커스텀"));
	}
	if (FrameLimitCombo)
	{
		FrameLimitCombo->ClearOptions();
		for (const TCHAR* Label : GFrameLimitLabels) FrameLimitCombo->AddOption(Label);
	}
	if (LanguageCombo)
	{
		LanguageCombo->ClearOptions();
		for (const FLangOption& Opt : GLanguages) LanguageCombo->AddOption(Opt.Label);
	}
	if (ColorblindCombo)
	{
		ColorblindCombo->ClearOptions();
		for (const TCHAR* Label : GColorblindLabels) ColorblindCombo->AddOption(Label);
	}
}

void USettingsMenu::PopulateResolutionCombo()
{
	if (!ResolutionCombo)
	{
		return;
	}
	ResolutionCombo->ClearOptions();

	TArray<FIntPoint> Resolutions;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	for (const FIntPoint& Res : Resolutions)
	{
		ResolutionCombo->AddOption(FString::Printf(TEXT("%d x %d"), Res.X, Res.Y));
	}
}

// ============================================================
// 저장값 → 위젯
// ============================================================
void USettingsMenu::SetPercentLabel(UTextBlock* Label, float Value01) const
{
	if (Label)
	{
		Label->SetText(FText::AsNumber(FMath::RoundToInt(Value01 * 100.f)));
	}
}

void USettingsMenu::PopulateFromSettings()
{
	UGODGameUserSettings* S = GetSettings();
	if (!S)
	{
		return;
	}

	// 오디오
	if (MasterVolumeSlider) MasterVolumeSlider->SetValue(S->MasterVolume);
	if (BGMVolumeSlider)    BGMVolumeSlider->SetValue(S->BGMVolume);
	if (SFXVolumeSlider)    SFXVolumeSlider->SetValue(S->SFXVolume);
	if (UIVolumeSlider)     UIVolumeSlider->SetValue(S->UIVolume);
	if (VoiceOutputSlider)  VoiceOutputSlider->SetValue(S->VoiceOutputVolume);
	if (VoiceInputSlider)   VoiceInputSlider->SetValue(S->VoiceInputVolume);
	if (MuteToggle)         MuteToggle->SetIsChecked(S->bMuted);
	SetPercentLabel(MasterVolumeValue, S->MasterVolume);
	SetPercentLabel(BGMVolumeValue, S->BGMVolume);
	SetPercentLabel(SFXVolumeValue, S->SFXVolume);
	SetPercentLabel(UIVolumeValue, S->UIVolume);
	SetPercentLabel(VoiceOutputValue, S->VoiceOutputVolume);
	SetPercentLabel(VoiceInputValue, S->VoiceInputVolume);

	// 그래픽 — 해상도
	if (ResolutionCombo)
	{
		const FIntPoint Cur = S->GetScreenResolution();
		ResolutionCombo->SetSelectedOption(FString::Printf(TEXT("%d x %d"), Cur.X, Cur.Y));
	}
	if (WindowModeCombo)
	{
		const int32 Mode = (int32)S->GetFullscreenMode();
		if (GWindowModeLabels[FMath::Clamp(Mode, 0, 2)])
		{
			WindowModeCombo->SetSelectedIndex(FMath::Clamp(Mode, 0, 2));
		}
	}
	if (QualityPresetCombo)
	{
		const int32 Level = S->GetOverallScalabilityLevel(); // -1=커스텀
		QualityPresetCombo->SetSelectedIndex(Level < 0 ? 4 : FMath::Clamp(Level, 0, 3));
	}
	if (FrameLimitCombo)
	{
		FrameLimitCombo->SetSelectedOption(FrameLimitToLabel(S->GetFrameRateLimit()));
	}
	if (VSyncToggle) VSyncToggle->SetIsChecked(S->IsVSyncEnabled());

	// 세부 화질 (0~3)
	if (ShadowQualitySlider)  ShadowQualitySlider->SetValue(S->GetShadowQuality());
	if (TextureQualitySlider) TextureQualitySlider->SetValue(S->GetTextureQuality());
	if (EffectsQualitySlider) EffectsQualitySlider->SetValue(S->GetVisualEffectQuality());
	if (ViewDistanceSlider)   ViewDistanceSlider->SetValue(S->GetViewDistanceQuality());
	if (AAQualitySlider)      AAQualitySlider->SetValue(S->GetAntiAliasingQuality());
	if (PostProcessSlider)    PostProcessSlider->SetValue(S->GetPostProcessingQuality());
	if (FoliageSlider)        FoliageSlider->SetValue(S->GetFoliageQuality());
	if (ShadingSlider)        ShadingSlider->SetValue(S->GetShadingQuality());

	// 영상 추가
	if (GammaSlider) GammaSlider->SetValue(S->Gamma);
	if (FOVSlider)   FOVSlider->SetValue(S->FieldOfView);
	if (GammaValue)  GammaValue->SetText(FText::AsNumber(FMath::RoundToInt(S->Gamma * 100.f) / 100.f));
	if (FOVValue)    FOVValue->SetText(FText::AsNumber(FMath::RoundToInt(S->FieldOfView)));
	if (ColorblindCombo) ColorblindCombo->SetSelectedIndex((int32)S->ColorblindType);
	if (SubtitleToggle)  SubtitleToggle->SetIsChecked(S->bSubtitlesEnabled);
	if (SubtitleScaleSlider) SubtitleScaleSlider->SetValue(S->SubtitleScale);
	if (LanguageCombo)
	{
		int32 LangIdx = 0;
		for (int32 i = 0; i < UE_ARRAY_COUNT(GLanguages); ++i)
		{
			if (S->LanguageCulture.Equals(GLanguages[i].Culture))
			{
				LangIdx = i;
				break;
			}
		}
		LanguageCombo->SetSelectedIndex(LangIdx);
	}

	// 조작
	if (MouseSensitivitySlider) MouseSensitivitySlider->SetValue(S->MouseSensitivity);
	if (MouseSensitivityValue)  MouseSensitivityValue->SetText(FText::AsNumber(FMath::RoundToInt(S->MouseSensitivity * 100.f) / 100.f));
	if (InvertYToggle)          InvertYToggle->SetIsChecked(S->bInvertYAxis);
}

// ============================================================
// 오디오 콜백 — 실시간 적용(GI) + 값 라벨 + Dirty
// ============================================================
void USettingsMenu::OnMasterVolumeChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->MasterVolume = V; }
	SetPercentLabel(MasterVolumeValue, V);
	if (UPlayerGameInstance* GI = GetGI()) { GI->ApplyAudioSettings(); }
	bDirty = true;
}

void USettingsMenu::OnBGMVolumeChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->BGMVolume = V; }
	SetPercentLabel(BGMVolumeValue, V);
	if (UPlayerGameInstance* GI = GetGI()) { GI->ApplyAudioSettings(); }
	bDirty = true;
}

void USettingsMenu::OnSFXVolumeChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->SFXVolume = V; }
	SetPercentLabel(SFXVolumeValue, V);
	if (UPlayerGameInstance* GI = GetGI()) { GI->ApplyAudioSettings(); }
	bDirty = true;
}

void USettingsMenu::OnUIVolumeChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->UIVolume = V; }
	SetPercentLabel(UIVolumeValue, V);
	if (UPlayerGameInstance* GI = GetGI()) { GI->ApplyAudioSettings(); }
	// UI 볼륨 미리듣기: 클릭음 재생.
	PlayUISound(SoundRows::UIClick);
	bDirty = true;
}

void USettingsMenu::OnVoiceOutputChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->VoiceOutputVolume = V; }
	SetPercentLabel(VoiceOutputValue, V);
	bDirty = true;
}

void USettingsMenu::OnVoiceInputChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->VoiceInputVolume = V; }
	SetPercentLabel(VoiceInputValue, V);
	bDirty = true;
}

void USettingsMenu::OnMuteChanged(bool bChecked)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->bMuted = bChecked; }
	if (UPlayerGameInstance* GI = GetGI()) { GI->ApplyAudioSettings(); }
	bDirty = true;
}

// ============================================================
// 그래픽 콜백
// ============================================================
void USettingsMenu::OnResolutionSelected(FString Item, ESelectInfo::Type Type)
{
	if (Type == ESelectInfo::Direct) return; // 코드로 세팅한 초기화는 무시
	UGODGameUserSettings* S = GetSettings();
	if (!S) return;

	int32 W = 0, H = 0;
	// "1920 x 1080" 파싱
	TArray<FString> Parts;
	Item.ParseIntoArray(Parts, TEXT("x"), true);
	if (Parts.Num() == 2)
	{
		W = FCString::Atoi(*Parts[0].TrimStartAndEnd());
		H = FCString::Atoi(*Parts[1].TrimStartAndEnd());
		if (W > 0 && H > 0)
		{
			S->SetScreenResolution(FIntPoint(W, H));
			bDirty = true;
		}
	}
}

void USettingsMenu::OnWindowModeSelected(FString Item, ESelectInfo::Type Type)
{
	if (Type == ESelectInfo::Direct) return;
	if (UGODGameUserSettings* S = GetSettings())
	{
		const int32 Idx = WindowModeCombo ? WindowModeCombo->GetSelectedIndex() : 0;
		S->SetFullscreenMode((EWindowMode::Type)FMath::Clamp(Idx, 0, 2));
		bDirty = true;
	}
}

void USettingsMenu::OnQualityPresetSelected(FString Item, ESelectInfo::Type Type)
{
	if (Type == ESelectInfo::Direct) return;
	UGODGameUserSettings* S = GetSettings();
	if (!S || !QualityPresetCombo) return;

	const int32 Idx = QualityPresetCombo->GetSelectedIndex();
	if (Idx >= 0 && Idx <= 3)
	{
		S->SetOverallScalabilityLevel(Idx);
		S->ApplyNonResolutionSettings();
		PopulateFromSettings(); // 세부 슬라이더도 프리셋값으로 갱신
		bDirty = true;
	}
	// "커스텀"(Idx==4) 선택은 아무것도 안 함 — 세부 슬라이더로 조절.
}

void USettingsMenu::OnFrameLimitSelected(FString Item, ESelectInfo::Type Type)
{
	if (Type == ESelectInfo::Direct) return;
	UGODGameUserSettings* S = GetSettings();
	if (!S || !FrameLimitCombo) return;

	const int32 Idx = FrameLimitCombo->GetSelectedIndex();
	if (Idx >= 0 && Idx < 4)
	{
		S->SetFrameRateLimit(GFrameLimitValues[Idx]);
		S->ApplyNonResolutionSettings();
		bDirty = true;
	}
}

void USettingsMenu::OnLanguageSelected(FString Item, ESelectInfo::Type Type)
{
	if (Type == ESelectInfo::Direct) return;
	UGODGameUserSettings* S = GetSettings();
	if (!S || !LanguageCombo) return;

	const int32 Idx = LanguageCombo->GetSelectedIndex();
	if (Idx >= 0 && Idx < UE_ARRAY_COUNT(GLanguages))
	{
		S->LanguageCulture = GLanguages[Idx].Culture;
		if (UPlayerGameInstance* GI = GetGI()) { GI->ApplyLanguage(); }
		bDirty = true;
	}
}

void USettingsMenu::OnColorblindSelected(FString Item, ESelectInfo::Type Type)
{
	if (Type == ESelectInfo::Direct) return;
	UGODGameUserSettings* S = GetSettings();
	if (!S || !ColorblindCombo) return;

	const int32 Idx = ColorblindCombo->GetSelectedIndex();
	S->ColorblindType = (EGODColorblindType)FMath::Clamp(Idx, 0, 3);
	S->bColorblindMode = (Idx > 0);
	bDirty = true;
}

void USettingsMenu::OnVSyncChanged(bool bChecked)
{
	if (UGODGameUserSettings* S = GetSettings())
	{
		S->SetVSyncEnabled(bChecked);
		S->ApplyNonResolutionSettings();
		bDirty = true;
	}
}

void USettingsMenu::OnSubtitleChanged(bool bChecked)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->bSubtitlesEnabled = bChecked; bDirty = true; }
}

void USettingsMenu::SyncPresetComboToCustom()
{
	if (QualityPresetCombo)
	{
		QualityPresetCombo->SetSelectedIndex(4); // 커스텀
	}
}

void USettingsMenu::OnShadowQualityChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->SetShadowQuality(FMath::RoundToInt(V)); S->ApplyNonResolutionSettings(); }
	SyncPresetComboToCustom();
	bDirty = true;
}

void USettingsMenu::OnTextureQualityChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->SetTextureQuality(FMath::RoundToInt(V)); S->ApplyNonResolutionSettings(); }
	SyncPresetComboToCustom();
	bDirty = true;
}

void USettingsMenu::OnEffectsQualityChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->SetVisualEffectQuality(FMath::RoundToInt(V)); S->ApplyNonResolutionSettings(); }
	SyncPresetComboToCustom();
	bDirty = true;
}

void USettingsMenu::OnViewDistanceChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->SetViewDistanceQuality(FMath::RoundToInt(V)); S->ApplyNonResolutionSettings(); }
	SyncPresetComboToCustom();
	bDirty = true;
}

void USettingsMenu::OnAAQualityChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->SetAntiAliasingQuality(FMath::RoundToInt(V)); S->ApplyNonResolutionSettings(); }
	SyncPresetComboToCustom();
	bDirty = true;
}

void USettingsMenu::OnPostProcessChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->SetPostProcessingQuality(FMath::RoundToInt(V)); S->ApplyNonResolutionSettings(); }
	SyncPresetComboToCustom();
	bDirty = true;
}

void USettingsMenu::OnFoliageChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->SetFoliageQuality(FMath::RoundToInt(V)); S->ApplyNonResolutionSettings(); }
	SyncPresetComboToCustom();
	bDirty = true;
}

void USettingsMenu::OnShadingChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->SetShadingQuality(FMath::RoundToInt(V)); S->ApplyNonResolutionSettings(); }
	SyncPresetComboToCustom();
	bDirty = true;
}

void USettingsMenu::OnGammaChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->Gamma = V; S->ApplyGamma(); }
	if (GammaValue) GammaValue->SetText(FText::AsNumber(FMath::RoundToInt(V * 100.f) / 100.f));
	bDirty = true;
}

void USettingsMenu::OnFOVChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->FieldOfView = V; }
	if (FOVValue) FOVValue->SetText(FText::AsNumber(FMath::RoundToInt(V)));
	// 인게임이면 즉시 미리보기.
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (PC->PlayerCameraManager) { PC->PlayerCameraManager->SetFOV(V); }
	}
	bDirty = true;
}

void USettingsMenu::OnSubtitleScaleChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->SubtitleScale = V; bDirty = true; }
}

// ============================================================
// 조작 콜백
// ============================================================
void USettingsMenu::OnMouseSensitivityChanged(float V)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->MouseSensitivity = V; }
	if (MouseSensitivityValue) MouseSensitivityValue->SetText(FText::AsNumber(FMath::RoundToInt(V * 100.f) / 100.f));
	bDirty = true;
}

void USettingsMenu::OnInvertYChanged(bool bChecked)
{
	if (UGODGameUserSettings* S = GetSettings()) { S->bInvertYAxis = bChecked; bDirty = true; }
}

// ============================================================
// 탭
// ============================================================
void USettingsMenu::ShowAudioTab()    { SetTab(ESettingsTab::Audio); }
void USettingsMenu::ShowGraphicsTab() { SetTab(ESettingsTab::Graphics); }
void USettingsMenu::ShowControlsTab() { SetTab(ESettingsTab::Controls); }

void USettingsMenu::SetTab(ESettingsTab Tab)
{
	CurrentTab = Tab;
	if (SettingsSwitcher)
	{
		SettingsSwitcher->SetActiveWidgetIndex((int32)Tab);
	}
}

// ============================================================
// 하단 버튼: 적용 / 취소 / 초기화 / 닫기
// ============================================================
void USettingsMenu::ApplyAll()
{
	UGODGameUserSettings* S = GetSettings();
	if (!S) return;

	// 해상도/화면모드 변경 여부 판단 → 변경됐으면 되돌림 카운트다운.
	const FIntPoint NewRes = S->GetScreenResolution();
	const int32 NewMode = (int32)S->GetFullscreenMode();
	// PrevResolution/PrevWindowMode 는 메뉴 열 때 또는 직전 적용 시점의 값 (되돌림 기준).
	const bool bResolutionChanged = (NewRes != PrevResolution || NewMode != PrevWindowMode);

	S->ApplyResolutionSettings(false);
	S->ApplyNonResolutionSettings();
	S->SaveSettings();
	bDirty = false;

	// 오디오도 확정 반영.
	if (UPlayerGameInstance* GI = GetGI()) { GI->ApplyAudioSettings(); }

	if (bResolutionChanged)
	{
		// 되돌림 카운트다운 — 확정("유지")/시간초과("되돌림")에서 PrevResolution 갱신.
		StartResolutionRevertCountdown();
	}
	else
	{
		// 해상도 외 변경만 저장 — 기준값을 현재로 최신화.
		PrevResolution = NewRes;
		PrevWindowMode = NewMode;
	}
}

void USettingsMenu::OnApplyClicked()
{
	ApplyAll();
}

void USettingsMenu::OnCancelClicked()
{
	if (bDirty)
	{
		ShowUnsavedPopup(true);
	}
	else
	{
		CloseInternal();
	}
}

void USettingsMenu::OnResetClicked()
{
	UGODGameUserSettings* S = GetSettings();
	if (!S) return;

	switch (CurrentTab)
	{
	case ESettingsTab::Audio:    S->ResetAudioToDefaults(); break;
	case ESettingsTab::Graphics: S->ResetGraphicsToDefaults(); break;
	case ESettingsTab::Controls: S->ResetControlsToDefaults(); break;
	}
	S->ApplyNonResolutionSettings();
	if (UPlayerGameInstance* GI = GetGI()) { GI->ApplyAudioSettings(); }
	PopulateFromSettings();
	bDirty = true;
}

void USettingsMenu::OnResetAllClicked()
{
	UGODGameUserSettings* S = GetSettings();
	if (!S) return;

	S->SetToDefaults();
	S->ApplyNonResolutionSettings();
	if (UPlayerGameInstance* GI = GetGI()) { GI->ApplyAudioSettings(); }
	PopulateFromSettings();
	bDirty = true;
}

void USettingsMenu::OnCloseClicked()
{
	OnCancelClicked();
}

// ============================================================
// 미저장 팝업
// ============================================================
void USettingsMenu::ShowUnsavedPopup(bool bShow)
{
	if (UnsavedChangesPopup)
	{
		UnsavedChangesPopup->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	else if (bShow)
	{
		// 팝업 위젯이 없으면 그냥 스냅샷 복원 후 닫기 (안전 폴백).
		RevertToSnapshot();
		CloseInternal();
	}
}

void USettingsMenu::OnUnsavedSave()
{
	ApplyAll();
	ShowUnsavedPopup(false);
	CloseInternal();
}

void USettingsMenu::OnUnsavedDiscard()
{
	RevertToSnapshot();
	ShowUnsavedPopup(false);
	CloseInternal();
}

void USettingsMenu::OnUnsavedCancel()
{
	ShowUnsavedPopup(false); // 계속 편집
}

void USettingsMenu::RevertToSnapshot()
{
	UGODGameUserSettings* S = GetSettings();
	if (!S) return;

	// 디스크의 마지막 저장값으로 되돌리고 실시간 적용분(오디오/감마/화질)을 원복.
	S->LoadSettings();
	S->ApplyNonResolutionSettings();
	S->ApplyGamma();
	if (UPlayerGameInstance* GI = GetGI()) { GI->ApplyAudioSettings(); }
	PopulateFromSettings();
	bDirty = false;
}

void USettingsMenu::CloseInternal()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RevertTimerHandle);
	}
	PlaySlideOut();
	RemoveFromParent();
}

// ============================================================
// 해상도 되돌림 (15초 카운트다운)
// ============================================================
void USettingsMenu::StartResolutionRevertCountdown()
{
	RevertSecondsLeft = 15;
	if (ResolutionRevertPopup)
	{
		ResolutionRevertPopup->SetVisibility(ESlateVisibility::Visible);
	}
	UpdateRevertCountdown(RevertSecondsLeft);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RevertTimerHandle, this, &USettingsMenu::TickRevertCountdown, 1.f, true);
	}
}

void USettingsMenu::TickRevertCountdown()
{
	--RevertSecondsLeft;
	UpdateRevertCountdown(RevertSecondsLeft);
	if (RevertSecondsLeft <= 0)
	{
		FinishRevert(false); // 시간 초과 → 이전 해상도로 되돌림
	}
}

void USettingsMenu::OnRevertKeep() { FinishRevert(true); }
void USettingsMenu::OnRevertBack() { FinishRevert(false); }

void USettingsMenu::FinishRevert(bool bKeep)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RevertTimerHandle);
	}

	UGODGameUserSettings* S = GetSettings();
	if (!bKeep && S && PrevResolution != FIntPoint(0, 0))
	{
		// 이전 해상도/모드로 되돌림.
		S->SetScreenResolution(PrevResolution);
		S->SetFullscreenMode((EWindowMode::Type)PrevWindowMode);
		S->ApplyResolutionSettings(false);
		S->SaveSettings();
		PopulateFromSettings();
	}
	else if (bKeep && S)
	{
		PrevResolution = S->GetScreenResolution();
		PrevWindowMode = (int32)S->GetFullscreenMode();
	}

	if (ResolutionRevertPopup)
	{
		ResolutionRevertPopup->SetVisibility(ESlateVisibility::Collapsed);
	}
	UpdateRevertCountdown(0);
}

// ============================================================
// 키 리바인딩 (Enhanced Input User Settings)
// ============================================================
namespace
{
	UEnhancedInputUserSettings* GetInputUserSettings(ULocalPlayer* LP)
	{
		if (!LP) return nullptr;
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			return Sub->GetUserSettings();
		}
		return nullptr;
	}
}

TArray<FKeybindRowData> USettingsMenu::GetKeybindRows() const
{
	TArray<FKeybindRowData> Rows;
	UEnhancedInputUserSettings* IUS = GetInputUserSettings(GetOwningLocalPlayer());
	if (!IUS)
	{
		// User Settings 미활성 — Project Settings → Enhanced Input 에서 "Enable User Settings" 필요.
		return Rows;
	}

	UEnhancedPlayerMappableKeyProfile* Profile = IUS->GetCurrentKeyProfile();
	if (!Profile)
	{
		return Rows;
	}

	for (const TPair<FName, FKeyMappingRow>& Pair : Profile->GetPlayerMappingRows())
	{
		for (const FPlayerKeyMapping& Mapping : Pair.Value.Mappings)
		{
			FKeybindRowData Data;
			Data.MappingName = Mapping.GetMappingName();
			Data.DisplayName = Mapping.GetDisplayName();
			Data.KeyName = Mapping.GetCurrentKey().GetDisplayName();
			Rows.Add(Data);
			break; // 슬롯 1개만 표시
		}
	}
	return Rows;
}

void USettingsMenu::BeginRebind(FName MappingName)
{
	bAwaitingKey = true;
	PendingMappingName = MappingName;
	if (DuplicateKeyWarningText)
	{
		DuplicateKeyWarningText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 버튼 클릭 직후엔 Slate 가 포커스를 그 버튼으로 되돌리므로, 같은 프레임에 포커스를 잡으면
	// 곧바로 뺏긴다. 한 프레임 뒤에 잡아야 NativeOnKeyDown 이 다음 키 입력을 받는다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick([this]()
		{
			SetKeyboardFocus();
		});
	}
	else
	{
		SetKeyboardFocus();
	}
}

void USettingsMenu::CancelRebind()
{
	bAwaitingKey = false;
	PendingMappingName = NAME_None;
}

bool USettingsMenu::TryApplyRebind(FName MappingName, const FKey& NewKey)
{
	UEnhancedInputUserSettings* IUS = GetInputUserSettings(GetOwningLocalPlayer());
	if (!IUS)
	{
		return false;
	}

	// 중복 감지: 다른 매핑이 이미 이 키를 쓰면 경고 후 차단.
	if (UEnhancedPlayerMappableKeyProfile* Profile = IUS->GetCurrentKeyProfile())
	{
		for (const TPair<FName, FKeyMappingRow>& Pair : Profile->GetPlayerMappingRows())
		{
			for (const FPlayerKeyMapping& Mapping : Pair.Value.Mappings)
			{
				if (Mapping.GetMappingName() != MappingName && Mapping.GetCurrentKey() == NewKey)
				{
					if (DuplicateKeyWarningText)
					{
						DuplicateKeyWarningText->SetText(FText::Format(
							NSLOCTEXT("Settings", "DupKey", "'{0}' 키는 이미 '{1}'에 할당되어 있습니다."),
							NewKey.GetDisplayName(), Mapping.GetDisplayName()));
						DuplicateKeyWarningText->SetVisibility(ESlateVisibility::Visible);
					}
					return false;
				}
			}
		}
	}

	FMapPlayerKeyArgs Args;
	Args.MappingName = MappingName;
	Args.Slot = EPlayerMappableKeySlot::First;
	Args.NewKey = NewKey;
	Args.bCreateMatchingSlotIfNeeded = true;

	FGameplayTagContainer FailureReason;
	IUS->MapPlayerKey(Args, FailureReason);
	IUS->ApplySettings();
	IUS->SaveSettings();

	RefreshKeybindList();
	return true;
}

// ============================================================
// 입력 캡처 (키 재설정 대기 중) + ESC
// ============================================================
FReply USettingsMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (bAwaitingKey)
	{
		if (Key == EKeys::Escape)
		{
			CancelRebind(); // ESC 로 재설정 취소
			return FReply::Handled();
		}
		const FName Mapping = PendingMappingName;
		CancelRebind();
		TryApplyRebind(Mapping, Key);
		return FReply::Handled();
	}

	if (Key == EKeys::Escape)
	{
		// 되돌림 팝업이 떠 있으면 무시.
		if (ResolutionRevertPopup && ResolutionRevertPopup->IsVisible())
		{
			return FReply::Handled();
		}
		OnCancelClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply USettingsMenu::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bAwaitingKey)
	{
		const FKey Key = InMouseEvent.GetEffectingButton();
		const FName Mapping = PendingMappingName;
		CancelRebind();
		TryApplyRebind(Mapping, Key);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
