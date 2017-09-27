// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliGameModeLobby.h"
#include "HeliGame.h"
#include "HeliPlayerController.h"
#include "HeliGameSession.h"
#include "HeliPlayerState.h"
#include "HeliLobbyGameState.h"
#include "HeliGameInstance.h"
#include "Engine/World.h"


AHeliGameModeLobby::AHeliGameModeLobby(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerControllerClass = AHeliPlayerController::StaticClass();

	PlayerStateClass = AHeliPlayerState::StaticClass();

	GameStateClass = AHeliLobbyGameState::StaticClass();

	bUseSeamlessTravel = true;

	NumTeams = 2;
}

void AHeliGameModeLobby::InitGameState()
{
	Super::InitGameState();

	AHeliLobbyGameState* const MyGameState = Cast<AHeliLobbyGameState>(GameState);
	if (MyGameState)
	{
		MyGameState->NumTeams = NumTeams;

		// setting custom player configuration in the game state when hosting a match like custom server name, selected game mode, selected map, etc.
		UHeliGameInstance* MyGameInstance = Cast<UHeliGameInstance>(GetGameInstance());
		if (MyGameInstance)
		{
			MyGameState->ServerName = MyGameInstance->CustomServerName;
			MyGameState->GameModeName = MyGameInstance->SelectedGameMode;
			MyGameState->MapName = MyGameInstance->SelectedMapName;
			MyGameState->MaxNumberOfPlayers = MyGameInstance->MaxNumberOfPlayers;
			MyGameState->MaxRoundTime = MyGameInstance->RoundTime;
			MyGameState->MaxWarmupTime = MyGameInstance->WarmupTime;
		}

	}
}

void AHeliGameModeLobby::PostLogin(APlayerController* NewPlayer)
{
	// Place player on a team before Super (VoIP team based init, findplayerstart, etc)
	AHeliPlayerState* NewPlayerState = CastChecked<AHeliPlayerState>(NewPlayer->PlayerState);
	const int32 TeamNum = ChooseTeam(NewPlayerState);
	NewPlayerState->SetTeamNumber(TeamNum);



	Super::PostLogin(NewPlayer);

	AHeliPlayerController* NewPC = Cast<AHeliPlayerController>(NewPlayer);
	// notify new player if match is already in progress
	if (NewPC && IsMatchInProgress())
	{
		NewPC->ClientGameStarted();
		NewPC->ClientStartOnlineGame();
	}

	// debug players roles
	//PrintNetModeInfo(NewPlayer->GetNetMode());
	//PrintRoleInfo(NewPlayer->Role);
	//PrintController(NewPlayer);

	// if i'm the host, (hosting a match, e.g., the server) set my custom states (playername, etc...)	
	UHeliGameInstance* MyGameInstance = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;		
	//if (NewPlayer->GetNetMode() == NM_ListenServer && MyGameInstance)	
	//if (NewPlayer->IsLocalController() && MyGameInstance)
	if (NewPlayer->IsLocalPlayerController() && MyGameInstance) // using IsLocalPlayerController() since it is controlled by humans 
	{		
		if (!MyGameInstance->CustomPlayerName.IsEmpty())
		{
			NewPlayer->PlayerState->SetPlayerName(MyGameInstance->CustomPlayerName);
		}		
	}


	
	AHeliLobbyGameState* MyGameState = Cast<AHeliLobbyGameState>(GameState);
	if (MyGameState)
	{
		// force the replication of players custom atributes on clients for displaying name, etc on lobby widget
		MyGameState->bShouldUpdateLobbyWidget++;
	}
}


