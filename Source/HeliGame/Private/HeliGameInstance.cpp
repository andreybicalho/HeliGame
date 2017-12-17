// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliGameInstance.h"
#include "HeliGame.h"
#include "Online.h"
#include "OnlineKeyValuePair.h"
#include "HeliPlayerController.h"
#include "HeliPlayerState.h"
#include "HeliGameSession.h"
#include "HeliGameState.h"
#include "HeliLobbyGameState.h"
#include "Helicopter.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "Misc/CoreDelegates.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/GameMode.h"
#include "Public/Engine.h" // avoid the monolithic header file Runtime\Engine\Classes\Engine\Engine.h
#include "Engine/Canvas.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"


UHeliGameInstance::UHeliGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsOnline(false) // Default to LAN
	, bIsLicensed(true) // Default to licensed (should have been checked by OS on boot)
{
	// loading screen widget
	static ConstructorHelpers::FClassFinder<UUserWidget> LoadingScreenWidget(TEXT("/Game/HeliBattle/UI/UMG/Menu/LoadingScreen"));
	if (LoadingScreenWidget.Class != NULL)
	{
		LoadingScreenWidgetTemplate = LoadingScreenWidget.Class;
	}

	// main menu widget
	static ConstructorHelpers::FClassFinder<UUserWidget> MainMenuWidget(TEXT("/Game/HeliBattle/UI/UMG/Menu/MainMenu"));
	if (MainMenuWidget.Class != NULL)
	{
		MainMenuWidgetTemplate = MainMenuWidget.Class;
	}

	// hosting menu widget
	static ConstructorHelpers::FClassFinder<UUserWidget> HostingMenuWidget(TEXT("/Game/HeliBattle/UI/UMG/Menu/HostingMenu"));
	if (HostingMenuWidget.Class != NULL)
	{
		HostingMenuWidgetTemplate = HostingMenuWidget.Class;
	}

	// find server menu widget
	static ConstructorHelpers::FClassFinder<UUserWidget> FindServerMenuWidget(TEXT("/Game/HeliBattle/UI/UMG/Menu/ServerList"));
	if (FindServerMenuWidget.Class != NULL)
	{
		FindServerMenuWidgetTemplate = FindServerMenuWidget.Class;
	}

	// options menu widget
	static ConstructorHelpers::FClassFinder<UUserWidget> OptionsMenuWidget(TEXT("/Game/HeliBattle/UI/UMG/Menu/Options/MainOptions"));
	if (OptionsMenuWidget.Class != NULL)
	{
		OptionsMenuWidgetTemplate = OptionsMenuWidget.Class;
	}

	// lobby menu widget
	static ConstructorHelpers::FClassFinder<UUserWidget> LobbyMenuWidget(TEXT("/Game/HeliBattle/UI/UMG/Lobby/LobbyMenu"));
	if (LobbyMenuWidget.Class != NULL)
	{
		LobbyMenuWidgetTemplate = LobbyMenuWidget.Class;
	}

	// about menu widget
	static ConstructorHelpers::FClassFinder<UUserWidget> AboutMenuWidget(TEXT("/Game/HeliBattle/UI/UMG/Menu/About"));
	if (AboutMenuWidget.Class != NULL)
	{
		AboutMenuWidgetTemplate = AboutMenuWidget.Class;
	}
	



	CurrentState = EHeliGameInstanceState::None;

	MainMenuMap = *FString::Printf(TEXT("/Game/Maps/EntryMenu"));

	MaxNumberOfPlayers = 16;

	WarmupTime = 10;
	RoundTime = 1200;
	TimeBetweenMatches = 30;
	bAllowFriendFireDamage = false;

	IsInSearchingServerProcess = false;

	LoadingMessage = FString(TEXT("Loading..."));

	bRequiresWidgetUpdate = true;

	CustomServerName = FString(TEXT("Server Name"));
	SelectedGameMode = FString(TEXT("TDM"));
	SelectedMapName = FString(TEXT("Dev"));
	//CustomPlayerName = FString(TEXT(""));

	bShouldUpdateLobbyWidget = false;
}

void UHeliGameInstance::Init()
{
	Super::Init();

	IgnorePairingChangeForControllerId = -1;
	CurrentConnectionStatus = EOnlineServerConnectionStatus::Connected;

	// game requires the ability to ID users.
	const auto OnlineSub = IOnlineSubsystem::Get();
	//check(OnlineSub);
	// TODO: check pointesrs
	if (OnlineSub) {
		const auto IdentityInterface = OnlineSub->GetIdentityInterface();
		//check(IdentityInterface.IsValid());

		const auto SessionInterface = OnlineSub->GetSessionInterface();
		//check(SessionInterface.IsValid());

		// bind any OSS delegates we needs to handle
		for (int i = 0; i < MAX_LOCAL_PLAYERS; ++i)
		{
			IdentityInterface->AddOnLoginStatusChangedDelegate_Handle(i, FOnLoginStatusChangedDelegate::CreateUObject(this, &UHeliGameInstance::HandleUserLoginChanged));
		}

		IdentityInterface->AddOnControllerPairingChangedDelegate_Handle(FOnControllerPairingChangedDelegate::CreateUObject(this, &UHeliGameInstance::HandleControllerPairingChanged));

		OnlineSub->AddOnConnectionStatusChangedDelegate_Handle(FOnConnectionStatusChangedDelegate::CreateUObject(this, &UHeliGameInstance::HandleNetworkConnectionStatusChanged));

		SessionInterface->AddOnSessionFailureDelegate_Handle(FOnSessionFailureDelegate::CreateUObject(this, &UHeliGameInstance::HandleSessionFailure));
		
	}

	FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &UHeliGameInstance::HandleAppWillDeactivate);

	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UHeliGameInstance::HandleAppSuspend);
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &UHeliGameInstance::HandleAppResume);

	FCoreDelegates::OnSafeFrameChangedEvent.AddUObject(this, &UHeliGameInstance::HandleSafeFrameChanged);
	FCoreDelegates::OnControllerConnectionChange.AddUObject(this, &UHeliGameInstance::HandleControllerConnectionChange);
	//FCoreDelegates::ApplicationLicenseChange.AddUObject(this, &UHeliGameInstance::HandleAppLicenseUpdate); 

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UHeliGameInstance::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UHeliGameInstance::OnPostLoadMap);

	

	OnEndSessionCompleteDelegate = FOnEndSessionCompleteDelegate::CreateUObject(this, &UHeliGameInstance::OnEndSessionComplete);

	// Register delegate for ticker callback
	TickDelegate = FTickerDelegate::CreateUObject(this, &UHeliGameInstance::Tick);
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
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

