// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliGameSession.h"
#include "HeliGame.h"
#include "HeliPlayerController.h"
#include "HeliOnlineGameSettings.h"
#include "OnlineSubsystemSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"


namespace
{
	const FString GameVersionKeyword = GameVersionName;
}


/**************************************************************************************************************************
*                                                                                                                         *
***************************************************************************************************************************/

AHeliGameSession::AHeliGameSession(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &AHeliGameSession::OnCreateSessionComplete);
		OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &AHeliGameSession::OnDestroySessionComplete);

		OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &AHeliGameSession::OnFindSessionsComplete);
		OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &AHeliGameSession::OnJoinSessionComplete);

		OnStartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &AHeliGameSession::OnStartOnlineGameComplete);
	}
}

const TArray<FOnlineSessionSearchResult> & AHeliGameSession::GetSearchResults() const
{
	return SearchSettings->SearchResults;
};

void AHeliGameSession::StartSession()
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{			
			UE_LOG(LogTemp, Warning, TEXT("AHeliGameSession::StartSession ~ Starting session"));
			Sessions->StartSession(GameSessionName);
		}
	}
}

/**
* Ends a game session
*/
void AHeliGameSession::HandleMatchHasEnded()
{
	// start online game locally and wait for completion
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("AHeliGameSession::HandleMatchHasEnded ~ Ending session"));			
			Sessions->EndSession(GameSessionName);
		}
	}
}

void AHeliGameSession::OnStartOnlineGameComplete(FName InSessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
		}
	}

	if (bWasSuccessful)
	{
		// tell non-local players to start online game
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			AHeliPlayerController* PC = Cast<AHeliPlayerController>(*It);
			if (PC && !PC->IsLocalPlayerController())
			{
				PC->ClientStartOnlineGame();
			}
		}
	}
}











/*
* Host 
*/

bool AHeliGameSession::HostSession(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, const FString& GameType, const FString& MapName, const FString& CustomServerName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers)
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		UE_LOG(LogTemp, Display, TEXT("AHeliGameSession::HostSession ~ Using %s Online Subsystem with %s Online Service."), *OnlineSub->GetSubsystemName().ToString(), *OnlineSub->GetOnlineServiceName().ToString());

		if(OnlineSub->GetSubsystemName().IsEqual("NULL") && !bIsLAN)
		{ 
			UE_LOG(LogTemp, Warning, TEXT("AHeliGameSession::HostSession ~ 3rd party OnlineSubsystem NOT FOUND, using %s and LAN."), *OnlineSub->GetSubsystemName().ToString());
			bIsLAN = true;
		}

		CurrentSessionParams.CustomServerName = CustomServerName;
		CurrentSessionParams.SelectedGameModeName = GameType;
		CurrentSessionParams.SelectedMapName = MapName;
		CurrentSessionParams.SessionName = InSessionName;
		CurrentSessionParams.bIsLAN = bIsLAN;
		CurrentSessionParams.bIsPresence = bIsPresence;
		CurrentSessionParams.UserId = UserId;
		MaxPlayers = MaxNumPlayers;

		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && CurrentSessionParams.UserId.IsValid())
		{
			auto ExistingSession = Sessions->GetNamedSession(CurrentSessionParams.SessionName);
			if (ExistingSession != nullptr)
			{
				Sessions->DestroySession(CurrentSessionParams.SessionName);
			}

			UE_LOG(LogTemp, Display, TEXT("AHeliGameSession::HostSession ~ Found Session Interface."));

			HostSettings = MakeShareable(new FHeliOnlineSessionSettings(bIsLAN, bIsPresence, MaxPlayers));
			HostSettings->Set(GAME_VERSION_SETTINGS_KEY, GameVersionKeyword, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			HostSettings->Set(SERVER_NAME_SETTINGS_KEY, CustomServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

			// TODO(andrey): create session with other attributes such as game type, map in order to search it by those
			HostSettings->Set(SETTING_GAMEMODE, GameType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			HostSettings->Set(SETTING_MAPNAME, MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

			OnCreateSessionCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
			return Sessions->CreateSession(*CurrentSessionParams.UserId, CurrentSessionParams.SessionName, *HostSettings);			
		}
	}
#if !UE_BUILD_SHIPPING
	else
	{
		// Hack workflow in development
		OnCreatePresenceSessionComplete().Broadcast(InSessionName, true);
		return true;
	}
#endif


	UE_LOG(LogTemp, Warning, TEXT("AHeliGameSession::HostSession ~ Online Subsystem NOT FOUND!"));
	return false;
}

void AHeliGameSession::OnCreateSessionComplete(FName NewSessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("OnCreateSessionComplete %s bSuccess: %d"), *NewSessionName.ToString(), bWasSuccessful);

	UE_LOG(LogTemp, Log, TEXT("OnCreateSessionComplete %s bSuccess: %d"), *NewSessionName.ToString(), bWasSuccessful);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
	}

	OnCreatePresenceSessionComplete().Broadcast(NewSessionName, bWasSuccessful);
}















