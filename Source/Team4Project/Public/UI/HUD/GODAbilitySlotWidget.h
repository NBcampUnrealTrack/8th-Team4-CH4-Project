#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GODAbilitySlotWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UButton;
class UWidget;
class UAbilitySystemComponent;
class UGameplayAbility;
class UTexture2D;

/** 능력 슬롯 하나에 표시할 데이터 */
USTRUCT(BlueprintType)
struct FAbilitySlotConfig
{
	GENERATED_BODY()

	/** 이 슬롯의 어빌리티 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Slot")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** 쿨타임을 식별하는 Gameplay Tag (GE GrantedTags 에 설정된 태그) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Slot")
	FGameplayTag CooldownTag;

	/** 슬롯에 표시할 능력 아이콘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Slot")
	TObjectPtr<UTexture2D> Icon;

	/** 툴팁에 표시되는 능력 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Slot")
	FText AbilityName = FText::GetEmpty();

	/** 툴팁에 표시되는 능력 설명 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Slot")
	FText AbilityDescription = FText::GetEmpty();

	/** 입력 키 힌트 (예: "E", "Q") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Slot")
	FText InputHint = FText::GetEmpty();
};

/**
 * 역할 능력 버튼 하나를 담당하는 위젯.
 * - 쿨타임 오버레이 & 남은 시간 텍스트를 자동으로 갱신
 * - 마우스 호버 시 능력 이름/설명 툴팁 표시
 *
 * [에디터 작업]
 * WBP_AbilitySlot (이 클래스 상속) 을 만들고 아래 이름으로 위젯을 배치:
 *   Image_Icon         — 능력 아이콘 이미지
 *   PB_Cooldown        — 쿨다운 오버레이 프로그레스바 (0→1: 완충→완료)
 *   TB_CooldownTime    — 남은 쿨타임 초 텍스트
 *   TB_InputHint       — 입력 키 힌트 텍스트
 *   Btn_AbilitySlot    — 호버 감지용 투명 버튼
 *   Panel_Tooltip      — 툴팁 컨테이너 (기본 Hidden)
 *   TB_AbilityName     — 툴팁 내 능력 이름
 *   TB_AbilityDescription — 툴팁 내 능력 설명
 */
UCLASS()
class TEAM4PROJECT_API UGODAbilitySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 슬롯 데이터를 설정하고 ASC 를 연결.
	 * GODMainHUDWidget 에서 역할 결정 후 호출.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability Slot")
	void SetupSlot(const FAbilitySlotConfig& InConfig, UAbilitySystemComponent* InASC);

	/** ASC 만 교체 (역할 정보 유지). 폰 재빙의 시 호출. */
	UFUNCTION(BlueprintCallable, Category = "Ability Slot")
	void SetAbilitySystemComponent(UAbilitySystemComponent* InASC);

	/**
	 * 슬롯의 어빌리티 발동 시도. 버튼 클릭에 자동 바인딩되며,
	 * 나중에 키보드 입력으로 바꿀 때 BP/C++ 어디서든 이 함수를 호출하면 된다.
	 * DT 에 BP 서브클래스(GA_xxx 상속 BP)로 부여된 경우도 잡히도록 자식 클래스까지 매칭한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability Slot")
	bool TryActivateSlotAbility();

	/** 에디터 기본값으로 슬롯을 미리 설정할 수도 있음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Slot")
	FAbilitySlotConfig SlotConfig;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ─── BindWidget: BP 에서 반드시 동일 이름으로 위젯을 배치 ───

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Cooldown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_CooldownTime;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_InputHint;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_AbilitySlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Panel_Tooltip;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_AbilityName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_AbilityDescription;

private:
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> WeakASC;

	void RefreshStaticInfo();
	void RefreshCooldownUI();

	UFUNCTION()
	void OnSlotHovered();

	UFUNCTION()
	void OnSlotUnhovered();

	UFUNCTION()
	void OnSlotClicked();
};