int32 AHeliGameModeLobby::ChooseTeam(AHeliPlayerState* ForPlayerState) const
{
	// choose one of the teams that are least populated

	TArray<int32> TeamBalance;
	TeamBalance.AddZeroed(NumTeams);

	// get current team balance
	for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
	{
		AHeliPlayerState const* const TestPlayerState = Cast<AHeliPlayerState>(GameState->PlayerArray[i]);
		if (TestPlayerState && TestPlayerState != ForPlayerState && TeamBalance.IsValidIndex(TestPlayerState->GetTeamNumber()))
		{
			TeamBalance[TestPlayerState->GetTeamNumber()]++;
		}
	}

	// find least populated one
	int32 BestTeamScore = TeamBalance[0];
	for (int32 i = 1; i < TeamBalance.Num(); i++)
	{
		if (BestTeamScore > TeamBalance[i])
		{
			BestTeamScore = TeamBalance[i];
		}
	}

	// there could be more than one...
	TArray<int32> BestTeams;
	for (int32 i = 0; i < TeamBalance.Num(); i++)
	{
		if (TeamBalance[i] == BestTeamScore)
		{
			BestTeams.Add(i);
		}
	}

	// get random from best list
	const int32 RandomBestTeam = BestTeams[FMath::RandHelper(BestTeams.Num())];
	return RandomBestTeam;
}

//AActor* AHeliGameModeLobby::ChoosePlayerStart_Implementation(AController* Player)
//{
//	TArray<APlayerStart*> PreferredSpawns;
//	TArray<APlayerStart*> FallbackSpawns;
//
//	APlayerStart* BestStart = NULL;
//	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
//	{
//		APlayerStart* TestSpawn = *It;
//		if (TestSpawn->IsA<APlayerStartPIE>())
//		{
//			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
//			BestStart = TestSpawn;
//			break;
//		}
//		else
//		{
//			PreferredSpawns.Add(TestSpawn);			
//		}
//	}
//
//
//	if (BestStart == NULL)
//	{
//		if (PreferredSpawns.Num() > 0)
//		{
//			BestStart = PreferredSpawns[FMath::RandHelper(PreferredSpawns.Num())];
//		}
//		else if (FallbackSpawns.Num() > 0)
//		{
//			BestStart = FallbackSpawns[FMath::RandHelper(FallbackSpawns.Num())];
//		}
//	}
//
//	return BestStart ? BestStart : Super::ChoosePlayerStart_Implementation(Player);
//}

/** Returns game session class to use */
TSubclassOf<AGameSession> AHeliGameModeLobby::GetGameSessionClass() const
{
	return AHeliGameSession::StaticClass();
}


void AHeliGameModeLobby::RequestFinishAndExitToMainMenu()
{
	//FinishMatch();

	AHeliPlayerController* LocalPrimaryController = nullptr;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AHeliPlayerController* Controller = Cast<AHeliPlayerController>(*Iterator);

		if (Controller == NULL)
		{
			continue;
		}

		if (!Controller->IsLocalController())
		{			
			const FString RemoteReturnReason = TEXT("AHeliGameModeLobby::RequestFinishAndExitToMainMenu: controller is remote");
			Controller->ClientReturnToMainMenu(RemoteReturnReason);
		}
		else
		{
			LocalPrimaryController = Controller;
		}
	}

	// GameInstance should be calling this from an EndState.  So call the PC function that performs cleanup, not the one that sets GI state.
	if (LocalPrimaryController != NULL)
	{
		LocalPrimaryController->HandleReturnToMainMenu();
	}
}

// tell all clients to go to lobby state
void AHeliGameModeLobby::RequestClientsGoToLobbyState()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AHeliPlayerController* Controller = Cast<AHeliPlayerController>(*Iterator);

		if (Controller && !Controller->IsLocalController())
		{
			Controller->ClientReturnToLobbyState();
		}
	}
}

void AHeliGameModeLobby::RequestClientsGoToPlayingState()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AHeliPlayerController* Controller = Cast<AHeliPlayerController>(*Iterator);

		if (Controller && !Controller->IsLocalController())
		{
			Controller->ClientGoToPlayingState();
		}
	}
}