/*
* Find
*/
void AHeliGameSession::FindSessions(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, bool bIsLAN, bool bIsPresence)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		UE_LOG(LogTemp, Display, TEXT("AHeliGameSession::FindSessions ~ Using %s Online Subsystem with %s Online Service."), *OnlineSub->GetSubsystemName().ToString(), *OnlineSub->GetOnlineServiceName().ToString());

		UE_LOG(LogTemp, Display, TEXT("AHeliGameSession::FindSessions ~ GameVersionKeyword = %s, UserId = %s, InSessionName = %s, bIsLAN = %s"), *GameVersionKeyword, *UserId->ToString(), *InSessionName.ToString(), bIsLAN ? *FString::Printf(TEXT("true")) : *FString::Printf(TEXT("false")));

		CurrentSessionParams.SessionName = InSessionName;
		CurrentSessionParams.bIsLAN = bIsLAN;
		CurrentSessionParams.bIsPresence = bIsPresence;
		CurrentSessionParams.UserId = UserId;

		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && CurrentSessionParams.UserId.IsValid())
		{
			SearchSettings = MakeShareable(new FHeliOnlineSearchSettings(bIsLAN, bIsPresence));
			SearchSettings->QuerySettings.Set(GAME_VERSION_SETTINGS_KEY, GameVersionKeyword, EOnlineComparisonOp::Equals);

			TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SearchSettings.ToSharedRef();

			OnFindSessionsCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
			Sessions->FindSessions(*CurrentSessionParams.UserId, SearchSettingsRef);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Online Subsystem NOT FOUND!"));		
		OnFindSessionsComplete(false);
	}
}

void AHeliGameSession::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Display, TEXT("AHeliGameSession::OnFindSessionsComplete ~ bSuccess: %d"), bWasSuccessful);

	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);

			UE_LOG(LogTemp, Display, TEXT("AHeliGameSession::OnFindSessionsComplete ~ Num Search Results: %d"), SearchSettings->SearchResults.Num());
			for (int32 SearchIdx = 0; SearchIdx < SearchSettings->SearchResults.Num(); SearchIdx++)
			{
				const FOnlineSessionSearchResult& SearchResult = SearchSettings->SearchResults[SearchIdx];
				DumpSession(&SearchResult.Session);
			}

			OnFindSessionsComplete().Broadcast(bWasSuccessful);
		}
	}
}













/*
* Join
*/
bool AHeliGameSession::JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, int32 SessionIndexInSearchResults)
{
	bool bResult = false;

	if (SessionIndexInSearchResults >= 0 && SessionIndexInSearchResults < SearchSettings->SearchResults.Num())
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid() && UserId.IsValid())
			{
				OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
				bResult = Sessions->JoinSession(*UserId, InSessionName, SearchSettings->SearchResults[SessionIndexInSearchResults]);
			}
		}
	}

	return bResult;
}

void AHeliGameSession::OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("AHeliGameSession::OnJoinSessionComplete ~ %s bSuccess: %d"), *InSessionName.ToString(), static_cast<int32>(Result));

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlineSessionPtr Sessions = NULL;
	if (OnlineSub)
	{
		Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
	}

	OnJoinSessionComplete().Broadcast(Result);
}

bool AHeliGameSession::TravelToSession(int32 ControllerId, FName InSessionName)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		FString URL;
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && Sessions->GetResolvedConnectString(InSessionName, URL))
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), ControllerId);
			if (PC)
			{
				UE_LOG(LogTemp, Display, TEXT("AHeliGameSession::TravelToSession ~ %s, %s"), *InSessionName.ToString(), *URL);
				PC->ClientTravel(URL, TRAVEL_Absolute);
				return true;
			}
		}
		else
		{
			UE_LOG(LogOnlineGame, Error, TEXT("Failed to join session %s"), *InSessionName.ToString());
		}
	}
#if !UE_BUILD_SHIPPING
	else
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), ControllerId);
		if (PC)
		{
			FString LocalURL(TEXT("127.0.0.1"));
			PC->ClientTravel(LocalURL, TRAVEL_Absolute);
			return true;
		}
	}
#endif //!UE_BUILD_SHIPPING

	return false;
}










