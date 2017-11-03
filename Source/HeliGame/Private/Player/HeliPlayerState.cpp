// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliPlayerState.h"
#include "HeliGame.h"
#include "HeliGameState.h"
#include "HeliGameInstance.h"
#include "HeliPlayerController.h"
#include "HeliLobbyGameState.h"
#include "Helicopter.h"
#include "UObject/CoreOnline.h"
#include "Net/UnrealNetwork.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"



AHeliPlayerState::AHeliPlayerState()
{
	TeamNumber = -1;

	NumKills = 0;
	NumDeaths = 0;

	bQuitter = false;

	RankInCurrentMatch = 0;
	Level = 0;
	bPlayerReady = false;

	bDead = false;
}

void AHeliPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	// TODO(andrey): should it be another player state, only for lobby? and this one should be only for playing?
	AHeliPlayerState* MyPlayerState = Cast<AHeliPlayerState>(PlayerState);
	if (MyPlayerState)
	{
		MyPlayerState->TeamNumber = TeamNumber;
		MyPlayerState->PlayerName = PlayerName;
		MyPlayerState->bPlayerReady = bPlayerReady;
	}
}

/*
* called from server when a client is initialized
*/
void AHeliPlayerState::ClientInitialize(AController* InController)
{
	Super::ClientInitialize(InController);

	// local client will notify server about his new custom name	
	UHeliGameInstance* MyGameInstance = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;
	if (MyGameInstance && !MyGameInstance->CustomPlayerName.IsEmpty())
	{	
		Server_SetPlayerName(MyGameInstance->CustomPlayerName);

		// if we are in lobby, tell other players to update their lobby widget because this one changed its player name
		AHeliLobbyGameState* LobbyGameState = GetWorld() != NULL ? GetWorld()->GetGameState<AHeliLobbyGameState>() : NULL;
		if (LobbyGameState)
		{
			LobbyGameState->bShouldUpdateLobbyWidget++;
		}
	}
}

void AHeliPlayerState::Reset()
{
	Super::Reset();

	NumKills = 0;
	NumDeaths = 0;
	Score = 0;
	bQuitter = false;
	RankInCurrentMatch = 0;
	Level = 0;
	NumHits = 0;
	bDead = false;
}

void AHeliPlayerState::ScoreKill(AHeliPlayerState* Victim, int32 Points)
{
	NumKills++;
	ScorePoints(Points);
}

void AHeliPlayerState::ScoreDeath(AHeliPlayerState* KilledBy, int32 Points)
{
	NumDeaths++;
	ScorePoints(Points);
}

void AHeliPlayerState::ScorePoints(int32 Points)
{
	Score += Points;

	/* Add the score to the global score count */
	AHeliGameState* const MyGameState = Cast<AHeliGameState>(GetWorld()->GetGameState());
	if (MyGameState)
	{
		// making sure we dont screw up arrays
		if (TeamNumber >= MyGameState->TeamScores.Num())
		{
			MyGameState->TeamScores.AddZeroed(TeamNumber - MyGameState->TeamScores.Num() + 1);
		}

		MyGameState->TeamScores[TeamNumber] += Points;
	}
}

void AHeliPlayerState::ScoreHit(int Points)
{
	NumHits++;
	ScorePoints(Points);
}


void AHeliPlayerState::SetTeamNumber(int32 NewTeamNumber)
{
	TeamNumber = NewTeamNumber;
}


int32 AHeliPlayerState::GetTeamNumber() const
{
	return TeamNumber;
}

int32 AHeliPlayerState::GetKills() const
{
	return NumKills;
}

int32 AHeliPlayerState::GetDeaths() const
{
	return NumDeaths;
}


float AHeliPlayerState::GetScore() const
{
	return Score;
}

float AHeliPlayerState::GetRankInCurrentMatch()
{
	return RankInCurrentMatch;
}

void AHeliPlayerState::SetRankInCurrentMatch(float NewRankInCurrentMatch)
{
	RankInCurrentMatch = NewRankInCurrentMatch;
}

float AHeliPlayerState::GetLevel()
{
	return Level;
}

void AHeliPlayerState::SetLevel(float NewLevel)
{
	Level = NewLevel;
}

FString AHeliPlayerState::GetPlayerName() const
{
	return PlayerName;
}

void AHeliPlayerState::OnRep_TeamNumber()
{	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{		
		AHeliPlayerController* myPC = Cast<AHeliPlayerController>(*It);
		if (myPC)
		{
			myPC->UpdateTeamNumber(TeamNumber);						
		}
	}
}

