// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "Online.h"
#include "HeliPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	/** Handle for efficient management of ClientStartOnlineGame timer */
	FTimerHandle TimerHandle_ClientStartOnlineGame;

	void AddHeliHudWidgetInHudForFirstPersonView();

	void AddHeliHudWidgetInHudForThirdPersonView();

protected:
	/** if set, gameplay related actions (movement, weapon usage, etc) are allowed */
	uint8 bAllowGameActions : 1;

	// For tracking whether or not to send the end event
	bool bHasSentStartEvents;

	/** Achievements write object */
	FOnlineAchievementsWritePtr WriteObject;

	/** Causes the player to commit suicide */
	UFUNCTION(BlueprintCallable, exec, Category = "Inputs")
	virtual void Suicide();

	/** Notifies the server that the client has suicided */
	UFUNCTION(reliable, server, WithValidation)
	void ServerSuicide();


	/** Updates achievements based on the PersistentUser stats at the end of a round */
	// TODO: void UpdateAchievementsOnGameEnd();

	/** Updates leaderboard stats at the end of a round */
	// TODO: void UpdateLeaderboardsOnGameEnd();

	/** Updates the save file at the end of a round */
	// TODO: void UpdateSaveFileOnGameEnd(bool bIsWinner);
public:
	AHeliPlayerController();

	/** shows scoreboard */
	void OnShowScoreboard();

	/** hides scoreboard */
	void OnHideScoreboard();	

	/* shows in game menu UI */
	void OnShowInGameMenu();

	UFUNCTION(BlueprintCallable, Category = "InGameMenu")
	void HideInGameMenu();

	UFUNCTION(BlueprintCallable, Category = "InGameMenu")
	void ShowInGameOptionsMenu();

	UFUNCTION(BlueprintCallable, Category = "InGameMenu")
	void HideInGameOptionsMenu();

	UFUNCTION(BlueprintCallable, Category = "InGameMenu|Controls")
	void SetMouseSensitivity(float inMouseSensitivity);

	UFUNCTION(BlueprintCallable, Category = "InGameMenu|Controls")
	float GetMouseSensitivity();

	UFUNCTION(BlueprintCallable, Category = "InGameMenu|Controls")
	void SetKeyboardSensitivity(float inKeyboardSensitivity);

	UFUNCTION(BlueprintCallable, Category = "InGameMenu|Controls")
	float GetKeyboardSensitivity();

	/** check if gameplay related actions (movement, weapon usage, etc) are allowed right now */
	bool IsGameInputAllowed() const;

	/** notify player about started match */
	UFUNCTION(reliable, client)
	void ClientGameStarted();

	/** Starts the online game using the session name in the PlayerState */
	UFUNCTION(reliable, client)
	void ClientStartOnlineGame();

	/** Ends the online game using the session name in the PlayerState */
	UFUNCTION(reliable, client)
	void ClientEndOnlineGame();

	/** notify player about finished match */
	virtual void ClientGameEnded_Implementation(class AActor* EndGameFocus, bool bIsWinner);

	/** Notifies clients to send the end-of-round event */
	UFUNCTION(reliable, client)
	void ClientSendRoundEndEvent(bool bIsWinner, int32 ExpendedTimeInSeconds);

	/** Cleans up any resources necessary to return to main menu.  Does not modify GameInstance state. */
	virtual void HandleReturnToMainMenu();

	/** Ends and/or destroys game session */
	void CleanupSessionOnReturnToMenu();

	/** notify local client about deaths */
	void OnDeathMessage(class AHeliPlayerState* KillerPlayerState, class AHeliPlayerState* KilledPlayerState, const UDamageType* KillerDamageType);

	/** Informs that player fragged someone */
	void OnKill();

	/**
	* Called when the read achievements request from the server is complete
	*
	* @param PlayerId The player id who is responsible for this delegate being fired
	* @param bWasSuccessful true if the server responded successfully to the request
	*/
	void OnQueryAchievementsComplete(const FUniqueNetId& PlayerId, const bool bWasSuccessful);

	/**
	* Reads achievements to precache them before first use
	*/
	void QueryAchievements();

	/**
	* Writes a single achievement (unless another write is in progress).
	*
	* @param Id achievement id (string)
	* @param Percent number 1 to 100
	*/
	void UpdateAchievementProgress(const FString& Id, float Percent);

	/** Associate a new UPlayer with this PlayerController. */
	virtual void SetPlayer(UPlayer* Player);

	UFUNCTION(BlueprintCallable, Category = "Inputs")
	void SetAllowGameActions(bool bNewAllowGameActions);

	UFUNCTION(Reliable, Client)
	void ClientReturnToLobbyState();

	UFUNCTION(Reliable, Client)
	void ClientGoToPlayingState();

	void UpdateTeamNumber(int32 teamNumber);
	
public:
	/* Debug helpers */
	void FlushDebugLines();

/*
* overrides
*/

protected:
	/** respawn after dying */
	virtual void UnFreeze() override;

	/** sets up input */
	virtual void SetupInputComponent() override;

	/** Return the client to the main menu gracefully.  ONLY sets GI state. */
	void ClientReturnToMainMenu_Implementation(const FString& ReturnReason) override;

	/** transition to dead state, retries spawning later */
	virtual void FailedToSpawnPawn() override;

	/** Pawn has been possessed, so changing state to NAME_Playing. Start it walking and begin playing with it. */
	void BeginPlayingState() override;

public:
	void BeginPlay() override;

	/** initialize the input system from the player settings */
	virtual void InitInputSystem() override;

	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

	/**
	* Called from game info upon end of the game, used to transition to proper state.
	*
	* @param EndGameFocus Actor to set as the view target on end game
	* @param bIsWinner true if this controller is on winning team
	*/
	virtual void GameHasEnded(class AActor* EndGameFocus = NULL, bool bIsWinner = false) override;
};
