// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliGameInstance.h"
#include "HeliPlayerController.h"
#include "HeliPlayerState.h"
#include "HeliGameSession.h"
#include "HeliGameState.h"
#include "HeliLobbyGameState.h"
#include "Helicopter.h"
#include "MainMenu.h"
#include "FindServersMenu.h"

#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "GameFramework/GameMode.h"
#include "Public/Engine.h" // avoid the monolithic header file Runtime\Engine\Classes\Engine\Engine.h
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Online.h"


UHeliGameInstance::UHeliGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsOnline(false) // Default to LAN
	, bIsLicensed(true) // Default to licensed (should have been checked by OS on boot)
{
	// loading screen widget
	static ConstructorHelpers::FClassFinder<UUserWidget> LoadingScreenWidget(TEXT("/Game/MenuSystem/WBP_LoadingScreen"));
	if (LoadingScreenWidget.Class != nullptr)
	{
		LoadingScreenWidgetTemplate = LoadingScreenWidget.Class;
	}

	// main menu widget
	static ConstructorHelpers::FClassFinder<UUserWidget> MainMenuWidgetBP(TEXT("/Game/MenuSystem/WBP_MainMenu"));
	if (MainMenuWidgetBP.Class != nullptr)
	{
		MainMenuWidgetTemplate = MainMenuWidgetBP.Class;
	}

	this->


	CurrentState = EHeliGameInstanceState::None;

	MainMenuMap = *FString::Printf(TEXT("/Game/Maps/EntryMenu"));

	IsInSearchingServerProcess = false;

	LoadingMessage = FString(TEXT("Loading..."));

	bRequiresWidgetUpdate = true;
}

void UHeliGameInstance::Init()
{
	Super::Init();

	CurrentConnectionStatus = EOnlineServerConnectionStatus::Connected;

	// game requires the ability to ID users.
	const auto OnlineSub = IOnlineSubsystem::Get();
	//check(OnlineSub);
	// TODO: check pointesrs
	if (OnlineSub) {
		const auto IdentityInterface = OnlineSub->GetIdentityInterface();		
		const auto SessionInterface = OnlineSub->GetSessionInterface();
		

		OnlineSub->AddOnConnectionStatusChangedDelegate_Handle(FOnConnectionStatusChangedDelegate::CreateUObject(this, &UHeliGameInstance::HandleNetworkConnectionStatusChanged));

		SessionInterface->AddOnSessionFailureDelegate_Handle(FOnSessionFailureDelegate::CreateUObject(this, &UHeliGameInstance::HandleSessionFailure));
		
	}

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UHeliGameInstance::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UHeliGameInstance::OnPostLoadMap);

	

	OnEndSessionCompleteDelegate = FOnEndSessionCompleteDelegate::CreateUObject(this, &UHeliGameInstance::OnEndSessionComplete);

	// Register delegate for ticker callback
	TickDelegate = FTickerDelegate::CreateUObject(this, &UHeliGameInstance::Tick);
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate, 0.5);
}

void UHeliGameInstance::Shutdown()
{
	Super::Shutdown();

	// Unregister ticker delegate
	FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
}

void UHeliGameInstance::HandleNetworkConnectionStatusChanged(EOnlineServerConnectionStatus::Type LastConnectionStatus, EOnlineServerConnectionStatus::Type ConnectionStatus)
{
	UE_LOG(LogOnlineGame, Warning, TEXT("UHeliGameInstance::HandleNetworkConnectionStatusChanged: %s"), EOnlineServerConnectionStatus::ToString(ConnectionStatus));

	// If we are disconnected from server, and not currently at (or heading to) the welcome screen
	// then display a message on consoles
	if (bIsOnline &&
		PendingState != EHeliGameInstanceState::WelcomeScreen &&
		CurrentState != EHeliGameInstanceState::WelcomeScreen &&
		ConnectionStatus != EOnlineServerConnectionStatus::Connected)
	{
		UE_LOG(LogOnlineGame, Log, TEXT("UHelirGameInstance::HandleNetworkConnectionStatusChanged: Going to main menu"));

		GotoState(EHeliGameInstanceState::MainMenu);
	}

	CurrentConnectionStatus = ConnectionStatus;
}

void UHeliGameInstance::HandleSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType)
{
	UE_LOG(LogOnlineGame, Warning, TEXT("UHeliGameInstance::HandleSessionFailure: %u"), (uint32)FailureType);

	// TODO: show some message and go to main menu
	GotoState(EHeliGameInstanceState::MainMenu);
}


void UHeliGameInstance::OnPreLoadMap(const FString& MapName)
{
	UE_LOG(LogLoad, Log, TEXT("UHeliGameInstance->OnPreLoadMap: Loading %s Map..."), *MapName);
	
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport)
	{
		MyViewport->SetDisableSplitscreenOverride(true);
	}


}

void UHeliGameInstance::OnPostLoadMap(UWorld*)
{
	// Make sure we hide the loading screen when the level is done loading
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && LoadingScreen.IsValid() && LoadingScreen->IsInViewport())
	{
		LoadingScreen->RemoveFromViewport();
	}	
}

