// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliPlayerController.h"
#include "HeliGame.h"
#include "HeliHud.h"
#include "Helicopter.h"
#include "HeliPlayerState.h"
#include "HeliGameInstance.h"
#include "HeliGameUserSettings.h"
#include "HeliGameMode.h"
#include "HeliMoveComp.h"

#include "Online.h"
#include "OnlineAchievementsInterface.h"
#include "OnlineEventsInterface.h"
#include "OnlineIdentityInterface.h"
#include "OnlineSessionInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "Public/TimerManager.h"
#include "Public/DrawDebugHelpers.h"
#include "Public/Engine.h"
#include "Components/DecalComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

#define  ACH_FRAG_SOMEONE	TEXT("ACH_FRAG_SOMEONE")

AHeliPlayerController::AHeliPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bAllowGameActions = true;
	this->SetReplicates(true);
}


void AHeliPlayerController::ClientGameStarted_Implementation()
{
	bAllowGameActions = true;

	// Enable controls mode now the game has started
	SetIgnoreMoveInput(false);



	// TODO: query achievements in online subsystem
	//QueryAchievements();

	// Send round start event
	const auto Events = Online::GetEventsInterface();
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if (LocalPlayer != nullptr && Events.IsValid())
	{
		auto UniqueId = LocalPlayer->GetPreferredUniqueNetId();

		if (UniqueId.IsValid())
		{
			// Generate a new session id
			Events->SetPlayerSessionId(*UniqueId, FGuid::NewGuid());

			FString MapName = *FPackageName::GetShortName(GetWorld()->PersistentLevel->GetOutermost()->GetName());

			// Fire session start event for all cases
			FOnlineEventParms Params;
			Params.Add(TEXT("GameplayModeId"), FVariantData((int32)1)); // TODO: determine game mode (ffa v tdm)
			Params.Add(TEXT("DifficultyLevelId"), FVariantData((int32)0)); // unused
			Params.Add(TEXT("MapName"), FVariantData(MapName));

			Events->TriggerEvent(*UniqueId, TEXT("PlayerSessionStart"), Params);

			// Online matches require the MultiplayerRoundStart event as well
			UHeliGameInstance* MyGameInstance = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;

			if (MyGameInstance)
			{				
				if (MyGameInstance->GetIsOnline())
				{
					FOnlineEventParms MultiplayerParams;

					// TODO: fill in with real values
					MultiplayerParams.Add(TEXT("SectionId"), FVariantData((int32)0)); // unused
					MultiplayerParams.Add(TEXT("GameplayModeId"), FVariantData((int32)1)); // TODO: determine game mode (ffa v tdm)
					MultiplayerParams.Add(TEXT("MatchTypeId"), FVariantData((int32)1)); // TODO: abstract the specific meaning of this value across platforms

					Events->TriggerEvent(*UniqueId, TEXT("MultiplayerRoundStart"), MultiplayerParams);
				}
			}

			bHasSentStartEvents = true;
		}
	}
}

/** Starts the online game using the session name in the PlayerState */
void AHeliPlayerController::ClientStartOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	AHeliPlayerState* HeliPlayerState = Cast<AHeliPlayerState>(PlayerState);
	if (HeliPlayerState)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid())
			{
				UE_LOG(LogOnline, Log, TEXT("Starting session %s on client"), *HeliPlayerState->SessionName.ToString());
				Sessions->StartSession(HeliPlayerState->SessionName);
			}
		}
	}
	else
	{
		// Keep retrying until player state is replicated
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ClientStartOnlineGame, this, &AHeliPlayerController::ClientStartOnlineGame_Implementation, 0.2f, false);
	}
}

/** Ends the online game using the session name in the PlayerState */
void AHeliPlayerController::ClientEndOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	AHeliPlayerState* HeliPlayerState = Cast<AHeliPlayerState>(PlayerState);
	if (HeliPlayerState)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid())
			{
				UE_LOG(LogOnline, Log, TEXT("Ending session %s on client"), *HeliPlayerState->SessionName.ToString());
				Sessions->EndSession(HeliPlayerState->SessionName);
			}
		}
	}
}

