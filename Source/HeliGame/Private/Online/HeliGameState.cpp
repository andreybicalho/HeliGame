// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliGameState.h"
#include "HeliGame.h"
#include "HeliPlayerController.h"
#include "HeliGameInstance.h"
#include "HeliPlayerState.h"
#include "HeliGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"


AHeliGameState::AHeliGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NumTeams = 0;
	RemainingTime = 0;
	MaxNumberOfPlayers = 10;
	MaxRoundTime = 0;
	ServerName = FString(TEXT("Unknown_GameState"));
	GameModeName = FString(TEXT("Unknown_GameState"));
	MapName = FString(TEXT("Unknown_GameState"));
	bAllowFriendFireDamage = false;
}


/* As with Server side functions, NetMulticast functions have a _Implementation body */
void AHeliGameState::BroadcastGameMessage_Implementation(const FString& NewMessage)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		AHeliPlayerController* MyController = Cast<AHeliPlayerController>(*It);
		if (MyController && MyController->IsLocalController())
		{
			// TODO: message HUD 
			/*ASHUD* MyHUD = Cast<ASHUD>(MyController->GetHUD());
			if (MyHUD)
			{
				MyHUD->MessageReceived(NewMessage);
			}*/
		}
	}
}

void AHeliGameState::RequestFinishAndExitToMainMenu()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AHeliGameMode* const GameMode = Cast<AHeliGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->RequestFinishAndExitToMainMenu();
		}
	}
	else
	{
		// we are client, handle our own business
		UHeliGameInstance* GameInstance = Cast<UHeliGameInstance>(GetGameInstance());
		if (GameInstance)
		{
			GameInstance->RemoveSplitScreenPlayers();
		}

		AHeliPlayerController* const PrimaryPC = Cast<AHeliPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
		if (PrimaryPC)
		{
			check(PrimaryPC->GetNetMode() == ENetMode::NM_Client);
			PrimaryPC->HandleReturnToMainMenu();
		}
	}

}

void AHeliGameState::RequestFinishMatchAndGoToLobbyState()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AHeliGameMode* const GameMode = Cast<AHeliGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->FinishMatch();
			GameMode->RequestClientsGoToLobbyState();
		}
	}
}

void AHeliGameState::RequestClientsGoToLobbyState()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AHeliGameMode* const GameMode = Cast<AHeliGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->RequestClientsGoToLobbyState();
		}
	}
}


void AHeliGameState::GetRankedMap(int32 TeamIndex, RankedPlayerMap& OutRankedMap) const
{
	OutRankedMap.Empty();

	//first, we need to go over all the PlayerStates, grab their score, and rank them
	TMultiMap<int32, AHeliPlayerState*> SortedMap;
	for (int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		int32 Score = 0;
		AHeliPlayerState* CurPlayerState = Cast<AHeliPlayerState>(PlayerArray[i]);
		if (CurPlayerState && (CurPlayerState->GetTeamNumber() == TeamIndex))
		{
			SortedMap.Add(FMath::TruncToInt(CurPlayerState->Score), CurPlayerState);
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

TArray<AHeliPlayerState*> AHeliGameState::GetPlayersStatesFromTeamNumber(int32 TeamNumber)
{
	TArray<AHeliPlayerState*> PlayersStates;

	for (int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		AHeliPlayerState* CurPlayerState = Cast<AHeliPlayerState>(PlayerArray[i]);
		if(CurPlayerState && CurPlayerState->GetTeamNumber() == TeamNumber)
		{			
			PlayersStates.Add(CurPlayerState);
		}
		
	}

	return PlayersStates;

}

void AHeliGameState::RequestEndRoundAndRestartMatch()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AHeliGameMode* const GameMode = Cast<AHeliGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->FinishMatch();
			GameMode->RestartGame();
		}
	}
}

void AHeliGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeliGameState, NumTeams);
	DOREPLIFETIME(AHeliGameState, TeamScores);
	DOREPLIFETIME(AHeliGameState, RemainingTime);
	DOREPLIFETIME(AHeliGameState, ServerName);
	DOREPLIFETIME(AHeliGameState, GameModeName);
	DOREPLIFETIME(AHeliGameState, MapName); 
	DOREPLIFETIME(AHeliGameState, MaxNumberOfPlayers);
	DOREPLIFETIME(AHeliGameState, MaxRoundTime); 
	DOREPLIFETIME(AHeliGameState, MaxWarmupTime);
	DOREPLIFETIME(AHeliGameState, bAllowFriendFireDamage);
}