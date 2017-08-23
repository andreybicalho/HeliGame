// Fill out your copyright notice in the Description page of Project Settings.

#include "HeliGame.h"
#include "HeliGameModeTDM.h"
#include "HeliPlayerState.h"
#include "HeliGameState.h"
#include "HeliTeamStart.h"
#include "HeliGameInstance.h"

AHeliGameModeTDM::AHeliGameModeTDM(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NumTeams = 2;
	bDelayedStart = true;

	WarmupTime = 5;
	RoundTime = 300;
	TimeBetweenMatches = 15;
	KillScore = 100;
	DeathScore = -1;

	bUseSeamlessTravel = true;

	bAllowRespawn = false;
}

void AHeliGameModeTDM::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	if (!bAllowRespawn) {
		GetWorldTimerManager().SetTimer(TimerHandle_CompetitiveRoundManagerTimer, this, &AHeliGameModeTDM::CompetitiveRoundManager, GetWorldSettings()->GetEffectiveTimeDilation(), true);
	}
}


void AHeliGameModeTDM::CompetitiveRoundManager()
{		
	TArray<bool> bAnyPlayerAliveFromTeam;
	bAnyPlayerAliveFromTeam.Init(false, NumTeams);


	for (int32 TeamNumberIndex = 0; TeamNumberIndex < NumTeams; TeamNumberIndex++)
	{		
		AHeliGameState* MyGameState = Cast<AHeliGameState>(GameState);
		if (MyGameState)
		{
			TArray<AHeliPlayerState*> PlayersStates = MyGameState->GetPlayersStatesFromTeamNumber(TeamNumberIndex);

			for (int32 PlayerIndex = 0; PlayerIndex < PlayersStates.Num(); PlayerIndex++)
			{
				if (!PlayersStates[PlayerIndex]->bDead)
				{
					bAnyPlayerAliveFromTeam[TeamNumberIndex] = true;
				}
			}
		}				
	}

	// TODO(andrey): should finish the match if there aren't any players alive?
	bool bShouldFinishMatch = bAnyPlayerAliveFromTeam.Contains(false);
	//bool bShouldFinishMatch = false;

	if(bShouldFinishMatch)
	{
		if (IsMatchInProgress())
		{
			//FinishMatch();
		}
		else if (GetMatchState() == MatchState::WaitingPostMatch)
		{
			RestartGame();
		}
		else if (GetMatchState() == MatchState::WaitingToStart)
		{
			StartMatch();
		}
	}
}

void AHeliGameModeTDM::PostLogin(APlayerController* NewPlayer)
{
	// Place player on a team before Super (VoIP team based init, findplayerstart, etc)
	AHeliPlayerState* NewPlayerState = CastChecked<AHeliPlayerState>(NewPlayer->PlayerState);
	const int32 TeamNum = ChooseTeam(NewPlayerState);
	NewPlayerState->SetTeamNumber(TeamNum);

	Super::PostLogin(NewPlayer);
}

// TODO(andrey): remove choose team since we choose team in lobby
int32 AHeliGameModeTDM::ChooseTeam(AHeliPlayerState* ForPlayerState) const
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

void AHeliGameModeTDM::InitGameState()
{
	Super::InitGameState();

	AHeliGameState* const MyGameState = Cast<AHeliGameState>(GameState);
	if (MyGameState)
	{
		MyGameState->NumTeams = NumTeams;
		MyGameState->ServerName = ServerName;
		MyGameState->GameModeName = GetGameModeName();
		MyGameState->MapName = MapName;
		MyGameState->MaxNumberOfPlayers = MaxNumberOfPlayers;
		MyGameState->MaxRoundTime = RoundTime;
		MyGameState->MaxWarmupTime = WarmupTime;
		MyGameState->bAllowFriendFireDamage = bAllowFriendlyFireDamage;
	}
}


bool AHeliGameModeTDM::CanDealDamage(AHeliPlayerState* DamageInstigator, class AHeliPlayerState* DamagedPlayer) const
{
	// do not allow damage to self
	if (DamagedPlayer == DamageInstigator)
		return false;
	
	// check if friendly fire is allowed
	if (DamagedPlayer->GetTeamNumber() == DamageInstigator->GetTeamNumber())
		return DamageInstigator && DamagedPlayer && bAllowFriendlyFireDamage;

	return DamageInstigator && DamagedPlayer;
}


void AHeliGameModeTDM::DetermineMatchWinner()
{
	AHeliGameState const* const MyGameState = Cast<AHeliGameState>(GameState);
	int32 BestScore = MAX_uint32;
	int32 BestTeam = -1;
	int32 NumBestTeams = 1;

	for (int32 i = 0; i < MyGameState->TeamScores.Num(); i++)
	{
		const int32 TeamScore = MyGameState->TeamScores[i];
		if (BestScore < TeamScore)
		{
			BestScore = TeamScore;
			BestTeam = i;
			NumBestTeams = 1;
		}
		else if (BestScore == TeamScore)
		{
			NumBestTeams++;
		}
	}

	WinnerTeam = (NumBestTeams == 1) ? BestTeam : NumTeams;
}

bool AHeliGameModeTDM::IsWinner(AHeliPlayerState* PlayerState) const
{
	return PlayerState && !PlayerState->IsQuitter() && PlayerState->GetTeamNumber() == WinnerTeam;
}

bool AHeliGameModeTDM::IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const
{
	if (Player)
	{
		AHeliTeamStart* TeamStart = Cast<AHeliTeamStart>(SpawnPoint);
		AHeliPlayerState* PlayerState = Cast<AHeliPlayerState>(Player->PlayerState);

		if ((PlayerState && TeamStart && TeamStart->SpawnTeam != PlayerState->GetTeamNumber()) || TeamStart->isTaken) 
		{
			return false;
		}
	}

	return Super::IsSpawnpointAllowed(SpawnPoint, Player);
}

void AHeliGameModeTDM::EndGame()
{
	// TODO: end game, end session, take all players to the main menu
}

void AHeliGameModeTDM::setAllowFriendlyFireDamage(bool bNewAllowFriendlyFireDamage)
{
	bAllowFriendlyFireDamage = bNewAllowFriendlyFireDamage;
}

bool AHeliGameModeTDM::IsImmediatelyPlayerRestartAllowedAfterDeath()
{
	return true;
}

FString AHeliGameModeTDM::GetGameModeName()
{
	return FString(TEXT("Team Deathmatch"));
}