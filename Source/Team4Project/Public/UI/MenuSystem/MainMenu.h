// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MenuSystem/MenuWidget.h"
#include "MainMenu.generated.h"

USTRUCT()
struct FServerData
{
	GENERATED_BODY()

	FString Name;
	uint16 CurrentPlayers;
	uint16 MaxPlayers;
	FString HostUsername;
};

UENUM(BlueprintType)
enum class EMenuState : uint8
{
	MainMenu    UMETA(DisplayName = "Main Menu"),
	Host        UMETA(DisplayName = "Host Menu"),
	Join        UMETA(DisplayName = "Join Menu"),
	Skin        UMETA(DisplayName = "Skin Menu")
};

UCLASS()
class TEAM4PROJECT_API UMainMenu : public UMenuWidget
{
	GENERATED_BODY()
	
public:
	UMainMenu(const FObjectInitializer& ObjectInitializer);

	void SetServerList(TArray<FServerData> ServerNames);

	void SelectIndex(uint32 Index);
protected:
	virtual bool Initialize() override;

	// ESC 로 스킨 메뉴 → 메인 메뉴 복귀
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI Animation")
	void PlayMenuTransition(EMenuState TargetIndex);

	// 스킨 미리보기가 바뀔 때 호출 — WBP 에서 구현해 레벨의 BP_SkinPreview 액터 메시를 교체.
	UFUNCTION(BlueprintImplementableEvent, Category = "Skin")
	void OnSkinPreviewChanged(int32 SkinIndex);

private:
	TSubclassOf<class UUserWidget> ServerRowClass;

	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* JoinButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* CancelJoinMenuButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* ConfirmJoinMenuButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* QuitButton;

	UPROPERTY(meta = (BindWidget))
	class UWidgetSwitcher* MenuSwitcher;

	UPROPERTY(meta = (BindWidget))
	class UWidget* JoinMenu;

	UPROPERTY(meta = (BindWidget))
	class UWidget* HostMenu;

	UPROPERTY(meta = (BindWidget))
	class UEditableTextBox* ServerHostName;

	UPROPERTY(meta = (BindWidget))
	class UButton* CancelHostMenuButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* ConfirmHostMenuButton;

	UPROPERTY(meta = (BindWidget))
	class UWidget* MainMenu;

	UPROPERTY(meta = (BindWidget))
	class UPanelWidget* ServerList;

	// ── 스킨 선택 (WBP 에 아직 없어도 되도록 Optional 바인딩) ──

	// 메인 페이지: 스킨 메뉴 열기 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SkinButton;

	// WidgetSwitcher 의 스킨 선택 페이지
	UPROPERTY(meta = (BindWidgetOptional))
	class UWidget* SkinMenu;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* BackFromSkinMenuButton;

	// 동물 스킨 버튼 (인덱스 = BaseCharacter.SkinOptions 순서)
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SkinDogButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SkinFrogButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SkinPandaButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SkinRabbitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SkinRaccoonButton;

	// 미리보기 중인 스킨을 확정(GameInstance 저장)하는 "선택" 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* ConfirmSkinButton;

	// 현재 선택된 스킨 이름 표시 (선택 사항)
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SelectedSkinText;

	UFUNCTION()
	void HostServer();

	UFUNCTION()
	void JoinServer();

	UFUNCTION()
	void OpenHostMenu();

	UFUNCTION()
	void OpenJoinMenu();

	UFUNCTION()
	void OpenMainMenu();

	UFUNCTION()
	void QuitPressed();

	// ── 스킨 선택 ──
	UFUNCTION()
	void OpenSkinMenu();

	UFUNCTION() void SelectSkinDog();
	UFUNCTION() void SelectSkinFrog();
	UFUNCTION() void SelectSkinPanda();
	UFUNCTION() void SelectSkinRabbit();
	UFUNCTION() void SelectSkinRaccoon();

	// 아이콘 클릭: 미리보기만 변경 (저장은 ConfirmSkinSelection 에서)
	void PreviewSkin(int32 SkinIndex);

	// "선택" 버튼: 미리보기 중인 스킨을 GameInstance 에 저장 후 메인 메뉴 복귀
	UFUNCTION()
	void ConfirmSkinSelection();

	// 현재 미리보기 중인 스킨 인덱스 (스킨 메뉴 열 때 저장값으로 초기화)
	int32 PreviewSkinIndex = 0;

	void UpdateSelectedSkinText();

	TOptional<uint32> SelectedIndex;

	void UpdateChildren();
};