void AHeliPlayerController::ClientGameEnded_Implementation(class AActor* EndGameFocus, bool bIsWinner)
{
	Super::ClientGameEnded_Implementation(EndGameFocus, bIsWinner);

	// Disable controls now the game has ended
	SetIgnoreMoveInput(true);

	bAllowGameActions = false;

	// Make sure that we still have valid view target
	SetViewTarget(GetPawn());

	// TODO: UpdateSaveFileOnGameEnd(bIsWinner);
	// TODO: UpdateAchievementsOnGameEnd();
	// TODO: UpdateLeaderboardsOnGameEnd();

	// Flag that the game has just ended (if it's ended due to host loss we want to wait for ClientReturnToMainMenu_Implementation first, incase we don't want to process)
	//bGameEndedFrame = true;
}

void AHeliPlayerController::ClientSendRoundEndEvent_Implementation(bool bIsWinner, int32 ExpendedTimeInSeconds)
{
	const auto Events = Online::GetEventsInterface();
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if (bHasSentStartEvents && LocalPlayer != nullptr && Events.IsValid())
	{
		auto UniqueId = LocalPlayer->GetPreferredUniqueNetId();

		if (UniqueId.IsValid())
		{
			FString MapName = *FPackageName::GetShortName(GetWorld()->PersistentLevel->GetOutermost()->GetName());
			AHeliPlayerState* HeliPlayerState = Cast<AHeliPlayerState>(PlayerState);
			int32 PlayerScore = HeliPlayerState ? HeliPlayerState->GetScore() : 0;

			// Fire session end event for all cases
			FOnlineEventParms Params;
			Params.Add(TEXT("GameplayModeId"), FVariantData((int32)1)); // TODO: determine game mode (ffa v tdm)
			Params.Add(TEXT("DifficultyLevelId"), FVariantData((int32)0)); // unused
			Params.Add(TEXT("ExitStatusId"), FVariantData((int32)0)); // unused
			Params.Add(TEXT("PlayerScore"), FVariantData((int32)PlayerScore));
			Params.Add(TEXT("PlayerWon"), FVariantData((bool)bIsWinner));
			Params.Add(TEXT("MapName"), FVariantData(MapName));
			Params.Add(TEXT("MapNameString"), FVariantData(MapName)); // @todo workaround for a bug in backend service, remove when fixed

			Events->TriggerEvent(*UniqueId, TEXT("PlayerSessionEnd"), Params);

			// Online matches require the MultiplayerRoundEnd event as well
			UHeliGameInstance* SGI = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;
			if (SGI->GetIsOnline())
			{
				FOnlineEventParms MultiplayerParams;

				AHeliGameState* const MyGameState = GetWorld() != NULL ? GetWorld()->GetGameState<AHeliGameState>() : NULL;
				if (ensure(MyGameState != nullptr))
				{
					MultiplayerParams.Add(TEXT("SectionId"), FVariantData((int32)0)); // unused
					MultiplayerParams.Add(TEXT("GameplayModeId"), FVariantData((int32)1)); // @todo determine game mode (ffa v tdm)
					MultiplayerParams.Add(TEXT("MatchTypeId"), FVariantData((int32)1)); // @todo abstract the specific meaning of this value across platforms
					MultiplayerParams.Add(TEXT("TimeInSeconds"), FVariantData((float)ExpendedTimeInSeconds));

					Events->TriggerEvent(*UniqueId, TEXT("MultiplayerRoundEnd"), MultiplayerParams);
				}
			}
		}

		bHasSentStartEvents = false;
	}

	// show scoreboard
	AHeliHud* HeliHUD = Cast<AHeliHud>(GetHUD());
	if (HeliHUD != nullptr)
	{
		HeliHUD->ShowScoreboard();
	}

	// enable third person view
	AHelicopter* MyPawn = Cast<AHelicopter>(GetPawn());
	if (MyPawn)
	{
		MyPawn->EnableThirdPersonViewpoint();
		AddHeliHudWidgetInHudForThirdPersonView();		
	}
}


void AHeliPlayerController::HandleReturnToMainMenu()
{
	OnHideScoreboard();	

	CleanupSessionOnReturnToMenu();
}