void UHeliGameInstance::StartGameInstance()
{

#if PLATFORM_PS4 == 0
	TCHAR Parm[4096] = TEXT("");

	const TCHAR* Cmd = FCommandLine::Get();

	// Catch the case where we want to override the map name on startup (used for connecting to other MP instances)
	if (FParse::Token(Cmd, Parm, ARRAY_COUNT(Parm), 0) && Parm[0] != '-')
	{
		// if we're 'overriding' with the default map anyway, don't set a bogus 'playing' state.
		if (!MainMenuMap.Contains(Parm))
		{
			FURL DefaultURL;
			DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);

			FURL URL(&DefaultURL, Parm, TRAVEL_Partial);

			if (URL.Valid)
			{
				UEngine* const Engine = GetEngine();

				FString Error;

				const EBrowseReturnVal::Type BrowseRet = Engine->Browse(*WorldContext, URL, Error);

				if (BrowseRet == EBrowseReturnVal::Success)
				{
					// Success, we loaded the map, go directly to playing state
					GotoState(EHeliGameInstanceState::Playing);
					return;
				}
				else if (BrowseRet == EBrowseReturnVal::Pending)
				{
					// Assume network connection
					LoadFrontEndMap(MainMenuMap);
					AddNetworkFailureHandlers();
					ShowLoadingScreen(FString(TEXT("Loading...")));
					GotoState(EHeliGameInstanceState::Playing);
					return;
				}
			}
		}
	}
#endif


	GotoInitialState();
}

EHeliGameInstanceState UHeliGameInstance::GetInitialState()
{
	// On PC, go directly to the main menu
	return EHeliGameInstanceState::MainMenu;
}

EHeliGameInstanceState UHeliGameInstance::GetCurrentState()
{	
	return CurrentState;
}

void UHeliGameInstance::GotoInitialState()
{
	GotoState(GetInitialState());
}

void UHeliGameInstance::ShowLoadingScreen(const FString& NewLoadingMessage)
{
	LoadingMessage = *NewLoadingMessage;

	UE_LOG(LogLoad, Log, TEXT(">> ShowLoadingScreen() -> %s"), *FString::Printf(TEXT("CurrentState: %s, LoadingMessage: %s"), *GetEHeliGameInstanceStateEnumAsString(CurrentState), *LoadingMessage));
		
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport)
	{
		if (LoadingScreen.IsValid() && !LoadingScreen->IsInViewport())
		{
			LoadingScreen->AddToViewport();
		}
		else if (!LoadingScreen.IsValid())
		{
			if (LoadingScreenWidgetTemplate)
			{
				APlayerController* const FirstPC = GetFirstLocalPlayerController();
				LoadingScreen = CreateWidget<UUserWidget>(FirstPC, LoadingScreenWidgetTemplate);
				LoadingScreen->AddToViewport();
			}			
		}
	}
}

void UHeliGameInstance::StopLoadingScreen()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport)
	{
		if (LoadingScreen.IsValid() && LoadingScreen->IsInViewport())
		{
			LoadingScreen->RemoveFromViewport();
		}
	}
}

bool UHeliGameInstance::LoadFrontEndMap(const FString& MapName)
{
	bool bSuccess = true;

	// if already loaded, do nothing
	UWorld* const World = GetWorld();
	if (World)
	{
		// NOTE(andrey): hack for crashing when playing from within editor
		if (World->IsPlayInEditor()) return true;

		FString const CurrentMapName = *World->PersistentLevel->GetOutermost()->GetName();
		UE_LOG(LogLoad, Log, TEXT("%s"), *FString::Printf(TEXT("CurrentMapName: %s       , MapName: %s"), *CurrentMapName, *MapName));
		//if (MapName.Find(TEXT("EntryMenu")) != -1)
		//if (CurrentMapName == MapName)
		if(CurrentMapName.Contains(MapName))
		{
			return bSuccess;
		}
	}

	FString Error;
	EBrowseReturnVal::Type BrowseRet = EBrowseReturnVal::Failure;
	FURL URL(
		*FString::Printf(TEXT("%s"), *MapName)
		);

	if (URL.Valid && !HasAnyFlags(RF_ClassDefaultObject)) //CastChecked<UEngine>() will fail if using Default__GameInstance, so make sure that we're not default
	{
		BrowseRet = GetEngine()->Browse(*WorldContext, URL, Error);

		// Handle failure.
		if (BrowseRet != EBrowseReturnVal::Success)
		{
			UE_LOG(LogLoad, Fatal, TEXT("%s"), *FString::Printf(TEXT("Failed to enter %s: %s. Please check the log for errors."), *MapName, *Error));
			bSuccess = false;
		}
	}
	return bSuccess;
}

AHeliGameSession* UHeliGameInstance::GetGameSession() const
{
	UWorld* const World = GetWorld();
	if (World)
	{
		AGameMode* const Game = World->GetAuthGameMode<AGameMode>();
		if (Game)
		{
			return Cast<AHeliGameSession>(Game->GameSession);
		}
	}

	return nullptr;
}

