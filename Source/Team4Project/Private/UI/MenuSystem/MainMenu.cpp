// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MenuSystem/MainMenu.h"
#include "UObject/ConstructorHelpers.h"

#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Misc/Crc.h"

#include "UI/MenuSystem/ServerRow.h"
#include "UI/MenuSystem/SettingsMenu.h"
#include "Game/PlayerGameInstance.h"
#include "Sound/GameSoundTypes.h"

namespace
{
	// 인덱스 = BaseCharacter.SkinOptions / BP_TestCharacter 스킨 배열 순서
	const TCHAR* GSkinDisplayNames[] = { TEXT("강아지"), TEXT("개구리"), TEXT("판다"), TEXT("토끼"), TEXT("너구리") };
}

UMainMenu::UMainMenu(const FObjectInitializer& ObjectInitializer)
{
    ConstructorHelpers::FClassFinder<UUserWidget> ServerRowBPClass(TEXT("/Game/GameSystem/UI/MenuSystem/WBP_ServerRow"));
    if (!ensure(ServerRowBPClass.Class != nullptr))
        return;

    ServerRowClass = ServerRowBPClass.Class;
}

bool UMainMenu::Initialize()
{
    bool Success = Super::Initialize();
    if (!Success) return false;

    if (!ensure(HostButton != nullptr))
        return false;
    HostButton->OnClicked.AddDynamic(this, &UMainMenu::OpenHostMenu);

    if (!ensure(CancelHostMenuButton != nullptr)) return false;
    CancelHostMenuButton->OnClicked.AddDynamic(this, &UMainMenu::OpenMainMenu);

    if (!ensure(ConfirmHostMenuButton != nullptr)) return false;
    ConfirmHostMenuButton->OnClicked.AddDynamic(this, &UMainMenu::HostServer);

    if (!ensure(JoinButton != nullptr))
        return false;
    JoinButton->OnClicked.AddDynamic(this, &UMainMenu::OpenJoinMenu);

    if (!ensure(CancelJoinMenuButton != nullptr))
        return false;
    CancelJoinMenuButton->OnClicked.AddDynamic(this, &UMainMenu::OpenMainMenu);

    if (!ensure(ConfirmJoinMenuButton != nullptr))
        return false;
    ConfirmJoinMenuButton->OnClicked.AddDynamic(this, &UMainMenu::JoinServer);

    if (!ensure(QuitButton != nullptr))
        return false;
    QuitButton->OnClicked.AddDynamic(this, &UMainMenu::QuitPressed);

    // 빠른 방찾기 버튼은 Optional — WBP 에 추가된 경우에만 바인딩
    if (QuickJoinButton) QuickJoinButton->OnClicked.AddDynamic(this, &UMainMenu::QuickJoinPressed);

    // 비밀방 참여 팝업 — 위젯이 WBP 에 있는 경우에만 바인딩
    if (ConfirmPasswordButton) ConfirmPasswordButton->OnClicked.AddDynamic(this, &UMainMenu::ConfirmJoinPasswordPressed);
    if (CancelPasswordButton) CancelPasswordButton->OnClicked.AddDynamic(this, &UMainMenu::CancelJoinPasswordPressed);
    if (JoinPassword) JoinPassword->OnTextCommitted.AddDynamic(this, &UMainMenu::OnJoinPasswordCommitted);
    if (JoinPasswordPopup) JoinPasswordPopup->SetVisibility(ESlateVisibility::Collapsed);

    // 스킨 선택 위젯은 Optional — WBP 에 추가된 경우에만 바인딩
    if (SkinButton) SkinButton->OnClicked.AddDynamic(this, &UMainMenu::OpenSkinMenu);
    if (BackFromSkinMenuButton) BackFromSkinMenuButton->OnClicked.AddDynamic(this, &UMainMenu::OpenMainMenu);
    if (SkinDogButton) SkinDogButton->OnClicked.AddDynamic(this, &UMainMenu::SelectSkinDog);
    if (SkinFrogButton) SkinFrogButton->OnClicked.AddDynamic(this, &UMainMenu::SelectSkinFrog);
    if (SkinPandaButton) SkinPandaButton->OnClicked.AddDynamic(this, &UMainMenu::SelectSkinPanda);
    if (SkinRabbitButton) SkinRabbitButton->OnClicked.AddDynamic(this, &UMainMenu::SelectSkinRabbit);
    if (SkinRaccoonButton) SkinRaccoonButton->OnClicked.AddDynamic(this, &UMainMenu::SelectSkinRaccoon);
    if (ConfirmSkinButton) ConfirmSkinButton->OnClicked.AddDynamic(this, &UMainMenu::ConfirmSkinSelection);
    UpdateSelectedSkinText();

    // 설정창 버튼 — Optional
    if (SettingsButton) SettingsButton->OnClicked.AddDynamic(this, &UMainMenu::OpenSettings);

    // 모든 버튼에 클릭/호버 사운드 바인딩 (UISoundTable 의 UI.Click / UI.Hover 행. null 은 무시됨)
    BindButtonSounds(HostButton);
    BindButtonSounds(CancelHostMenuButton);
    BindButtonSounds(ConfirmHostMenuButton);
    BindButtonSounds(JoinButton);
    BindButtonSounds(CancelJoinMenuButton);
    BindButtonSounds(ConfirmJoinMenuButton);
    BindButtonSounds(QuitButton);
    BindButtonSounds(QuickJoinButton);
    BindButtonSounds(ConfirmPasswordButton);
    BindButtonSounds(CancelPasswordButton);
    BindButtonSounds(SkinButton);
    BindButtonSounds(BackFromSkinMenuButton);
    BindButtonSounds(SkinDogButton);
    BindButtonSounds(SkinFrogButton);
    BindButtonSounds(SkinPandaButton);
    BindButtonSounds(SkinRabbitButton);
    BindButtonSounds(SkinRaccoonButton);
    BindButtonSounds(ConfirmSkinButton);
    BindButtonSounds(SettingsButton);

    return true;
}