void AHeliPlayerController::ClientReturnToLobbyState_Implementation()
{
	UHeliGameInstance* SGI = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;

	if (!ensure(SGI != NULL))
	{
		return;
	}

	if (GetNetMode() == NM_Client)
	{
		SGI->GotoState(EHeliGameInstanceState::LobbyMenu);
	}
}

void AHeliPlayerController::ClientGoToPlayingState_Implementation()
{
	UHeliGameInstance* SGI = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;

	if (!ensure(SGI != NULL))
	{
		return;
	}

	if (GetNetMode() == NM_Client)
	{
		SGI->GotoState(EHeliGameInstanceState::Playing);
	}
}

/** Ends and/or destroys game session */
void AHeliPlayerController::CleanupSessionOnReturnToMenu()
{
	UHeliGameInstance * SGI = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;

	if (ensure(SGI != NULL))
	{
		SGI->CleanupSessionOnReturnToMenu();
	}
}

void AHeliPlayerController::OnShowInGameMenu()
{
	AHelicopter* helicopter = Cast<AHelicopter>(GetPawn());

	if (helicopter)
	{
		helicopter->EnableThirdPersonViewpoint();
	}

	AHeliHud* MyHud = Cast<AHeliHud>(GetHUD());

	if (MyHud)
	{
		bAllowGameActions = false;
		MyHud->ShowInGameMenu();
	}

}

void AHeliPlayerController::HideInGameMenu()
{
	AHeliHud* MyHud = Cast<AHeliHud>(GetHUD());

	if (MyHud && bAllowGameActions)
	{
		MyHud->HideInGameMenu();
	}

	bAllowGameActions = true;
}

void AHeliPlayerController::ShowInGameOptionsMenu()
{
	AHeliHud* MyHud = Cast<AHeliHud>(GetHUD());
	if (MyHud)
	{
		bAllowGameActions = false;
		MyHud->ShowInGameOptionsMenu();
	}
}

void AHeliPlayerController::HideInGameOptionsMenu()
{
	AHeliHud* MyHud = Cast<AHeliHud>(GetHUD());
	if (MyHud)
	{
		MyHud->HideInGameOptionsMenu();
	}

	bAllowGameActions = true;
}

void AHeliPlayerController::OnShowScoreboard()
{
	AHeliHud* MyHud = Cast<AHeliHud>(GetHUD());
	if (MyHud && bAllowGameActions)
	{
		MyHud->ShowScoreboard();
	}
}

void AHeliPlayerController::OnHideScoreboard()
{
	AHeliHud* MyHud = Cast<AHeliHud>(GetHUD());
	
	if (MyHud && bAllowGameActions)
	{
		MyHud->HideScoreboard();

		UHeliGameInstance* MyGI = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;
		if (MyGI)
		{
			MyGI->bRequiresWidgetUpdate = false;
		}
	}
}

void AHeliPlayerController::AddHeliHudWidgetInHudForFirstPersonView()
{
	// turning cockpit hud on/off
	AHeliHud* MyHud = Cast<AHeliHud>(GetHUD());

	if (MyHud)
	{
		if (MyHud->HeliHudWidget.IsValid())
		{
			if (MyHud->HeliHudWidget->IsInViewport())
			{
				return;
			}
			MyHud->HeliHudWidget->AddToViewport();

			// turn crosshair on
			MyHud->CrosshairOn = true;
			MyHud->DotCrosshairOn = false;
		}
		else
		{
			if (MyHud->HeliHudWidgetTemplate)
			{
				MyHud->HeliHudWidget = CreateWidget<UUserWidget>(this, MyHud->HeliHudWidgetTemplate);
				MyHud->HeliHudWidget->AddToViewport();

				// turn crosshair on
				MyHud->CrosshairOn = true;
				MyHud->DotCrosshairOn = false;
			}
		}
	}
}

