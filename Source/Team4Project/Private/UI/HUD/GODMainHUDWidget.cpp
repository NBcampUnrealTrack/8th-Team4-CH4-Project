#include "UI/HUD/GODMainHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

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

	// 💡 스킬 슬롯 / 역할 아이콘 초기 숨김 (새로운 변수명으로 변경)
	if (Slot_Passive) Slot_Passive->SetVisibility(ESlateVisibility::Collapsed);
	if (Slot_Active1) Slot_Active1->SetVisibility(ESlateVisibility::Collapsed);
	if (Slot_Active2) Slot_Active2->SetVisibility(ESlateVisibility::Collapsed);
	if (Btn_RoleIcon) Btn_RoleIcon->SetVisibility(ESlateVisibility::Collapsed);

	// 툴팁 패널 초기 숨김
	//if (Panel_RoleTooltip) Panel_RoleTooltip->SetVisibility(ESlateVisibility::Collapsed);

	// 경고 문구 초기 숨김
	if (TB_PressureWarning) TB_PressureWarning->SetVisibility(ESlateVisibility::Hidden);
	if (TB_FuelWarning)     TB_FuelWarning->SetVisibility(ESlateVisibility::Hidden);

	// 인터랙트 프롬프트 초기 숨김 (대상 생기면 OnInteractTargetChanged가 켠다)
	if (TB_InteractPrompt)  TB_InteractPrompt->SetVisibility(ESlateVisibility::Collapsed);

	// 알림 방송 배너 초기 숨김
	if (TB_Announcement) TB_Announcement->SetVisibility(ESlateVisibility::Collapsed);

	/* 역할 아이콘 호버 버튼 바인딩
	if (Btn_RoleIcon)
	{
		Btn_RoleIcon->OnHovered.AddDynamic(this, &UGODMainHUDWidget::OnRoleIconHovered);
		Btn_RoleIcon->OnUnhovered.AddDynamic(this, &UGODMainHUDWidget::OnRoleIconUnhovered);
	}*/

	/*if (PB_Pressure)
	{
		if (UMaterialInterface* FillMat = Cast<UMaterialInterface>(PB_Pressure->WidgetStyle.FillImage.GetResourceObject()))
		{
			PressureFillMID = UMaterialInstanceDynamic::Create(FillMat, this);
			if (PressureFillMID)
			{
				PB_Pressure->WidgetStyle.FillImage.SetResourceObject(PressureFillMID);
				UE_LOG(LogTemp, Warning, TEXT("[PressureBar] MID 생성 성공 (부모: %s)"), *FillMat->GetName());
			}
		}
		else
		{
			// Fill 브러시가 머테리얼이 아니면(텍스처/None) 색 변경 불가.
			UObject* Res = PB_Pressure->WidgetStyle.FillImage.GetResourceObject();
			UE_LOG(LogTemp, Error, TEXT("[PressureBar] Fill이 머테리얼이 아님(%s) → MI_ProgressBar_Gauge 로 지정 필요"),
				Res ? *Res->GetName() : TEXT("None"));
		}
	}*/

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
		GS->OnAnnouncement.RemoveAll(this);
	}

	// 캐릭터 태그 변경 델리게이트 언바인딩
	if (ABaseCharacter* Char = CachedCharacter.Get())
	{
		Char->OnCharacterTagChanged.RemoveAll(this);
	}

	// 인터랙트 델리게이트 언바인딩
	if (UInteractComponent* IC = CachedInteractComp.Get())
	{
		IC->OnInteractTargetChanged.RemoveAll(this);
	}

	// 압력 게이지 동적 머테리얼/브러시 참조 해제 (GC 정리 순서 경고 예방)
	/*if (PB_Pressure)
	{
		PB_Pressure->WidgetStyle.FillImage.SetResourceObject(nullptr);
	}
	PressureFillMID = nullptr; */

	Super::NativeDestruct();
}

void UGODMainHUDWidget::TryBindGameState()
{
	AGODGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGODGameState>() : nullptr;
	if (!GS || CachedGameState.Get() == GS) return;

	CachedGameState = GS;

	GS->OnRemainingTimeChanged.AddDynamic(this, &UGODMainHUDWidget::OnRemainingTimeChanged);
	GS->OnDistanceToDestinationChanged.AddDynamic(this, &UGODMainHUDWidget::OnDistanceChanged);
	GS->OnPressureLevelChanged.AddDynamic(this, &UGODMainHUDWidget::OnPressureLevelChanged);
	GS->OnFuelLevelChanged.AddDynamic(this, &UGODMainHUDWidget::OnFuelLevelChanged);
	GS->OnAnnouncement.AddDynamic(this, &UGODMainHUDWidget::OnAnnouncement);

	// 현재 값으로 즉시 갱신
	CurrentTime = GS->RemainingTime;
	CurrentDistance = GS->DistanceToDestination;
	CurrentPressure = GS->PressureLevel;
	CurrentFuel = GS->FuelLevel;

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
		CachedASC = ASC;

		// 💡 능력 슬롯에 ASC 전달 (새로운 슬롯 변수명 3개로 업데이트)
		if (Slot_Passive) Slot_Passive->SetAbilitySystemComponent(ASC);
		if (Slot_Active1) Slot_Active1->SetAbilitySystemComponent(ASC);
		if (Slot_Active2) Slot_Active2->SetAbilitySystemComponent(ASC);
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
	{
		FString CombinedString = FString::Printf(TEXT("시민에게 남은 시간 %s"), *FormatTime(CurrentTime));

		TB_RemainingTime->SetText(FText::FromString(CombinedString));
	}
}