void UHeliGameInstance::OnUserCanPlayInvite(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();

	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{

	}
	else
	{
		//DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
		//GotoState(EHeliGameInstanceState::WelcomeScreen); // since we dont have a welcomescreen
		GotoState(EHeliGameInstanceState::MainMenu);
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

void UHeliGameInstance::SetMaxNumberOfPlayers(int32 NewMaxNumberOfPlayers)
{
	MaxNumberOfPlayers = NewMaxNumberOfPlayers;
		
	UpdateSessionSettings(GetFirstGamePlayer(), SelectedGameMode, GameSessionName, SelectedMapName, FName(*CustomServerName), GetIsOnline(), true, MaxNumberOfPlayers);
}

int32 UHeliGameInstance::GetMaxNumberOfPlayers()
{
	return MaxNumberOfPlayers;
}

void UHeliGameInstance::SetSelectedMapName(const FString& NewMapName)
{
	SelectedMapName = NewMapName;
}

void UHeliGameInstance::SetSelectedGameMode(const FString& NewGameMode)
{
	SelectedGameMode = NewGameMode;

	UpdateSessionSettings(GetFirstGamePlayer(), SelectedGameMode, GameSessionName, SelectedMapName, FName(*CustomServerName), GetIsOnline(), true, MaxNumberOfPlayers);
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

	if (NewState == EHeliGameInstanceState::PendingInvite)
	{
		BeginPendingInviteState();
	}
	else if (NewState == EHeliGameInstanceState::WelcomeScreen)
	{
		BeginWelcomeScreenState();
	}
	else if (NewState == EHeliGameInstanceState::MainMenu)
	{
		BeginMainMenuState();
	}
	else if (NewState == EHeliGameInstanceState::HostingMenu)
	{
		BeginHostingMenuState();
	}
	else if (NewState == EHeliGameInstanceState::FindServerMenu)
	{
		BeginFindServerMenuState();
	}
	else if (NewState == EHeliGameInstanceState::MessageMenu)
	{
		BeginMessageMenuState();
	}
	else if (NewState == EHeliGameInstanceState::Playing)
	{
		BeginPlayingState();
	}
	else if (NewState == EHeliGameInstanceState::OptionsMenu)
	{
		BeginOptionsMenuState();
	}
	else if (NewState == EHeliGameInstanceState::LobbyMenu)
	{
		BeginLobbyMenuState();
	}
	else if (NewState == EHeliGameInstanceState::AboutMenu)
	{
		BeginAboutMenuState();
	}

	CurrentState = NewState;
}

void UHeliGameInstance::EndCurrentState(EHeliGameInstanceState NextState)
{
	// per-state custom ending code here
	if (CurrentState == EHeliGameInstanceState::PendingInvite)
	{
		EndPendingInviteState();
	}
	else if (CurrentState == EHeliGameInstanceState::WelcomeScreen)
	{
		EndWelcomeScreenState();
	}
	else if (CurrentState == EHeliGameInstanceState::MainMenu)
	{
		EndMainMenuState();
	}
	else if (CurrentState == EHeliGameInstanceState::HostingMenu)
	{
		EndHostingMenuState();
	}
	else if (CurrentState == EHeliGameInstanceState::FindServerMenu)
	{
		EndFindServerMenuState();
	}
	else if (CurrentState == EHeliGameInstanceState::MessageMenu)
	{
		EndMessageMenuState();
	}
	else if (CurrentState == EHeliGameInstanceState::Playing)
	{
		EndPlayingState();
	}
	else if (CurrentState == EHeliGameInstanceState::OptionsMenu)
	{
		EndOptionsMenuState();
	}
	else if (CurrentState == EHeliGameInstanceState::LobbyMenu)
	{
		EndLobbyMenuState(NextState);
	}
	else if (CurrentState == EHeliGameInstanceState::AboutMenu)
	{
		EndAboutMenuState();
	}

	CurrentState = EHeliGameInstanceState::None;
}

void UHeliGameInstance::BeginWelcomeScreenState()
{
	//this must come before split screen player removal so that the OSS sets all players to not using online features.
	SetIsOnline(false);

	// Remove any possible splitscren players
	//RemoveSplitScreenPlayers();

	LoadFrontEndMap(WelcomeScreenMap);

	ULocalPlayer* const LocalPlayer = GetFirstGamePlayer();
	LocalPlayer->SetCachedUniqueNetId(nullptr);
	// TODO: add welcome screen to the viewport

	// Disallow splitscreen (we will allow while in the playing state)
	//GetGameViewportClient()->SetDisableSplitscreenOverride(true);
}

void UHeliGameInstance::EndWelcomeScreenState()
{
	// TODO: remove welcome screen from viewport
}

void UHeliGameInstance::BeginPendingInviteState()
{
	if (LoadFrontEndMap(MainMenuMap))
	{
		//StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &UHeliGameInstance::OnUserCanPlayInvite), EUserPrivileges::CanPlayOnline, PendingInvite.UserId);
	}
	else
	{
		//GotoState(EHeliGameInstanceState::WelcomeScreen);
		GotoState(EHeliGameInstanceState::MainMenu);
	}
}

void UHeliGameInstance::EndPendingInviteState()
{
	// cleanup in case the state changed before the pending invite was handled.
	CleanupOnlinePrivilegeTask();
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
	// Make sure we're not showing the loadscreen
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && LoadingScreen.IsValid())
	{
		LoadingScreen->RemoveFromViewport();
	}

	// Disallow splitscreen
	// GetGameViewportClient()->SetDisableSplitscreenOverride(true);

	// Set presence to menu state for the owning player
	SetPresenceForLocalPlayers(FVariantData(FString(TEXT("OnMenu"))));

	// load startup map
	LoadFrontEndMap(MainMenuMap);

	if(MyViewport && MainMenuWidgetTemplate)
	{
		if (MainMenu.IsValid()) 
		{
			MainMenu->AddToViewport();
		}
		else
		{
			APlayerController* const FirstPC = GetFirstLocalPlayerController();
			FirstPC->SetIgnoreLookInput(true);
			FirstPC->SetIgnoreMoveInput(true);
			FirstPC->bShowMouseCursor = true;

			MainMenu = CreateWidget<UUserWidget>(FirstPC, MainMenuWidgetTemplate);
			MainMenu->AddToViewport();
			MainMenu->SetUserFocus(FirstPC);
			MainMenu->SetKeyboardFocus();
			
		}
	}



	// player 0 gets to own the UI
	ULocalPlayer* const Player = GetFirstGamePlayer();

	if (Player != nullptr)
	{
		Player->SetControllerId(0);
		Player->SetCachedUniqueNetId(Player->GetUniqueNetIdFromCachedControllerId());
	}

	RemoveNetworkFailureHandlers();
}

void UHeliGameInstance::EndMainMenuState()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && MainMenu.IsValid())
	{
		MainMenu->RemoveFromViewport();
	}
}

