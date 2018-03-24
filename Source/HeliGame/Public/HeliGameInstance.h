// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"

#include "HeliGameState.h"
#include "MenuInterface.h"

#include "HeliGameInstance.generated.h"

class FVariantData;
class AHeliGameSession;

UENUM(BlueprintType)
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
	LobbyMenu 				UMETA(DisplayName = "LobbyMenu"),
	AboutMenu 				UMETA(DisplayName = "AboutMenu")
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
class HELIGAME_API UHeliGameInstance : public UGameInstance, public IMenuInterface
{
	GENERATED_BODY()
	
	// Reference UMG Asset in the Editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UiMenu", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UUserWidget> MainMenuWidgetTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UiMenu", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UUserWidget> LoadingScreenWidgetTemplate;	
	
public:
	UHeliGameInstance(const FObjectInitializer& ObjectInitializer);

	bool Tick(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "GameSession")
	AHeliGameSession* GetGameSession() const;

	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void StartGameInstance() override;

	
	

	/*
	* MenuInterface
	*/
	bool HostGame(FGameParams InGameSessionParams) override;

	bool FindServers(ULocalPlayer* PlayerOwner, bool bLAN) override;

	bool JoinServer(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults) override;
	// end of MenuInterface
	
	void RefreshServerList();

	bool JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults);

	UFUNCTION(BlueprintCallable, Category = "ServerInfo")
	void JoinFromServerList(ULocalPlayer* LocalPlayer, FServerEntry Server);

	bool FindSessions(ULocalPlayer* PlayerOwner, bool bLANMatch);

	UPROPERTY(BlueprintReadWrite, Category = "ServerInfo")
	TArray<FServerEntry> AvailableServers;

	void UpdateAvailableServers();

	UPROPERTY(BlueprintReadWrite, Category = "ServerInfo")
	bool IsInSearchingServerProcess;



	UFUNCTION(BlueprintCallable, Category = "PlayerSettings")
	void ChangePlayerName(FString NewPlayerName);

	UFUNCTION(BlueprintCallable, Category = "GameType")
	void EndRoundAndRestartMatch();

	UFUNCTION(BlueprintCallable, Category = "ServerInfo")
	void BeginServerSearch(ULocalPlayer* PlayerOwner, bool bLANMatch);

	/* updates current session settings */
	UFUNCTION(BlueprintCallable, Category = "GameType")
	bool UpdateSessionSettings(ULocalPlayer* LocalPlayer, const FString& GameType, FName SessionName, const FString& MapName, const FString& CustomServerName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers);

	/** Join a a server directly (bypassing online subsystem) */
	UFUNCTION(BlueprintCallable, Category = "Network", exec)
	void TravelToIP(const FString& IpAddress);

	/** Sends the game to the specified state. */
	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void GotoState(EHeliGameInstanceState NewState);

	/** Obtains the initial welcome state, which can be different based on platform */	
	EHeliGameInstanceState GetInitialState();

	/** Sends the game to the initial startup/frontend state  */
	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void GotoInitialState();

	/** Returns true if the game is in online mode */
	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	bool GetIsOnline() const { return bIsOnline; }

	/** Sets the online mode of the game */
	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void SetIsOnline(bool bInIsOnline);

	/** Returns true if the passed in local player is signed in and online */
	bool IsLocalPlayerOnline(ULocalPlayer* LocalPlayer);

	/** Shuts down the session, and frees any net driver */
	void CleanupSessionOnReturnToMenu();

	/** Flag the local player when they quit the game */
	void LabelPlayerAsQuitter(ULocalPlayer* LocalPlayer) const;

	bool HasLicense() const { return bIsLicensed; }

	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	EHeliGameInstanceState GetCurrentState();

	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void ShowLoadingScreen(const FString& NewLoadingMessage);

	UPROPERTY(BlueprintReadWrite, Category = "Loading")
	FString LoadingMessage;

	UFUNCTION(BlueprintCallable, Category = "GameInstanceState")
	void StopLoadingScreen();


	// dev helpers
	FString GetEHeliGameInstanceStateEnumAsString(EHeliGameInstanceState EnumValue);

	UFUNCTION(BlueprintCallable, Category = "Dev|Helpers")
	FString GetEHeliMapEnumAsString(EHeliMap EnumValue);

	UFUNCTION(BlueprintCallable, Category = "Dev|Helpers")
	EHeliMap GetEHeliMapEnumValueFromString(const FString& EnumName);

	UFUNCTION(BlueprintCallable, Category = "Dev|Helpers")
	FString GetGameVersion();

