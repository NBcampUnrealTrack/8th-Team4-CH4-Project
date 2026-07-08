#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Player/GODPlayerState.h"      // EMainRole, ECitizenClass
#include "UI/HUD/GODAbilitySlotWidget.h" // FAbilitySlotConfig
#include "GODMainHUDWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UButton;
class UWidget;
class UAbilitySystemComponent;
class UInteractComponent;
class ABaseCharacter;
class AGODGameState;
class AGODPlayerState;
class UTexture2D;
class UGODAbilitySlotWidget;
class UDataTable;
class UAudioComponent;

// ─────────────────────────────────────────────
// 역할 아이콘 / 설명 정보
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FRoleDisplayInfo
{
	GENERATED_BODY()

	/** 역할 아이콘 이미지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role")
	TObjectPtr<UTexture2D> Icon;

	/** 역할 이름 (툴팁에 표시) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role")
	FText RoleName = FText::GetEmpty();

	/** 역할 설명 (툴팁에 표시) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role")
	FText RoleDescription = FText::GetEmpty();
};

// ─────────────────────────────────────────────
// 역할별 HUD 세팅 (아이콘 + 능력 슬롯 최대 2개)
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FRoleHUDSetup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role")
	FRoleDisplayInfo DisplayInfo;

	/** 이 역할의 능력 슬롯 설정 (최대 2개). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role")
	TArray<FAbilitySlotConfig> AbilitySlots;
};

/**
 * 인게임 메인 HUD 위젯.
 */
UCLASS()
class TEAM4PROJECT_API UGODMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ─── 에디터 설정 ─────────────────────────────

	/**
	 * 역할 태그 → 아이콘/설명/능력슬롯 매핑.
	 * 키 예시: Character.Special.Mafia, Character.Crew.Mechanic …
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Role Setup")
	TMap<FGameplayTag, FRoleHUDSetup> RoleSetupMap;

	/** 열차 전체 이동 거리 (GODTrain::TotalDistance 와 동일하게 설정) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Train")
	float TotalDistance = 600000.f;

	/** 연료 경고 임계값 (0~1). 이 이하이면 경고 표시 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Warning")
	float FuelWarningThreshold = 0.2f;

	/** 압력 경고 임계값 (0~100). 이 이상이면 경고 표시 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Warning")
	float PressureWarningLevel = 80.f;

	/** 경고 문구 점멸 간격(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Warning")
	float WarningBlinkInterval = 0.5f;

	/** UI 사운드 DT (BGM.InGame 행으로 인게임 BGM 재생). WBP 디폴트에서 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Sound")
	TObjectPtr<UDataTable> UISoundTable;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	// ─── BindWidget: BP 에서 반드시 동일 이름으로 위젯을 배치 ─────

	// 상단 중앙 — 열차 진행도
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_TrainProgress;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_TrainProgressLabel;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|Train")
	TObjectPtr<UImage> Train_img;

	// 우 상단 — 압력
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Pressure;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_PressureValue;

	// 우 상단 — 연료
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Fuel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_FuelValue;

	// 우 상단 — 남은 시간
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_RemainingTime;

	// 중앙 — 경고 문구
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_PressureWarning;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_FuelWarning;

	// 중앙 — 조준선
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Crosshair;

	//우 하단 — 역할 아이콘 및 툴팁
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_RoleIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_RoleIcon;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|Role")
	TObjectPtr<UWidget> Panel_RoleTooltip;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|Role")
	TObjectPtr<UTextBlock> TB_RoleName;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|Role")
	TObjectPtr<UTextBlock> TB_RoleDescription;

	// 우 하단 — 능력 슬롯 (WBP_AbilitySlot 상속 위젯을 배치)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGODAbilitySlotWidget> Slot_Ability1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGODAbilitySlotWidget> Slot_Ability2;

	// 우 하단 — 탄약 (총 장착 시 표시)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Panel_AmmoDisplay;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_AmmoCount;

	// 중앙 하단 — 인터랙트 프롬프트 ("[F] 열기" 등).
	// WBP에 이 이름의 TextBlock을 배치하면 자동 연결된다 (Optional이라 없어도 컴파일됨).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_InteractPrompt;

	// 중앙 — 3분 발포 잠금 해제 알림 ("총기 제한 해제").
	// WBP에 이 이름의 TextBlock을 배치하면 해제 시점에 잠깐 표시된다 (Optional).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_GunsUnlockedNotice;

	// 알림 표시 유지 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Warning")
	float GunsUnlockedNoticeDuration = 4.f;

	// 발포 잠금 해제 시 WBP 확장 포인트 (사운드/애니메이션 등 BP 연출용)
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Warning")
	void BP_OnGunsUnlocked();

	// 현재 인터랙트 대상 (없으면 None). WBP에서 월드 마커 위치 투영 등에 사용.
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Interact")
	TObjectPtr<AActor> CurrentInteractTarget;

	// 대상/프롬프트 변경 시 WBP 확장 포인트 (마커 표시/애니메이션 등을 BP에서 구현)
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Interact")
	void BP_OnInteractTargetChanged(AActor* NewTarget, const FText& Prompt);

private:
	// ─── 런타임 상태 ─────────────────────────────

	UPROPERTY()
	TWeakObjectPtr<AGODGameState> CachedGameState;

	UPROPERTY()
	TWeakObjectPtr<ABaseCharacter> CachedCharacter;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	UPROPERTY()
	TWeakObjectPtr<UInteractComponent> CachedInteractComp;

	float CurrentPressure = 0.f;
	float CurrentFuel = 0.f;
	float CurrentDistance = 0.f;
	int32 CurrentTime = 0;

	bool bWarningPressureActive = false;
	bool bWarningFuelActive = false;
	bool bWarningVisible = true;
	float WarningBlinkTimer = 0.f;

	bool bGunEquipped = false;

	// 현재 폰 변경 감지용
	UPROPERTY()
	TWeakObjectPtr<APawn> LastKnownPawn;

	// ─── 초기화 / 이벤트 바인딩 ──────────────────

	void TryBindGameState();
	void InitializeForPawn(APawn* NewPawn);

	UFUNCTION()
	void OnRemainingTimeChanged(int32 NewTime);

	UFUNCTION()
	void OnDistanceChanged(float NewDistance);

	UFUNCTION()
	void OnPressureLevelChanged(float NewPressure);

	UFUNCTION()
	void OnFuelLevelChanged(float NewFuel);

	// InteractComponent::OnInteractTargetChanged 콜백 — 프롬프트 표시/숨김
	UFUNCTION()
	void OnInteractTargetChanged(AActor* NewTarget, FText Prompt);

	// BaseCharacter::OnCharacterTagChanged 콜백 — 게임 시작 시 역할 배정(SetCharacterTag)에
	// 맞춰 역할 아이콘/능력 슬롯 갱신 (리스폰이 없어 폰 변경 감지로는 잡히지 않는다).
	UFUNCTION()
	void OnCharacterTagChanged(const FGameplayTag& NewTag);

	// GODGameState::OnGunsUnlocked 콜백 — "총기 제한 해제" 알림 표시
	UFUNCTION()
	void OnGunsUnlocked();

	void HideGunsUnlockedNotice();
	FTimerHandle GunsNoticeTimer;

	// 인게임 BGM (NativeConstruct 에서 시작, NativeDestruct 에서 정지)
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BGMAudio;

	// ─── UI 갱신 ────────────────────────────────

	void UpdateTimeDisplay();
	void UpdateTrainProgress();
	void UpdatePressureDisplay();
	void UpdateFuelDisplay();
	void UpdateWarnings(float DeltaTime);
	void UpdateAmmoDisplay();
	void UpdateCrosshair();

	void SetupRoleHUD(const FGameplayTag& CharTag);

	UFUNCTION()
	void OnRoleIconHovered();

	UFUNCTION()
	void OnRoleIconUnhovered();

	static FString FormatTime(int32 TotalSeconds);
};