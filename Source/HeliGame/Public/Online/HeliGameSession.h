// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "Online.h"
#include "GameFramework/GameSession.h"
#include "HeliGameSession.generated.h"


struct FHeliGameSessionParams
{	
	FString CustomServerName;
	FString SelectedGameModeName;
	FString SelectedMapName;
	FName SessionName;
	bool bIsLAN;
	bool bIsPresence;
	TSharedPtr<const FUniqueNetId> UserId;
	int32 NumberOfPlayers;
	int32 BestSessionIdx;

	FHeliGameSessionParams()
		: CustomServerName(FString(TEXT("None")))
		, SelectedGameModeName(FString(TEXT("None")))
		, SelectedMapName(FString(TEXT("None")))
		, SessionName(NAME_None)
		, bIsLAN(false)
		, bIsPresence(false)
		, NumberOfPlayers(10)
		, BestSessionIdx(0)
	{
	}
};

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliGameSession : public AGameSession
{
	GENERATED_BODY()

protected:
	/** Delegate for creating a new session */
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	/** Delegate after starting a session */
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;
	/** Delegate for destroying a session */
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	/** Delegate for searching for sessions */
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	/** Delegate after joining a session */
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	
	/** Transient properties of a session during game creation/matchmaking */
	FHeliGameSessionParams CurrentSessionParams;

	/** Current host settings */
	TSharedPtr<class FHeliOnlineSessionSettings> HostSettings;
	/** Current search settings */
	TSharedPtr<class FHeliOnlineSearchSettings> SearchSettings;

	/**
	* Delegate fired when a session create request has completed
	*
	* @param SessionName the name of the session this callback is for
	* @param bWasSuccessful true if the async action completed without error, false if there was an error
	*/
	virtual void OnCreateSessionComplete(FName NewSessionName, bool bWasSuccessful);

	/**
	* Delegate fired when a session start request has completed
	*
	* @param SessionName the name of the session this callback is for
	* @param bWasSuccessful true if the async action completed without error, false if there was an error
	*/
	void OnStartOnlineGameComplete(FName InSessionName, bool bWasSuccessful);

	/**
	* Delegate fired when a session search query has completed
	*
	* @param bWasSuccessful true if the async action completed without error, false if there was an error
	*/
	void OnFindSessionsComplete(bool bWasSuccessful);

	/**
	* Delegate fired when a session join request has completed
	*
	* @param SessionName the name of the session this callback is for
	* @param bWasSuccessful true if the async action completed without error, false if there was an error
	*/
	void OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);

	/**
	* Delegate fired when a destroying an online session has completed
	*
	* @param SessionName the name of the session this callback is for
	* @param bWasSuccessful true if the async action completed without error, false if there was an error
	*/
	virtual void OnDestroySessionComplete(FName InSessionName, bool bWasSuccessful);


	/**
	* Reset the variables the are keeping track of session join attempts
	*/
	void ResetBestSessionVars();

	/**
	* Choose the best session from a list of search results based on game criteria
	*/
	void ChooseBestSession();

	/**
	* Entry point for matchmaking after search results are returned
	*/
	void StartMatchmaking();

	/**
	* Return point after each attempt to join a search result
	*/
	void ContinueMatchmaking();

	/**
	* Delegate triggered when no more search results are available
	*/
	void OnNoMatchesAvailable();

	/*
	* Event triggered when a presence session is created
	*
	* @param SessionName name of session that was created
	* @param bWasSuccessful was the create successful
	*/
	DECLARE_EVENT_TwoParams(AHeliGameSession, FOnCreatePresenceSessionComplete, FName /*InSessionName*/, bool /*bWasSuccessful*/);
	FOnCreatePresenceSessionComplete CreatePresenceSessionCompleteEvent;

	/*
	* Event triggered when a session is joined
	*
	* @param SessionName name of session that was joined
	* @param bWasSuccessful was the create successful
	*/
	//DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnJoinSessionComplete, FName /*SessionName*/, bool /*bWasSuccessful*/);
	DECLARE_EVENT_OneParam(AHeliGameSession, FOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type /*Result*/);
	FOnJoinSessionComplete JoinSessionCompleteEvent;

	/*
	* Event triggered after session search completes
	*/
	//DECLARE_EVENT(AShooterGameSession, FOnFindSessionsComplete);
	DECLARE_EVENT_OneParam(AHeliGameSession, FOnFindSessionsComplete, bool /*bWasSuccessful*/);
	FOnFindSessionsComplete FindSessionsCompleteEvent;