void AHeliPlayerController::AddHeliHudWidgetInHudForThirdPersonView()
{
	// turning cockpit hud on/off
	AHeliHud* MyHud = Cast<AHeliHud>(GetHUD());

	if (MyHud)
	{
		if (MyHud->HeliHudWidget.IsValid())
		{
			if (MyHud->HeliHudWidget->IsInViewport())
			{
				MyHud->HeliHudWidget->RemoveFromViewport();

				// turn crosshair off
				MyHud->CrosshairOn = false;
				MyHud->DotCrosshairOn = true;
			}
		}
		else
		{
			if (MyHud->HeliHudWidgetTemplate)
			{
				MyHud->HeliHudWidget = CreateWidget<UUserWidget>(this, MyHud->HeliHudWidgetTemplate);
				MyHud->HeliHudWidget->AddToViewport();

				// turn crosshair on
				MyHud->CrosshairOn = true;
				MyHud->DotCrosshairOn = false;
			}
		}
	}
}

bool AHeliPlayerController::IsGameInputAllowed() const
{
	return bAllowGameActions;
}


void AHeliPlayerController::OnDeathMessage(class AHeliPlayerState* KillerPlayerState, class AHeliPlayerState* KilledPlayerState, const UDamageType* KillerDamageType)
{
	AHeliHud* MyHud = Cast<AHeliHud>(GetHUD());
	if (MyHud)
	{
		// TODO: MyHud->ShowDeathMessage(KillerPlayerState, KilledPlayerState, KillerDamageType);
	}

	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetCachedUniqueNetId().IsValid() && KilledPlayerState->UniqueId.IsValid())
	{
		// if this controller is the player who died, update the hero stat.
		if (*LocalPlayer->GetCachedUniqueNetId() == *KilledPlayerState->UniqueId)
		{
			const auto Events = Online::GetEventsInterface();
			const auto Identity = Online::GetIdentityInterface();

			if (Events.IsValid() && Identity.IsValid())
			{
				const int32 UserIndex = LocalPlayer->GetControllerId();
				TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
				if (UniqueID.IsValid())
				{					

					// TODO: get weapon type, game mode, and send event to steam
					/*AHelicopter* Pawn = Cast<AHelicopter>(GetPawn());
					AWeapon* Weapon = Pawn ? Pawn->GetWeapon() : NULL;
					int32 WeaponType = Weapon ? (int32)Weapon->GetAmmoType() : 0;*/

					FOnlineEventParms Params;
					Params.Add(TEXT("SectionId"), FVariantData((int32)0)); // unused
					Params.Add(TEXT("GameplayModeId"), FVariantData((int32)1)); // TODO: determine game mode (ffa v tdm)
					//Params.Add(TEXT("PlayerWeaponId"), FVariantData((int32)WeaponType));

					Events->TriggerEvent(*UniqueID, TEXT("PlayerDeath"), Params);
				}
			}
		}
	}
}

void AHeliPlayerController::OnKill()
{
	// TODO: UpdateAchievementProgress(ACH_FRAG_SOMEONE, 100.0f);

	const auto Events = Online::GetEventsInterface();
	const auto Identity = Online::GetIdentityInterface();

	if (Events.IsValid() && Identity.IsValid())
	{
		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
		if (LocalPlayer)
		{
			int32 UserIndex = LocalPlayer->GetControllerId();
			TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
			if (UniqueID.IsValid())
			{
				// TODO: get which weapon was used to kill
				/*AHelicopter* Pawn = Cast<AHelicopter>(GetPawn());				
				AWeapon* Weapon = Pawn ? Pawn->GetWeapon() : NULL;
				int32 WeaponType = Weapon ? (int32)Weapon->GetAmmoType() : 0;*/

				FOnlineEventParms Params;

				Params.Add(TEXT("SectionId"), FVariantData((int32)0)); // unused
				Params.Add(TEXT("GameplayModeId"), FVariantData((int32)1)); // TODO: determine game mode (ffa v tdm)
				//Params.Add(TEXT("PlayerWeaponId"), FVariantData((int32)WeaponType));

				Events->TriggerEvent(*UniqueID, TEXT("KillOponent"), Params);
			}
		}
	}
}

