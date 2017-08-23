#include "OnlineSessionSettings.h"

#pragma once

/**
 * General session settings for a Shooter game
 */
class FHeliOnlineSessionSettings : public FOnlineSessionSettings
{
public:

	FHeliOnlineSessionSettings(bool bIsLAN = false, bool bIsPresence = false, int32 MaxNumPlayers = 4);

	virtual ~FHeliOnlineSessionSettings() {}

	void SetMaxNumPlayers(int32 NewMaxNumPlayers);
};

/**
 * General search setting for a Shooter game
 */
class FHeliOnlineSearchSettings : public FOnlineSessionSearch
{
public:
	FHeliOnlineSearchSettings(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FHeliOnlineSearchSettings() {}
};

/**
 * Search settings for an empty dedicated server to host a match
 */
class FHeliOnlineSearchSettingsEmptyDedicated : public FHeliOnlineSearchSettings
{
public:
	FHeliOnlineSearchSettingsEmptyDedicated(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FHeliOnlineSearchSettingsEmptyDedicated() {}
};