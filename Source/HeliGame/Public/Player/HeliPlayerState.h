// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerState.h"
#include "HeliPlayerState.generated.h"

class AHeliLobbyGameState;

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliPlayerState : public APlayerState
{
	GENERATED_BODY()	

	/** clear scores */
	virtual void Reset() override;

	/**
	* Set the team
	*
	* @param	InController	The controller to initialize state with
	*/
	virtual void ClientInitialize(class AController* InController) override;
	
	/* Team number assigned to player */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_TeamColor)
	int32 TeamNumber;

	/** replicate team colors. Updated the players colors appropriately */
	UFUNCTION()
	void OnRep_TeamColor();

protected:
	UPROPERTY(Transient, Replicated)
	int32 NumKills;

	UPROPERTY(Transient, Replicated)
	int32 NumDeaths;

	int32 NumHits;

	/** whether the user quit the match */
	UPROPERTY()
	uint8 bQuitter : 1;

	UPROPERTY(Transient, Replicated)
	float RankInCurrentMatch;

	/** Set the colors based on the current teamnum variable */
	void UpdateTeamColors();

public:
	AHeliPlayerState();	

	/* player level */
	UPROPERTY(Transient, Replicated)
	float Level;

	UPROPERTY(Transient, Replicated, BlueprintReadWrite, Category = "PlayerStatus")
	bool bDead;
	
	/* tell whether the player is ready or not*/
	UPROPERTY(Category = "PlayerStatus", EditAnywhere, BlueprintReadWrite, Transient, Replicated)
	bool bPlayerReady;

	/* Copy properties when travelling player */
	virtual void CopyProperties(class APlayerState* PlayerState) override;

	/** player killed someone */
	void ScoreKill(AHeliPlayerState* Victim, int32 Points);

	/** player died */
	void ScoreDeath(AHeliPlayerState* KilledBy, int32 Points);

	/** helper for scoring points */
	void ScorePoints(int32 Points);

	/* player hit enemy */
	void ScoreHit(int Points);

	/**
	* Set new team and update pawn. Also updates player pawn team colors.
	*
	* @param	NewTeamNumber	Team we want to be on.
	*/
	void SetTeamNumber(int32 NewTeamNumber);

	/** get current team */
	UFUNCTION(BlueprintCallable, Category = "Teams")
	int32 GetTeamNumber() const;

	/** get number of kills */
	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetKills() const;

	/** get number of deaths */
	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetDeaths() const;

	/** get number of points */
	UFUNCTION(BlueprintCallable, Category = "Score")
	float GetScore() const;

	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	float GetRankInCurrentMatch();

	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	void SetRankInCurrentMatch(float NewRankInCurrentMatch);

	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	float GetLevel();
	
	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	void SetLevel(float NewLevel);

	/** Sends kill (excluding self) to clients */
	UFUNCTION(Reliable, Client)
	void InformAboutKill(class AHeliPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AHeliPlayerState* KilledPlayerState);

	/** broadcast death to local clients */
	UFUNCTION(Reliable, NetMulticast)
	void BroadcastDeath(class AHeliPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AHeliPlayerState* KilledPlayerState);

	/** gets player name for log and scoreboards */
	FString GetPlayerName() const;

	/** Set whether the player is a quitter */
	void SetQuitter(bool bInQuitter);

	/** get whether the player quit the match */
	bool IsQuitter() const;

	// RPC for the Server tell clients they should update lobby widget
	UFUNCTION(Reliable, Client, BlueprintCallable, Category = "Lobby")
	void Client_UpdateLobbyWidget();

	// RPC for clients tell the server that other players should update theirs Lobby Widget
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UpdateEverybodyLobbyWidget();

	UFUNCTION()
	void OnRep_PlayerName() override;	

	// RPC for clients tell the server to switch team 
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SwitchTeams();

	// Clients tell the server they are ready to play
	UFUNCTION(Reliable, Server, WithValidation, BlueprintCallable, Category = "Lobby")
	void Server_SetPlayerReady(bool bNewPlayerReady);

private:
	UFUNCTION(Reliable, Server, WithValidation)
	void Server_SetPlayerName(const FString& NewPlayerName);

	
};