void AHeliPlayerController::OnQueryAchievementsComplete(const FUniqueNetId& PlayerId, const bool bWasSuccessful)
{
	UE_LOG(LogOnline, Display, TEXT("AHeliPlayerController::OnQueryAchievementsComplete(bWasSuccessful = %s)"), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
}

void AHeliPlayerController::QueryAchievements()
{
	// precache achievements
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetControllerId() != -1)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());

				if (UserId.IsValid())
				{
					IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();

					if (Achievements.IsValid())
					{
						Achievements->QueryAchievements(*UserId.Get(), FOnQueryAchievementsCompleteDelegate::CreateUObject(this, &AHeliPlayerController::OnQueryAchievementsComplete));
					}
				}
				else
				{
					UE_LOG(LogOnline, Warning, TEXT("No valid user id for this controller."));
				}
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("No valid identity interface."));
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("No default online subsystem."));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No local player, cannot read achievements."));
	}
}

void AHeliPlayerController::UpdateAchievementProgress(const FString& Id, float Percent)
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = LocalPlayer->GetCachedUniqueNetId();

				if (UserId.IsValid())
				{

					IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();
					if (Achievements.IsValid() && (!WriteObject.IsValid() || WriteObject->WriteState != EOnlineAsyncTaskState::InProgress))
					{
						WriteObject = MakeShareable(new FOnlineAchievementsWrite());
						WriteObject->SetFloatStat(*Id, Percent);

						FOnlineAchievementsWriteRef WriteObjectRef = WriteObject.ToSharedRef();
						Achievements->WriteAchievements(*UserId, WriteObjectRef);
					}
					else
					{
						UE_LOG(LogOnline, Warning, TEXT("No valid achievement interface or another write is in progress."));
					}
				}
				else
				{
					UE_LOG(LogOnline, Warning, TEXT("No valid user id for this controller."));
				}
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("No valid identity interface."));
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("No default online subsystem."));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No local player, cannot update achievements."));
	}
}

void AHeliPlayerController::Suicide()
{
	if (IsInState(NAME_Playing) && bAllowGameActions)
	{
		ServerSuicide();
	}
}


bool AHeliPlayerController::ServerSuicide_Validate()
{
	return true;
}

void AHeliPlayerController::ServerSuicide_Implementation()
{
	if ((GetPawn() != NULL) && ((GetWorld()->TimeSeconds - GetPawn()->CreationTime > 1) || (GetNetMode() == NM_Standalone)))
	{
		AHelicopter* MyPawn = Cast<AHelicopter>(GetPawn());
		if (MyPawn)
		{
			MyPawn->Suicide();
		}
	}
}

void AHeliPlayerController::SetPlayer(UPlayer* InPlayer)
{
	Super::SetPlayer(InPlayer);	

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void AHeliPlayerController::SetAllowGameActions(bool bNewAllowGameActions)
{
	this->bAllowGameActions = bNewAllowGameActions;
}

void AHeliPlayerController::SetMouseSensitivity(float inMouseSensitivity)
{
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	AHelicopter* helicopter = Cast<AHelicopter>(GetPawn());
	
	if (helicopter)
	{
		helicopter->SetMouseSensitivity(inMouseSensitivity);
	}

	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetMouseSensitivity(inMouseSensitivity);		
		heliGameUserSettings->ApplySettings(false);
	}
}

float AHeliPlayerController::GetMouseSensitivity()
{
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());

	if (heliGameUserSettings)
	{
		return heliGameUserSettings->GetMouseSensitivity();
	}

	return 0.f;
}

void AHeliPlayerController::SetKeyboardSensitivity(float inKeyboardSensitivity)
{
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	AHelicopter* helicopter = Cast<AHelicopter>(GetPawn());

	if (helicopter)
	{
		helicopter->SetKeyboardSensitivity(inKeyboardSensitivity);
	}

	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetKeyboardSensitivity(inKeyboardSensitivity);
		heliGameUserSettings->ApplySettings(false);
	}
}

float AHeliPlayerController::GetKeyboardSensitivity()
{
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());

	if (heliGameUserSettings)
	{
		return heliGameUserSettings->GetKeyboardSensitivity();
	}

	return 0.f;
}

void AHeliPlayerController::SetInvertedAim(int32 inInvertedAim)
{
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	AHelicopter* helicopter = Cast<AHelicopter>(GetPawn());

	inInvertedAim = FMath::Clamp(inInvertedAim, -1, 1);

	if (helicopter)
	{
		helicopter->SetInvertedAim(inInvertedAim);
	}

	if (heliGameUserSettings)
	{		
		heliGameUserSettings->SetInvertedAim(inInvertedAim);
		heliGameUserSettings->ApplySettings(false);
	}
}