void UMainMenu::OpenSettings()
{
    if (!SettingsMenuClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Settings] SettingsMenuClass 미지정 — WBP_MainMenu 에 WBP_SettingsMenu 를 지정하세요."));
        return;
    }
    // 이미 떠 있으면 중복 생성 방지.
    if (SettingsMenuInstance && SettingsMenuInstance->IsInViewport())
    {
        return;
    }
    SettingsMenuInstance = CreateWidget<USettingsMenu>(GetOwningPlayer(), SettingsMenuClass);
    if (SettingsMenuInstance)
    {
        SettingsMenuInstance->SetMenuInterface(MenuInterface);
        // 메인 메뉴 위에 오버레이 (Z-Order 높게)
        SettingsMenuInstance->AddToViewport(10);
    }
}

void UMainMenu::NativeConstruct()
{
    Super::NativeConstruct();
    PlayUIMusic(SoundRows::BGMMainMenu);
}

void UMainMenu::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MenuTransitionTimer);
    }
    StopUIMusic();
    Super::NativeDestruct();
}

FReply UMainMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // 비밀번호 팝업이 떠 있으면 ESC → 팝업 닫기
    if (InKeyEvent.GetKey() == EKeys::Escape &&
        JoinPasswordPopup && JoinPasswordPopup->GetVisibility() == ESlateVisibility::Visible)
    {
        CloseJoinPasswordPopup();
        return FReply::Handled();
    }

    // 스킨 메뉴에서 ESC → 메인 메뉴 복귀
    if (InKeyEvent.GetKey() == EKeys::Escape &&
        MenuSwitcher && SkinMenu && MenuSwitcher->GetActiveWidget() == SkinMenu)
    {
        OpenMainMenu();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ============================================================
// 스킨 선택
// ============================================================

void UMainMenu::OpenSkinMenu()
{
    if (!ensure(MenuSwitcher != nullptr)) return;
    if (!ensure(SkinMenu != nullptr)) return;

    // Out 애니 → 전환 → In 애니. 실제 전환은 TransitionDuration 뒤 CommitMenuTransition 에서.
    BeginMenuTransition(EMenuState::Skin);

    // 미리보기를 현재 저장된 스킨으로 초기화
    UWorld* World = GetWorld();
    if (const UPlayerGameInstance* GI = World ? World->GetGameInstance<UPlayerGameInstance>() : nullptr)
    {
        PreviewSkinIndex = GI->SelectedSkinIndex;
    }
    OnSkinPreviewChanged(PreviewSkinIndex);
    UpdateSelectedSkinText();

    // ESC 키를 받으려면 이 위젯에 포커스가 있어야 한다.
    SetKeyboardFocus();
}

void UMainMenu::SelectSkinDog()     { PreviewSkin(0); }
void UMainMenu::SelectSkinFrog()    { PreviewSkin(1); }
void UMainMenu::SelectSkinPanda()   { PreviewSkin(2); }
void UMainMenu::SelectSkinRabbit()  { PreviewSkin(3); }
void UMainMenu::SelectSkinRaccoon() { PreviewSkin(4); }

void UMainMenu::PreviewSkin(int32 SkinIndex)
{
    PreviewSkinIndex = SkinIndex;
    OnSkinPreviewChanged(SkinIndex);
    UpdateSelectedSkinText();
}

void UMainMenu::ConfirmSkinSelection()
{
    UWorld* World = GetWorld();
    if (!ensure(World != nullptr)) return;

    if (UPlayerGameInstance* GI = World->GetGameInstance<UPlayerGameInstance>())
    {
        GI->SelectedSkinIndex = PreviewSkinIndex;
    }
    OpenMainMenu();
}

void UMainMenu::UpdateSelectedSkinText()
{
    if (!SelectedSkinText) return;

    if (PreviewSkinIndex >= 0 && PreviewSkinIndex < UE_ARRAY_COUNT(GSkinDisplayNames))
    {
        SelectedSkinText->SetText(FText::Format(
            NSLOCTEXT("MainMenu", "SelectedSkin", "{0}"),
            FText::FromString(GSkinDisplayNames[PreviewSkinIndex])));
    }
}

void UMainMenu::OpenHostMenu()
{
    BeginMenuTransition(EMenuState::Host);
}

void UMainMenu::HostServer()
{
    if (MenuInterface == nullptr) return;

    FString ServerName = ServerHostName->GetText().ToString();

    // 비밀번호 입력창만으로 판별: 빈칸 = 공개방, 값이 있으면 그 값을 비밀번호로 하는 비밀방.
    FString Password = ServerPassword
        ? ServerPassword->GetText().ToString().TrimStartAndEnd()
        : FString();

    MenuInterface->Host(ServerName, Password);
}

void UMainMenu::SetServerList(TArray<FServerData> ServerNames)
{
    UWorld* World = this->GetWorld();
    if (!ensure(World != nullptr))
        return;

    // 참여 시 비밀방 여부/해시 조회용으로 보관
    ServerListData = ServerNames;

    ServerList->ClearChildren();

    uint32 i = 0;

    for (const FServerData& ServerData : ServerNames)
    {
        UServerRow* Row = CreateWidget<UServerRow>(this, ServerRowClass);
        if (!ensure(Row != nullptr))
            return;

        Row->ServerName->SetText(FText::FromString(ServerData.Name));
        Row->HostUser->SetText(FText::FromString(ServerData.HostUsername));
        FString FractionText = FString::Printf(TEXT("%d/%d"), ServerData.CurrentPlayers, ServerData.MaxPlayers);
        Row->ConnectionFraction->SetText(FText::FromString(FractionText));
        // 비밀번호 방 자물쇠 표시 (WBP_ServerRow에 LockIcon 위젯이 있는 경우)
        if (Row->LockIcon)
        {
            Row->LockIcon->SetVisibility(ServerData.bHasPassword
                ? ESlateVisibility::HitTestInvisible
                : ESlateVisibility::Collapsed);
        }
        Row->Setup(this, i);
        ++i;

        ServerList->AddChild(Row);
    }
}

void UMainMenu::SelectIndex(uint32 Index)
{
    SelectedIndex = Index;
    UpdateChildren();
}

void UMainMenu::UpdateChildren()
{
    for (int32 i = 0; i < ServerList->GetChildrenCount(); ++i)
    {
        auto Row = Cast<UServerRow>(ServerList->GetChildAt(i));
        if (Row != nullptr)
        {
            Row->Selected = (SelectedIndex.IsSet() && SelectedIndex.GetValue() == i);
        }
    }
}

void UMainMenu::JoinServer()
{
    if (!SelectedIndex.IsSet() || MenuInterface == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Selected index not set"));
        return;
    }

    const uint32 Index = SelectedIndex.GetValue();
    UE_LOG(LogTemp, Warning, TEXT("Selected index %d."), Index);

    const bool bHasPassword = ServerListData.IsValidIndex(Index) && ServerListData[Index].bHasPassword;

    // 비밀방이면 바로 입장하지 않고 비밀번호 팝업을 띄운다
    if (bHasPassword && JoinPasswordPopup)
    {
        if (JoinPassword)
        {
            JoinPassword->SetText(FText::GetEmpty());
        }
        if (PasswordErrorText)
        {
            PasswordErrorText->SetVisibility(ESlateVisibility::Collapsed);
        }
        JoinPasswordPopup->SetVisibility(ESlateVisibility::Visible);
        if (JoinPassword)
        {
            JoinPassword->SetKeyboardFocus();
        }
        return;
    }

    // 공개방 — 바로 입장 (팝업 위젯이 없는 옛 레이아웃이면 상시 입력창 값 사용)
    const FString Password = JoinPassword
        ? JoinPassword->GetText().ToString().TrimStartAndEnd()
        : FString();
    MenuInterface->Join(Index, Password);
}

void UMainMenu::ConfirmJoinPasswordPressed()
{
    if (!SelectedIndex.IsSet() || MenuInterface == nullptr)
        return;

    const uint32 Index = SelectedIndex.GetValue();
    const FString Password = JoinPassword
        ? JoinPassword->GetText().ToString().TrimStartAndEnd()
        : FString();

    // 클라 선검증: 해시 불일치면 팝업을 유지한 채 에러만 표시.
    // (최종 검증은 서버 PreLogin — 여기 통과해도 서버에서 한 번 더 확인한다)
    if (ServerListData.IsValidIndex(Index) && ServerListData[Index].bHasPassword)
    {
        const int32 ExpectedHash = ServerListData[Index].PasswordHash;
        if (Password.IsEmpty() ||
            static_cast<int32>(FCrc::StrCrc32(*Password)) != ExpectedHash)
        {
            if (PasswordErrorText)
            {
                PasswordErrorText->SetText(NSLOCTEXT("MainMenu", "WrongPassword",
                    "비밀번호가 올바르지 않습니다."));
                PasswordErrorText->SetVisibility(ESlateVisibility::HitTestInvisible);
                PlayPasswordErrorAnim();
            }
            else if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Red,
                    TEXT("비밀번호가 올바르지 않습니다."));
            }
            return;
        }
    }

    CloseJoinPasswordPopup();
    MenuInterface->Join(Index, Password);
}

