#include "UI/HUD/GODMainHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"

#include "Game/GODGameState.h"
#include "Player/BaseCharacter.h"
#include "Player/Component/BaseAttributeSet.h"
#include "Component/InteractComponent.h"
#include "TimerManager.h"
#include "Components/AudioComponent.h"
#include "Sound/GameSoundStatics.h"
#include "Sound/GameSoundTypes.h"

// ─────────────────────────────────────────────────────────────────────────────
// 초기화
// ─────────────────────────────────────────────────────────────────────────────

void UGODMainHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 툴팁 패널 초기 숨김
	if (Panel_RoleTooltip) Panel_RoleTooltip->SetVisibility(ESlateVisibility::Collapsed);
	if (Panel_AmmoDisplay)  Panel_AmmoDisplay->SetVisibility(ESlateVisibility::Collapsed);

	// 경고 문구 초기 숨김
	if (TB_PressureWarning) TB_PressureWarning->SetVisibility(ESlateVisibility::Hidden);
	if (TB_FuelWarning)     TB_FuelWarning->SetVisibility(ESlateVisibility::Hidden);

	// 인터랙트 프롬프트 초기 숨김 (대상 생기면 OnInteractTargetChanged가 켠다)
	if (TB_InteractPrompt)  TB_InteractPrompt->SetVisibility(ESlateVisibility::Collapsed);

	// 총기 해제 알림 초기 숨김
	if (TB_GunsUnlockedNotice) TB_GunsUnlockedNotice->SetVisibility(ESlateVisibility::Collapsed);

	// 알림 방송 배너 초기 숨김
	if (TB_Announcement) TB_Announcement->SetVisibility(ESlateVisibility::Collapsed);

	// 역할 아이콘 호버 버튼 바인딩
	if (Btn_RoleIcon)
	{
		Btn_RoleIcon->OnHovered.AddDynamic(this, &UGODMainHUDWidget::OnRoleIconHovered);
		Btn_RoleIcon->OnUnhovered.AddDynamic(this, &UGODMainHUDWidget::OnRoleIconUnhovered);
	}

	TryBindGameState();

	// 인게임 BGM 시작 (루프는 사운드 애셋에서 설정)
	if (!BGMAudio)
	{
		BGMAudio = UGameSoundStatics::SpawnSound2DFromTable(this, UISoundTable, SoundRows::BGMInGame);
	}
}