void UHeliGameInstance::BeginHostingMenuState()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && MainMenu.IsValid()) {
		MainMenu->RemoveFromViewport();
	}

	if (MyViewport && HostingMenuWidgetTemplate)
	{
		if (HostingMenu.IsValid())
		{
			HostingMenu->AddToViewport();
		}
		else
		{
			APlayerController* const FirstPC = GetFirstLocalPlayerController();
			FirstPC->SetIgnoreLookInput(true);
			FirstPC->SetIgnoreMoveInput(true);
			FirstPC->bShowMouseCursor = true;

			HostingMenu = CreateWidget<UUserWidget>(FirstPC, HostingMenuWidgetTemplate);
			HostingMenu->AddToViewport();
			HostingMenu->SetUserFocus(FirstPC);
			HostingMenu->SetKeyboardFocus();

		}
	}
}

void UHeliGameInstance::EndHostingMenuState()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && HostingMenu.IsValid())
	{
		HostingMenu->RemoveFromViewport();
	}
}


void UHeliGameInstance::BeginFindServerMenuState()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && MainMenu.IsValid()) {
		MainMenu->RemoveFromViewport();
	}

	if (MyViewport && FindServerMenuWidgetTemplate)
	{
		if (FindServerMenu.IsValid())
		{
			FindServerMenu->AddToViewport();
		}
		else
		{
			APlayerController* const FirstPC = GetFirstLocalPlayerController();
			FirstPC->SetIgnoreLookInput(true);
			FirstPC->SetIgnoreMoveInput(true);
			FirstPC->bShowMouseCursor = true;

			FindServerMenu = CreateWidget<UUserWidget>(FirstPC, FindServerMenuWidgetTemplate);
			FindServerMenu->AddToViewport();
			FindServerMenu->SetUserFocus(FirstPC);
			FindServerMenu->SetKeyboardFocus();
		}
	}
}

void UHeliGameInstance::EndFindServerMenuState()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && FindServerMenu.IsValid())
	{
		FindServerMenu->RemoveFromViewport();
	}
}


void UHeliGameInstance::BeginMessageMenuState()
{
	// TODO: add message menu widget to the viewport
}

void UHeliGameInstance::EndMessageMenuState()
{
	// TODO: remove message menu from viewport
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
	// Disallow splitscreen
	//GetGameViewportClient()->SetDisableSplitscreenOverride(true);

	// Clear the players' presence information
	SetPresenceForLocalPlayers(FVariantData(FString(TEXT("OnLobby"))));

	UWorld* const World = GetWorld();
	AHeliGameState* const GameState = World != NULL ? World->GetGameState<AHeliGameState>() : NULL;

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

void UHeliGameInstance::BeginOptionsMenuState()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && MainMenu.IsValid()) {
		MainMenu->RemoveFromViewport();
	}

	if (MyViewport && OptionsMenuWidgetTemplate)
	{
		if (OptionsMenu.IsValid())
		{
			OptionsMenu->AddToViewport();
		}
		else
		{
			APlayerController* const FirstPC = GetFirstLocalPlayerController();
			FirstPC->SetIgnoreLookInput(true);
			FirstPC->SetIgnoreMoveInput(true);
			FirstPC->bShowMouseCursor = true;

			OptionsMenu = CreateWidget<UUserWidget>(FirstPC, OptionsMenuWidgetTemplate);
			OptionsMenu->AddToViewport();
			OptionsMenu->SetUserFocus(FirstPC);
			OptionsMenu->SetKeyboardFocus();
		}
	}
}

void UHeliGameInstance::EndOptionsMenuState()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && OptionsMenu.IsValid())
	{
		OptionsMenu->RemoveFromViewport();
	}
}


void UHeliGameInstance::BeginLobbyMenuState()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport) {	
		MyViewport->RemoveAllViewportWidgets();
	}

	StopLoadingScreen();

	if (MyViewport && LobbyMenuWidgetTemplate)
	{
		if (LobbyMenu.IsValid())
		{
			LobbyMenu->AddToViewport();
		}
		else
		{
			APlayerController* const FirstPC = GetFirstLocalPlayerController();
			FirstPC->SetIgnoreLookInput(true);
			FirstPC->SetIgnoreMoveInput(true);
			FirstPC->bShowMouseCursor = true;

			LobbyMenu = CreateWidget<UUserWidget>(FirstPC, LobbyMenuWidgetTemplate);
			LobbyMenu->AddToViewport();
			LobbyMenu->SetUserFocus(FirstPC);
			LobbyMenu->SetKeyboardFocus();
		}
	}
}

