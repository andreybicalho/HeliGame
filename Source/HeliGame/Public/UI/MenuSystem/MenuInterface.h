// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "HeliGame.h"
#include "Online.h"
#include "MenuInterface.generated.h"

struct FGameParams
{
	FString CustomServerName;
	FString SelectedGameModeName;
	FString SelectedMapName;
	FName SessionName;
	bool bIsLAN;
	bool bIsPresence;
	TSharedPtr<const FUniqueNetId> UserId;
	int32 NumberOfPlayers;
	int32 BestSessionIdx;
	int32 WarmupTime;
	int32 RoundTime;
	int32 TimeBetweenMatches;
	bool bAllowFriendFireDamage;

	FGameParams()
		: CustomServerName(FString(TEXT("None")))
		, SelectedGameModeName(FString(TEXT("None")))
		, SelectedMapName(FString(TEXT("None")))
		, SessionName(NAME_None)
		, bIsLAN(false)
		, bIsPresence(false)
		, NumberOfPlayers(10)
		, BestSessionIdx(0)
		, WarmupTime(1)
		, RoundTime(999)
		, TimeBetweenMatches(999)
		, bAllowFriendFireDamage(false)
	{
	}
};


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMenuInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class HELIGAME_API IMenuInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual bool HostGame(FGameParams InGameSessionParams) = 0;
	
};
