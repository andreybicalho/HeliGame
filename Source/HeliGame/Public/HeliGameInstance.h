// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/GameInstance.h"
#include "OnlineIdentityInterface.h"
#include "OnlineSessionInterface.h"
#include "HeliGameState.h"
#include "HeliGameInstance.generated.h"

class FVariantData;
class AHeliGameSession;

UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EHeliGameInstanceState : uint8
{
	None					UMETA(DisplayName = "None"),
	WelcomeScreen 			UMETA(DisplayName = "WelcomeScreen"),
	PendingInvite			UMETA(DisplayName = "PendingInvite"),
	MainMenu				UMETA(DisplayName = "MainMenu"),
	MessageMenu				UMETA(DisplayName = "MessageMenu"),
	Playing					UMETA(DisplayName = "Playing"),
	HostingMenu 			UMETA(DisplayName = "HostingMenu"),
	FindServerMenu 			UMETA(DisplayName = "FindServerMenu"),
	OptionsMenu 			UMETA(DisplayName = "OptionsMenu"),
	LobbyMenu 				UMETA(DisplayName = "LobbyMenu")
};

UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EHeliMap : uint8
{
	None			UMETA(DisplayName = "None"),
	Dev				UMETA(DisplayName = "Dev"),
	EntryMenu 		UMETA(DisplayName = "EntryMenu"),
	Lobby    		UMETA(DisplayName = "Lobby"),
	TheDesert		UMETA(DisplayName = "TheDesert"),
	BattleGround	UMETA(DisplayName = "BattleGround")
};

/** This class holds the value of what message to display when we are in the "MessageMenu" state */
class FHeliPendingMessage
{
public:
	FText	DisplayString;				// This is the display message in the main message body
	FText	OKButtonString;				// This is the ok button text
	FText	CancelButtonString;			// If this is not empty, it will be the cancel button text
	FName	NextState;					// Final destination state once message is discarded

	TWeakObjectPtr< ULocalPlayer > PlayerOwner;		// Owner of dialog who will have focus (can be NULL)
};

// struct for displaying servers info on a UMG Widget
USTRUCT(BlueprintType)
struct FServerEntry
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "ServerInfo")
	FString ServerName;
	
	UPROPERTY(BlueprintReadWrite, Category = "ServerInfo")
	FString CurrentPlayers;
	
	UPROPERTY(BlueprintReadWrite, Category = "ServerInfo")
	FString MaxPlayers;
	
	UPROPERTY(BlueprintReadWrite, Category = "ServerInfo")
	FString GameType;
	
	UPROPERTY(BlueprintReadWrite, Category = "ServerInfo")
	FString MapName;
	
	UPROPERTY(BlueprintReadWrite, Category = "ServerInfo")
	FString Ping;
	
	UPROPERTY(BlueprintReadWrite, Category = "ServerInfo")
	int32 SearchResultsIndex;

	FServerEntry()
	{
		ServerName = CurrentPlayers = MaxPlayers = GameType = MapName = Ping = "-";
		SearchResultsIndex = -1; // invalid
	}

	FServerEntry(
		FString ServName,
		FString CurPlayers,
		FString MaxP,
		FString GType,
		FString MName,
		FString P,
		int32 index
		)
		: ServerName(ServName)
		, CurrentPlayers(CurPlayers)
		, MaxPlayers(MaxP)
		, GameType(GType)
		, MapName(MName)
		, Ping(P)
		, SearchResultsIndex(index)

	{}
};



USTRUCT(BlueprintType)
struct FTeamPlayer
{
	GENERATED_USTRUCT_BODY()

	/** The team the player belongs to */
	UPROPERTY(BlueprintReadWrite, Category = "ScoreBoardInfo")
		uint8 TeamNum;

	/** The number within that team */
	UPROPERTY(BlueprintReadWrite, Category = "ScoreBoardInfo")
		int32 PlayerId;