void UHeliGameInstance::GotoState(EHeliGameInstanceState NewState)
{
	//UE_LOG(LogOnline, Log, TEXT("GotoState: NewState: %s"), *NewState.ToString());	
	UE_LOG(LogLoad, Log, TEXT("GotoState: NewState: %s"), *GetEHeliGameInstanceStateEnumAsString(NewState));

	PendingState = NewState;
}

void UHeliGameInstance::MaybeChangeState()
{
	if ((PendingState != CurrentState) && (PendingState != EHeliGameInstanceState::None))
	{
		//FName const OldState = CurrentState;
		EHeliGameInstanceState const OldState = CurrentState;

		// end current state
		EndCurrentState(PendingState);

		// begin new state
		BeginNewState(PendingState, OldState);

		// clear pending change
		PendingState = EHeliGameInstanceState::None;
	}
}

void UHeliGameInstance::BeginNewState(EHeliGameInstanceState NewState, EHeliGameInstanceState PrevState)
{
	// per-state custom starting code here
	if (NewState == EHeliGameInstanceState::MainMenu)
	{
		BeginMainMenuState();
	}	
	else if (NewState == EHeliGameInstanceState::Playing)
	{
		BeginPlayingState();
	}	

	CurrentState = NewState;
}

void UHeliGameInstance::EndCurrentState(EHeliGameInstanceState NextState)
{
	// per-state custom ending code here
	if (CurrentState == EHeliGameInstanceState::MainMenu)
	{
		EndMainMenuState();
	}
	else if (CurrentState == EHeliGameInstanceState::Playing)
	{
		EndPlayingState();
	}

	CurrentState = EHeliGameInstanceState::None;
}


void UHeliGameInstance::SetPresenceForLocalPlayers(const FVariantData& PresenceData)
{
	const auto Presence = Online::GetPresenceInterface();
	if (Presence.IsValid())
	{
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			const TSharedPtr<const FUniqueNetId> UserId = LocalPlayers[i]->GetPreferredUniqueNetId();

			if (UserId.IsValid())
			{
				FOnlineUserPresenceStatus PresenceStatus;
				PresenceStatus.Properties.Add(DefaultPresenceKey, PresenceData);

				Presence->SetPresence(*UserId, PresenceStatus);
			}
		}
	}
}


void UHeliGameInstance::BeginMainMenuState()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport)
	{
		MyViewport->RemoveAllViewportWidgets();		
	}

	// Set presence to menu state for the owning player
	SetPresenceForLocalPlayers(FVariantData(FString(TEXT("OnMenu"))));

	// load startup map
	LoadFrontEndMap(MainMenuMap);

	if(MyViewport && MainMenuWidgetTemplate)
	{
		if (MainMenu.IsValid() && !MainMenu->IsPendingKillOrUnreachable() && !MainMenu->IsInViewport())
		{
			MainMenu->Setup();			
		}
		else
		{
			APlayerController* const playercontroller = GetFirstLocalPlayerController();
			if (playercontroller)
			{
				MainMenu = CreateWidget<UMainMenu>(playercontroller, MainMenuWidgetTemplate);
				MainMenu->Setup();
				MainMenu->SetMenuInterface(this);		
			}
		}
	}
}

void UHeliGameInstance::EndMainMenuState()
{
	if (MainMenu.IsValid() && !MainMenu->IsPendingKillOrUnreachable() && MainMenu->IsInViewport())
	{
		MainMenu->Teardown();
	}
}


void UHeliGameInstance::BeginPlayingState()
{
	// Set presence for playing in a map
	SetPresenceForLocalPlayers(FVariantData(FString(TEXT("InGame"))));

	// TODO: Make sure viewport has focus
	APlayerController* const FirstPC = GetFirstLocalPlayerController();
	FirstPC->SetIgnoreLookInput(false);
	FirstPC->SetIgnoreMoveInput(false);
	FirstPC->bShowMouseCursor = false;
}

void UHeliGameInstance::EndPlayingState()
{
	UWorld* const World = GetWorld();
	AHeliGameState* const GameState = World != nullptr ? World->GetGameState<AHeliGameState>() : nullptr;

	if (GameState)
	{
		// Send round end events for local players
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			auto HeliPC = Cast<AHeliPlayerController>(LocalPlayers[i]->PlayerController);
			if (HeliPC)
			{
				// Assuming you can't win if you quit early
				HeliPC->ClientSendRoundEndEvent(false, GameState->ElapsedTime);
			}
		}

		// Give the game state a chance to cleanup first	
		GameState->RequestFinishAndExitToMainMenu();
	}
	else
	{
		// If there is no game state, make sure the session is in a good state
		CleanupSessionOnReturnToMenu();
	}
}

