#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GODHUD.generated.h"

class UGODMainHUDWidget;
class UGearQTEWidget;
class UPressureMinigameWidget;
class UQuestMinigameWidget;
class AGearSlot;
class APressureValve;
class AQuestStation;

UCLASS()
class TEAM4PROJECT_API AGODHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 에디터(BP_GODHUD)에서 WBP_GODMainHUD 위젯 클래스를 지정.
	 * GODMainHUDWidget 을 상속한 위젯 블루프린트를 할당하면 된다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UGODMainHUDWidget> MainHUDWidgetClass;

	/** 런타임에 생성된 메인 HUD 위젯 */
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UGODMainHUDWidget> MainHUDWidget;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowMainHUD();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideMainHUD();

	// ============================================================
	// 미니게임 오버레이 (기어 QTE / 압력밸브 바늘)
	// ============================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Minigame")
	TSubclassOf<UGearQTEWidget> GearQTEWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Minigame")
	TObjectPtr<UGearQTEWidget> GearQTEWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Minigame")
	TSubclassOf<UPressureMinigameWidget> PressureMinigameWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Minigame")
	TObjectPtr<UPressureMinigameWidget> PressureMinigameWidget;

	void ShowGearQTE(AGearSlot* Slot);
	void HideGearQTE(bool bSuccess);

	void ShowPressureMinigame(APressureValve* Valve);
	void HidePressureMinigame(bool bSuccess);

	// 퀘스트 팝업. 위젯 클래스는 HUD 가 아니라 스테이션의 DT 행에서 온다
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Quest")
	TObjectPtr<UQuestMinigameWidget> QuestMinigameWidget;

	void ShowQuestMinigame(AQuestStation* Station);
	void HideQuestMinigame(bool bSuccess);
};