public:
	AHeliGameSession(const FObjectInitializer& ObjectInitializer);

	/** Default number of players allowed in a game */
	static const int32 DEFAULT_NUM_PLAYERS = 64;

	/**
	* Host a new online session
	*
	* @param UserId user that initiated the request
	* @param SessionName name of session
	* @param bIsLAN is this going to hosted over LAN
	* @param bIsPresence is the session to create a presence session
	* @param MaxNumPlayers Maximum number of players to allow in the session
	*
	* @return bool true if successful, false otherwise
	*/
	bool HostSession(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, const FString& GameType, const FString& MapName, const FString& CustomServerName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers);

	/**
	* Find an online session
	*
	* @param UserId user that initiated the request
	* @param SessionName name of session this search will generate
	* @param bIsLAN are we searching LAN matches
	* @param bIsPresence are we searching presence sessions
	*/
	void FindSessions(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, bool bIsLAN, bool bIsPresence);

	/**
	* Joins one of the session in search results
	*
	* @param UserId user that initiated the request
	* @param SessionName name of session
	* @param SessionIndexInSearchResults Index of the session in search results
	*
	* @return bool true if successful, false otherwise
	*/
	bool JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, int32 SessionIndexInSearchResults);

	/**
	* Joins a session via a search result
	*
	* @param SessionName name of session
	* @param SearchResult Session to join
	*
	* @return bool true if successful, false otherwise
	*/
	bool JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, const FOnlineSessionSearchResult& SearchResult);

	/** @return true if any online async work is in progress, false otherwise */
	bool IsBusy() const;

	/**
	* Get the search results found and the current search result being probed
	*
	* @param SearchResultIdx idx of current search result accessed
	* @param NumSearchResults number of total search results found in FindGame()
	*
	* @return State of search result query
	*/
	EOnlineAsyncTaskState::Type GetSearchResultStatus(int32& SearchResultIdx, int32& NumSearchResults);

	/**
	* Get the search results.
	*
	* @return Search results
	*/
	const TArray<FOnlineSessionSearchResult> & GetSearchResults() const;


	/** @return the delegate fired when creating a presence session */
	FOnCreatePresenceSessionComplete& OnCreatePresenceSessionComplete() { return CreatePresenceSessionCompleteEvent; }

	/** @return the delegate fired when joining a session */
	FOnJoinSessionComplete& OnJoinSessionComplete() { return JoinSessionCompleteEvent; }

	/** @return the delegate fired when search of session completes */
	FOnFindSessionsComplete& OnFindSessionsComplete() { return FindSessionsCompleteEvent; }

	/** Handle starting the match */
	virtual void HandleMatchHasStarted() override;

	/** Handles when the match has ended */
	virtual void HandleMatchHasEnded() override;






	/**
	* Travel to a session URL (as client) for a given session
	*
	* @param ControllerId controller initiating the session travel
	* @param SessionName name of session to travel to
	*
	* @return true if successful, false otherwise
	*/
	bool TravelToSession(int32 ControllerId, FName InSessionName);

	/** Handles to various registered delegates */
	FDelegateHandle OnStartSessionCompleteDelegateHandle;
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;


	/* updates current session settings */
	bool UpdateSessionSettings(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, const FString& GameType, const FString& MapName, const FString& CustomServerName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers);
};
