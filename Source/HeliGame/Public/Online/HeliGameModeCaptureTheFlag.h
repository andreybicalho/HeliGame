// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Online/HeliGameMode.h"
#include "HeliGameModeCaptureTheFlag.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliGameModeCaptureTheFlag : public AHeliGameMode
{
	GENERATED_BODY()

	/*
	* manager for round based TDM rules without respawn
	*/
	void CompetitiveRoundManager();

	UPROPERTY(EditDefaultsOnly, Category = "Cap. The Flag Rules", meta = (AllowPrivateAccess = "true"))
	bool bAllowRespawn;
	
protected:	
	/** number of teams */
	int32 NumTeams;

	/** best team */
	int32 WinnerTeam;

	/** check who won */
	void DetermineMatchWinner() override;

	/** check if PlayerState is a winner */
	bool IsWinner(AHeliPlayerState* PlayerState) const override;

	/** check team constraints */
	bool IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const override;

	/** pick team with least players in or random when it's equal */
	int32 ChooseTeam(AHeliPlayerState* ForPlayerState) const;

	FTimerHandle TimerHandle_CompetitiveRoundManagerTimer;		

public:
	AHeliGameModeCaptureTheFlag(const FObjectInitializer& ObjectInitializer);

	void setAllowFriendlyFireDamage(bool bInAllowFriendlyFireDamage);

	FString GetGameModeName() override;
	
	/** can players damage each other? */
	bool CanDealDamage(AHeliPlayerState* DamageInstigator, AHeliPlayerState* DamagedPlayer) const override;

	bool IsImmediatelyPlayerRestartAllowedAfterDeath();

	void RestartRound();

	void EndRound();

	void EndGame() override;

/*
* overrides from UE4 API
*/
private:

protected:
	
public:
	/** setup team changes at player login */
	void PostLogin(APlayerController* NewPlayer) override;

	/** initialize replicated game data */
	void InitGameState() override;

	void PreInitializeComponents() override;


	
};