int32 AHeliPlayerController::GetInvertedAim()
{
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());

	if (heliGameUserSettings)
	{
		return heliGameUserSettings->GetInvertedAim();
	}

	return 1;
}

void AHeliPlayerController::SetPilotAssist(bool bInPilotAssist)
{
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	AHelicopter* helicopter = Cast<AHelicopter>(GetPawn());	

	if (heliGameUserSettings)
	{
		UHeliMoveComp* heliMoveComp = Cast<UHeliMoveComp>(helicopter->GetMovementComponent());
		if (heliMoveComp)
		{
			heliMoveComp->SetAutoRollStabilization(bInPilotAssist);
		}

		heliGameUserSettings->SetPilotAssist(bInPilotAssist == true ? 1 : -1);
		heliGameUserSettings->ApplySettings(false);
	}
}

bool AHeliPlayerController::IsPilotAssist()
{
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());	

	if (heliGameUserSettings)
	{
		return heliGameUserSettings->GetPilotAssist() > 0 ? true : false;
	}

	return false;
}

float AHeliPlayerController::GetNetworkSmoothingFactor()
{
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());

	if (heliGameUserSettings)
	{
		return heliGameUserSettings->GetNetworkSmoothingFactor();
	}

	return 100;
}

void AHeliPlayerController::SetNetworkSmoothingFactor(float inNetworkSmoothingFactor)
{
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	AHelicopter* helicopter = Cast<AHelicopter>(GetPawn());	

	if (helicopter)
	{
		helicopter->SetNetworkSmoothingFactor(inNetworkSmoothingFactor);
	}

	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetNetworkSmoothingFactor(inNetworkSmoothingFactor);
		heliGameUserSettings->ApplySettings(false);
	}
}

bool AHeliPlayerController::Server_RestartPlayer_Validate()
{
	return true;
}

void AHeliPlayerController::Server_RestartPlayer_Implementation()
{
	AHeliGameMode* heliGameMode = Cast<AHeliGameMode>(GetWorld()->GetAuthGameMode());
	if (heliGameMode && heliGameMode->IsImmediatelyPlayerRestartAllowedAfterDeath())
	{				
		heliGameMode->RestartPlayer(this);

		UE_LOG(LogTemp, Display, TEXT("AHeliPlayerController::Server_RestartPlayer_Implementation ~ %s %s -- Role %d - RemoteRole %d -- SpawnLocation: %s"), IsLocalPlayerController() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *GetName(), (int32)Role, (int32)GetRemoteRole(), *GetSpawnLocation().ToCompactString());
	}
}

/*
Debug Helpers
*/
void AHeliPlayerController::FlushDebugLines()
{
	FlushPersistentDebugLines(GetWorld());
}












/*
* overrides
*/

void AHeliPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("Scoreboard", IE_Pressed, this, &AHeliPlayerController::OnShowScoreboard);
	InputComponent->BindAction("Scoreboard", IE_Released, this, &AHeliPlayerController::OnHideScoreboard);

	InputComponent->BindAction("InGameMenu", IE_Released, this, &AHeliPlayerController::OnShowInGameMenu);


	InputComponent->BindAction("FlushDebugLines", IE_Released, this, &AHeliPlayerController::FlushDebugLines);

	InputComponent->BindAction("Suicide", IE_Pressed, this, &AHeliPlayerController::Suicide);
}

void AHeliPlayerController::ClientReturnToMainMenu_Implementation(const FString& InReturnReason)
{
	UHeliGameInstance* SGI = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;

	if (!ensure(SGI != NULL))
	{
		return;
	}

	if (GetNetMode() == NM_Client)
	{
		//const FText ReturnReason = NSLOCTEXT("NetworkErrors", "HostQuit", "The host has quit the match.");
		//const FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");

		//SGI->ShowMessageThenGotoState(ReturnReason, OKButton, FText::GetEmpty(), ShooterGameInstanceState::MainMenu);

		SGI->GotoState(EHeliGameInstanceState::MainMenu);
	}
	else
	{
		SGI->GotoState(EHeliGameInstanceState::MainMenu);
	}

	// Clear the flag so we don't do normal end of round stuff next
	//bGameEndedFrame = false;
}

