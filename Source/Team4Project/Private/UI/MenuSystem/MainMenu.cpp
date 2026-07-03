// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MenuSystem/MainMenu.h"
#include "UObject/ConstructorHelpers.h"

#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"

#include "UI/MenuSystem/ServerRow.h"
#include "Game/PlayerGameInstance.h"

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

    return true;
}

FReply UMainMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
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

    MenuSwitcher->SetActiveWidget(SkinMenu);

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

    PlayMenuTransition(EMenuState::Skin);
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
    MenuSwitcher->SetActiveWidget(HostMenu);
    
    PlayMenuTransition(EMenuState::Host);
}

void UMainMenu::HostServer()
{
    if (MenuInterface != nullptr)
    {
        FString ServerName = ServerHostName->Text.ToString();
        MenuInterface->Host(ServerName);
    }
}

void UMainMenu::SetServerList(TArray<FServerData> ServerNames)
{
    UWorld* World = this->GetWorld();
    if (!ensure(World != nullptr))
        return;

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
    if (SelectedIndex.IsSet() && MenuInterface != nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Selected index %d."), SelectedIndex.GetValue());
        MenuInterface->Join(SelectedIndex.GetValue());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Selected index not set"));
    }
}

void UMainMenu::OpenJoinMenu()
{
    if (!ensure(MenuSwitcher != nullptr))
        return;
    if (!ensure(JoinMenu != nullptr))
        return;
    
    MenuSwitcher->SetActiveWidget(JoinMenu);

    PlayMenuTransition(EMenuState::Join);
    
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
   
    MenuSwitcher->SetActiveWidget(MainMenu);
    
    PlayMenuTransition(EMenuState::MainMenu);
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