void UGODMainHUDWidget::NativeDestruct()
{
	if (BGMAudio)
	{
		BGMAudio->Stop();
		BGMAudio = nullptr;
	}

	// GameState 델리게이트 언바인딩
	if (AGODGameState* GS = CachedGameState.Get())
	{
		GS->OnRemainingTimeChanged.RemoveAll(this);
		GS->OnDistanceToDestinationChanged.RemoveAll(this);
		GS->OnPressureLevelChanged.RemoveAll(this);
		GS->OnFuelLevelChanged.RemoveAll(this);
		GS->OnGunsUnlocked.RemoveAll(this);
		GS->OnAnnouncement.RemoveAll(this);
	}

	// 캐릭터 태그 변경 델리게이트 언바인딩
	if (ABaseCharacter* Char = CachedCharacter.Get())
	{
		Char->OnCharacterTagChanged.RemoveAll(this);
	}

	// ASC 어트리뷰트 델리게이트 언바인딩
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UBaseAttributeSet::GetCurrentAmmoAttribute()).RemoveAll(this);
	}

	// 인터랙트 델리게이트 언바인딩
	if (UInteractComponent* IC = CachedInteractComp.Get())
	{
		IC->OnInteractTargetChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UGODMainHUDWidget::TryBindGameState()
{
	AGODGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGODGameState>() : nullptr;
	if (!GS || CachedGameState.Get() == GS) return;

	CachedGameState = GS;

	GS->OnRemainingTimeChanged.AddDynamic(this,       &UGODMainHUDWidget::OnRemainingTimeChanged);
	GS->OnDistanceToDestinationChanged.AddDynamic(this, &UGODMainHUDWidget::OnDistanceChanged);
	GS->OnPressureLevelChanged.AddDynamic(this,       &UGODMainHUDWidget::OnPressureLevelChanged);
	GS->OnFuelLevelChanged.AddDynamic(this,           &UGODMainHUDWidget::OnFuelLevelChanged);
	GS->OnGunsUnlocked.AddDynamic(this,               &UGODMainHUDWidget::OnGunsUnlocked);
	GS->OnAnnouncement.AddDynamic(this,               &UGODMainHUDWidget::OnAnnouncement);

	// 현재 값으로 즉시 갱신
	CurrentTime     = GS->RemainingTime;
	CurrentDistance = GS->DistanceToDestination;
	CurrentPressure = GS->PressureLevel;
	CurrentFuel     = GS->FuelLevel;

	UpdateTimeDisplay();
	UpdateTrainProgress();
	UpdatePressureDisplay();
	UpdateFuelDisplay();
}

void UGODMainHUDWidget::InitializeForPawn(APawn* NewPawn)
{
	// 이전 캐릭터의 태그 변경 델리게이트 언바인딩
	if (ABaseCharacter* OldChar = CachedCharacter.Get())
	{
		OldChar->OnCharacterTagChanged.RemoveAll(this);
	}

	ABaseCharacter* Char = Cast<ABaseCharacter>(NewPawn);
	CachedCharacter = Char;

	if (!Char) return;

	// 게임 시작 시 리스폰 없이 SetCharacterTag 만 호출되므로(역할 배정),
	// 태그 변경 델리게이트로 역할 HUD 를 갱신한다. (없으면 로비 BP 기본 태그로 고정됨)
	Char->OnCharacterTagChanged.AddDynamic(this, &UGODMainHUDWidget::OnCharacterTagChanged);

	// ASC 연결
	UAbilitySystemComponent* ASC = Cast<UAbilitySystemComponent>(Char->GetAbilitySystemComponent());
	if (ASC && CachedASC.Get() != ASC)
	{
		// 이전 ASC 언바인딩
		if (UAbilitySystemComponent* OldASC = CachedASC.Get())
		{
			OldASC->GetGameplayAttributeValueChangeDelegate(
				UBaseAttributeSet::GetCurrentAmmoAttribute()).RemoveAll(this);
		}

		CachedASC = ASC;

		// 탄약 변화 바인딩
		ASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetCurrentAmmoAttribute())
			.AddLambda([this](const FOnAttributeChangeData&) { UpdateAmmoDisplay(); });

		// 능력 슬롯에 ASC 전달
		if (Slot_Ability1) Slot_Ability1->SetAbilitySystemComponent(ASC);
		if (Slot_Ability2) Slot_Ability2->SetAbilitySystemComponent(ASC);

		UpdateAmmoDisplay();
	}

	// 역할 태그로 HUD 세팅
	FGameplayTag CharTag = Char->GetCharacterTag();
	if (CharTag.IsValid())
	{
		SetupRoleHUD(CharTag);
	}

	// 인터랙트 프롬프트 연결 (재빙의 시 이전 컴포넌트는 언바인딩)
	if (UInteractComponent* IC = Char->InteractComponent)
	{
		if (CachedInteractComp.Get() != IC)
		{
			if (UInteractComponent* OldIC = CachedInteractComp.Get())
			{
				OldIC->OnInteractTargetChanged.RemoveAll(this);
			}
			CachedInteractComp = IC;
			IC->OnInteractTargetChanged.AddDynamic(this, &UGODMainHUDWidget::OnInteractTargetChanged);
		}
		// 새 폰 기준으로 초기화 (대상 없음 상태에서 시작)
		OnInteractTargetChanged(nullptr, FText::GetEmpty());
	}
}

void UGODMainHUDWidget::OnCharacterTagChanged(const FGameplayTag& NewTag)
{
	if (NewTag.IsValid())
	{
		SetupRoleHUD(NewTag);
	}
}

void UGODMainHUDWidget::OnGunsUnlocked()
{
	// 알림음 (게임 사운드 DT 의 Game.GunsUnlocked 행 — GameState 에서 읽음)
	if (const AGODGameState* GS = CachedGameState.Get())
	{
		UGameSoundStatics::PlaySound2DFromTable(this, GS->GameSoundTable, SoundRows::GameGunsUnlocked);
	}

	// "총기 제한 해제" 알림 — 일정 시간 표시 후 자동 숨김. BP 연출 확장 포인트도 호출.
	if (TB_GunsUnlockedNotice)
	{
		TB_GunsUnlockedNotice->SetText(NSLOCTEXT("HUD", "GunsUnlocked", "총기 제한 해제"));
		TB_GunsUnlockedNotice->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(GunsNoticeTimer, this,
				&UGODMainHUDWidget::HideGunsUnlockedNotice, GunsUnlockedNoticeDuration, /*bLoop=*/false);
		}
	}

	BP_OnGunsUnlocked();
}