void UHeliGameInstance::LabelPlayerAsQuitter(ULocalPlayer* LocalPlayer) const
{
	AHeliPlayerState* const PlayerState = LocalPlayer && LocalPlayer->PlayerController ? Cast<AHeliPlayerState>(LocalPlayer->PlayerController->PlayerState) : nullptr;
	if (PlayerState)
	{
		PlayerState->SetQuitter(true);
	}
}

void UHeliGameInstance::AddNetworkFailureHandlers()
{
	// Add network/travel error handlers (if they are not already there)
	if (GEngine->OnTravelFailure().IsBoundToObject(this) == false)
	{
		TravelLocalSessionFailureDelegateHandle = GEngine->OnTravelFailure().AddUObject(this, &UHeliGameInstance::TravelLocalSessionFailure);
	}
}







/**********************************************************
*                                                         *
*      MenuInterface (Host, Join, Find)                   *
*                                                         *
***********************************************************/

/*
* Host
*/
bool UHeliGameInstance::HostGame(FGameParams InGameSessionParams)
{
	TravelURL = BuildTravelURLFromSessionParams(InGameSessionParams);

	UE_LOG(LogTemp, Display, TEXT("UHeliGameInstance::HostGame ~ TravelURL = %s"), *TravelURL);

	AHeliGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		// add callback delegate for completion
		OnCreatePresenceSessionCompleteDelegateHandle = GameSession->OnCreatePresenceSessionComplete().AddUObject(this, &UHeliGameInstance::OnCreatePresenceSessionComplete);

		if (GameSession->HostSession(InGameSessionParams.UserId, InGameSessionParams.SessionName, InGameSessionParams.SelectedGameModeName, InGameSessionParams.SelectedMapName, InGameSessionParams.CustomServerName, InGameSessionParams.bIsLAN, InGameSessionParams.bIsPresence, InGameSessionParams.NumberOfPlayers))
		{
			// If any error occurred in the above, pending state would be set
			if ((PendingState == CurrentState) || (PendingState == EHeliGameInstanceState::None))
			{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				ShowLoadingScreen("Loading...");
				GotoState(EHeliGameInstanceState::Playing);
				return true;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("UHeliGameInstance::HostGame ~ HostGame Failed!"));
	return false;
}

FString UHeliGameInstance::BuildTravelURLFromSessionParams(FGameParams InGameSessionParams)
{
	FString CustomServerName = InGameSessionParams.CustomServerName;
	FString SelectedGameModeName = FString(TEXT("?game=")) + InGameSessionParams.SelectedGameModeName;
	FString SelectedMapName = FString(TEXT("/Game/Maps/")) + InGameSessionParams.SelectedMapName;
	FName SessionName = InGameSessionParams.SessionName;
	FString LANOption = InGameSessionParams.bIsLAN ? FString(TEXT("")) : FString(TEXT("?bIsLanMatch"));
	bool bIsPresence = InGameSessionParams.bIsPresence;
	TSharedPtr<const FUniqueNetId> UserId = InGameSessionParams.UserId;
	int32 NumberOfPlayers = InGameSessionParams.NumberOfPlayers;
	int32 BestSessionIdx = InGameSessionParams.BestSessionIdx;
	FString FriendFire = InGameSessionParams.bAllowFriendFireDamage ? FString(TEXT("?bAllowFriendlyFireDamage")) : FString(TEXT(""));
	FString ListenServerOption = FString(TEXT("?listen"));
	FString WarmupTimeOption = FString(TEXT("?WarmupTime=")) + FString::FromInt(InGameSessionParams.WarmupTime);
	FString RoundTimeOption = FString(TEXT("?RoundTime=")) + FString::FromInt(InGameSessionParams.RoundTime);
	FString TimeBetweenMatchesOption = FString(TEXT("?TimeBetweenMatches=")) + FString::FromInt(InGameSessionParams.TimeBetweenMatches);
	FString MaxNumberOfPlayersOption = FString(TEXT("?MaxNumberOfPlayers=")) + FString::FromInt(InGameSessionParams.NumberOfPlayers);

	return SelectedMapName + SelectedGameModeName + FString(TEXT("?listen")) + LANOption + WarmupTimeOption + RoundTimeOption + TimeBetweenMatchesOption + FriendFire + FString(TEXT("?CustomServerName=")) + CustomServerName;// +FString(TEXT("?GameModeName=")) + InGameSessionParams.SelectedGameModeName;
}

/** Callback which is intended to be called upon session creation */
void UHeliGameInstance::OnCreatePresenceSessionComplete(FName SessionName, bool bWasSuccessful)
{
	AHeliGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		GameSession->OnCreatePresenceSessionComplete().Remove(OnCreatePresenceSessionCompleteDelegateHandle);

		FinishSessionCreation(bWasSuccessful ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UHeliGameInstance::FinishSessionCreation(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Display, TEXT("UHeliGameInstance::FinishSessionCreation ~ Travelling to TravelURL = %s"), *TravelURL);

		GetWorld()->ServerTravel(TravelURL);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UHeliGameInstance::FinishSessionCreation: CreateSessionFailed: EOnJoinSessionCompleteResult is not sucess, so going to MainMenu"));
		GotoState(EHeliGameInstanceState::MainMenu);
	}
}

/*
* Find 
*/
bool UHeliGameInstance::FindServers(class ULocalPlayer* PlayerOwner, bool bLAN)
{
	ShowLoadingScreen("Searching...");

	return FindSessions(PlayerOwner, bLAN);
}

void UHeliGameInstance::RefreshServerList()
{
	TArray<FServerData> AvailableServersData;
	AvailableServersData.Empty();
	
	for (FServerEntry server : AvailableServers)
	{
		FServerData serverData;
		serverData.MaxPlayers = server.MaxPlayers;
		serverData.CurrentPlayers = server.CurrentPlayers;
		serverData.GameType = server.GameType;
		serverData.MapName = server.MapName;
		serverData.ServerName = server.ServerName;
		serverData.Ping = server.Ping;
		serverData.SearchResultsIndex = server.SearchResultsIndex;

		AvailableServersData.Add(serverData);
	}

	UE_LOG(LogTemp, Display, TEXT("UHeliGameInstance::FindServers ~ Found %d servers"), AvailableServersData.Num());
	
	if (MainMenu.IsValid() && !MainMenu->IsPendingKillOrUnreachable())
	{		
		MainMenu->SetAvailableServerList(AvailableServersData);
	}
}

/** Initiates the session searching */
bool UHeliGameInstance::FindSessions(ULocalPlayer* PlayerOwner, bool bFindLAN)
{
	bool bResult = false;

	check(PlayerOwner != nullptr);
	if (PlayerOwner)
	{
		AHeliGameSession* const GameSession = GetGameSession();
		if (GameSession)
		{
			IsInSearchingServerProcess = true;

			GameSession->OnFindSessionsComplete().RemoveAll(this);
			OnSearchSessionsCompleteDelegateHandle = GameSession->OnFindSessionsComplete().AddUObject(this, &UHeliGameInstance::OnSearchSessionsComplete);

			GameSession->FindSessions(PlayerOwner->GetPreferredUniqueNetId(), GameSessionName, bFindLAN, true);

			bResult = true;
		}
	}

	return bResult;
}

/** Callback which is intended to be called upon finding sessions */
void UHeliGameInstance::OnSearchSessionsComplete(bool bWasSuccessful)
{
	AHeliGameSession* const Session = GetGameSession();
	if (Session)
	{
		Session->OnFindSessionsComplete().Remove(OnSearchSessionsCompleteDelegateHandle);
	}

	UpdateAvailableServers();

	RefreshServerList();

	IsInSearchingServerProcess = false;

	StopLoadingScreen();
}

void UHeliGameInstance::UpdateAvailableServers()
{

	if (AvailableServers.Num() > 0)
	{
		AvailableServers.Empty();
	}


	AHeliGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		int32 CurrentSearchIdx, NumSearchResults;
		EOnlineAsyncTaskState::Type SearchState = GameSession->GetSearchResultStatus(CurrentSearchIdx, NumSearchResults);

		UE_LOG(LogTemp, Display, TEXT("UHeliGameInstance::UpdateAvailableServers ~ %s, %d, %d"), EOnlineAsyncTaskState::ToString(SearchState), CurrentSearchIdx, NumSearchResults);
		switch (SearchState)
		{
		case EOnlineAsyncTaskState::InProgress:
			// TODO: show loading screen ?? for now...
			ShowLoadingScreen("Updating server list...");
			break;
		case EOnlineAsyncTaskState::Done:
			//
		{
			const TArray<FOnlineSessionSearchResult> & SearchResults = GameSession->GetSearchResults();
			check(SearchResults.Num() == NumSearchResults);
			if (NumSearchResults == 0)
			{
				// TODO: add text NoServersFound,"NO SERVERS FOUND, PRESS SPACE TO TRY AGAIN"
			}
			else
			{
				// TODO: add text: PRESS FIND TO REFRESH SERVER LIST
			}


			for (int32 IdxResult = 0; IdxResult < NumSearchResults; ++IdxResult)
			{
				const FOnlineSessionSearchResult& Result = SearchResults[IdxResult];

				FString currentPlayers = FString::FromInt(Result.Session.SessionSettings.NumPublicConnections
					+ Result.Session.SessionSettings.NumPrivateConnections
					- Result.Session.NumOpenPublicConnections
					- Result.Session.NumOpenPrivateConnections);

				FString maxPlayers = FString::FromInt(Result.Session.SessionSettings.NumPublicConnections
					+ Result.Session.SessionSettings.NumPrivateConnections);

				FString gameType = FString(TEXT("NONE"));
				Result.Session.SessionSettings.Get(SETTING_GAMEMODE, gameType);
				FString mapName = FString(TEXT("NONE"));
				Result.Session.SessionSettings.Get(SETTING_MAPNAME, mapName);

				FString serverName = FString(TEXT("NONE"));
				Result.Session.SessionSettings.Get(SERVER_NAME_SETTINGS_KEY, serverName);

				FServerEntry NewServerEntry(
					//Result.Session.OwningUserName,
					serverName,
					currentPlayers,
					maxPlayers,
					gameType,
					mapName,
					FString::FromInt(Result.PingInMs),
					IdxResult
				);

				AvailableServers.Add(NewServerEntry);
			}

		}
		break;

		case EOnlineAsyncTaskState::Failed:
			StopLoadingScreen();
			// TODO: failed message
			UE_LOG(LogTemp, Error, TEXT("UHeliGameInstance::UpdateAvailableServers ~ %s, %d, %d"), EOnlineAsyncTaskState::ToString(SearchState), CurrentSearchIdx, NumSearchResults);
			break;
		case EOnlineAsyncTaskState::NotStarted:
			UE_LOG(LogTemp, Warning, TEXT("UHeliGameInstance::UpdateAvailableServers ~ %s, %d, %d"), EOnlineAsyncTaskState::ToString(SearchState), CurrentSearchIdx, NumSearchResults);
			break;
		default:
			UE_LOG(LogTemp, Error, TEXT("UHeliGameInstance::UpdateAvailableServers ~ %s, %d, %d"), EOnlineAsyncTaskState::ToString(SearchState), CurrentSearchIdx, NumSearchResults);
			break;

		}
	}
}

