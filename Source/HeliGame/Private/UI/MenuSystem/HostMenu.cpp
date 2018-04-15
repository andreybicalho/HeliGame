// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HostMenu.h"

#include "Components/EditableText.h"
#include "Components/SpinBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"

bool UHostMenu::Initialize()
{
	bool Success = Super::Initialize();
	if (!Success) return false;

	if (!ensure(HostServerButton != nullptr)) return false;
	HostServerButton->OnClicked.AddDynamic(this, &UHostMenu::HostServer);

	if (!ensure(ToggleLANButton != nullptr)) return false;
	ToggleLANButton->OnClicked.AddDynamic(this, &UHostMenu::ToggleLan);

	if (!ensure(LanText != nullptr)) return false;
	LanText->SetText(FText::FromString(FString(TEXT("LAN"))));
	bIsLAN = true;

	if (!ensure(ServerHostName != nullptr)) return false;
	ServerHostName->SetHintText(FText::FromString(FString(TEXT("Your server name"))));
	ServerHostName->SetText(FText::FromString(FString(TEXT("### Birds of War ###"))));

	return true;
}

void UHostMenu::HostServer()
{
	FGameParams GameSessionParams;

	GameSessionParams.SessionName = GameSessionName;
	GameSessionParams.bIsPresence = true;
	GameSessionParams.bIsLAN = bIsLAN;
	GameSessionParams.CustomServerName = ServerHostName->GetText().ToString();
	GameSessionParams.SelectedGameModeName = GameModeComboBox->GetSelectedOption();
	GameSessionParams.SelectedMapName = MapComboBox->GetSelectedOption();
	GameSessionParams.NumberOfPlayers = FCString::Atoi(*NumberOfPlayersComboBox->GetSelectedOption());
	GameSessionParams.RoundTime = RoundTimeSpinBox->GetValue();
	GameSessionParams.WarmupTime = WarmupTimeSpinBox->GetValue();
	GameSessionParams.bAllowFriendFireDamage = true;
	GameSessionParams.TimeBetweenMatches = TimeBetweenMatchesSpinBox->GetValue();
	GameSessionParams.UserId = this->GetOwningLocalPlayer()->GetPreferredUniqueNetId();
	GameSessionParams.NumberOfBots = FCString::Atoi(*NumberOfBotsComboBox->GetSelectedOption());

	if (!ensure(MenuInterface != nullptr)) return;
	MenuInterface->HostGame(GameSessionParams);
}

void UHostMenu::ToggleLan()
{
	if (bIsLAN)
	{
		bIsLAN = false;
		LanText->SetText(FText::FromString(FString(TEXT("Internet"))));
	}
	else
	{
		bIsLAN = true;
		LanText->SetText(FText::FromString(FString(TEXT("LAN"))));
	}
}