	/** defaults */
	FTeamPlayer()
		: TeamNum(0)
		, PlayerId(-1) // -1 is kind of a special index isn't it?? hahahaha
	{
	}

	FTeamPlayer(uint8 InTeamNum, int32 InPlayerId)
		: TeamNum(InTeamNum)
		, PlayerId(InPlayerId)
	{
	}

	/** comparator */
	bool operator==(const FTeamPlayer& Other) const
	{
		return (TeamNum == Other.TeamNum && PlayerId == Other.PlayerId);
	}

	/** check to see if we have valid player data */
	bool IsValid() const
	{
		return !(*this == FTeamPlayer());
	}
};

/**
 * 
 */
UCLASS()
class HELIGAME_API UHeliGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
	// Reference UMG Asset in the Editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UiMenu", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UUserWidget> MainMenuWidgetTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UiMenu", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UUserWidget> LoadingScreenWidgetTemplate;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UiMenu", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UUserWidget> HostingMenuWidgetTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UiMenu", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UUserWidget> FindServerMenuWidgetTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UiMenu", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UUserWidget> OptionsMenuWidgetTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UiMenu", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UUserWidget> LobbyMenuWidgetTemplate; 

	
public:
	UHeliGameInstance(const FObjectInitializer& ObjectInitializer);

	bool Tick(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "GameSession")
	AHeliGameSession* GetGameSession() const;

	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void StartGameInstance() override;

	
	

	
	bool HostGame(ULocalPlayer* LocalPlayer, const FString& GameType, const FString& CustomServerName, const FString& InTravelURL);
	
	bool JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults);

	bool JoinSession(ULocalPlayer* LocalPlayer, const FOnlineSessionSearchResult& SearchResult);


	/** host a team death match game, to be called via blueprint, it will call HostGame(ULocalPlayer* LocalPlayer, const FString& GameType, const FString& InTravelURL)*/
	UFUNCTION(BlueprintCallable, Category = "GameType")
	void HostTeamDeathMatch(ULocalPlayer* LocalPlayer, EHeliMap SelectedMap);

	/* host a lobby for players to join */
	UFUNCTION(BlueprintCallable, Category = "GameType")
	void HostLobby(ULocalPlayer* LocalPlayer);

	UFUNCTION(BlueprintCallable, Category = "GameType")
	void BegingTeamDeathmatch(EHeliMap SelectedMap);

	UFUNCTION(BlueprintCallable, Category = "PlayerSettings")
	void SwitchTeam();

	UFUNCTION(BlueprintCallable, Category = "GameType")
	void EndRoundAndRestartMatch();

	UFUNCTION(BlueprintCallable, Category = "GameType")
	void EndRoundAndGoToLobby();


	UFUNCTION(BlueprintCallable, Category = "ServerInfo")
	void BeginServerSearch(ULocalPlayer* PlayerOwner, bool bLANMatch);

	UFUNCTION(BlueprintCallable, Category = "ServerInfo")
	void JoinFromServerList(ULocalPlayer* LocalPlayer, FServerEntry Server);

	/* updates current session settings */
	UFUNCTION(BlueprintCallable, Category = "GameType")
	bool UpdateSessionSettings(ULocalPlayer* LocalPlayer, const FString& GameType, FName SessionName, const FString& MapName, FName CustomServerName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers);


	/** Travel directly to the named session */
	void TravelToSession(const FName& SessionName);

	/** Begin a hosted quick match */
	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void BeginHostingQuickMatch();

	/** Initiates the session searching */
	bool FindSessions(ULocalPlayer* PlayerOwner, bool bLANMatch);

	// keep some servers info for displaying it in a UMG Widget
	UPROPERTY(BlueprintReadWrite, Category = "ServerInfo")
	TArray<FServerEntry> AvailableServers;

	UPROPERTY(BlueprintReadWrite, Category = "ServerInfo")
	bool IsInSearchingServerProcess;

	/** Sends the game to the specified state. */
	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void GotoState(EHeliGameInstanceState NewState);

	/** Obtains the initial welcome state, which can be different based on platform */	
	EHeliGameInstanceState GetInitialState();

	/** Sends the game to the initial startup/frontend state  */
	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void GotoInitialState();


	void RemoveExistingLocalPlayer(ULocalPlayer* ExistingPlayer);

	void RemoveSplitScreenPlayers();

	TSharedPtr< const FUniqueNetId > GetUniqueNetIdFromControllerId(const int ControllerId);

	/** Returns true if the game is in online mode */
	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	bool GetIsOnline() const { return bIsOnline; }

	/** Sets the online mode of the game */
	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void SetIsOnline(bool bInIsOnline);

	/** Sets the controller to ignore for pairing changes. Useful when we are showing external UI for manual profile switching. */
	void SetIgnorePairingChangeForControllerId(const int32 ControllerId);

	/** Returns true if the passed in local player is signed in and online */
	bool IsLocalPlayerOnline(ULocalPlayer* LocalPlayer);

	/** Returns true if owning player is online. Displays proper messaging if the user can't play */
	bool ValidatePlayerForOnlinePlay(ULocalPlayer* LocalPlayer);

	/** Shuts down the session, and frees any net driver */
	void CleanupSessionOnReturnToMenu();

	/** Flag the local player when they quit the game */
	void LabelPlayerAsQuitter(ULocalPlayer* LocalPlayer) const;

	bool HasLicense() const { return bIsLicensed; }

	/** Start task to get user privileges. */
	void StartOnlinePrivilegeTask(const IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate& Delegate, EUserPrivileges::Type Privilege, TSharedPtr< const FUniqueNetId > UserId);

	/** Common cleanup code for any Privilege task delegate */
	void CleanupOnlinePrivilegeTask();

	/** Show approved dialogs for various privileges failures */
	void DisplayOnlinePrivilegeFailureDialogs(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults);

	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	EHeliGameInstanceState GetCurrentState();

	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void SetMaxNumberOfPlayers(int32 NewMaxNumberOfPlayers);

	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	int32 GetMaxNumberOfPlayers();

	UFUNCTION(BlueprintCallable, Category = "LobbySettings")
	void SetSelectedMapName(const FString& NewMapName);

	UFUNCTION(BlueprintCallable, Category = "LobbySettings")
	void SetSelectedGameMode(const FString& NewGameMode);


	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void ShowLoadingScreen(const FString& NewLoadingMessage);

	UPROPERTY(BlueprintReadWrite, Category = "Loading")
	FString LoadingMessage;

	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void StopLoadingScreen();


	// will be sent to game mode in the AHeliGameMode::InitGame function
	/** delay between first player login and starting match */
	UPROPERTY(BlueprintReadWrite, Category = "GameSettings")
	int32 WarmupTime;

	/** match duration */
	UPROPERTY(BlueprintReadWrite, Category = "GameSettings")
	int32 RoundTime;

	UPROPERTY(BlueprintReadWrite, Category = "GameSettings")
	int32 TimeBetweenMatches;

	UPROPERTY(BlueprintReadWrite, Category = "GameSettings")
	FString CustomServerName;

	UPROPERTY(BlueprintReadWrite, Category = "GameSettings")
	FString SelectedGameMode;

	UPROPERTY(BlueprintReadWrite, Category = "GameSettings")
	FString SelectedMapName;

	UPROPERTY(BlueprintReadWrite, Category = "PlayerSettings")
	FString CustomPlayerName;

	UPROPERTY(BlueprintReadWrite, Category = "GameSettings")
	int32 MaxNumberOfPlayers;
	
	UPROPERTY(BlueprintReadWrite, Category = "GameSettings")
	bool bAllowFriendFireDamage;



	// enum helpers
	FString GetEHeliGameInstanceStateEnumAsString(EHeliGameInstanceState EnumValue);

	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	FString GetEHeliMapEnumAsString(EHeliMap EnumValue);

	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	EHeliMap GetEHeliMapEnumValueFromString(const FString& EnumName);