void UHeliGameInstance::EndLobbyMenuState(EHeliGameInstanceState NextState)
{
	// if we go back to main menu, then cleanup session
	if (NextState == EHeliGameInstanceState::MainMenu) {
		// Clear the players' presence information
		SetPresenceForLocalPlayers(FVariantData(FString(TEXT("OnMenu"))));

		UWorld* const World = GetWorld();
		AHeliLobbyGameState* const GameState = World != NULL ? World->GetGameState<AHeliLobbyGameState>() : NULL;

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
	

	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && LobbyMenu.IsValid())
	{
		LobbyMenu->RemoveFromViewport();
	}
}

void UHeliGameInstance::BeginAboutMenuState()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && MainMenu.IsValid()) {
		MainMenu->RemoveFromViewport();
	}

	if (MyViewport && AboutMenuWidgetTemplate)
	{
		if (AboutMenu.IsValid())
		{
			AboutMenu->AddToViewport();
		}
		else
		{
			APlayerController* const FirstPC = GetFirstLocalPlayerController();
			FirstPC->SetIgnoreLookInput(true);
			FirstPC->SetIgnoreMoveInput(true);
			FirstPC->bShowMouseCursor = true;

			AboutMenu = CreateWidget<UUserWidget>(FirstPC, AboutMenuWidgetTemplate);
			AboutMenu->AddToViewport();
			AboutMenu->SetUserFocus(FirstPC);
			AboutMenu->SetKeyboardFocus();

		}
	}
}

void UHeliGameInstance::EndAboutMenuState()
{
	UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

	if (MyViewport && AboutMenu.IsValid())
	{
		AboutMenu->RemoveFromViewport();
	}
}

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
	IOnlineSessionPtr Sessions = (OnlineSub != NULL) ? OnlineSub->GetSessionInterface() : NULL;

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