/*
* Join
*/

bool UHeliGameInstance::JoinServer(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults)
{
	UE_LOG(LogTemp, Display, TEXT("UHeliGameInstance::JoinServer ~ Joining in server from index = %d"), SessionIndexInSearchResults);

	return JoinSession(LocalPlayer, SessionIndexInSearchResults);
}
// end of MenuInterface


bool UHeliGameInstance::JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults)
{
	// needs to tear anything down based on current state?	
	
	ShowLoadingScreen("Loading...");

	AHeliGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		AddNetworkFailureHandlers();

		OnJoinSessionCompleteDelegateHandle = GameSession->OnJoinSessionComplete().AddUObject(this, &UHeliGameInstance::OnJoinSessionComplete);
		if (GameSession->JoinSession(LocalPlayer->GetPreferredUniqueNetId(), GameSessionName, SessionIndexInSearchResults))
		{
			// If any error occurred in the above, pending state would be set
			if ((PendingState == CurrentState) || (PendingState == EHeliGameInstanceState::None))
			{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				GotoState(EHeliGameInstanceState::Playing);
				return true;
			}
		}
	}

	return false;
}

/** Callback which is intended to be called upon finding sessions */
void UHeliGameInstance::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	// unhook the delegate
	AHeliGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		GameSession->OnJoinSessionComplete().Remove(OnJoinSessionCompleteDelegateHandle);
	}

	FinishJoinSession(Result);
}

