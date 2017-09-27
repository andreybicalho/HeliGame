// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "Online/HeliGameMode.h"
#include "HeliGameModeTDM.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliGameModeTDM : public AHeliGameMode
{
	GENERATED_BODY()
	
	/*
	* manager for round based TDM rules without respawn
	*/
	void CompetitiveRoundManager();

	UPROPERTY(EditDefaultsOnly, Category = "TDM Rules", meta = (AllowPrivateAccess = "true"))
	bool bAllowRespawn;

public:	
	AHeliGameModeTDM(const FObjectInitializer& ObjectInitializer);

	/** setup team changes at player login */
	void PostLogin(APlayerController* NewPlayer) override;

	/** initialize replicated game data */
	virtual void InitGameState() override;

	/** can players damage each other? */
	virtual bool CanDealDamage(AHeliPlayerState* DamageInstigator, AHeliPlayerState* DamagedPlayer) const override;

	void PreInitializeComponents() override;

	void EndGame() override;

	void setAllowFriendlyFireDamage(bool bNewAllowFriendlyFireDamage);

	bool IsImmediatelyPlayerRestartAllowedAfterDeath() override;

	FString GetGameModeName() override;
protected:	
	/** number of teams */
	int32 NumTeams;

	/** best team */
	int32 WinnerTeam;


	/** pick team with least players in or random when it's equal */
	int32 ChooseTeam(AHeliPlayerState* ForPlayerState) const;

	FTimerHandle TimerHandle_CompetitiveRoundManagerTimer;

	/** check who won */
	virtual void DetermineMatchWinner() override;

	/** check if PlayerState is a winner */
	virtual bool IsWinner(AHeliPlayerState* PlayerState) const override;

	/** check team constraints */
	virtual bool IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const;
	
};