private:
	 

	UPROPERTY(config)
	FString WelcomeScreenMap;

	UPROPERTY(config)
	FString MainMenuMap;
		
	EHeliGameInstanceState CurrentState;
	EHeliGameInstanceState PendingState;

	/** URL to travel to after pending network operations */
	FString TravelURL;

	/** Whether the match is online or not */
	bool bIsOnline;

	/** Whether the user has an active license to play the game */
	bool bIsLicensed;




	/** Main menu UI */
	TWeakObjectPtr<UUserWidget> MainMenu;

	/** Hosting menu UI */
	TWeakObjectPtr<UUserWidget> HostingMenu;

	/** Find Server menu UI */
	TWeakObjectPtr<UUserWidget> FindServerMenu;

	/** Options menu UI */
	TWeakObjectPtr<UUserWidget> OptionsMenu;

	// loading screen UI
	TWeakObjectPtr<UUserWidget> LoadingScreen; // LoadingScreen = CreateWidget<UUserWidget>(SomePlayerController, LoadingScreenWidgetTemplate);

	// Lobby menu UI
	TWeakObjectPtr<UUserWidget> LobbyMenu;





	/** Controller to ignore for pairing changes. -1 to skip ignore. */
	int32 IgnorePairingChangeForControllerId;

	/** Last connection status that was passed into the HandleNetworkConnectionStatusChanged hander */
	EOnlineServerConnectionStatus::Type	CurrentConnectionStatus;

	/** Delegate for callbacks to Tick */
	FTickerDelegate TickDelegate;

	/** Handle to various registered delegates */
	FDelegateHandle TickDelegateHandle;
	FDelegateHandle TravelLocalSessionFailureDelegateHandle;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;
	FDelegateHandle OnSearchSessionsCompleteDelegateHandle;
	FDelegateHandle OnStartSessionCompleteDelegateHandle;
	FDelegateHandle OnEndSessionCompleteDelegateHandle;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;
	FDelegateHandle OnCreatePresenceSessionCompleteDelegateHandle;

	void HandleNetworkConnectionStatusChanged(EOnlineServerConnectionStatus::Type LastConnectionStatus, EOnlineServerConnectionStatus::Type ConnectionStatus);

	void HandleSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType);

	void OnPreLoadMap(const FString& MapName);
	void OnPostLoadMap(UWorld*);



	/** Delegate function executed after checking privileges for starting quick match */
	void OnUserCanPlayInvite(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults);

	/** Delegate for ending a session */
	FOnEndSessionCompleteDelegate OnEndSessionCompleteDelegate;

	void OnEndSessionComplete(FName SessionName, bool bWasSuccessful);

	void MaybeChangeState();
	void EndCurrentState(EHeliGameInstanceState NextState);
	void BeginNewState(EHeliGameInstanceState NewState, EHeliGameInstanceState PrevState);

	void BeginPendingInviteState();
	void BeginWelcomeScreenState();
	void BeginMainMenuState();
	void BeginHostingMenuState();
	void BeginFindServerMenuState();
	void BeginPlayingState();
	void BeginMessageMenuState();
	void BeginOptionsMenuState();
	void BeginLobbyMenuState();

	void EndPendingInviteState();
	void EndWelcomeScreenState();
	void EndMainMenuState();
	void EndHostingMenuState();
	void EndFindServerMenuState();
	void EndPlayingState();
	void EndMessageMenuState();
	void EndOptionsMenuState();
	void EndLobbyMenuState(EHeliGameInstanceState NextState);
	

	void AddNetworkFailureHandlers();
	void RemoveNetworkFailureHandlers();




	/** Called when there is an error trying to travel to a local session */
	void TravelLocalSessionFailure(UWorld *World, ETravelFailure::Type FailureType, const FString& ErrorString);

	/** Callback which is intended to be called upon joining session */
	void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);

	/** Callback which is intended to be called upon session creation */
	void OnCreatePresenceSessionComplete(FName SessionName, bool bWasSuccessful);

	/** Callback which is called after adding local users to a session */
	void OnRegisterLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result);

	/** Called after all the local players are registered */
	void FinishSessionCreation(EOnJoinSessionCompleteResult::Type Result);

	/** Callback which is called after adding local users to a session we're joining */
	void OnRegisterJoiningLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result);

	/** Called after all the local players are registered in a session we're joining */
	void FinishJoinSession(EOnJoinSessionCompleteResult::Type Result);

	

	/** Callback which is intended to be called upon finding sessions */
	void OnSearchSessionsComplete(bool bWasSuccessful);

	bool LoadFrontEndMap(const FString& MapName);

	/** Sets a rich presence string for all local players. */
	void SetPresenceForLocalPlayers(const FVariantData& PresenceData);

	/** Travel directly to the named session */
	void InternalTravelToSession(const FName& SessionName);

	/** Show messaging and punt to welcome screen */
	void HandleSignInChangeMessaging();

	// OSS delegates to handle
	void HandleUserLoginChanged(int32 GameUserIndex, ELoginStatus::Type PreviousLoginStatus, ELoginStatus::Type LoginStatus, const FUniqueNetId& UserId);

	// Callback to handle controller pairing changes.
	void HandleControllerPairingChanged(int GameUserIndex, const FUniqueNetId& PreviousUser, const FUniqueNetId& NewUser);

	// Callback to pause the game when the OS has constrained our app.
	void HandleAppWillDeactivate();

	// Callback occurs when game being suspended
	void HandleAppSuspend();

	// Callback occurs when game resuming
	void HandleAppResume();

	// Callback to process game licensing change notifications.
	void HandleAppLicenseUpdate();

	// Callback to handle safe frame size changes.
	void HandleSafeFrameChanged();

	// Callback to handle controller connection changes.
	void HandleControllerConnectionChange(bool bIsConnection, int32 Unused, int32 GameUserIndex);


	// updates TArray<FServerEntry> AvailableServers
	void UpdateAvailableServers();

