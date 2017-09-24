// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/GameState.h"
#include "HeliGameState.generated.h"

class AHeliPlayerState;

/** ranked PlayerState map, created from the GameState */
typedef TMap<int32, TWeakObjectPtr<AHeliPlayerState> > RankedPlayerMap;

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliGameState : public AGameState
{
	GENERATED_BODY()

	// TODO(andrey): make properties private with respectively accessors
public:
	AHeliGameState(const FObjectInitializer& ObjectInitializer);

	/* NetMulticast will send this event to all clients that know about this object, in the case of GameState that means every client. */
	UFUNCTION(Reliable, NetMulticast)
	void BroadcastGameMessage(const FString& NewMessage);

	/** number of teams in current game (doesn't deprecate when no players are left in a team) */
	UPROPERTY(Transient, Replicated)
	int32 NumTeams;

	/** accumulated score per team */
	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	TArray<int32> TeamScores;

	/** time left for warmup / match */
	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	int32 RemainingTime;

	/** name of the server */
	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	FString ServerName;

	/** current game mode */
	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	FString GameModeName;

	/** current map name */
	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	FString MapName;

	/** number of players in game */
	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	int32 MaxNumberOfPlayers;

	/** number of players in game */
	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	int32 MaxRoundTime;

	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	int32 MaxWarmupTime;

	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	uint8 bAllowFriendFireDamage;

	/** gets ranked PlayerState map for specific team */
	void GetRankedMap(int32 TeamIndex, RankedPlayerMap& OutRankedMap) const;

	void RequestFinishAndExitToMainMenu();

	void RequestFinishMatchAndGoToLobbyState();

	void RequestClientsGoToLobbyState();

	TArray<AHeliPlayerState*> GetPlayersStatesFromTeamNumber(int32 TeamNumber);
	
	void RequestEndRoundAndRestartMatch();
};