void UHeliGameInstance::FinishJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		// TODO: failure message and go to main menu
		GotoState(EHeliGameInstanceState::MainMenu);
		return;
	}

	TravelToSession(GameSessionName);
}

void UHeliGameInstance::TravelToSession(const FName& SessionName)
{
	APlayerController* const PlayerController = GetFirstLocalPlayerController();

	if (PlayerController == nullptr)
	{
		RemoveNetworkFailureHandlers();
		GotoState(EHeliGameInstanceState::MainMenu);
		return;
	}

	// travel to session
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub == nullptr)
	{
		RemoveNetworkFailureHandlers();
		GotoState(EHeliGameInstanceState::MainMenu);
		return;
	}

	FString URL;
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

	if (!Sessions.IsValid() || !Sessions->GetResolvedConnectString(SessionName, URL))
	{
		GotoState(EHeliGameInstanceState::MainMenu);
		UE_LOG(LogOnlineGame, Warning, TEXT("Failed to travel to session upon joining it"));
		return;
	}

	UE_LOG(LogLoad, Log, TEXT("UHeliGameInstance->TravelToSession: %s"), *URL);
	
	PlayerController->ClientTravel(URL, TRAVEL_Absolute);
}

void UHeliGameInstance::TravelToIP(const FString& IpAddress)
{
	APlayerController* const playerController = GetFirstLocalPlayerController();

	if (playerController && *IpAddress && !IpAddress.IsEmpty())
	{
		UE_LOG(LogLoad, Log, TEXT("UHeliGameInstance::TravelToIP ~ %s"), *IpAddress);
		playerController->ClientTravel(IpAddress, TRAVEL_Absolute);
	}
}

/*
* Destroy
*/

void UHeliGameInstance::OnEndSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Log, TEXT("UHeliGameInstance::OnEndSessionComplete: Session=%s bWasSuccessful=%s"), *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"));

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
			Sessions->ClearOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegateHandle);
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		}
	}

	// continue
	CleanupSessionOnReturnToMenu();
}