/*
* Destroy
*/
void AHeliGameSession::OnDestroySessionComplete(FName InSessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("OnDestroySessionComplete %s bSuccess: %d"), *InSessionName.ToString(), bWasSuccessful);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		HostSettings = NULL;
	}
}

/**************************************************************************************************************************
*                                                                                                                         *
***************************************************************************************************************************/

EOnlineAsyncTaskState::Type AHeliGameSession::GetSearchResultStatus(int32& SearchResultIdx, int32& NumSearchResults)
{
	SearchResultIdx = 0;
	NumSearchResults = 0;

	if (SearchSettings.IsValid())
	{
		if (SearchSettings->SearchState == EOnlineAsyncTaskState::Done)
		{
			SearchResultIdx = CurrentSessionParams.BestSessionIdx;
			NumSearchResults = SearchSettings->SearchResults.Num();

			UE_LOG(LogTemp, Display, TEXT("AHeliGameSession::GetSearchResultStatus ~ SearchResultIdx = %d, NumSearchResults = %d"), SearchResultIdx, NumSearchResults);
		}
		return SearchSettings->SearchState;
	}

	return EOnlineAsyncTaskState::NotStarted;
}




bool AHeliGameSession::UpdateSessionSettings(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, const FString& GameType, const FString& MapName, const FString& CustomServerName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers)
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		CurrentSessionParams.CustomServerName = CustomServerName;
		CurrentSessionParams.SelectedGameModeName = GameType;
		CurrentSessionParams.SelectedMapName = MapName;
		CurrentSessionParams.SessionName = InSessionName;
		CurrentSessionParams.bIsLAN = bIsLAN;
		CurrentSessionParams.bIsPresence = bIsPresence;
		CurrentSessionParams.UserId = UserId;
		MaxPlayers = MaxNumPlayers;

		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && CurrentSessionParams.UserId.IsValid())
		{

			HostSettings = MakeShareable(new FHeliOnlineSessionSettings(bIsLAN, bIsPresence, MaxPlayers));
			HostSettings->Set(GAME_VERSION_SETTINGS_KEY, GameVersionKeyword, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			HostSettings->Set(SERVER_NAME_SETTINGS_KEY, CustomServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

			return Sessions->UpdateSession(CurrentSessionParams.SessionName, *HostSettings, true);
		}

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Online Subsystem NOT FOUND!"));
	}

	return false;
}

bool AHeliGameSession::IsBusy() const
{
	if (HostSettings.IsValid() || SearchSettings.IsValid())
	{
		return true;
	}

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			EOnlineSessionState::Type GameSessionState = Sessions->GetSessionState(GameSessionName);
			EOnlineSessionState::Type PartySessionState = Sessions->GetSessionState(PartySessionName);
			if (GameSessionState != EOnlineSessionState::NoSession || PartySessionState != EOnlineSessionState::NoSession)
			{
				return true;
			}
		}
	}

	return false;
}


/**************************************************************************************************************************
*                                             Matchmaking                                                                 *
***************************************************************************************************************************/

void AHeliGameSession::OnNoMatchesAvailable()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("Matchmaking complete, no sessions available."));
	UE_LOG(LogTemp, Warning, TEXT("Matchmaking complete, no sessions available."));
	SearchSettings = NULL;
}

void AHeliGameSession::ResetBestSessionVars()
{
	CurrentSessionParams.BestSessionIdx = -1;
}

void AHeliGameSession::ChooseBestSession()
{
	// Start searching from where we left off
	for (int32 SessionIndex = CurrentSessionParams.BestSessionIdx + 1; SessionIndex < SearchSettings->SearchResults.Num(); SessionIndex++)
	{
		// Found the match that we want
		CurrentSessionParams.BestSessionIdx = SessionIndex;
		return;
	}

	CurrentSessionParams.BestSessionIdx = -1;
}

void AHeliGameSession::StartMatchmaking()
{
	ResetBestSessionVars();
	ContinueMatchmaking();
}

void AHeliGameSession::ContinueMatchmaking()
{
	ChooseBestSession();
	if (CurrentSessionParams.BestSessionIdx >= 0 && CurrentSessionParams.BestSessionIdx < SearchSettings->SearchResults.Num())
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid() && CurrentSessionParams.UserId.IsValid())
			{
				OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
				Sessions->JoinSession(*CurrentSessionParams.UserId, CurrentSessionParams.SessionName, SearchSettings->SearchResults[CurrentSessionParams.BestSessionIdx]);
			}
		}
	}
	else
	{
		OnNoMatchesAvailable();
	}
}