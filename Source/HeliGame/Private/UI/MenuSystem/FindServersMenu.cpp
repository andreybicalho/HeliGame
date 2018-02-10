// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "FindServersMenu.h"
#include "ServerRow.h"

#include "UObject/ConstructorHelpers.h"
#include "Components/EditableText.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

UFindServersMenu::UFindServersMenu(const FObjectInitializer & ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<UUserWidget> ServerRowBPClass(TEXT("/Game/MenuSystem/WBP_ServerRow"));
	if (!ensure(ServerRowBPClass.Class != nullptr)) return;

	ServerRowClass = ServerRowBPClass.Class;
}

bool UFindServersMenu::Initialize()
{
	bool Success = Super::Initialize();
	if (!Success) return false;

	if (!ensure(FindServersButton != nullptr)) return false;
	FindServersButton->OnClicked.AddDynamic(this, &UFindServersMenu::FindServers);

	if (!ensure(JoinServerButton != nullptr)) return false;
	JoinServerButton->OnClicked.AddDynamic(this, &UFindServersMenu::JoinServer);

	if (!ensure(ToggleLANButton != nullptr)) return false;
	ToggleLANButton->OnClicked.AddDynamic(this, &UFindServersMenu::ToggleLan);

	if (!ensure(LanText != nullptr)) return false;
	LanText->SetText(FText::FromString(FString(TEXT("LAN"))));
	bIsLAN = true;

	return true;
}

void UFindServersMenu::FindServers()
{
	if (!ensure(MenuInterface != nullptr)) return;
	MenuInterface->FindServers(GetOwningLocalPlayer(), bIsLAN);
}

void UFindServersMenu::JoinServer()
{
	if (!ensure(MenuInterface != nullptr)) return;
	if (AvailableServers.IsValidIndex(SelectedServerIndex))
	{
		MenuInterface->JoinServer(GetOwningLocalPlayer(), AvailableServers[SelectedServerIndex].SearchResultsIndex);
	}
}

void UFindServersMenu::ToggleLan()
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

void UFindServersMenu::SetSelectServerIndex(int32 ServerIndex)
{
	SelectedServerIndex = ServerIndex;
}

void UFindServersMenu::SetAvailableServerList(TArray<FServerData> AvailableServersData)
{
	AvailableServers = AvailableServersData;

	RefreshServerList();
}

void UFindServersMenu::RefreshServerList()
{
	if (AvailableServers.Num() > 0)
	{
		ServerList->ClearChildren();

		uint32 serverIndex = 0;
		for (const FServerData& serverData : AvailableServers)
		{
			UServerRow* Row = CreateWidget<UServerRow>(GetOwningPlayer(), ServerRowClass);
			if (!ensure(Row != nullptr)) return;

			Row->SetServerName(serverData.ServerName);
			Row->SetMapName(serverData.MapName);
			FString playersFraction = FString::Printf(TEXT("%s/%s"), *serverData.CurrentPlayers, *serverData.MaxPlayers);
			Row->SetNumberOfPlayersFraction(playersFraction);
			Row->SetGameModeName(serverData.GameType);
			Row->SetPing(serverData.Ping);
			Row->Setup(this, serverIndex);
			++serverIndex;

			ServerList->AddChild(Row);
		}
	}
}