/** Pawn has been possessed. Start it walking and begin playing with it. */
void AHeliPlayerController::BeginPlayingState()
{
	Super::BeginPlayingState();

	//UE_LOG(LogTemp, Display, TEXT("AHeliPlayerController::BeginPlayingState - %s %s has Role %d, RemoteRole %d and SpawnLocation: %s"), IsLocalPlayerController() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *GetName(), (int32)Role, (int32)GetRemoteRole(), *GetSpawnLocation().ToString());

	AHelicopter* helicopter = Cast<AHelicopter>(GetPawn());
	AHeliPlayerState* heliPlayerState = Cast<AHeliPlayerState>(PlayerState);
	if (helicopter && heliPlayerState)
	{
		helicopter->UpdatePlayerInfo(FName(*heliPlayerState->GetPlayerName()), heliPlayerState->GetTeamNumber());
		
		//UE_LOG(LogTemp, Warning, TEXT("AHeliPlayyerController::BeginPlayingState - PlayerName: %s  TeamNumber: %d"), *, heliPlayerState->GetTeamNumber());
	}
}

void AHeliPlayerController::InitInputSystem()
{
	Super::InitInputSystem();

	// TODO: UHeliPersistentUser class
	/*UHeliPersistentUser* PersistentUser = GetPersistentUser();
	if (PersistentUser)
	{
	PersistentUser->TellInputAboutKeybindings();
	}*/
}

void AHeliPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);

	//UE_LOG(LogLoad, Display, TEXT("PreClientTravel --> %s"), *FString::Printf(TEXT("PendingURL: %s, bIsSeamlessTravel: %s"), *PendingURL, bIsSeamlessTravel ? *FString(TEXT("true")) : *FString(TEXT("false"))));

	if (GetWorld() != NULL)
	{
		UHeliGameInstance* SGI = GetWorld() != NULL ? Cast<UHeliGameInstance>(GetWorld()->GetGameInstance()) : NULL;

		if (SGI)
		{
			UGameViewportClient* MyViewport = Cast<UGameViewportClient>(SGI->GetGameViewportClient());

			if (MyViewport)
			{
				MyViewport->RemoveAllViewportWidgets();


				// show loading screen
				SGI->ShowLoadingScreen(*FString::Printf(TEXT("Loading...")));
			}
		}

		AHeliHud* HeliHud = Cast<AHeliHud>(GetHUD());
		if (HeliHud != nullptr)
		{
			// Passing true to bFocus here ensures that focus is returned to the game viewport.
			// TODO: HeliHud->ShowScoreboard(false, true);
		}
	}
}

void AHeliPlayerController::GameHasEnded(class AActor* EndGameFocus, bool bIsWinner)
{
	// TODO: UpdateSaveFileOnGameEnd(bIsWinner);
	// TODO: UpdateAchievementsOnGameEnd();
	// TODO: UpdateLeaderboardsOnGameEnd();

	Super::GameHasEnded(EndGameFocus, bIsWinner);
}

void AHeliPlayerController::UnFreeze()
{
	ServerRestartPlayer();
}

void AHeliPlayerController::FailedToSpawnPawn()
{
	UE_LOG(LogTemp, Error, TEXT("AHeliPlayerController::FailedToSpawnPawn ~ %s - SpawnLocation: %s"), *GetName(), *GetSpawnLocation().ToString());
	if (StateName == NAME_Inactive)
	{
		BeginInactiveState();
	}
	Super::FailedToSpawnPawn();
}

void AHeliPlayerController::SetSpawnLocation(const FVector& NewLocation)
{
	Super::SetSpawnLocation(NewLocation);
}

void AHeliPlayerController::Possess(APawn* aPawn)
{
	//UE_LOG(LogTemp, Warning, TEXT("AHeliPlayerController::Possess - %s %s has Role %d, RemoteRole %d, Pawn: %s and SpawnLocation: %s"), IsLocalPlayerController() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *GetName(), (int32)Role, (int32)GetRemoteRole(), *aPawn->GetName(), *GetSpawnLocation().ToString());

	Super::Possess(aPawn);
}
