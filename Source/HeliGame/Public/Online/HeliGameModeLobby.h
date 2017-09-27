// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "GameFramework/GameMode.h"
#include "HeliGameModeLobby.generated.h"


class AHeliPlayerState;

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliGameModeLobby : public AGameMode
{
	GENERATED_BODY()
	
	
public:
	AHeliGameModeLobby(const FObjectInitializer& ObjectInitializer);

	/** initialize replicated game data */
	virtual void InitGameState() override;

	/** setup team changes at player login */
	void PostLogin(APlayerController* NewPlayer) override;

	/** select best spawn point for player */
	//virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** Returns game session class to use */
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const override;
	// End AGameMode interface	

	void RequestFinishAndExitToMainMenu();

	void RequestClientsGoToLobbyState();

	void RequestClientsGoToPlayingState();

protected:
	/** number of teams */
	int32 NumTeams;

	/** pick team with least players in or random when it's equal */
	int32 ChooseTeam(AHeliPlayerState* ForPlayerState) const;
};