void UHeliGameInstance::LabelPlayerAsQuitter(ULocalPlayer* LocalPlayer) const
{
	AHeliPlayerState* const PlayerState = LocalPlayer && LocalPlayer->PlayerController ? Cast<AHeliPlayerState>(LocalPlayer->PlayerController->PlayerState) : nullptr;
	if (PlayerState)
	{
		PlayerState->SetQuitter(true);
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
*      Host, Join, Find                                   *
*                                                         *
***********************************************************/
// starts playing a game as the host
bool UHeliGameInstance::HostGame(ULocalPlayer* LocalPlayer, const FString& GameType, const FString& ServerName, const FString& InTravelURL)
{
	UE_LOG(LogLoad, Log, TEXT("%s"), *FString::Printf(TEXT("InTravelURL: %s"), *InTravelURL));

	AHeliGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		// add callback delegate for completion
		OnCreatePresenceSessionCompleteDelegateHandle = GameSession->OnCreatePresenceSessionComplete().AddUObject(this, &UHeliGameInstance::OnCreatePresenceSessionComplete);

		TravelURL = InTravelURL;
		bool const bIsLanMatch = InTravelURL.Contains(TEXT("?bIsLanMatch"));

		// TODO(andrey): tell game session whether the game has friend fire enabled or not
		bool const bAllowFriendlyFireDamage = InTravelURL.Contains(TEXT("?bAllowFriendlyFireDamage"));

		//determine the map name from the travelURL
		const FString& MapNameSubStr = "/Game/Maps/";
		const FString& ChoppedMapName = TravelURL.RightChop(MapNameSubStr.Len());
		const FString& MapName = ChoppedMapName.LeftChop(ChoppedMapName.Len() - ChoppedMapName.Find("?game"));

		//UE_LOG(LogLoad, Log, TEXT("%s"), *FString::Printf(TEXT("GameSessionName: %s, MapName: %s, bIsLanMatch: %s, MaxNumberOfPlayers: %d"), *GameSessionName.ToString(), *MapName, bIsLanMatch ? *FString(TEXT("true")) : *FString(TEXT("false")), MaxNumberOfPlayers));

		if (GameSession->HostSession(LocalPlayer->GetPreferredUniqueNetId(), GameSessionName, GameType, MapName, FName(*ServerName), bIsLanMatch, true, MaxNumberOfPlayers))//AHeliGameSession::DEFAULT_NUM_PLAYERS))
		{
			// If any error occured in the above, pending state would be set
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

	return false;
}

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

bool UHeliGameInstance::JoinSession(ULocalPlayer* LocalPlayer, const FOnlineSessionSearchResult& SearchResult)
{
	// needs to tear anything down based on current state?
	AHeliGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		AddNetworkFailureHandlers();

		OnJoinSessionCompleteDelegateHandle = GameSession->OnJoinSessionComplete().AddUObject(this, &UHeliGameInstance::OnJoinSessionComplete);
		if (GameSession->JoinSession(LocalPlayer->GetPreferredUniqueNetId(), GameSessionName, SearchResult))
		{
			// If any error occured in the above, pending state would be set
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

	InternalTravelToSession(GameSessionName);
}

void UHeliGameInstance::OnRegisterJoiningLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
{
	FinishJoinSession(Result);
}


void UHeliGameInstance::TravelToIP(const FString& IpAddress)
{
	APlayerController* const playerController = GetFirstLocalPlayerController();
	
	if(playerController && *IpAddress && !IpAddress.IsEmpty())
	{ 
		UE_LOG(LogLoad, Log, TEXT("UHeliGameInstance::TravelToIP ~ %s"), *IpAddress);
		playerController->ClientTravel(IpAddress, TRAVEL_Absolute);
	}
}

void UHeliGameInstance::InternalTravelToSession(const FName& SessionName)
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

	UE_LOG(LogLoad, Log, TEXT("UHeliGameInstance->InternalTravelToSession: %s"), *URL);
	
	PlayerController->ClientTravel(URL, TRAVEL_Absolute);
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

	IsInSearchingServerProcess = false;

	StopLoadingScreen();
}

void UHeliGameInstance::BeginServerSearch(ULocalPlayer* PlayerOwner, bool bLANMatch)
{
	ShowLoadingScreen("Searching...");

	FindSessions(PlayerOwner, bLANMatch);	
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
						Result.Session.SessionSettings.Get(SETTING_CUSTOMSEARCHINT1, serverName);

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
				// intended fall-through
				UE_LOG(LogTemp, Error, TEXT("UHeliGameInstance::UpdateAvailableServers ~ %s, %d, %d"), EOnlineAsyncTaskState::ToString(SearchState), CurrentSearchIdx, NumSearchResults);
				break;
			case EOnlineAsyncTaskState::NotStarted:
				// intended fall-through
				UE_LOG(LogTemp, Warning, TEXT("UHeliGameInstance::UpdateAvailableServers ~ %s, %d, %d"), EOnlineAsyncTaskState::ToString(SearchState), CurrentSearchIdx, NumSearchResults);
				break;
			default:
				UE_LOG(LogTemp, Error, TEXT("UHeliGameInstance::UpdateAvailableServers ~ %s, %d, %d"), EOnlineAsyncTaskState::ToString(SearchState), CurrentSearchIdx, NumSearchResults);
				break;
				
		}
	}
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
	
	//UGameViewportClient* MyViewport = Cast<UGameViewportClient>(GetGameViewportClient());

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

bool UHeliGameInstance::HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld)
{
	bool const bOpenSuccessful = Super::HandleOpenCommand(Cmd, Ar, InWorld);
	if (bOpenSuccessful)
	{
		GotoState(EHeliGameInstanceState::Playing);
	}

	return bOpenSuccessful;
}

void UHeliGameInstance::HandleSignInChangeMessaging()
{
	// Master user signed out, go to initial state (if we aren't there already)
	if (CurrentState != GetInitialState())
	{								
		GotoInitialState();
	}
}

void UHeliGameInstance::HandleUserLoginChanged(int32 GameUserIndex, ELoginStatus::Type PreviousLoginStatus, ELoginStatus::Type LoginStatus, const FUniqueNetId& UserId)
{
	const bool bDowngraded = (LoginStatus == ELoginStatus::NotLoggedIn && !GetIsOnline()) || (LoginStatus != ELoginStatus::LoggedIn && GetIsOnline());

	UE_LOG(LogOnline, Log, TEXT("HandleUserLoginChanged: bDownGraded: %i"), (int)bDowngraded);

	// TODO: get bIsLicensed from... i dont know... somewhere
	//TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
	//bIsLicensed = GenericApplication->ApplicationLicenseValid();

	// Find the local player associated with this unique net id
	ULocalPlayer * LocalPlayer = FindLocalPlayerFromUniqueNetId(UserId);

	// If this user is signed out, but was previously signed in, punt to welcome (or remove splitscreen if that makes sense)
	if (LocalPlayer != NULL)
	{
		if (bDowngraded)
		{
			UE_LOG(LogOnline, Log, TEXT("HandleUserLoginChanged: Player logged out: %s"), *UserId.ToString());

			LabelPlayerAsQuitter(LocalPlayer);

			// Check to see if this was the master, or if this was a split-screen player on the client
			if (LocalPlayer == GetFirstGamePlayer() || GetIsOnline())
			{
				HandleSignInChangeMessaging();
			}
			else
			{
				// Remove local split-screen players from the list
				RemoveExistingLocalPlayer(LocalPlayer);
			}
		}
	}
}

void UHeliGameInstance::HandleAppWillDeactivate()
{
	//if (CurrentState == EHeliGameInstanceState::Playing)
	//{
	//	// Just have the first player controller pause the game.
	//	UWorld* const GameWorld = GetWorld();
	//	if (GameWorld)
	//	{
	//		// protect against a second pause menu loading on top of an existing one if someone presses the Jewel / PS buttons.
	//		bool bNeedsPause = true;
	//		for (FConstControllerIterator It = GameWorld->GetControllerIterator(); It; ++It)
	//		{
	//			AHeliPlayerController* Controller = Cast<AHeliPlayerController>(*It);
	//			if (Controller && (Controller->IsPaused() || Controller->IsGameMenuVisible()))
	//			{
	//				bNeedsPause = false;
	//				break;
	//			}
	//		}

	//		if (bNeedsPause)
	//		{
	//			AHeliPlayerController* const Controller = Cast<AHeliPlayerController>(GameWorld->GetFirstPlayerController());
	//			if (Controller)
	//			{
	//				Controller->ShowInGameMenu();
	//			}
	//		}
	//	}
	//}
}

void UHeliGameInstance::HandleAppSuspend()
{
	// Players will lose connection on resume. However it is possible the game will exit before we get a resume, so we must kick off round end events here.
	UE_LOG(LogOnline, Warning, TEXT("UHeliGameInstance::HandleAppSuspend"));
	UWorld* const World = GetWorld();
	AHeliGameState* const GameState = World != NULL ? World->GetGameState<AHeliGameState>() : NULL;

	if (CurrentState != EHeliGameInstanceState::None && CurrentState != GetInitialState())
	{
		UE_LOG(LogOnline, Warning, TEXT("UHeliGameInstance::HandleAppSuspend: Sending round end event for players"));

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
	}
}

void UHeliGameInstance::HandleAppResume()
{
	UE_LOG(LogOnline, Log, TEXT("UHeliGameInstance::HandleAppResume"));

	if (CurrentState != EHeliGameInstanceState::None && CurrentState != GetInitialState())
	{
		UE_LOG(LogOnline, Warning, TEXT("UHeliGameInstance::HandleAppResume: Attempting to sign out players"));

		for (int32 i = 0; i < LocalPlayers.Num(); ++i)
		{
			if (LocalPlayers[i]->GetCachedUniqueNetId().IsValid() && !IsLocalPlayerOnline(LocalPlayers[i]))
			{
				UE_LOG(LogOnline, Log, TEXT("UHeliGameInstance::HandleAppResume: Signed out during resume."));
				HandleSignInChangeMessaging();
				break;
			}
		}
	}
}

void UHeliGameInstance::HandleAppLicenseUpdate()
{
	/*TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
	bIsLicensed = GenericApplication->ApplicationLicenseValid();*/
}

void UHeliGameInstance::HandleSafeFrameChanged()
{
	UCanvas::UpdateAllCanvasSafeZoneData();
}

void UHeliGameInstance::RemoveExistingLocalPlayer(ULocalPlayer* ExistingPlayer)
{
	check(ExistingPlayer);
	if (ExistingPlayer->PlayerController != NULL)
	{
		// Kill the player
		AHelicopter* MyPawn = Cast<AHelicopter>(ExistingPlayer->PlayerController->GetPawn());
		if (MyPawn)
		{
			MyPawn->KilledBy(NULL);
		}
	}

	// Remove local split-screen players from the list
	RemoveLocalPlayer(ExistingPlayer);
}

void UHeliGameInstance::RemoveSplitScreenPlayers()
{
	// if we had been split screen, toss the extra players now
	// remove every player, back to front, except the first one
	while (LocalPlayers.Num() > 1)
	{
		ULocalPlayer* const PlayerToRemove = LocalPlayers.Last();
		RemoveExistingLocalPlayer(PlayerToRemove);
	}
}

void UHeliGameInstance::HandleControllerPairingChanged(int GameUserIndex, const FUniqueNetId& PreviousUser, const FUniqueNetId& NewUser)
{
	UE_LOG(LogOnlineGame, Log, TEXT("UHeliGameInstance::HandleControllerPairingChanged GameUserIndex %d PreviousUser '%s' NewUser '%s'"),
		GameUserIndex, *PreviousUser.ToString(), *NewUser.ToString());

	if (CurrentState == EHeliGameInstanceState::WelcomeScreen)
	{
		// Don't care about pairing changes at welcome screen
		return;
	}

}

void UHeliGameInstance::HandleControllerConnectionChange(bool bIsConnection, int32 Unused, int32 GameUserIndex)
{
	UE_LOG(LogOnlineGame, Log, TEXT("UHeliGameInstance::HandleControllerConnectionChange bIsConnection %d GameUserIndex %d"),
		bIsConnection, GameUserIndex);

	if (!bIsConnection)
	{
		// Controller was disconnected

		// Find the local player associated with this user index
		ULocalPlayer * LocalPlayer = FindLocalPlayerFromControllerId(GameUserIndex);

		if (LocalPlayer == NULL)
		{
			return;		// We don't care about players we aren't tracking
		}

		// Invalidate this local player's controller id.
		LocalPlayer->SetControllerId(-1);
	}
}

TSharedPtr< const FUniqueNetId > UHeliGameInstance::GetUniqueNetIdFromControllerId(const int ControllerId)
{
	IOnlineIdentityPtr OnlineIdentityInt = Online::GetIdentityInterface();

	if (OnlineIdentityInt.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UniqueId = OnlineIdentityInt->GetUniquePlayerId(ControllerId);

		if (UniqueId.IsValid())
		{
			return UniqueId;
		}
	}

	return nullptr;
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

void UHeliGameInstance::TravelToSession(const FName& SessionName)
{
	// Added to handle failures when joining using quickmatch (handles issue of joining a game that just ended, i.e. during game ending timer)
	AddNetworkFailureHandlers();
	ShowLoadingScreen(FString(TEXT("Loading...")));
	GotoState(EHeliGameInstanceState::Playing);
	InternalTravelToSession(SessionName);
}

void UHeliGameInstance::SetIgnorePairingChangeForControllerId(const int32 ControllerId)
{
	IgnorePairingChangeForControllerId = ControllerId;
}

bool UHeliGameInstance::IsLocalPlayerOnline(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == NULL)
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

bool UHeliGameInstance::ValidatePlayerForOnlinePlay(ULocalPlayer* LocalPlayer)
{	
	// Don't let them play online if they aren't online
	if (!IsLocalPlayerOnline(LocalPlayer))
	{
		// TODO: show some message in viewport
		return false;
	}

	return true;
}

void UHeliGameInstance::StartOnlinePrivilegeTask(const IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate& Delegate, EUserPrivileges::Type Privilege, TSharedPtr< const FUniqueNetId > UserId)
{
	/*WaitMessageWidget = SNew(SShooterWaitDialog)
		.MessageText(NSLOCTEXT("NetworkStatus", "CheckingPrivilegesWithServer", "Checking privileges with server.  Please wait..."));*/

	// TODO: add some wait message widget
	/*if (GEngine && GEngine->GameViewport)
	{
		UGameViewportClient* const GVC = GEngine->GameViewport;
		GVC->AddViewportWidgetContent(WaitMessageWidget.ToSharedRef());
	}*/

	auto Identity = Online::GetIdentityInterface();
	if (Identity.IsValid() && UserId.IsValid())
	{
		Identity->GetUserPrivilege(*UserId, Privilege, Delegate);
	}
	else
	{
		// Can only get away with faking the UniqueNetId here because the delegates don't use it
		Delegate.ExecuteIfBound(FUniqueNetIdString(), Privilege, (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures);
	}
}

void UHeliGameInstance::CleanupOnlinePrivilegeTask()
{
	// TODO: add some wait message widget
	/*if (GEngine && GEngine->GameViewport && WaitMessageWidget.IsValid())
	{
		UGameViewportClient* const GVC = GEngine->GameViewport;
		GVC->RemoveViewportWidgetContent(WaitMessageWidget.ToSharedRef());
	}*/
}

void UHeliGameInstance::DisplayOnlinePrivilegeFailureDialogs(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	// TODO: Show warning that the user cannot play due to age restrictions
	
}

void UHeliGameInstance::OnRegisterLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
{
	FinishSessionCreation(Result);
}

void UHeliGameInstance::FinishSessionCreation(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		// Travel to the specified match URL
		GetWorld()->ServerTravel(TravelURL);
	}
	else
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("UHeliGameInstance::OnRegisterLocalPlayerComplete: CreateSessionFailed: EOnJoinSessionCompleteResult is not sucess, so going to MainMenu"));
		GotoState(EHeliGameInstanceState::MainMenu);
	}
}