void UHeliGameInstance::CleanupSessionOnReturnToMenu()
{
	bool bPendingOnlineOp = false;

	// end online game and then destroy it
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();
	IOnlineSessionPtr Sessions = (OnlineSub != nullptr) ? OnlineSub->GetSessionInterface() : nullptr;

	if (Sessions.IsValid())
	{
		EOnlineSessionState::Type SessionState = Sessions->GetSessionState(GameSessionName);
		//UE_LOG(LogOnline, Log, TEXT("Session %s is '%s'"), *GameSessionName.ToString(), EOnlineSessionState::ToString(SessionState));

		if (EOnlineSessionState::InProgress == SessionState)
		{
			//UE_LOG(LogOnline, Log, TEXT("Ending session %s on return to main menu"), *GameSessionName.ToString());
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			Sessions->EndSession(GameSessionName);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Ending == SessionState)
		{
			//UE_LOG(LogOnline, Log, TEXT("Waiting for session %s to end on return to main menu"), *GameSessionName.ToString());
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Ended == SessionState || EOnlineSessionState::Pending == SessionState)
		{
			//UE_LOG(LogOnline, Log, TEXT("Destroying session %s on return to main menu"), *GameSessionName.ToString());
			OnDestroySessionCompleteDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			Sessions->DestroySession(GameSessionName);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Starting == SessionState)
		{
			//UE_LOG(LogOnline, Log, TEXT("Waiting for session %s to start, and then we will end it to return to main menu"), *GameSessionName.ToString());
			OnStartSessionCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			bPendingOnlineOp = true;
		}
	}

	if (!bPendingOnlineOp)
	{
		//GEngine->HandleDisconnect( GetWorld(), GetWorld()->GetNetDriver() );
	}
}

void UHeliGameInstance::TravelLocalSessionFailure(UWorld *World, ETravelFailure::Type FailureType, const FString& ReasonString)
{
	// we wont have more than one local players... no split screen... so get first local player will do
	//AHeliPlayerController* const FirstPC = Cast<AHeliPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	//AHeliPlayerController* const FirstPC = Cast<AHeliPlayerController>(GetFirstLocalPlayerController());
	APlayerController* const FirstPC = GetFirstLocalPlayerController();
	if (FirstPC != nullptr)
	{
		// TODO: message to let player know it.... session failed
		GotoState(EHeliGameInstanceState::MainMenu);
	}
}

void UHeliGameInstance::RemoveNetworkFailureHandlers()
{
	// Remove the local session/travel failure bindings if they exist
	if (GEngine->OnTravelFailure().IsBoundToObject(this) == true)
	{
		GEngine->OnTravelFailure().Remove(TravelLocalSessionFailureDelegateHandle);
	}
}



void UHeliGameInstance::BeginServerSearch(ULocalPlayer* PlayerOwner, bool bLANMatch)
{
	ShowLoadingScreen("Searching...");

	FindSessions(PlayerOwner, bLANMatch);
}

void  UHeliGameInstance::JoinFromServerList(ULocalPlayer* LocalPlayer, FServerEntry Server)
{	
	int ServerToJoin = Server.SearchResultsIndex;

	UE_LOG(LogLoad, Log, TEXT("%s"), *FString::Printf(TEXT("JoinFromServerList: ServerName: %s, CurrentPlayers: %s, MaxPlayers: %s, GameType: %s, MapName: %s, Ping: %s, SearchResultsIndex: %s"), *Server.ServerName, *Server.CurrentPlayers, *Server.MaxPlayers, *Server.GameType, *Server.MapName, *Server.Ping, *FString::FromInt(Server.SearchResultsIndex)));

	JoinSession(LocalPlayer, ServerToJoin);
}



////////////////////////////////////////////////////////////////////
//                              Tick                              /
//////////////////////////////////////////////////////////////////
bool UHeliGameInstance::Tick(float DeltaSeconds)
{
	// Dedicated server doesn't need to worry about game state
	if (IsRunningDedicatedServer() == true)
	{
		return true;
	}

	MaybeChangeState();

	if (CurrentState != EHeliGameInstanceState::WelcomeScreen)
	{
		// If at any point we aren't licensed (but we are after welcome screen) bounce them back to the main menu - dude needs to purchase it... xD
		if (!bIsLicensed && CurrentState != EHeliGameInstanceState::None)
		{
			// TODO: inform dude he needs to purchase
			GotoState(EHeliGameInstanceState::MainMenu);
		}
	}

	return true;
}

void UHeliGameInstance::SetIsOnline(bool bInIsOnline)
{
	bIsOnline = bInIsOnline;
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		for (int32 i = 0; i < LocalPlayers.Num(); ++i)
		{
			ULocalPlayer* LocalPlayer = LocalPlayers[i];

			TSharedPtr<const FUniqueNetId> PlayerId = LocalPlayer->GetPreferredUniqueNetId();
			if (PlayerId.IsValid())
			{
				OnlineSub->SetUsingMultiplayerFeatures(*PlayerId, bIsOnline);
			}
		}
	}
}

bool UHeliGameInstance::IsLocalPlayerOnline(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr)
	{
		return false;
	}
	const auto OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		const auto IdentityInterface = OnlineSub->GetIdentityInterface();
		if (IdentityInterface.IsValid())
		{
			auto UniqueId = LocalPlayer->GetCachedUniqueNetId();
			if (UniqueId.IsValid())
			{
				const auto LoginStatus = IdentityInterface->GetLoginStatus(*UniqueId);
				if (LoginStatus == ELoginStatus::LoggedIn)
				{
					return true;
				}
			}
		}
	}

	return false;
}

