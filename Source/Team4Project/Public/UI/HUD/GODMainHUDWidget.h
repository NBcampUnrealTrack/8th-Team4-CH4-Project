#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Player/GODPlayerState.h"      // EMainRole, ECitizenClass
#include "Game/AnnouncementTypes.h"     // EAnnouncementType
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
class UMaterialInstanceDynamic;

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

	// 역할 이름 (툴팁에 표시) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role")
	FText RoleName = FText::GetEmpty(); 

	// 역할 설명 (툴팁에 표시) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role")
	FText RoleDescription = FText::GetEmpty();
};

// ─────────────────────────────────────────────
// 역할별 HUD 세팅 (아이콘 + 능력 슬롯 최대 3개)
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FRoleHUDSetup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role")
	FRoleDisplayInfo DisplayInfo;

	/** 패시브 스킬 설정 (없으면 에디터에서 None) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role")
	FAbilitySlotConfig PassiveSlot;

	/** 액티브 능력 슬롯 설정 (최대 2개 - 0번: 우하단 고정, 1번: 우상단 추가) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role")
	TArray<FAbilitySlotConfig> ActiveSlots;
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
	float TotalDistance = 120000.f;

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

	//조준점 ui 노출
	//UFUNCTION(BlueprintCallable, Category = "HUD")
	//void UpdateCrosshair(bool bShow);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	// ─── BindWidget: BP 에서 반드시 동일 이름으로 위젯을 배치 ─────

	// 상단 중앙 — 열차 진행도
	UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	TObjectPtr<UProgressBar> PB_TrainProgress;

	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UTextBlock> TB_TrainProgressLabel;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|Train")
	TObjectPtr<UImage> Train_img;

	// 우 상단 — 압력
	//UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	//TObjectPtr<UProgressBar> PB_Pressure;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_PressureValue;

	// 우 상단 — 연료
	//UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	//TObjectPtr<UProgressBar> PB_Fuel;

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
	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UImage> Img_Crosshair;

	//우 하단 — 역할 아이콘 및 툴팁
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_RoleIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_RoleIcon;

	//UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|Role")
	//TObjectPtr<UWidget> Panel_RoleTooltip;

	//UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|Role")
	//TObjectPtr<UTextBlock> TB_RoleName;

	//UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|Role")
	//TObjectPtr<UTextBlock> TB_RoleDescription;

	// 우 하단 — 능력 슬롯 (WBP_AbilitySlot 상속 위젯을 배치)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGODAbilitySlotWidget> Slot_Passive;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGODAbilitySlotWidget> Slot_Active1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGODAbilitySlotWidget> Slot_Active2;

	// 중앙 하단 — 인터랙트 프롬프트 ("[F] 열기" 등).
	// WBP에 이 이름의 TextBlock을 배치하면 자동 연결된다 (Optional이라 없어도 컴파일됨).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_InteractPrompt;

	// 중앙 상단 — 알림 방송 배너 (기어 파손, 압력 경고, 연료 부족 등).
	// WBP에 이 이름의 TextBlock을 배치하면 자동 연결된다 (Optional이라 없어도 컴파일됨).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Announcement;

	// 알림 배너 표시 유지 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Announcement")
	float AnnouncementDuration = 3.f;

	// 심각도별 배너 색상. WBP에서 덮어쓸 수 있다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Announcement")
	FLinearColor AnnouncementInfoColor = FLinearColor(0.7f, 0.9f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Announcement")
	FLinearColor AnnouncementWarningColor = FLinearColor(1.f, 0.75f, 0.2f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Announcement")
	FLinearColor AnnouncementCriticalColor = FLinearColor(1.f, 0.25f, 0.2f, 1.f);

	// 알림 수신 시 WBP 확장 포인트 (애니메이션/사운드 등). TB_Announcement 없이 이것만 써도 된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Announcement")
	void BP_OnAnnouncement(const FText& Message, EAnnouncementType Type);

	// 현재 인터랙트 대상 (없으면 None). WBP에서 월드 마커 위치 투영 등에 사용.
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Interact")
	TObjectPtr<AActor> CurrentInteractTarget;

	// 대상/프롬프트 변경 시 WBP 확장 포인트 (마커 표시/애니메이션 등을 BP에서 구현)
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Interact")
	void BP_OnInteractTargetChanged(AActor* NewTarget, const FText& Prompt);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Role")
	void BP_ShowRoleIntro(const FRoleDisplayInfo& DisplayInfo);

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

	// GODGameState::OnAnnouncement 콜백 — 배너 표시
	UFUNCTION()
	void OnAnnouncement(const FText& Message, EAnnouncementType Type);

	void HideAnnouncement();
	FTimerHandle AnnouncementTimer;

	// 인게임 BGM (NativeConstruct 에서 시작, NativeDestruct 에서 정지)
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BGMAudio;

	// 압력 게이지(PB_Pressure) 채움의 동적 머테리얼. NativeConstruct 에서 생성,
	// UpdatePressureDisplay 에서 "Percent" 스칼라를 넣어 바 전체 색을 값에 따라 보간한다.
	// (Fill 브러시 머테리얼을 MI_ProgressBar_Gauge 로 지정해 둔 경우에만 동작)
	//UPROPERTY(Transient)
	//TObjectPtr<UMaterialInstanceDynamic> PressureFillMID;

	// ─── UI 갱신 ────────────────────────────────

	void UpdateTimeDisplay();
	void UpdateTrainProgress();
	void UpdatePressureDisplay();
	void UpdateFuelDisplay();
	void UpdateWarnings(float DeltaTime);
	//void UpdateCrosshair();

	void SetupRoleHUD(const FGameplayTag& CharTag);

	//UFUNCTION()
	//void OnRoleIconHovered();

	//UFUNCTION()
	//void OnRoleIconUnhovered();

	static FString FormatTime(int32 TotalSeconds);

	bool bHasShownRoleIntro = false;
};