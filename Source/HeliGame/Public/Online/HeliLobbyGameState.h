// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "GameFramework/GameState.h"
#include "HeliLobbyGameState.generated.h"

class AHeliPlayerState;

/** ranked PlayerState map, created from the GameState */
typedef TMap<int32, TWeakObjectPtr<AHeliPlayerState> > RankedPlayerMap;

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliLobbyGameState : public AGameState
{
	GENERATED_BODY()

public:
	AHeliLobbyGameState(const FObjectInitializer& ObjectInitializer);
	
	/** number of teams in current game (doesn't deprecate when no players are left in a team) */
	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	int32 NumTeams;

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

	/* warmup time */
	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	int32 MaxWarmupTime;

	UPROPERTY(BlueprintReadWrite, Transient, Replicated, Category = "GameSettings")
	uint8 bAllowFriendFireDamage;

	UPROPERTY(BlueprintReadWrite, Transient, ReplicatedUsing=OnRep_bShouldUpdateLobbyWidget, Category = "Lobby")
	int32 bShouldUpdateLobbyWidget;

	// check if all players are ready
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	bool isAllPlayersReady();

	UFUNCTION()
	void OnRep_bShouldUpdateLobbyWidget();

	/** gets ranked PlayerState map for specific team by players level*/
	void GetRankedMapByLevel(int32 TeamIndex, RankedPlayerMap& OutRankedMap) const;

	void RequestFinishAndExitToMainMenu();

	void RequestClientsBeginLobbyMenuState();

	void RequestClientsBeginPlayingState();
};
