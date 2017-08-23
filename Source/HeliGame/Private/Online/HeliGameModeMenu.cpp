// Fill out your copyright notice in the Description page of Project Settings.

#include "HeliGame.h"
#include "HeliGameModeMenu.h"
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