void UHeliGameInstance::BeginHostingQuickMatch()
{
	ShowLoadingScreen(FString::Printf(TEXT("Loading...")));
	GotoState(EHeliGameInstanceState::Playing);

	// Travel to the specified match URL
	GetWorld()->ServerTravel(TEXT("/Game/Maps/default?game=HeliGameModeTDM?listen"));
}

// host a team death match game, to be called via blueprint
void UHeliGameInstance::HostTeamDeathMatch(ULocalPlayer* LocalPlayer, EHeliMap SelectedMap)
{
	FString MapNameSubStr = FString(TEXT("/Game/Maps/"));
	SelectedMapName = GetEHeliMapEnumAsString(SelectedMap);

	FString LANOption = GetIsOnline() ? FString(TEXT("")) : FString(TEXT("?bIsLanMatch"));

	FString FriendFire = bAllowFriendFireDamage ? FString(TEXT("?bAllowFriendlyFireDamage")) : FString(TEXT(""));

	// FString isListenServer = GetIsOnline() ? FString(TEXT("?listen")) : FString(TEXT(""));
	FString ListenServerOption = FString(TEXT("?listen"));

	// WarmupTime
	FString WarmupTimeOption = FString(TEXT("?WarmupTime=")) + FString::FromInt(WarmupTime);
	// RoundTime
	FString RoundTimeOption = FString(TEXT("?RoundTime=")) + FString::FromInt(RoundTime);
	// TimeBetweenMatches
	FString TimeBetweenMatchesOption = FString(TEXT("?TimeBetweenMatches=")) + FString::FromInt(TimeBetweenMatches);
	
	FString MaxNumberOfPlayersOption = FString(TEXT("?MaxNumberOfPlayers=")) + FString::FromInt(MaxNumberOfPlayers);

	// final TravelURL
	FString InTravelURL = MapNameSubStr + SelectedMapName + FString(TEXT("?game=HeliGameModeTDM")) + ListenServerOption + LANOption + WarmupTimeOption + RoundTimeOption + TimeBetweenMatchesOption + MaxNumberOfPlayersOption + FriendFire;

	UE_LOG(LogLoad, Log, TEXT("%s"), *FString::Printf(TEXT("InTravelURL: %s"), *InTravelURL));

	SelectedGameMode = FString(TEXT("TDM"));


	UpdateSessionSettings(GetFirstGamePlayer(), SelectedGameMode, GameSessionName, SelectedMapName, FName(*CustomServerName), GetIsOnline(), true, MaxNumberOfPlayers);


	GotoState(EHeliGameInstanceState::Playing);	

	// host the game
	HostGame(LocalPlayer, SelectedGameMode, CustomServerName, InTravelURL);
}