void UGODMainHUDWidget::HideGunsUnlockedNotice()
{
	if (TB_GunsUnlockedNotice)
	{
		TB_GunsUnlockedNotice->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGODMainHUDWidget::OnAnnouncement(const FText& Message, EAnnouncementType Type)
{
	// 연달아 들어오면 최신 것만 보여주고 타이머를 다시 잡는다.
	if (TB_Announcement)
	{
		FLinearColor Color = AnnouncementInfoColor;
		switch (Type)
		{
		case EAnnouncementType::Warning:  Color = AnnouncementWarningColor;  break;
		case EAnnouncementType::Critical: Color = AnnouncementCriticalColor; break;
		default: break;
		}

		TB_Announcement->SetText(Message);
		TB_Announcement->SetColorAndOpacity(FSlateColor(Color));
		TB_Announcement->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(AnnouncementTimer, this,
				&UGODMainHUDWidget::HideAnnouncement, AnnouncementDuration, /*bLoop=*/false);
		}
	}

	BP_OnAnnouncement(Message, Type);
}

void UGODMainHUDWidget::HideAnnouncement()
{
	if (TB_Announcement)
	{
		TB_Announcement->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGODMainHUDWidget::OnInteractTargetChanged(AActor* NewTarget, FText Prompt)
{
	CurrentInteractTarget = NewTarget;

	if (TB_InteractPrompt)
	{
		if (NewTarget && !Prompt.IsEmpty())
		{
			TB_InteractPrompt->SetText(
				FText::Format(NSLOCTEXT("HUD", "InteractPrompt", "[F] {0}"), Prompt));
			TB_InteractPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			TB_InteractPrompt->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 월드 마커 등 추가 연출은 WBP에서 이 이벤트로 구현
	BP_OnInteractTargetChanged(NewTarget, Prompt);
}

// ─────────────────────────────────────────────────────────────────────────────
// NativeTick
// ─────────────────────────────────────────────────────────────────────────────

void UGODMainHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// GameState 연결이 아직 안 됐으면 재시도
	if (!CachedGameState.IsValid())
	{
		TryBindGameState();
	}

	// 폰 변경 감지 (빙의·사망 후 재빙의 처리)
	APawn* CurrentPawn = GetOwningPlayerPawn();
	if (CurrentPawn != LastKnownPawn.Get())
	{
		LastKnownPawn = CurrentPawn;
		InitializeForPawn(CurrentPawn);
	}

	UpdateWarnings(InDeltaTime);
	//UpdateCrosshair();
}

// ─────────────────────────────────────────────────────────────────────────────
// 델리게이트 콜백
// ─────────────────────────────────────────────────────────────────────────────

void UGODMainHUDWidget::OnRemainingTimeChanged(int32 NewTime)
{
	CurrentTime = NewTime;
	UpdateTimeDisplay();
}

void UGODMainHUDWidget::OnDistanceChanged(float NewDistance)
{
	CurrentDistance = NewDistance;
	UpdateTrainProgress();
}

void UGODMainHUDWidget::OnPressureLevelChanged(float NewPressure)
{
	CurrentPressure = NewPressure;
	UpdatePressureDisplay();
	bWarningPressureActive = (CurrentPressure >= PressureWarningLevel);
}

void UGODMainHUDWidget::OnFuelLevelChanged(float NewFuel)
{
	CurrentFuel = NewFuel;
	UpdateFuelDisplay();
	bWarningFuelActive = (CurrentFuel <= FuelWarningThreshold);
}

// ─────────────────────────────────────────────────────────────────────────────
// UI 갱신 함수들
// ─────────────────────────────────────────────────────────────────────────────

void UGODMainHUDWidget::UpdateTimeDisplay()
{
	if (TB_RemainingTime)
		TB_RemainingTime->SetText(FText::FromString(FormatTime(CurrentTime)));
}

void UGODMainHUDWidget::UpdateTrainProgress()
{
	const float Progress = (TotalDistance > 0.f)
		? FMath::Clamp(1.f - CurrentDistance / TotalDistance, 0.f, 1.f)
		: 0.f;

	if (PB_TrainProgress)
		PB_TrainProgress->SetPercent(Progress);

	if (TB_TrainProgressLabel)
	{
		TB_TrainProgressLabel->SetText(
			FText::FromString(FString::Printf(TEXT("도착지까지 %.0f%%"), (1.f - Progress) * 100.f)));
	}

	if (Train_img)
	{
		// 오버레이(선로)의 실제 가로 픽셀 길이 (Mun님의 UI 크기에 맞게 조절 가능)
		float TrackTotalLength = 1000.f;

		// 왼쪽(0%)에서 오른쪽(100%)으로 정방향 전진하기 위한 진짜 진행율 계산
		float RealMovePercent = Progress;

		// 최종 X 좌표 계산
		float NewXPosition = RealMovePercent * TrackTotalLength;

		// 기차 위치 이동
		Train_img->SetRenderTranslation(FVector2D(NewXPosition, 0.f));
	}
}

void UGODMainHUDWidget::UpdatePressureDisplay()
{
	const float Percent = FMath::Clamp(CurrentPressure / 100.f, 0.f, 1.f);

	if (PB_Pressure)
		PB_Pressure->SetPercent(Percent);

	if (TB_PressureValue)
		TB_PressureValue->SetText(FText::FromString(FString::Printf(TEXT("압력: %.0f%%"), CurrentPressure)));
}

void UGODMainHUDWidget::UpdateFuelDisplay()
{
	const float Percent = FMath::Clamp(CurrentFuel, 0.f, 1.f);

	if (PB_Fuel)
		PB_Fuel->SetPercent(Percent);

	if (TB_FuelValue)
		TB_FuelValue->SetText(FText::FromString(FString::Printf(TEXT("연료: %.0f%%"), CurrentFuel * 100.f)));
}

void UGODMainHUDWidget::UpdateWarnings(float DeltaTime)
{
	const bool bAnyWarning = bWarningPressureActive || bWarningFuelActive;

	if (bAnyWarning)
	{
		WarningBlinkTimer += DeltaTime;
		if (WarningBlinkTimer >= WarningBlinkInterval)
		{
			WarningBlinkTimer = 0.f;
			bWarningVisible   = !bWarningVisible;
		}
	}
	else
	{
		bWarningVisible   = false;
		WarningBlinkTimer = 0.f;
	}

	const ESlateVisibility VisWhenActive = bWarningVisible
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Hidden;

	if (TB_PressureWarning)
	{
		TB_PressureWarning->SetVisibility(bWarningPressureActive ? VisWhenActive : ESlateVisibility::Hidden);
	}

	if (TB_FuelWarning)
	{
		TB_FuelWarning->SetVisibility(bWarningFuelActive ? VisWhenActive : ESlateVisibility::Hidden);
	}
}

void UGODMainHUDWidget::UpdateAmmoDisplay()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC || !Panel_AmmoDisplay || !TB_AmmoCount) return;

	ABaseCharacter* Char = CachedCharacter.Get();
	const bool bHasGun = Char && Char->GetCurrentWeapon() != nullptr;

	Panel_AmmoDisplay->SetVisibility(bHasGun ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (bHasGun)
	{
		const int32 Current = FMath::FloorToInt(ASC->GetNumericAttribute(UBaseAttributeSet::GetCurrentAmmoAttribute()));
		const int32 Max     = FMath::FloorToInt(ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxAmmoAttribute()));
		TB_AmmoCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Current, Max)));
	}
}

