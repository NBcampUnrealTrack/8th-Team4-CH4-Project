#include "UI/HUD/GODAbilitySlotWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"

void UGODAbilitySlotWidget::SetupSlot(const FAbilitySlotConfig& InConfig, UAbilitySystemComponent* InASC)
{
	SlotConfig = InConfig;
	WeakASC    = InASC;
	RefreshStaticInfo();
}

void UGODAbilitySlotWidget::SetAbilitySystemComponent(UAbilitySystemComponent* InASC)
{
	WeakASC = InASC;
}

void UGODAbilitySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Panel_Tooltip)
		Panel_Tooltip->SetVisibility(ESlateVisibility::Collapsed);

	if (Btn_AbilitySlot)
	{
		Btn_AbilitySlot->OnHovered.AddDynamic(this, &UGODAbilitySlotWidget::OnSlotHovered);
		Btn_AbilitySlot->OnUnhovered.AddDynamic(this, &UGODAbilitySlotWidget::OnSlotUnhovered);
	}

	// SlotConfig 가 에디터 기본값으로 채워져 있으면 즉시 반영
	RefreshStaticInfo();
}

void UGODAbilitySlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshCooldownUI();
}

void UGODAbilitySlotWidget::RefreshStaticInfo()
{
	// 아이콘
	if (Image_Icon && SlotConfig.Icon)
	{
		Image_Icon->SetBrushFromTexture(SlotConfig.Icon);
	}

	// 키 힌트
	if (TB_InputHint)
	{
		TB_InputHint->SetText(SlotConfig.InputHint);
	}

	// 툴팁 텍스트
	if (TB_AbilityName)
		TB_AbilityName->SetText(SlotConfig.AbilityName);

	if (TB_AbilityDescription)
		TB_AbilityDescription->SetText(SlotConfig.AbilityDescription);

	// 쿨타임 초기 숨김
	if (PB_Cooldown)
	{
		PB_Cooldown->SetPercent(0.f);
		PB_Cooldown->SetVisibility(ESlateVisibility::Hidden);
	}
	if (TB_CooldownTime)
		TB_CooldownTime->SetVisibility(ESlateVisibility::Hidden);
}

void UGODAbilitySlotWidget::RefreshCooldownUI()
{
	UAbilitySystemComponent* ASC = WeakASC.Get();
	if (!ASC || !SlotConfig.CooldownTag.IsValid())
	{
		if (PB_Cooldown)  PB_Cooldown->SetVisibility(ESlateVisibility::Hidden);
		if (TB_CooldownTime) TB_CooldownTime->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	FGameplayTagContainer CooldownTagContainer;
	CooldownTagContainer.AddTag(SlotConfig.CooldownTag);

	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTagContainer);

	TArray<float> TimeRemaining = ASC->GetActiveEffectsTimeRemaining(Query);
	TArray<float> Duration      = ASC->GetActiveEffectsDuration(Query);

	if (TimeRemaining.Num() == 0 || TimeRemaining[0] <= 0.f)
	{
		if (PB_Cooldown)  PB_Cooldown->SetVisibility(ESlateVisibility::Hidden);
		if (TB_CooldownTime) TB_CooldownTime->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	const float Remaining    = TimeRemaining[0];
	const float TotalDur     = (Duration.Num() > 0 && Duration[0] > 0.f) ? Duration[0] : 1.f;
	// PB_Cooldown: 1 → 0 (쿨타임 가득 → 소진)
	const float CooldownFill = FMath::Clamp(Remaining / TotalDur, 0.f, 1.f);

	if (PB_Cooldown)
	{
		PB_Cooldown->SetPercent(CooldownFill);
		PB_Cooldown->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (TB_CooldownTime)
	{
		TB_CooldownTime->SetText(FText::FromString(FString::Printf(TEXT("%.1fs"), Remaining)));
		TB_CooldownTime->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UGODAbilitySlotWidget::OnSlotHovered()
{
	if (Panel_Tooltip)
		Panel_Tooltip->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UGODAbilitySlotWidget::OnSlotUnhovered()
{
	if (Panel_Tooltip)
		Panel_Tooltip->SetVisibility(ESlateVisibility::Collapsed);
}