protected:
	bool HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld);


	/****************************************************************************************************
	*                                              SCORE BOARD                                          *
	*****************************************************************************************************/
	/** the Ranked PlayerState map...cleared every frame */
	TArray<RankedPlayerMap> PlayerStateMaps;

	/** player count in each team in the last tick */
	TArray<int32> LastTeamPlayerCount;	

public:
	/****************************************************************************************************
	*                                              SCORE BOARD                                          *
	*****************************************************************************************************/
	// updates the ranked PlayerState map
	// this function will be called from the score board widget inside the widget tick function
	UFUNCTION(BlueprintCallable, Category = "ScoreBoard")
	void UpdatePlayerStateMaps();

	// return the player state of the player which is in that ranked map, in order to allow access of player infos (name, team, kills, deaths, etc) in the blueprint row player widget
	UFUNCTION(BlueprintCallable, Category = "ScoreBoard")
	AHeliPlayerState* GetPlayerStateFromPlayerInRankedPlayerMap(const FTeamPlayer& TeamPlayer) const;

	// returns the number of teams in the ranked map
	UFUNCTION(BlueprintCallable, Category = "ScoreBoard")
	uint8 GetNumberOfTeams();

	// returns the number of players in a team in the ranked map
	UFUNCTION(BlueprintCallable, Category = "ScoreBoard")
	int32 GetNumberOfPlayersInTeam(uint8 TeamNum);

	// for checking whether we need to update the widget scoreboard
	UPROPERTY(BlueprintReadWrite, Category = "ScoreBoard")
	bool bRequiresWidgetUpdate;

	/****************************************************************************************************
	*                                              Lobby                                                *
	*****************************************************************************************************/
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void UpdatePlayerStateMapsForLobby();

	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	bool bShouldUpdateLobbyWidget;
};