void UGODMainHUDWidget::UpdateCrosshair(bool bShow)
{
	if (!Img_Crosshair) return;

	Img_Crosshair->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

// ─────────────────────────────────────────────────────────────────────────────
// 역할 HUD 세팅
// ─────────────────────────────────────────────────────────────────────────────

void UGODMainHUDWidget::SetupRoleHUD(const FGameplayTag& CharTag)
{
	const FRoleHUDSetup* Setup = RoleSetupMap.Find(CharTag);

	// 역할 아이콘 / 툴팁
	if (Setup)
	{
		if (Img_RoleIcon && Setup->DisplayInfo.Icon)
			Img_RoleIcon->SetBrushFromTexture(Setup->DisplayInfo.Icon);

		if (TB_RoleName)
			TB_RoleName->SetText(Setup->DisplayInfo.RoleName);

		if (TB_RoleDescription)
			TB_RoleDescription->SetText(Setup->DisplayInfo.RoleDescription);
	}

	// 능력 슬롯 설정
	UAbilitySystemComponent* ASC = CachedASC.Get();

	auto ConfigureSlot = [&](UGODAbilitySlotWidget* SlotWidget, int32 Index)
	{
		if (!SlotWidget) return;

		if (Setup && Setup->AbilitySlots.IsValidIndex(Index))
		{
			SlotWidget->SetupSlot(Setup->AbilitySlots[Index], ASC);
			SlotWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			SlotWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	};

	ConfigureSlot(Slot_Ability1, 0);
	ConfigureSlot(Slot_Ability2, 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// 역할 아이콘 호버
// ─────────────────────────────────────────────────────────────────────────────

void UGODMainHUDWidget::OnRoleIconHovered()
{
	if (Panel_RoleTooltip)
		Panel_RoleTooltip->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UGODMainHUDWidget::OnRoleIconUnhovered()
{
	if (Panel_RoleTooltip)
		Panel_RoleTooltip->SetVisibility(ESlateVisibility::Collapsed);
}

// ─────────────────────────────────────────────────────────────────────────────
// 유틸
// ─────────────────────────────────────────────────────────────────────────────

FString UGODMainHUDWidget::FormatTime(int32 TotalSeconds)
{
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;
	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}