/* host a lobby for players to join */
void UHeliGameInstance::HostLobby(ULocalPlayer* LocalPlayer)
{
	FString MapNameSubStr = FString(TEXT("/Game/Maps/"));
	SelectedMapName = GetEHeliMapEnumAsString(EHeliMap::Lobby);

	FString LANOption = GetIsOnline() ? FString(TEXT("")) : FString(TEXT("?bIsLanMatch"));

	// FString isListenServer = GetIsOnline() ? FString(TEXT("?listen")) : FString(TEXT(""));
	FString ListenServerOption = FString(TEXT("?listen"));

	// WarmupTime
	FString WarmupTimeOption = FString(TEXT("?WarmupTime=")) + FString::FromInt(WarmupTime);
	// RoundTime
	FString RoundTimeOption = FString(TEXT("?RoundTime=")) + FString::FromInt(RoundTime);
	// TimeBetweenMatches
	FString TimeBetweenMatchesOption = FString(TEXT("?TimeBetweenMatches=")) + FString::FromInt(TimeBetweenMatches);

	FString MaxNumberOfPlayersOption = FString(TEXT("?MaxNumberOfPlayers=")) + FString::FromInt(MaxNumberOfPlayers);

	FString FriendFire = bAllowFriendFireDamage ? FString(TEXT("?bAllowFriendlyFireDamage")) : FString(TEXT(""));

	// final TravelURL
	FString InTravelURL = MapNameSubStr + SelectedMapName + FString(TEXT("?game=HeliGameModeLobby")) + ListenServerOption + LANOption + WarmupTimeOption + RoundTimeOption + TimeBetweenMatchesOption + MaxNumberOfPlayersOption + FriendFire;

	UE_LOG(LogLoad, Log, TEXT("%s"), *FString::Printf(TEXT("InTravelURL: %s"), *InTravelURL));

	// host the game
	HostGame(LocalPlayer, SelectedGameMode, CustomServerName, InTravelURL);
}

void UHeliGameInstance::BegingTeamDeathmatch(EHeliMap SelectedMap)
{
	ShowLoadingScreen(FString(TEXT("Loading...")));
	GotoState(EHeliGameInstanceState::Playing);

	UWorld* const World = GetWorld();
	AHeliLobbyGameState* const GameState = World != NULL ? World->GetGameState<AHeliLobbyGameState>() : NULL;

	if (GameState)
	{
		GameState->RequestClientsBeginPlayingState();
	}

	FString MapNameSubStr = FString(TEXT("/Game/Maps/"));
	SelectedMapName = GetEHeliMapEnumAsString(SelectedMap);
	
	FString LANOption = GetIsOnline() ? FString(TEXT("")) : FString(TEXT("?bIsLanMatch"));
	// WarmupTime
	FString WarmupTimeOption = FString(TEXT("?WarmupTime=")) + FString::FromInt(WarmupTime);
	// RoundTime
	FString RoundTimeOption = FString(TEXT("?RoundTime=")) + FString::FromInt(RoundTime);
	// TimeBetweenMatches
	FString TimeBetweenMatchesOption = FString(TEXT("?TimeBetweenMatches=")) + FString::FromInt(TimeBetweenMatches);

	FString MaxNumberOfPlayersOption = FString(TEXT("?MaxNumberOfPlayers=")) + FString::FromInt(MaxNumberOfPlayers);

	FString FriendFire = bAllowFriendFireDamage ? FString(TEXT("?bAllowFriendlyFireDamage")) : FString(TEXT(""));
	
	FString ServerName = FString(TEXT("?CustomServerName=")) + CustomServerName;

	FString InTravelURL = MapNameSubStr + SelectedMapName + FString(TEXT("?game=HeliGameModeTDM")) + FString(TEXT("?listen")) + LANOption + WarmupTimeOption + RoundTimeOption + TimeBetweenMatchesOption + MaxNumberOfPlayersOption + FriendFire + ServerName;

	UE_LOG(LogLoad, Log, TEXT("%s"), *FString::Printf(TEXT("InTravelURL: %s"), *InTravelURL));

	GetWorld()->ServerTravel(InTravelURL);
}

