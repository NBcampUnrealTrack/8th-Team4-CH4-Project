// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MenuSystem/MenuWidget.h"
#include "SettingsMenu.generated.h"

class USlider;
class UComboBoxString;
class UCheckBox;
class UButton;
class UTextBlock;
class UWidgetSwitcher;
class UScrollBox;
class UGODGameUserSettings;
class UPlayerGameInstance;

// 설정 카테고리 탭.
UENUM(BlueprintType)
enum class ESettingsTab : uint8
{
	Audio    UMETA(DisplayName = "Audio"),
	Graphics UMETA(DisplayName = "Graphics"),
	Controls UMETA(DisplayName = "Controls")
};

// 조작 탭의 키바인딩 한 행 (WBP 가 목록을 그릴 때 사용).
USTRUCT(BlueprintType)
struct FKeybindRowData
{
	GENERATED_BODY()

	// Enhanced Input PlayerMappableKeySettings 의 Name (리바인딩 식별자).
	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	FName MappingName;

	// 표시용 이름
	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	FText DisplayName;

	// 현재 키 표시 문자열
	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	FText KeyName;
};

UCLASS()
class TEAM4PROJECT_API USettingsMenu : public UMenuWidget
{
	GENERATED_BODY()

public:
	// 메뉴가 열릴 때 호출 — 저장값 스냅샷 + 위젯 초기화 + 슬라이드 인.
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void OpenSettings();

	// 조작 탭 키바인딩 목록을 반환 (WBP RefreshKeybindList 에서 사용).
	UFUNCTION(BlueprintCallable, Category = "Settings")
	TArray<FKeybindRowData> GetKeybindRows() const;

	// 특정 매핑의 키 재설정 시작 (WBP 의 키 행 버튼이 호출). 이후 다음 입력을 캡처.
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void BeginRebind(FName MappingName);

	// 슬라이드 아웃 애니가 끝난 뒤 실제로 위젯을 화면에서 제거.
	// 타이머가 자동 호출하지만, WBP 애니 종료 콜백(Bind to Animation Finished)에서 직접 불러도 됨(더 정확).
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void FinishClose();

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// ── 애니메이션/목록 렌더는 WBP 담당 ──
	UFUNCTION(BlueprintImplementableEvent, Category = "Settings")
	void PlaySlideIn();

	UFUNCTION(BlueprintImplementableEvent, Category = "Settings")
	void PlaySlideOut();

	// 키바인딩 목록을 다시 그리라고 WBP 에 알림 (GetKeybindRows 로 데이터 조회).
	UFUNCTION(BlueprintImplementableEvent, Category = "Settings")
	void RefreshKeybindList();

	// 해상도 되돌림 카운트다운 갱신 (WBP 가 텍스트 표시). Seconds<=0 이면 팝업 닫기.
	UFUNCTION(BlueprintImplementableEvent, Category = "Settings")
	void UpdateRevertCountdown(int32 Seconds);

private:
	// ======================= 탭 =======================
	UPROPERTY(meta = (BindWidgetOptional))
	UWidgetSwitcher* SettingsSwitcher;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* AudioTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* GraphicsTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* ControlsTabButton;

	// 탭 버튼 색: 활성 탭만 강조색, 나머지는 기본색. WBP 클래스 디폴트에서 조정.
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Tabs")
	FLinearColor TabActiveColor = FLinearColor(1.0f, 0.78f, 0.25f, 1.0f); // 예: 노란 강조

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Tabs")
	FLinearColor TabInactiveColor = FLinearColor::White; // 틴트 없음 = 버튼 기본 스타일 색

	// 활성 탭에 맞춰 탭 버튼 색을 갱신.
	void UpdateTabButtonColors();

	// ======================= 오디오 =======================
	UPROPERTY(meta = (BindWidgetOptional))
	USlider* MasterVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* BGMVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* SFXVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* UIVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* VoiceOutputSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* VoiceInputSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	UCheckBox* MuteToggle;

	// 슬라이더 옆 실시간 숫자 표시 (있으면 갱신).
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MasterVolumeValue;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BGMVolumeValue;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* SFXVolumeValue;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* UIVolumeValue;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* VoiceOutputValue;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* VoiceInputValue;

	// ======================= 그래픽 =======================
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString* ResolutionCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString* WindowModeCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString* QualityPresetCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString* FrameLimitCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString* LanguageCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString* ColorblindCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	UCheckBox* VSyncToggle;

	UPROPERTY(meta = (BindWidgetOptional))
	UCheckBox* SubtitleToggle;

	// 세부 화질 슬라이더 (0~3). 조절 시 프리셋을 "커스텀"으로 전환.
	UPROPERTY(meta = (BindWidgetOptional))
	USlider* ShadowQualitySlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* TextureQualitySlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* EffectsQualitySlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* ViewDistanceSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* AAQualitySlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* PostProcessSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* FoliageSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* ShadingSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* GammaSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* FOVSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* SubtitleScaleSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* GammaValue;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* FOVValue;

	// ======================= 조작 =======================
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	UScrollBox* KeybindList;

	UPROPERTY(meta = (BindWidgetOptional))
	USlider* MouseSensitivitySlider;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MouseSensitivityValue;

	UPROPERTY(meta = (BindWidgetOptional))
	UCheckBox* InvertYToggle;

	// 키 중복 시 경고 문구 (있으면 표시).
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* DuplicateKeyWarningText;

	// ======================= 하단 바 =======================
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* ApplyButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* CancelButton;

	// 현재 탭 카테고리만 초기화.
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* ResetButton;

	// 전체 초기화 (선택).
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* ResetAllButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* CloseButton;

