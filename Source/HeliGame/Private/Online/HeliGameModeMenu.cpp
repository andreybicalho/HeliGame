// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliGameModeMenu.h"
#include "HeliGame.h"
#include "HeliPlayerController.h"
#include "HeliGameSession.h"


AHeliGameModeMenu::AHeliGameModeMenu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerControllerClass = AHeliPlayerController::StaticClass();
}

void AHeliGameModeMenu::RestartPlayer(class AController* NewPlayer)
{
	// don't restart
}

/** Returns game session class to use */
TSubclassOf<AGameSession> AHeliGameModeMenu::GetGameSessionClass() const
{
	return AHeliGameSession::StaticClass();
}


