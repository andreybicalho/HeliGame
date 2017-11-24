// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "GameFramework/GameMode.h"
#include "HeliGameMode.generated.h"

class APlayerStart;

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	AHeliGameMode(const FObjectInitializer& ObjectInitializer);

	virtual void PreInitializeComponents() override;

	/** initialize replicated game data */
	virtual void InitGameState() override;

	/** Initialize the game. This is called before actors' PreInitializeComponents. */
	virtual void InitGame(const FString& InMapName, const FString& Options, FString& ErrorMessage) override;

	/** Accept or reject a player attempting to join the server.  Fails login if you set the ErrorMessage to a non-empty string. */
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	/** starts match warmup */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** select best spawn point for player */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** Return true of FindPlayerStart should use the StartSpot stored on Player instead of calling ChoosePlayerStart. */
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;

	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;

	/** Tries to spawn the player's pawn, at the location returned by FindPlayerStart */	
	virtual void RestartPlayer(AController* NewPlayer) override;

	void RestartPlayerAtTransform(AController* NewPlayer, const FTransform& SpawnTransform) override;

	/** update remaining time */
	virtual void DefaultTimer();

	/** called before startmatch */
	virtual void HandleMatchIsWaitingToStart() override;

	/** starts new match */
	virtual void HandleMatchHasStarted() override;

	/** hides the onscreen hud and restarts the map */
	UFUNCTION(BlueprintCallable, Category = "GameRules")
	virtual void RestartGame() override;

	/* end the game, end session, load main menu */
	virtual void EndGame();


	/** finish current match and lock players */
	UFUNCTION(BlueprintCallable, Category = "GameRules", exec)
	void FinishMatch();

	/*Finishes the match and bumps everyone to main menu.*/
	/*Only GameInstance should call this function */
	void RequestFinishAndExitToMainMenu();

	void RequestClientsGoToLobbyState();

	/************************************************************************/
	/* Damage & Killing                                                     */
	/************************************************************************/

	/** notify about kills */
	virtual void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType);

	/* Can the player deal damage according to gamemode rules (eg. friendly-fire disabled) */
	virtual bool CanDealDamage(class AHeliPlayerState* DamageCauser, class AHeliPlayerState* DamagedPlayer) const;

	/** prevents friendly fire */
	virtual float ModifyDamage(float Damage, AActor* DamagedActor, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const;

	/* make score for each shot that was hit */
	virtual void ScoreHit(AController* Attacker, AController* VictimPlayer, const int Damage);


	/* Can we deal damage to players in the same team */
	UPROPERTY(EditDefaultsOnly, Category = "Rules")
	bool bAllowFriendlyFireDamage;

	/* The teamnumber assigned to Players */
	int32 PlayerTeamNum;

	/** Handle for efficient management of DefaultTimer timer */
	FTimerHandle TimerHandle_DefaultTimer;

	/** delay between first player login and starting match */
	UPROPERTY(BlueprintReadWrite, Category = "GameRules")
	int32 WarmupTime;

	/** match duration */
	UPROPERTY(BlueprintReadWrite, Category = "GameRules")
	int32 RoundTime;

	UPROPERTY(BlueprintReadWrite, Category = "GameRules")
	int32 TimeBetweenMatches;

	UPROPERTY(BlueprintReadWrite, Category = "GameRules")
	FString MapName;

	UPROPERTY(BlueprintReadWrite, Category = "GameRules")
	FString ServerName;

	UPROPERTY(BlueprintReadWrite, Category = "GameRules")
	int32 MaxNumberOfPlayers;

	/* check if immediately player restart after the player is dead is allowed */
	virtual bool IsImmediatelyPlayerRestartAllowedAfterDeath();

	virtual FString GetGameModeName();

protected:
	/** score for kill */
	UPROPERTY(config)
	int32 KillScore;

	/** score for death */
	UPROPERTY(config)
	int32 DeathScore;

	/** check who won */
	virtual void DetermineMatchWinner();

	/** check if PlayerState is a winner */
	virtual bool IsWinner(AHeliPlayerState* PlayerState) const;

	/** check if player can use spawnpoint */
	virtual bool IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const;

	/** check if player should use spawnpoint */
	virtual bool IsSpawnpointPreferred(APlayerStart* SpawnPoint, AController* Player) const;


	/** Returns game session class to use
	* AGameSession is instantiated on the AGameMode and is the class meant to handle the interactions with the OnlineSubsystem as well as various other "I'm the host" kind of duties.
	* Things like accepting login (possibly checking ban lists and server capacity), spectator permissions, voice chat push to talk requirements, starting/ending a session with the platform.
	*/
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const override;
	
	
};