void AHeliPlayerState::SetQuitter(bool bInQuitter)
{
	bQuitter = bInQuitter;
}

bool AHeliPlayerState::IsQuitter() const
{
	return bQuitter;
}

void AHeliPlayerState::InformAboutKill_Implementation(class AHeliPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AHeliPlayerState* KilledPlayerState)
{
	//id can be null for bots
	if (KillerPlayerState->UniqueId.IsValid())
	{
		//search for the actual killer before calling OnKill()	
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			AHeliPlayerController* TestPC = Cast<AHeliPlayerController>(*It);
			if (TestPC && TestPC->IsLocalController())
			{
				// a local player might not have an ID if it was created with CreateDebugPlayer.
				ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(TestPC->Player);
				TSharedPtr<const FUniqueNetId> LocalID = LocalPlayer->GetCachedUniqueNetId();
				if (LocalID.IsValid() && *LocalPlayer->GetCachedUniqueNetId() == *KillerPlayerState->UniqueId)
				{
					TestPC->OnKill();
				}
			}
		}
	}
}

void AHeliPlayerState::BroadcastDeath_Implementation(class AHeliPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AHeliPlayerState* KilledPlayerState)
{
	// all local players get death messages so they can update their huds.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		// all local players get death messages so they can update their huds.
		AHeliPlayerController* TestPC = Cast<AHeliPlayerController>(*It);
		if (TestPC && TestPC->IsLocalController())
		{
			TestPC->OnDeathMessage(KillerPlayerState, this, KillerDamageType);
		}
	}
}


bool AHeliPlayerState::Server_SetPlayerName_Validate(const FString& NewPlayerName)
{
	return true;
}

void AHeliPlayerState::Server_SetPlayerName_Implementation(const FString& NewPlayerName)
{
	SetPlayerName(NewPlayerName);
}

void AHeliPlayerState::Client_UpdateLobbyWidget_Implementation()
{
	UHeliGameInstance* MyGameInstance = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;
	
	if (MyGameInstance)
	{
		// updates settings
		MyGameInstance->bShouldUpdateLobbyWidget = true;
		// updates player list
		MyGameInstance->bRequiresWidgetUpdate = true;
	}
}

bool AHeliPlayerState::Server_UpdateEverybodyLobbyWidget_Validate()
{
	return true;
}

void AHeliPlayerState::Server_UpdateEverybodyLobbyWidget_Implementation()
{
	// enforce other player to update their lobby widget if we are in lobby game mode
	AHeliLobbyGameState* MyGameState = Cast<AHeliLobbyGameState>(GetWorld()->GetGameState());
	if (MyGameState)
	{
		MyGameState->bShouldUpdateLobbyWidget++;
	}
	
}

void AHeliPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();
	
	// enforce other player to update their lobby widget if we are in lobby game mode
	AHeliLobbyGameState* MyGameState = Cast<AHeliLobbyGameState>(GetWorld()->GetGameState());
	if (MyGameState)
	{
		MyGameState->bShouldUpdateLobbyWidget++;
	}

}


bool AHeliPlayerState::Server_SwitchTeams_Validate()
{
	return true;
}

void AHeliPlayerState::Server_SwitchTeams_Implementation()
{
	if (TeamNumber == 0)
	{
		TeamNumber = 1;
	}
	else
	{
		TeamNumber = 0;
	}
}

bool AHeliPlayerState::Server_SetPlayerReady_Validate(bool bNewPlayerReady)
{
	return true;
}

void AHeliPlayerState::Server_SetPlayerReady_Implementation(bool bNewPlayerReady)
{
	bPlayerReady = bNewPlayerReady;

	// tell other clients to update their lobby widget
	AHeliLobbyGameState* MyGameState = Cast<AHeliLobbyGameState>(GetWorld()->GetGameState());
	if (MyGameState)
	{
		MyGameState->bShouldUpdateLobbyWidget++;
	}

	// updates server's lobby widget 
	UHeliGameInstance* MyGameInstance = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;

	if (MyGameInstance)
	{
		// updates settings
		MyGameInstance->bShouldUpdateLobbyWidget = true;
		// updates player list
		MyGameInstance->bRequiresWidgetUpdate = true;
	}
}

//                                  replication list
void AHeliPlayerState::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeliPlayerState, NumKills);
	DOREPLIFETIME(AHeliPlayerState, NumDeaths);
	DOREPLIFETIME(AHeliPlayerState, TeamNumber); 
	DOREPLIFETIME(AHeliPlayerState, Level);
	DOREPLIFETIME(AHeliPlayerState, bPlayerReady); 
	DOREPLIFETIME(AHeliPlayerState, bDead);
}