// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/SlateEnums.h" // ETextCommit
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
	bool bHasPassword = false;
	// 비밀번호 CRC32 해시 — 참여 팝업의 클라 선검증용 (원문은 호스트에만 보관)
	int32 PasswordHash = 0;
};

UENUM(BlueprintType)
enum class EMenuState : uint8
{
	MainMenu    UMETA(DisplayName = "Main Menu"),
	Host        UMETA(DisplayName = "Host Menu"),
	Join        UMETA(DisplayName = "Join Menu"),
	Skin        UMETA(DisplayName = "Skin Menu"),
	Settings    UMETA(DisplayName = "Settings Menu")
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

	// 메인 메뉴 BGM 시작/정지 (UISoundTable 의 BGM.MainMenu 행)
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ESC 로 스킨 메뉴 → 메인 메뉴 복귀
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 새 페이지가 나타날 때(In) — 전환 완료 후 호출. WBP 가 등장 애니 재생.
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Animation")
	void PlayMenuTransition(EMenuState TargetIndex);

	// 현재 페이지가 사라질 때(Out) — 전환 시작 시 호출. WBP 가 퇴장 애니 재생.
	// LeavingIndex = 지금 나가는 페이지 (Host/Join 등 페이지별 다른 Out 애니를 주고 싶을 때 분기용).
	// 이 애니가 끝난 뒤(TransitionDuration) 실제 페이지 전환 + PlayMenuTransition(In) 이 실행됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Animation")
	void PlayMenuOut(EMenuState LeavingIndex);

	// 스킨 미리보기가 바뀔 때 호출 — WBP 에서 구현해 레벨의 BP_SkinPreview 액터 메시를 교체.
	UFUNCTION(BlueprintImplementableEvent, Category = "Skin")
	void OnSkinPreviewChanged(int32 SkinIndex);

	// 비밀번호 틀림 문구가 표시될 때 호출 — WBP 에서 PasswordErrorText 의
	// Opacity 1 유지 → 0 페이드아웃 애니메이션을 재생 (유지 시간/곡선은 WBP 에서 조절).
	// 미구현 시 문구가 팝업이 닫힐 때까지 남는 기존 동작 유지.
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Animation")
	void PlayPasswordErrorAnim();

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

	// ── 방 비밀번호 / 빠른 방찾기 (WBP 에 아직 없어도 되도록 Optional 바인딩) ──

	// Host 메뉴: 방 비밀번호 입력.
	// 빈칸이면 공개방, 값이 있으면 그 값을 비밀번호로 하는 비밀방으로 생성된다.
	// (별도의 공개/비밀 체크박스 없이 이 입력창만으로 판별)
	UPROPERTY(meta = (BindWidgetOptional))
	class UEditableTextBox* ServerPassword;
	
	// 닉네임 입력 (빈칸이면 스팀 닉네임 사용)
	UPROPERTY(meta = (BindWidgetOptional))
	class UEditableTextBox* NicknameInput;
	
	UPROPERTY(meta = (BindWidgetOptional))
	class UEditableTextBox* JoinNicknameInput;

	// ── 비밀방 참여 비밀번호 팝업 ──
	// 비밀방을 선택하고 Join 을 누르면 이 팝업이 뜬다.
	// 팝업 패널(Border/Overlay 등) — 있으면 팝업 방식, 없으면 JoinPassword 상시 노출 방식으로 동작
	UPROPERTY(meta = (BindWidgetOptional))
	class UWidget* JoinPasswordPopup;

	// 팝업 안의 비밀번호 입력창 (팝업 없이 단독으로 두면 기존 상시 노출 방식)
	UPROPERTY(meta = (BindWidgetOptional))
	class UEditableTextBox* JoinPassword;

	// 팝업: 확인(입장) 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* ConfirmPasswordButton;

	// 팝업: 취소 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* CancelPasswordButton;

	// 팝업: 비밀번호 틀림 안내 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* PasswordErrorText;

	// 빠른 방찾기 버튼 (메인 페이지나 Join 메뉴 어디든 배치 가능)
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* QuickJoinButton;

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

	// ── 설정창 (WBP 에 아직 없어도 되도록 Optional) ──
	// 메인 페이지: 설정창 열기 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SettingsButton;

	// 설정창 위젯 클래스 (WBP_SettingsMenu). WBP_MainMenu 디폴트에서 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	TSubclassOf<class USettingsMenu> SettingsMenuClass;

	// 열려 있는 설정창 인스턴스 (재사용).
	UPROPERTY()
	TObjectPtr<class USettingsMenu> SettingsMenuInstance;

	UFUNCTION()
	void OpenSettings();

	UFUNCTION()
	void HostServer();

	UFUNCTION()
	void JoinServer();

	// ── 비밀방 참여 팝업 ──
	UFUNCTION()
	void ConfirmJoinPasswordPressed();

	UFUNCTION()
	void CancelJoinPasswordPressed();

	// 팝업 입력창에서 엔터 → 확인과 동일
	UFUNCTION()
	void OnJoinPasswordCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	void CloseJoinPasswordPopup();

	UFUNCTION()
	void QuickJoinPressed();

	UFUNCTION()
	void OpenHostMenu();

	UFUNCTION()
	void OpenJoinMenu();

	UFUNCTION()
	void OpenMainMenu();

	// ── 페이지 전환 (Out 애니 → 전환 → In 애니) ──
	// 각 Open* 이 직접 SetActiveWidget 하지 않고 이걸 거쳐 Out→전환→In 순서를 만든다.
	void BeginMenuTransition(EMenuState Target);
	void CommitMenuTransition();
	class UWidget* GetPageForState(EMenuState State) const;

	// 퇴장 애니 길이(초). WBP 의 MenuOut 애니 길이에 맞춰 조정. 이 시간 뒤 페이지 전환.
	UPROPERTY(EditDefaultsOnly, Category = "UI Animation")
	float TransitionDuration = 0.3f;

	FTimerHandle MenuTransitionTimer;
	bool bMenuTransitioning = false;
	EMenuState PendingState = EMenuState::MainMenu;
	// 지금 화면에 떠 있는 페이지 (Out 애니에 "나가는 페이지"를 넘기기 위해 추적).
	EMenuState CurrentState = EMenuState::MainMenu;

	UFUNCTION()
	void QuitPressed();
	
	void SaveNicknameToGameInstance(class UEditableTextBox* SourceBox);
	
	// 두 닉네임 입력창 값 동기화
	UFUNCTION()
	void OnNicknameChanged(const FText& Text);

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

	// 마지막으로 받은 서버 목록 — 선택한 방의 비밀방 여부/해시 조회용
	TArray<FServerData> ServerListData;

	void UpdateChildren();
};