void UHeliGameInstance::ChangePlayerName(FString NewPlayerName)
{
	APlayerController* const FirstPC = GetFirstLocalPlayerController();

	if (FirstPC != nullptr)
	{
		AHeliPlayerState* PlayerState = Cast<AHeliPlayerState>(FirstPC->PlayerState);
		if (PlayerState)
		{
			PlayerState->Server_SetPlayerName(NewPlayerName);
		}
	}
}

bool UHeliGameInstance::UpdateSessionSettings(ULocalPlayer* LocalPlayer, const FString& GameType, FName SessionName, const FString& MapName, const FString& ServerName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers)
{
	AHeliGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{				
		return GameSession->UpdateSessionSettings(LocalPlayer->GetPreferredUniqueNetId(), SessionName, GameType, MapName, ServerName, bIsLAN, bIsPresence, MaxNumPlayers);
	}

	return false;
}

void UHeliGameInstance::EndRoundAndRestartMatch()
{
	UWorld* const World = GetWorld();
	AHeliGameState* const GameState = World != nullptr ? World->GetGameState<AHeliGameState>() : nullptr;

	if (GameState)
	{
		GameState->RequestEndRoundAndRestartMatch();
	}
}

/****************************************************************************************************
*                                              SCORE BOARD                                          *
*****************************************************************************************************/

void UHeliGameInstance::UpdatePlayerStateMaps()
{
	APlayerController* const FirstPC = GetFirstLocalPlayerController();

	if (FirstPC != nullptr && FirstPC->GetWorld() != nullptr)
	{
		AHeliGameState* const GameState = Cast<AHeliGameState>(FirstPC->GetWorld()->GetGameState());
		if (GameState)
		{
			const int32 NumTeams = FMath::Max(GameState->NumTeams, 1);
			LastTeamPlayerCount.Reset();
			LastTeamPlayerCount.AddZeroed(PlayerStateMaps.Num());
			for (int32 i = 0; i < PlayerStateMaps.Num(); i++)
			{
				LastTeamPlayerCount[i] = PlayerStateMaps[i].Num();
			}

			PlayerStateMaps.Reset();
			PlayerStateMaps.AddZeroed(NumTeams);

			for (int32 i = 0; i < NumTeams; i++)
			{
				GameState->GetRankedMap(i, PlayerStateMaps[i]);

				if (LastTeamPlayerCount.Num() > 0 && PlayerStateMaps[i].Num() != LastTeamPlayerCount[i])
				{
					bRequiresWidgetUpdate = true;
				}
			}
		}
	}
}

uint8 UHeliGameInstance::GetNumberOfTeams()
{
	return PlayerStateMaps.Num();
}

int32 UHeliGameInstance::GetNumberOfPlayersInTeam(uint8 TeamNum)
{
	if (PlayerStateMaps.IsValidIndex(TeamNum))
	{
		return PlayerStateMaps[TeamNum].Num();
	}

	UE_LOG(LogTemp, Warning, TEXT("UHeliGameInstance::GetNumberOfPlayersInTeam - found 0 players in team %d"), TeamNum);
	return 0;
}

AHeliPlayerState* UHeliGameInstance::GetPlayerStateFromPlayerInRankedPlayerMap(const FTeamPlayer& TeamPlayer) const
{
	if (PlayerStateMaps.IsValidIndex(TeamPlayer.TeamNum) && PlayerStateMaps[TeamPlayer.TeamNum].Contains(TeamPlayer.PlayerId))
	{
		return PlayerStateMaps[TeamPlayer.TeamNum].FindRef(TeamPlayer.PlayerId).Get();
	}

	return nullptr;
}


/****************************** HELPERS ***************************************************/

FString UHeliGameInstance::GetEHeliGameInstanceStateEnumAsString(EHeliGameInstanceState EnumValue)
{
	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EHeliGameInstanceState"), true);
	if (!EnumPtr) return FString("Invalid");

	return EnumPtr->GetNameStringByIndex((uint32)EnumValue); // for EnumValue == VE_Dance returns "VE_Dance"
}

FString UHeliGameInstance::GetEHeliMapEnumAsString(EHeliMap EnumValue)
{
	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EHeliMap"), true);
	if (!EnumPtr) return FString("Invalid");

	return EnumPtr->GetNameStringByIndex((uint32)EnumValue);
}

EHeliMap UHeliGameInstance::GetEHeliMapEnumValueFromString(const FString& EnumName)
{
	UEnum* Enum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EHeliMap"), true);
	if (!Enum)
	{
		return EHeliMap(0);
	}
	return (EHeliMap)Enum->GetIndexByName(FName(*EnumName));
}

FString UHeliGameInstance::GetGameVersion()
{
	return GameVersionName;
}