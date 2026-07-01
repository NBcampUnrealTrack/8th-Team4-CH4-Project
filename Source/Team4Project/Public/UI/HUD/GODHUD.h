#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GODHUD.generated.h"

class UGODMainHUDWidget;

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
};
