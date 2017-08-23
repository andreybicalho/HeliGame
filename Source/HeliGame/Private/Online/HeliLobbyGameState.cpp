// Fill out your copyright notice in the Description page of Project Settings.

/*
 Tips:
 1 - check if we are the server
 if (AuthorityGameMode)
 {
 // we are server, tell the gamemode
 AHeliLobbyGameState* const GameMode = Cast<AHeliLobbyGameState>(AuthorityGameMode);
 if (GameMode)
 {
	// do stuff
 }

 2 - 
*/

#include "HeliGame.h"
#include "HeliLobbyGameState.h"
#include "HeliPlayerState.h"
#include "HeliGameInstance.h"
#include "HeliGameModeLobby.h"
#include "HeliPlayerController.h"



AHeliLobbyGameState::AHeliLobbyGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NumTeams = 0;
	MaxNumberOfPlayers = 10;
	MaxRoundTime = 0;
	MaxWarmupTime = 0;
	ServerName = FString(TEXT("Unknown_HeliLobbyGameState"));
	GameModeName = FString(TEXT("Unknown_HeliLobbyGameState"));
	MapName = FString(TEXT("Unknown_HeliLobbyGameState"));
	bAllowFriendFireDamage = false;
}

void AHeliLobbyGameState::GetRankedMapByLevel(int32 TeamIndex, RankedPlayerMap& OutRankedMap) const
{
	OutRankedMap.Empty();

	//first, we need to go over all the PlayerStates, grab their level, and rank them
	TMultiMap<int32, AHeliPlayerState*> SortedMap;
	for (int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		int32 Score = 0;
		AHeliPlayerState* CurPlayerState = Cast<AHeliPlayerState>(PlayerArray[i]);
		if (CurPlayerState && (CurPlayerState->GetTeamNumber() == TeamIndex))
		{
			SortedMap.Add(FMath::TruncToInt(CurPlayerState->Level), CurPlayerState);
		}
	}

	//sort by the keys
	SortedMap.KeySort(TGreater<int32>());

	//now, add them back to the ranked map
	OutRankedMap.Empty();

	int32 Rank = 0;
	for (TMultiMap<int32, AHeliPlayerState*>::TIterator It(SortedMap); It; ++It)
	{
		OutRankedMap.Add(Rank++, It.Value());
	}

}


void AHeliLobbyGameState::OnRep_bShouldUpdateLobbyWidget()
{
	// when it bShouldUpdateLobbySettingsInWidget is finally replicated we must inform all clients they should update theirs lobby widget
	for (int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		AHeliPlayerState* CurPlayerState = Cast<AHeliPlayerState>(PlayerArray[i]);
		if (CurPlayerState)
		{
			CurPlayerState->Client_UpdateLobbyWidget();
		}
	}
}

// check if all players are ready
bool AHeliLobbyGameState::isAllPlayersReady()
{
	bool result = false;
	for (int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		AHeliPlayerState* CurPlayerState = Cast<AHeliPlayerState>(PlayerArray[i]);
		if (CurPlayerState && CurPlayerState->bPlayerReady)
		{
			result = true;
		}
		else
		{
			return false;
		}
	}

	return result;
}

void AHeliLobbyGameState::RequestFinishAndExitToMainMenu()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AHeliGameModeLobby* const GameMode = Cast<AHeliGameModeLobby>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->RequestFinishAndExitToMainMenu();
		}
	}
	else
	{
		// we are client, handle our own business
		AHeliPlayerController* const PrimaryPC = Cast<AHeliPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
		if (PrimaryPC)
		{
			check(PrimaryPC->GetNetMode() == ENetMode::NM_Client);
			PrimaryPC->HandleReturnToMainMenu();
		}
	}

}

void AHeliLobbyGameState::RequestClientsBeginLobbyMenuState()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AHeliGameModeLobby* const GameMode = Cast<AHeliGameModeLobby>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->RequestClientsGoToLobbyState();
		}
	}
}

void AHeliLobbyGameState::RequestClientsBeginPlayingState()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AHeliGameModeLobby* const GameMode = Cast<AHeliGameModeLobby>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->RequestClientsGoToPlayingState();
		}
	}
}


void AHeliLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeliLobbyGameState, NumTeams);
	DOREPLIFETIME(AHeliLobbyGameState, ServerName);
	DOREPLIFETIME(AHeliLobbyGameState, GameModeName);
	DOREPLIFETIME(AHeliLobbyGameState, MapName);
	DOREPLIFETIME(AHeliLobbyGameState, MaxNumberOfPlayers);
	DOREPLIFETIME(AHeliLobbyGameState, MaxRoundTime);
	DOREPLIFETIME(AHeliLobbyGameState, MaxWarmupTime); 
	DOREPLIFETIME(AHeliLobbyGameState, bShouldUpdateLobbyWidget); 
	DOREPLIFETIME(AHeliLobbyGameState, bAllowFriendFireDamage);
}