private:
	/** Main menu UI */
	TWeakObjectPtr<class UMainMenu> MainMenu;

	// loading screen UI
	TWeakObjectPtr<UUserWidget> LoadingScreen;

	UPROPERTY(config)
	FString MainMenuMap;
		
	EHeliGameInstanceState CurrentState;
	EHeliGameInstanceState PendingState;	

	/** Whether the user has an active license to play the game */
	bool bIsLicensed;




	/** Last connection status that was passed into the HandleNetworkConnectionStatusChanged hander */
	EOnlineServerConnectionStatus::Type	CurrentConnectionStatus;

	/** Delegate for callbacks to Tick */
	FTickerDelegate TickDelegate;
	FDelegateHandle TickDelegateHandle;

	void OnPreLoadMap(const FString& MapName);
	
	void OnPostLoadMap(UWorld*);

	bool LoadFrontEndMap(const FString& MapName);

	void MaybeChangeState();
	void EndCurrentState(EHeliGameInstanceState NextState);
	void BeginNewState(EHeliGameInstanceState NewState, EHeliGameInstanceState PrevState);

	void BeginMainMenuState();
	void BeginPlayingState();
	void EndMainMenuState();
	void EndPlayingState();

	void AddNetworkFailureHandlers();



	/** URL to travel to after pending network operations */
	FString TravelURL;

	FString BuildTravelURLFromSessionParams(FGameParams InGameSessionParams);

	/** Whether the match is online or not */
	bool bIsOnline;

	/** Handle to various registered delegates */	
	FDelegateHandle TravelLocalSessionFailureDelegateHandle;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;
	FDelegateHandle OnSearchSessionsCompleteDelegateHandle;
	FDelegateHandle OnStartSessionCompleteDelegateHandle;
	FDelegateHandle OnEndSessionCompleteDelegateHandle;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;
	FDelegateHandle OnCreatePresenceSessionCompleteDelegateHandle;


	// Host

	/** Callback which is intended to be called upon session creation */
	void OnCreatePresenceSessionComplete(FName SessionName, bool bWasSuccessful);

	/** Called after all the local players are registered */
	void FinishSessionCreation(EOnJoinSessionCompleteResult::Type Result);

	// Find

	/** Callback which is intended to be called upon finding sessions */
	void OnSearchSessionsComplete(bool bWasSuccessful);


	// Join

	/** Callback which is intended to be called upon joining session */
	void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);

	/** Called after all the local players are registered in a session we're joining */
	void FinishJoinSession(EOnJoinSessionCompleteResult::Type Result);

	/** Travel directly to the named session */
	void TravelToSession(const FName& SessionName);


	// Destroy
	
	/** Delegate for ending a session */
	FOnEndSessionCompleteDelegate OnEndSessionCompleteDelegate;

	void OnEndSessionComplete(FName SessionName, bool bWasSuccessful);

	/** Called when there is an error trying to travel to a local session */
	void TravelLocalSessionFailure(UWorld *World, ETravelFailure::Type FailureType, const FString& ErrorString);

	void RemoveNetworkFailureHandlers();





	

	/** Sets a rich presence string for all local players. */
	void SetPresenceForLocalPlayers(const FVariantData& PresenceData);	
	
	void HandleNetworkConnectionStatusChanged(EOnlineServerConnectionStatus::Type LastConnectionStatus, EOnlineServerConnectionStatus::Type ConnectionStatus);

	void HandleSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType);

protected:
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
};