	// ── 미저장 변경 확인 팝업 ──
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* UnsavedChangesPopup;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* UnsavedSaveButton;     // 저장 후 닫기

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* UnsavedDiscardButton;  // 버리고 닫기

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* UnsavedCancelButton;   // 계속 편집

	// ── 해상도 되돌림 팝업 ──
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* ResolutionRevertPopup;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* RevertKeepButton;      // 유지

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* RevertBackButton;      // 되돌림

	// ======================= 내부 상태 =======================
	// 취소 시 되돌릴 저장 시점 스냅샷.
	bool bDirty = false;
	ESettingsTab CurrentTab = ESettingsTab::Audio;

	// 키 재설정 대기 중 여부 + 대상.
	bool bAwaitingKey = false;
	FName PendingMappingName;

	// 해상도 되돌림용 이전 값 + 카운트다운.
	FIntPoint PrevResolution = FIntPoint(0, 0);
	int32 PrevWindowMode = 0;
	int32 RevertSecondsLeft = 0;
	FTimerHandle RevertTimerHandle;

	// 슬라이드 아웃 애니 길이(초). WBP 의 SlideOut 애니 길이에 맞춰 조정. 이 시간 뒤 위젯 제거.
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float SlideOutDuration = 0.3f;

	// 닫는 중 중복 호출/입력 방지 + 지연 제거용 타이머.
	FTimerHandle CloseTimerHandle;
	bool bClosing = false;

	UGODGameUserSettings* GetSettings() const;
	UPlayerGameInstance* GetGI() const;

	// 위젯 초기값을 저장된 설정으로 채운다.
	void PopulateFromSettings();
	void PopulateResolutionCombo();
	void PopulateStaticCombos();

	void BindCallbacks();

	// ── 오디오 콜백 ──
	UFUNCTION() void OnMasterVolumeChanged(float V);
	UFUNCTION() void OnBGMVolumeChanged(float V);
	UFUNCTION() void OnSFXVolumeChanged(float V);
	UFUNCTION() void OnUIVolumeChanged(float V);
	UFUNCTION() void OnVoiceOutputChanged(float V);
	UFUNCTION() void OnVoiceInputChanged(float V);
	UFUNCTION() void OnMuteChanged(bool bChecked);

	// ── 그래픽 콜백 ──
	UFUNCTION() void OnResolutionSelected(FString Item, ESelectInfo::Type Type);
	UFUNCTION() void OnWindowModeSelected(FString Item, ESelectInfo::Type Type);
	UFUNCTION() void OnQualityPresetSelected(FString Item, ESelectInfo::Type Type);
	UFUNCTION() void OnFrameLimitSelected(FString Item, ESelectInfo::Type Type);
	UFUNCTION() void OnLanguageSelected(FString Item, ESelectInfo::Type Type);
	UFUNCTION() void OnColorblindSelected(FString Item, ESelectInfo::Type Type);
	UFUNCTION() void OnVSyncChanged(bool bChecked);
	UFUNCTION() void OnSubtitleChanged(bool bChecked);
	UFUNCTION() void OnShadowQualityChanged(float V);
	UFUNCTION() void OnTextureQualityChanged(float V);
	UFUNCTION() void OnEffectsQualityChanged(float V);
	UFUNCTION() void OnViewDistanceChanged(float V);
	UFUNCTION() void OnAAQualityChanged(float V);
	UFUNCTION() void OnPostProcessChanged(float V);
	UFUNCTION() void OnFoliageChanged(float V);
	UFUNCTION() void OnShadingChanged(float V);
	UFUNCTION() void OnGammaChanged(float V);
	UFUNCTION() void OnFOVChanged(float V);
	UFUNCTION() void OnSubtitleScaleChanged(float V);

	// ── 조작 콜백 ──
	UFUNCTION() void OnMouseSensitivityChanged(float V);
	UFUNCTION() void OnInvertYChanged(bool bChecked);

	// ── 탭 ──
	UFUNCTION() void ShowAudioTab();
	UFUNCTION() void ShowGraphicsTab();
	UFUNCTION() void ShowControlsTab();
	void SetTab(ESettingsTab Tab);

	// ── 하단 버튼 ──
	UFUNCTION() void OnApplyClicked();
	UFUNCTION() void OnCancelClicked();
	UFUNCTION() void OnResetClicked();
	UFUNCTION() void OnResetAllClicked();
	UFUNCTION() void OnCloseClicked();

	// ── 미저장 팝업 ──
	UFUNCTION() void OnUnsavedSave();
	UFUNCTION() void OnUnsavedDiscard();
	UFUNCTION() void OnUnsavedCancel();
	void ShowUnsavedPopup(bool bShow);

	// ── 해상도 되돌림 ──
	UFUNCTION() void OnRevertKeep();
	UFUNCTION() void OnRevertBack();
	void StartResolutionRevertCountdown();
	void TickRevertCountdown();
	void FinishRevert(bool bKeep);

	// 적용/취소 공통.
	void ApplyAll();          // 저장 + 디스크 반영
	void RevertToSnapshot();  // 실시간 적용분을 스냅샷으로 되돌림
	void CloseInternal();     // 슬라이드 아웃 + RemoveFromParent

	// 키 재설정 실제 처리 (중복 감지 포함). 성공 시 true.
	bool TryApplyRebind(FName MappingName, const FKey& NewKey);
	void CancelRebind();

	// 세부 화질 슬라이더 → 프리셋 콤보 "커스텀" 표시 동기화.
	void SyncPresetComboToCustom();

	// 값 라벨 갱신 헬퍼 (0~1 → "75" 형태).
	void SetPercentLabel(UTextBlock* Label, float Value01) const;
};