void UMainMenu::CancelJoinPasswordPressed()
{
    CloseJoinPasswordPopup();
}

void UMainMenu::OnJoinPasswordCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    // 팝업이 떠 있을 때 입력창에서 엔터 → 확인 버튼과 동일하게 처리
    if (CommitMethod == ETextCommit::OnEnter &&
        JoinPasswordPopup && JoinPasswordPopup->GetVisibility() == ESlateVisibility::Visible)
    {
        ConfirmJoinPasswordPressed();
    }
}

void UMainMenu::CloseJoinPasswordPopup()
{
    if (JoinPasswordPopup)
    {
        JoinPasswordPopup->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (JoinPassword)
    {
        JoinPassword->SetText(FText::GetEmpty());
    }
    if (PasswordErrorText)
    {
        PasswordErrorText->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMainMenu::QuickJoinPressed()
{
    if (MenuInterface != nullptr)
    {
        MenuInterface->QuickJoin();
    }
}

void UMainMenu::OpenJoinMenu()
{
    if (!ensure(MenuSwitcher != nullptr))
        return;
    if (!ensure(JoinMenu != nullptr))
        return;
    
    BeginMenuTransition(EMenuState::Join);

    if (MenuInterface != nullptr)
    {
        MenuInterface->RefreshServerList();
    }
}

void UMainMenu::OpenMainMenu()
{
    if (!ensure(MenuSwitcher != nullptr))
        return;
    if (!ensure(MainMenu != nullptr))
        return;

    BeginMenuTransition(EMenuState::MainMenu);
}

// ============================================================
// 페이지 전환 (Out 애니 → 전환 → In 애니)
// ============================================================

UWidget* UMainMenu::GetPageForState(EMenuState State) const
{
    switch (State)
    {
    case EMenuState::Host:     return HostMenu;
    case EMenuState::Join:     return JoinMenu;
    case EMenuState::Skin:     return SkinMenu;
    case EMenuState::MainMenu:
    default:                   return MainMenu;
    }
}

void UMainMenu::BeginMenuTransition(EMenuState Target)
{
    if (!MenuSwitcher)
        return;

    UWidget* TargetPage = GetPageForState(Target);
    if (!TargetPage)
        return;

    // 이미 그 페이지면 아무것도 안 함.
    if (MenuSwitcher->GetActiveWidget() == TargetPage)
        return;

    // 전환 애니 진행 중이면 무시 (버튼 연타 방지).
    if (bMenuTransitioning)
        return;

    PendingState = Target;

    // 현재(나가는) 페이지 Out 애니 재생 (WBP 구현). 미구현이면 no-op.
    PlayMenuOut(CurrentState);

    if (TransitionDuration > 0.f)
    {
        if (UWorld* World = GetWorld())
        {
            bMenuTransitioning = true;
            World->GetTimerManager().SetTimer(
                MenuTransitionTimer, this, &UMainMenu::CommitMenuTransition, TransitionDuration, false);
            return;
        }
    }

    // 애니 길이 0 또는 월드 없음 → 즉시 전환.
    CommitMenuTransition();
}

void UMainMenu::CommitMenuTransition()
{
    bMenuTransitioning = false;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MenuTransitionTimer);
    }

    // Out 애니가 끝난 지금 실제로 페이지를 바꾸고, 새 페이지 In 애니를 재생.
    if (MenuSwitcher)
    {
        if (UWidget* Page = GetPageForState(PendingState))
        {
            MenuSwitcher->SetActiveWidget(Page);
        }
    }
    CurrentState = PendingState;   // 이제 이 페이지가 화면에 떠 있음 (다음 Out 애니 기준).
    PlayMenuTransition(PendingState);
}

void UMainMenu::QuitPressed()
{
    UWorld* World = GetWorld();
    if (!ensure(World != nullptr))
        return;

    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (!ensure(PlayerController != nullptr))
        return;

    PlayerController->ConsoleCommand("quit");
}