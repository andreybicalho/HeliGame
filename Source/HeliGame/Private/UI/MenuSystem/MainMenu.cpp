// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "MainMenu.h"
#include "HostMenu.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"

UMainMenu::UMainMenu(const FObjectInitializer & ObjectInitializer)
{
}

bool UMainMenu::Initialize()
{
	bool Success = Super::Initialize();
	if (!Success) return false;

	if (!ensure(HostButton != nullptr)) return false;
	HostButton->OnClicked.AddDynamic(this, &UMainMenu::OpenHostMenu);

	if (!ensure(ServersButton != nullptr)) return false;
	ServersButton->OnClicked.AddDynamic(this, &UMainMenu::OpenServersMenu);

	if (!ensure(OptionsButton != nullptr)) return false;
	OptionsButton->OnClicked.AddDynamic(this, &UMainMenu::OpenOptionsMenu);

	if (!ensure(AboutButton != nullptr)) return false;
	AboutButton->OnClicked.AddDynamic(this, &UMainMenu::OpenAboutMenu);

	if (!ensure(QuitButton != nullptr)) return false;
	QuitButton->OnClicked.AddDynamic(this, &UMainMenu::QuitGame);

	if (!ensure(HostMenu != nullptr)) return false;

	return true;
}

void UMainMenu::OpenMainMenu()
{
	if (!ensure(MenuSwitcher != nullptr)) return;
	if (!ensure(MainMenu != nullptr)) return;
	MenuSwitcher->SetActiveWidget(MainMenu);
}

void UMainMenu::OpenHostMenu()
{
	if (!ensure(MenuSwitcher != nullptr)) return;
	if (!ensure(HostMenu != nullptr)) return;
	HostMenu->SetMenuInterface(this->GetMenuInterface());
	MenuSwitcher->SetActiveWidget(HostMenu);
}

void UMainMenu::OpenServersMenu()
{
	if (!ensure(MenuSwitcher != nullptr)) return;
	if (!ensure(ServersMenu != nullptr)) return;
	MenuSwitcher->SetActiveWidget(ServersMenu);
}

void UMainMenu::OpenOptionsMenu()
{
}

void UMainMenu::OpenAboutMenu()
{
}

void UMainMenu::QuitGame()
{
	UWorld* World = GetWorld();
	if (!ensure(World != nullptr)) return;

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!ensure(PlayerController != nullptr)) return;

	PlayerController->ConsoleCommand("Quit");
}