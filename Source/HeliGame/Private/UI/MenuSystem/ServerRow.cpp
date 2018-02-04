// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "ServerRow.h"
#include "FindServersMenu.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"


void UServerRow::Setup(UFindServersMenu* InParent, uint32 InIndex)
{
	Parent = InParent;
	Index = InIndex;
	RowButton->OnClicked.AddDynamic(this, &UServerRow::OnClicked);
}

void UServerRow::OnClicked()
{
	Parent->SetSelectServerIndex(Index);
}

void UServerRow::SetServerName(FString InServerName)
{
	if (!ensure(ServerName != nullptr)) return;
	ServerName->SetText(FText::FromString(InServerName));
}

void UServerRow::SetMapName(FString InMapName)
{
	if (!ensure(MapName != nullptr)) return;
	MapName->SetText(FText::FromString(InMapName));
}

void UServerRow::SetGameModeName(FString InGameModeName)
{
	if (!ensure(GameModeName != nullptr)) return;
	GameModeName->SetText(FText::FromString(InGameModeName));
}

void UServerRow::SetNumberOfPlayersFraction(FString InNumberOfPlayersFraction)
{
	if (!ensure(NumberOfPlayersFraction != nullptr)) return;
	NumberOfPlayersFraction->SetText(FText::FromString(InNumberOfPlayersFraction));
}

void UServerRow::SetPing(FString InPing)
{
	if (!ensure(Ping != nullptr)) return;
	Ping->SetText(FText::FromString(InPing));
}