void UHeliGameInstance::SwitchTeam()
{
	APlayerController* const FirstPC = GetFirstLocalPlayerController();

	if (FirstPC != nullptr)
	{
		AHeliPlayerState* PlayerState = Cast<AHeliPlayerState>(FirstPC->PlayerState);
		if (PlayerState)
		{
			PlayerState->Server_SwitchTeams();
		}
	}
}

void UHeliGameInstance::ChangePlayerName()
{
	APlayerController* const FirstPC = GetFirstLocalPlayerController();

	if (FirstPC != nullptr)
	{
		AHeliPlayerState* PlayerState = Cast<AHeliPlayerState>(FirstPC->PlayerState);
		if (PlayerState)
		{
			PlayerState->Server_SetPlayerName(CustomPlayerName);
		}
	}
}

bool UHeliGameInstance::UpdateSessionSettings(ULocalPlayer* LocalPlayer, const FString& GameType, FName SessionName, const FString& MapName, FName ServerName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers)
{
	AHeliGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{				
		return GameSession->UpdateSessionSettings(LocalPlayer->GetPreferredUniqueNetId(), SessionName, GameType, MapName, ServerName, bIsLAN, bIsPresence, MaxNumPlayers);
	}

	return false;
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
	return PlayerStateMaps[TeamNum].Num();
}

AHeliPlayerState* UHeliGameInstance::GetPlayerStateFromPlayerInRankedPlayerMap(const FTeamPlayer& TeamPlayer) const
{
	if (PlayerStateMaps.IsValidIndex(TeamPlayer.TeamNum) && PlayerStateMaps[TeamPlayer.TeamNum].Contains(TeamPlayer.PlayerId))
	{
		return PlayerStateMaps[TeamPlayer.TeamNum].FindRef(TeamPlayer.PlayerId).Get();
	}

	return nullptr;
}


/****************************************************************************************************
*                                              Lobby                                                *
*****************************************************************************************************/

void UHeliGameInstance::UpdatePlayerStateMapsForLobby()
{
	APlayerController* const FirstPC = GetFirstLocalPlayerController();

	if (FirstPC != nullptr && FirstPC->GetWorld() != nullptr)
	{
		AHeliLobbyGameState* const GameState = Cast<AHeliLobbyGameState>(FirstPC->GetWorld()->GetGameState());
		if (GameState)
		{
			const int32 NumTeams = FMath::Max(GameState->NumTeams, 1);
			LastTeamPlayerCount.Reset();
			LastTeamPlayerCount.AddZeroed(PlayerStateMaps.Num());

			// hold last playername for checking it later
			/*TArray<FString> lastPlayerName;
			for (int i = 0; i < GameState->PlayerArray.Num(); i++)
			{
				lastPlayerName.Add(GameState->PlayerArray[i]->PlayerName);
			}*/

			for (int32 i = 0; i < PlayerStateMaps.Num(); i++)
			{
				LastTeamPlayerCount[i] = PlayerStateMaps[i].Num();


				// 
				/*for (RankedPlayerMap::TIterator it(PlayerStateMaps[i]); it; ++it)
				{
					if (it.Value().IsValid() && it.Value() != nullptr)
					{
						lastPlayerName.Add(it.Value()->PlayerName);
					}
				}*/


			}


			PlayerStateMaps.Reset();
			PlayerStateMaps.AddZeroed(NumTeams);

			for (int32 i = 0; i < NumTeams; i++)
			{
				GameState->GetRankedMapByLevel(i, PlayerStateMaps[i]);

				if (LastTeamPlayerCount.Num() > 0 && PlayerStateMaps[i].Num() != LastTeamPlayerCount[i])
				{
					bRequiresWidgetUpdate = true;
				}				

				// check for player names update
				/*int32 index = 0;
				for (TMap<int32, TWeakObjectPtr<AHeliPlayerState>>::TIterator it(PlayerStateMaps[i]); it; ++it)
				{					
					if (it.Value().IsValid() && it.Value() != nullptr && (lastPlayerName.Num() > index))
					{
						if (!lastPlayerName[index].Equals(it.Value()->PlayerName))
						{
							bRequiresWidgetUpdate = true;
						}
					}
					index++;
				}*/
			}

			/*if (GameState->PlayerArray.Num() == lastPlayerName.Num()) {
				for (int i = 0; i < GameState->PlayerArray.Num(); i++)
				{
					if (!lastPlayerName[i].Equals(GameState->PlayerArray[i]->PlayerName))
					{
						bRequiresWidgetUpdate = true;
					}
				}
			}*/

		}
	}
}

void UHeliGameInstance::EndRoundAndGoToLobby()
{
	UWorld* const World = GetWorld();
	AHeliGameState* const GameState = World != NULL ? World->GetGameState<AHeliGameState>() : NULL;

	if (GameState)
	{			
		GameState->RequestFinishMatchAndGoToLobbyState();
		GetWorld()->ServerTravel(TEXT("/Game/Maps/Lobby?game=HeliGameModeLobby?listen"));
	}
}

void UHeliGameInstance::EndRoundAndRestartMatch()
{
	UWorld* const World = GetWorld();
	AHeliGameState* const GameState = World != NULL ? World->GetGameState<AHeliGameState>() : NULL;

	if (GameState)
	{
		GameState->RequestEndRoundAndRestartMatch();	
	}
}





/****************************** ENUM HELPERS ***************************************************/

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

void UHeliGameInstance::ResquestRestartAllPlayers()
{
	UWorld* const World = GetWorld();
	AHeliGameState* const GameState = World != NULL ? World->GetGameState<AHeliGameState>() : NULL;

	if (GameState)
	{
		GameState->ResquestRestartAllPlayers();
	}
}