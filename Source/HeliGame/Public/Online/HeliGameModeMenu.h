// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "GameFramework/GameMode.h"
#include "HeliGameModeMenu.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliGameModeMenu : public AGameMode
{
	GENERATED_BODY()
	
	
public:
	AHeliGameModeMenu(const FObjectInitializer& ObjectInitializer);
	// Begin AGameMode interface
	/** skip it, menu doesn't require player start or pawn */
	virtual void RestartPlayer(class AController* NewPlayer) override;

	/** Returns game session class to use */
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const override;
	// End AGameMode interface	
	
};
