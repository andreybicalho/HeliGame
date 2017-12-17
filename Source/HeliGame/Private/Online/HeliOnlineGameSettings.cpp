// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "HeliOnlineGameSettings.h"
#include "HeliGame.h"


FHeliOnlineSessionSettings::FHeliOnlineSessionSettings(bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers)
{
	NumPublicConnections = MaxNumPlayers;
	if (NumPublicConnections < 0)
	{
		NumPublicConnections = 0;
	}
	NumPrivateConnections = 0;
	bIsLANMatch = bIsLAN;
	bShouldAdvertise = true;
	bAllowJoinInProgress = true;
	bAllowInvites = true;
	bUsesPresence = bIsPresence;
	bAllowJoinViaPresence = true;
	bAllowJoinViaPresenceFriendsOnly = false;
}

void FHeliOnlineSessionSettings::SetMaxNumPlayers(int32 NewMaxNumPlayers)
{
	NumPublicConnections = NewMaxNumPlayers;
}


FHeliOnlineSearchSettings::FHeliOnlineSearchSettings(bool bSearchingLAN, bool bSearchingPresence)
{
	bIsLanQuery = bSearchingLAN;
	MaxSearchResults = 10000;
	PingBucketSize = 999;
	TimeoutInSeconds = 60.f;

	if (bSearchingPresence)
	{
		QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	}
}

FHeliOnlineSearchSettingsEmptyDedicated::FHeliOnlineSearchSettingsEmptyDedicated(bool bSearchingLAN, bool bSearchingPresence) :
	FHeliOnlineSearchSettings(bSearchingLAN, bSearchingPresence)
{
	QuerySettings.Set(SEARCH_DEDICATED_ONLY, true, EOnlineComparisonOp::Equals);
	QuerySettings.Set(SEARCH_EMPTY_SERVERS_ONLY, true, EOnlineComparisonOp::Equals);
}