void UGODMainHUDWidget::UpdateTrainProgress()
{
	const float Progress = (TotalDistance > 0.f)
		? FMath::Clamp(1.f - CurrentDistance / TotalDistance, 0.f, 1.f)
		: 0.f;

	if (PB_TrainProgress)
		PB_TrainProgress->SetPercent(Progress);

	//if (TB_TrainProgressLabel)
	//{
		//TB_TrainProgressLabel->SetText(
			//FText::FromString(FString::Printf(TEXT("도착지까지 %.0f%%"), (1.f - Progress) * 100.f)));
	//}

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

	//if (PB_Pressure)
		//PB_Pressure->SetPercent(Percent);

	// 바 전체 색을 현재 값에 따라 Safe→Mid→Danger 로 보간 (MI_ProgressBar_Gauge)
	//if (PressureFillMID)
		//PressureFillMID->SetScalarParameterValue(TEXT("Percent"), Percent);

	// ── 디버그(확인 후 삭제): 압력/퍼센트/MID 상태를 화면 좌상단에 표시 ──
/*#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			(uint64)(UPTRINT)this, 0.f, FColor::Yellow,
			FString::Printf(TEXT("[PressureBar] Pressure=%.1f  Percent=%.2f  MID=%s"),
				CurrentPressure, Percent, PressureFillMID ? TEXT("OK") : TEXT("NULL")));
	}*/
	//#endif

	if (TB_PressureValue)
		TB_PressureValue->SetText(FText::FromString(FString::Printf(TEXT("압력: %.0f%%"), CurrentPressure)));
}

void UGODMainHUDWidget::UpdateFuelDisplay()
{
	const float Percent = FMath::Clamp(CurrentFuel, 0.f, 1.f);

	//if (PB_Fuel)
		//PB_Fuel->SetPercent(Percent);

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
			bWarningVisible = !bWarningVisible;
		}
	}
	else
	{
		bWarningVisible = false;
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

/*void UGODMainHUDWidget::UpdateCrosshair(bool bShow)
{
	if (!Img_Crosshair) return;

	Img_Crosshair->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}*/

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

		if (Btn_RoleIcon) Btn_RoleIcon->SetVisibility(ESlateVisibility::Visible);

		//if (TB_RoleName)
			//TB_RoleName->SetText(Setup->DisplayInfo.RoleName);

		//if (TB_RoleDescription)
			//TB_RoleDescription->SetText(Setup->DisplayInfo.RoleDescription);

		// ─────────────────────────────────────────────────────────────────────────────
		// 💡 [여기에 추가] 처음 역할을 배정받았을 때만 전면 역할 소개 창을 트리거합니다.
		// ─────────────────────────────────────────────────────────────────────────────
		if (!bHasShownRoleIntro)
		{
			BP_ShowRoleIntro(Setup->DisplayInfo);
			bHasShownRoleIntro = true;
		}
	}

	// 능력 슬롯 설정
	UAbilitySystemComponent* ASC = CachedASC.Get();

	// ─────────────────────────────────────────────────────────────────────────────
	// 💡 [패시브 슬롯 처리] 에디터에서 패시브 아이콘을 등록했을 때만 왼쪽 슬롯을 켭니다.
	// ─────────────────────────────────────────────────────────────────────────────
	if (Setup && Setup->PassiveSlot.Icon != nullptr)
	{
		if (Slot_Passive)
		{
			Slot_Passive->SetupSlot(Setup->PassiveSlot, ASC);
			Slot_Passive->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
	else
	{
		if (Slot_Passive) Slot_Passive->SetVisibility(ESlateVisibility::Collapsed);
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// 💡 [액티브 슬롯 처리 람다 식]
	// ─────────────────────────────────────────────────────────────────────────────
	auto ConfigureActiveSlot = [&](UGODAbilitySlotWidget* SlotWidget, int32 Index)
		{
			if (!SlotWidget) return;

			if (Setup && Setup->ActiveSlots.IsValidIndex(Index))
			{
				SlotWidget->SetupSlot(Setup->ActiveSlots[Index], ASC);
				SlotWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			else
			{
				SlotWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
		};

	// 💡 요청하신 정확한 화면 자리에 인덱스를 매핑합니다.
	ConfigureActiveSlot(Slot_Active1, 0); // 0번 데이터(액티브1) ➡️ 우하단 슬롯(`Slot_Active1`) 배치
	ConfigureActiveSlot(Slot_Active2, 1); // 1번 데이터(액티브2) ➡️ 우상단 슬롯(`Slot_Active2`) 배치
}

// ─────────────────────────────────────────────────────────────────────────────
// 역할 아이콘 호버
// ─────────────────────────────────────────────────────────────────────────────

/*void UGODMainHUDWidget::OnRoleIconHovered()
{
	if (Panel_RoleTooltip)
		Panel_RoleTooltip->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UGODMainHUDWidget::OnRoleIconUnhovered()
{
	if (Panel_RoleTooltip)
		Panel_RoleTooltip->SetVisibility(ESlateVisibility::Collapsed);
}*/

// ─────────────────────────────────────────────────────────────────────────────
// 유틸
// ─────────────────────────────────────────────────────────────────────────────

FString UGODMainHUDWidget::FormatTime(int32 TotalSeconds)
{
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;
	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}