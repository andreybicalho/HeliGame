// Copyright 2016 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliGameMode.h"
#include "HeliGame.h"
#include "HeliPlayerController.h"
#include "HeliPlayerState.h"
#include "Helicopter.h"
#include "HeliGameState.h"
#include "HeliHud.h"
#include "HeliGameSession.h"
#include "HeliTeamStart.h"
#include "HeliGameInstance.h"
#include "HeliAIController.h"

#include "UObject/ConstructorHelpers.h"
#include "Public/TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerStart.h"
#include "Public/EngineUtils.h"
#include "Engine/PlayerStartPIE.h"
#include "Engine/World.h"

AHeliGameMode::AHeliGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	/* Assign the class types used by this gamemode */
		
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/Helicopter_BP"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<AHeliHud> HudBPClass(TEXT("/Game/Blueprints/HeliHud_BP"));
	if (HudBPClass.Class != NULL)
	{
		HUDClass = HudBPClass.Class;
	}
	
	PlayerControllerClass = AHeliPlayerController::StaticClass();
	PlayerStateClass = AHeliPlayerState::StaticClass();
	GameStateClass = AHeliGameState::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> BotPawnOb(TEXT("/Game/Blueprints/Bots/EnemyBot_BP"));
	BotPawnClass = BotPawnOb.Class;

	bAllowBots = true;
	bNeedsBotCreation = true;
	
	bAllowFriendlyFireDamage = false;

	PlayerTeamNum = 0;
}

void AHeliGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	GetWorldTimerManager().SetTimer(TimerHandle_DefaultTimer, this, &AHeliGameMode::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation(), true);
}

void AHeliGameMode::InitGame(const FString& InMapName, const FString& Options, FString& ErrorMessage)
{
	// TODO: game options
	WarmupTime = UGameplayStatics::GetIntOption(Options, "WarmupTime", 2);
	RoundTime = UGameplayStatics::GetIntOption(Options, "RoundTime", 30);
	TimeBetweenMatches = UGameplayStatics::GetIntOption(Options, "TimeBetweenMatches", 15);
	MaxNumberOfPlayers = UGameplayStatics::GetIntOption(Options, "MaxNumberOfPlayers", 10);	
	MapName = *InMapName;
			
	// finding server name
	const FString &optionsRightChop = Options.RightChop( Options.Find("?CustomServerName=") );
	FString Key, Value;
	UGameplayStatics::GetKeyValue(optionsRightChop, Key, Value);
	ServerName = Value;

	bAllowFriendlyFireDamage = Options.Contains(TEXT("?bAllowFriendlyFireDamage"));

	const int32 BotsCountOptionValue = UGameplayStatics::GetIntOption(Options, "Bots", 0);
	SetAllowBots(BotsCountOptionValue > 0 ? true : false, BotsCountOptionValue);

	Super::InitGame(MapName, Options, ErrorMessage);

	// this is a multiplayer game we should never pause it
	bPauseable = false;
}

void AHeliGameMode::SetAllowBots(bool bInAllowBots, int32 InMaxBots)
{
	bAllowBots = bInAllowBots;
	MaxBots = InMaxBots;
}

void AHeliGameMode::InitGameState()
{
	Super::InitGameState();
}

void AHeliGameMode::DefaultTimer()
{
	// don't update timers for Play In Editor mode, it's not real match
	if (GetWorld()->IsPlayInEditor())
	{
		// start match if necessary.
		if (GetMatchState() == MatchState::WaitingToStart)
		{
			StartMatch();
		}
		return;
	}


	AHeliGameState* const MyGameState = Cast<AHeliGameState>(GameState);
	if (MyGameState && MyGameState->RemainingTime > 0)
	{
		MyGameState->RemainingTime--;

		if (MyGameState->RemainingTime <= 0)
		{
			if (GetMatchState() == MatchState::WaitingPostMatch)
			{
				RestartGame();
			}
			else if (GetMatchState() == MatchState::InProgress)
			{
				FinishMatch();				
			}
			else if (GetMatchState() == MatchState::WaitingToStart)
			{
				StartMatch();
			}
		}
	}
}

void AHeliGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	AHeliGameState* const MyGameState = Cast<AHeliGameState>(GameState);
	const bool bMatchIsOver = MyGameState && MyGameState->HasMatchEnded();
	if (bMatchIsOver)
	{
		ErrorMessage = TEXT("Match is over!");
	}
	else
	{
		// GameSession can be NULL if the match is over
		Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	}
}

void AHeliGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AHeliPlayerController* NewPC = Cast<AHeliPlayerController>(NewPlayer);
	// TODO: update spectator location for client
	/*if (NewPC && NewPC->GetPawn() == NULL)
	{
	NewPC->ClientSetSpectatorCamera(NewPC->GetSpawnLocation(), NewPC->GetControlRotation());
	}*/

	// notify new player if match is already in progress
	if (NewPC && IsMatchInProgress())
	{
		NewPC->ClientGameStarted();
		NewPC->ClientStartOnlineGame();
	}
}

AActor* AHeliGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> PreferredSpawns;
	TArray<APlayerStart*> FallbackSpawns;

	APlayerStart* BestStart = nullptr;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* TestSpawn = *It;
		if (TestSpawn->IsA<APlayerStartPIE>())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			BestStart = TestSpawn;
			UE_LOG(LogTemp, Warning, TEXT("AHeliGameMode::ChoosePlayerStart_Implementation ~ %s - SpawnLocation: %s (%s is APlayerStartPIE)"), *Player->GetName(), *BestStart->GetActorLocation().ToString(), *BestStart->GetName());

			break;
		}
		else
		{
			if (IsSpawnpointAllowed(TestSpawn, Player))
			{
				if (IsSpawnpointPreferred(TestSpawn, Player))
				{
					PreferredSpawns.Add(TestSpawn);
				}
				else
				{
					FallbackSpawns.Add(TestSpawn);
				}
			}
		}
	}


	if (BestStart == nullptr)
	{
		if (PreferredSpawns.Num() > 0)
		{
			BestStart = PreferredSpawns[FMath::RandHelper(PreferredSpawns.Num())];
		}
		else if (FallbackSpawns.Num() > 0)
		{
			BestStart = FallbackSpawns[FMath::RandHelper(FallbackSpawns.Num())];
		}
	}

	/*if(BestStart) 
	{ 
		UE_LOG(LogTemp, Display, TEXT("AHeliGameMode::ChoosePlayerStart_Implementation ~ %s %s has Role %d and RemoteRole %d: PreferredSpawns = %d, FallbackSpawns = %d PlayerStart = %s"), Player->IsLocalPlayerController() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *Player->GetName(), (int32)Player->Role, (int32)Player->GetRemoteRole(), PreferredSpawns.Num(), FallbackSpawns.Num(), *BestStart->GetName());
	} 
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AHeliGameMode::ChoosePlayerStart_Implementation ~ %s %s has Role %d and RemoteRole %d: PreferredSpawns = %d, FallbackSpawns = %d"), Player->IsLocalPlayerController() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *Player->GetName(), (int32)Player->Role, (int32)Player->GetRemoteRole(), PreferredSpawns.Num(), FallbackSpawns.Num());
	}*/

	return BestStart ? BestStart : Super::ChoosePlayerStart_Implementation(Player);
}

bool AHeliGameMode::IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const
{	
	//UE_LOG(LogTemp, Display, TEXT("AHeliGameMode::IsSpawnpointAllowed ~ %s ALLOWED for %s by default"), *SpawnPoint->GetName(), *Player->GetName());
	return true;
}

bool AHeliGameMode::IsSpawnpointPreferred(APlayerStart* SpawnPoint, AController* Player) const
{
	AHelicopter* MyPawn = Cast<AHelicopter>((*DefaultPawnClass)->GetDefaultObject<AHelicopter>());
	if (MyPawn)
	{
		const FVector SpawnLocation = SpawnPoint->GetActorLocation();
		for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			AHelicopter* OtherPawn = Cast<AHelicopter>(*It);
			if (OtherPawn && OtherPawn != MyPawn)
			{
				const float CombinedRadius = MyPawn->GetSimpleCollisionRadius() + OtherPawn->GetSimpleCollisionRadius();
				const FVector OtherLocation = OtherPawn->GetActorLocation();

				// check if player start overlaps this pawn
				if ((SpawnLocation - OtherLocation).Size2D() < CombinedRadius)
				{
					UE_LOG(LogTemp, Warning, TEXT("AHeliGameMode::IsSpawnpointPreferred ~ %s NOT PREFERRED for %s because OVERLAPING!"), *SpawnPoint->GetName(), *Player->GetName());
					return false;
				}
			}
		}
	}

	AHeliTeamStart* heliTeamStart = Cast<AHeliTeamStart>(SpawnPoint);
	AHeliPlayerState* heliPlayerState = Cast<AHeliPlayerState>(Player->PlayerState);
	if(heliTeamStart && heliPlayerState && heliTeamStart->PlayerStartTag.IsEqual(FName(*heliPlayerState->GetPlayerName())))
	{
		UE_LOG(LogTemp, Display, TEXT("AHeliGameMode::IsSpawnpointPreferred ~ %s PREFERRED for %s because it has been tagged with %s"), *SpawnPoint->GetName(), *Player->GetName(), *heliTeamStart->PlayerStartTag.ToString());
		
		return true;
	}

	return true;
}

bool AHeliGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	//  Always pick new random spawn	
	return false;
}

void AHeliGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	
	//UE_LOG(LogTemp, Warning, TEXT("AHeliGameMode::RestartPlayer ~ %s %s -- Role %d - RemoteRole %d"), NewPlayer->IsLocalPlayerController() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *NewPlayer->GetName(), (int32)NewPlayer->Role, (int32)NewPlayer->GetRemoteRole());
}

void AHeliGameMode::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);

	/*AHeliPlayerController* heliPlayerController = Cast<AHeliPlayerController>(NewPlayer);
	if (heliPlayerController)
	{
		heliPlayerController->SetSpawnLocation(StartSpot->GetActorLocation());

		//UE_LOG(LogTemp, Display, TEXT("AHeliGameMode::RestartPlayerAtPlayerStart ~ %s %s -- Role %d - RemoteRole %d, PlayerStart = %s -- set spawn location = %s"), NewPlayer->IsLocalPlayerController() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *NewPlayer->GetName(), (int32)NewPlayer->Role, (int32)NewPlayer->GetRemoteRole(), *StartSpot->GetName(), *heliPlayerController->GetSpawnLocation().ToCompactString());
	}

	if (GetDefaultPawnClassForController(NewPlayer) != nullptr)
	{
		// Try to create a pawn to use of the default class for this player
		NewPlayer->SetPawn(SpawnDefaultPawnFor(NewPlayer, StartSpot));
	}

	if (NewPlayer->GetPawn() == nullptr)
	{
		NewPlayer->FailedToSpawnPawn();
	}
	else
	{
		NewPlayer->Possess(NewPlayer->GetPawn());
		//FinishRestartPlayer(NewPlayer, StartSpot->GetActorRotation());
	}*/
}

void AHeliGameMode::RestartPlayerAtTransform(AController* NewPlayer, const FTransform& SpawnTransform)
{
	//UE_LOG(LogTemp, Display, TEXT("AHeliGameMode::RestartPlayerAtTransform ~ %s %s -- Role %d - RemoteRole %d, -- set spawn location %s"), NewPlayer->IsLocalPlayerController() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *NewPlayer->GetName(), (int32)NewPlayer->Role, (int32)NewPlayer->GetRemoteRole(), *SpawnTransform.GetLocation().ToCompactString());

	Super::RestartPlayerAtTransform(NewPlayer, SpawnTransform);
}

void AHeliGameMode::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	if (bNeedsBotCreation)
	{
		CreateBotControllers();
		bNeedsBotCreation = false;
	}

	if (bDelayedStart)
	{
		// start warmup if needed
		AHeliGameState* const MyGameState = Cast<AHeliGameState>(GameState);
		if (MyGameState && MyGameState->RemainingTime == 0)
		{
			const bool bWantsMatchWarmup = !GetWorld()->IsPlayInEditor();
			if (bWantsMatchWarmup && WarmupTime > 0)
			{
				MyGameState->RemainingTime = WarmupTime;
			}
			else
			{
				MyGameState->RemainingTime = 0.0f;
			}
		}
	}
}

void AHeliGameMode::HandleMatchHasStarted()
{
	bNeedsBotCreation = true;
	Super::HandleMatchHasStarted();	

	AHeliGameState* const MyGameState = Cast<AHeliGameState>(GameState);
	MyGameState->RemainingTime = RoundTime;
	StartBots();

	// notify players
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AHeliPlayerController* PC = Cast<AHeliPlayerController>(*It);
		if (PC)
		{
			PC->ClientGameStarted();
		}
	}
}


void AHeliGameMode::RestartGame()
{	
	bUseSeamlessTravel = false;

	Super::RestartGame();
}

void AHeliGameMode::EndGame()
{
	// do nothing in this base class
}


void AHeliGameMode::FinishMatch()
{	
	AHeliGameState* const MyGameState = Cast<AHeliGameState>(GameState);
	if (IsMatchInProgress())
	{
		EndMatch();
		DetermineMatchWinner();

		// notify players
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AHeliPlayerController* PlayerController = Cast<AHeliPlayerController>(*It);			
			
			if (PlayerController && MyGameState)
			{				
				AHeliPlayerState* PlayerState = Cast<AHeliPlayerState>(PlayerController->PlayerState);
				const bool bIsWinner = IsWinner(PlayerState);
				PlayerController->ClientSendRoundEndEvent(bIsWinner, MyGameState->ElapsedTime);
				// PlayerController->GameHasEnded(NULL, bIsWinner);
			}
		}


		// lock all pawns
		// pawns are not marked as keep for seamless travel, so we will create new pawns on the next match rather than
		// turning these back on.
		/*for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			(*It)->TurnOff();
		}*/

		// set up to restart the match
		MyGameState->RemainingTime = TimeBetweenMatches;
	}
}

void AHeliGameMode::RequestFinishAndExitToMainMenu()
{
	FinishMatch();

	AHeliPlayerController* LocalPrimaryController = nullptr;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AHeliPlayerController* Controller = Cast<AHeliPlayerController>(*Iterator);

		if (Controller == NULL)
		{
			continue;
		}

		if (!Controller->IsLocalController())
		{			
			const FText remoteReturnReason = FText::FromString("AHeliGameMode::RequestFinishAndExitToMainMenu: controller is remote");
			Controller->ClientReturnToMainMenuWithTextReason(remoteReturnReason);
		}
		else
		{
			LocalPrimaryController = Controller;
		}
	}

	// GameInstance should be calling this from an EndState.  So call the PC function that performs cleanup, not the one that sets GI state.
	if (LocalPrimaryController != NULL)
	{
		LocalPrimaryController->HandleReturnToMainMenu();
	}
}

// tell all clients to go to lobby state
void AHeliGameMode::RequestClientsGoToLobbyState()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AHeliPlayerController* Controller = Cast<AHeliPlayerController>(*Iterator);

		if (Controller && !Controller->IsLocalController())
		{			
			Controller->ClientReturnToLobbyState();
		}
	}
}

void AHeliGameMode::RestartAllPlayers()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AHeliPlayerController* Controller = Cast<AHeliPlayerController>(*Iterator);

		if (Controller && !Controller->IsLocalController())
		{
			RestartPlayer(Controller);
		}
	}
}

void AHeliGameMode::DetermineMatchWinner()
{
	// nothing to do here
	// classes derived from this one may have something to do
}

bool AHeliGameMode::IsWinner(class AHeliPlayerState* PlayerState) const
{
	// nothing to do here
	// classes derived from this one may have something to do

	return false;
}

/** Returns game session class to use */
TSubclassOf<AGameSession> AHeliGameMode::GetGameSessionClass() const
{
	return AHeliGameSession::StaticClass();
}



/************************************************************************
* Damage & Killing                                                      *
************************************************************************/
void AHeliGameMode::Killed(AController* Killer, AController* VictimPlayer, APawn* VictimPawn, const UDamageType* DamageType)
{
	// apply score and keep track of kill count
	AHeliPlayerState* KillerPlayerState = Killer ? Cast<AHeliPlayerState>(Killer->PlayerState) : NULL;
	AHeliPlayerState* VictimPlayerState = VictimPlayer ? Cast<AHeliPlayerState>(VictimPlayer->PlayerState) : NULL;

	if (KillerPlayerState && KillerPlayerState != VictimPlayerState)
	{
		KillerPlayerState->ScoreKill(VictimPlayerState, KillScore);
		KillerPlayerState->InformAboutKill(KillerPlayerState, DamageType, VictimPlayerState);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->ScoreDeath(KillerPlayerState, DeathScore);
		VictimPlayerState->BroadcastDeath(KillerPlayerState, DamageType, VictimPlayerState);
		VictimPlayerState->bDead = true;
	}
}


bool AHeliGameMode::CanDealDamage(class AHeliPlayerState* DamageCauser, class AHeliPlayerState* DamagedPlayer) const
{
	/* Do not allow damage to self */
	if (DamagedPlayer == DamageCauser)
		return false;

	return true;
}

float AHeliGameMode::ModifyDamage(float Damage, AActor* DamagedActor, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const
{
	float ActualDamage = Damage;

	AHelicopter* DamagedPawn = Cast<AHelicopter>(DamagedActor);
	if (DamagedPawn && EventInstigator)
	{
		AHeliPlayerState* DamagedPlayerState = Cast<AHeliPlayerState>(DamagedPawn->PlayerState);
		AHeliPlayerState* InstigatorPlayerState = Cast<AHeliPlayerState>(EventInstigator->PlayerState);

		// Check for friendly fire
		if (!CanDealDamage(InstigatorPlayerState, DamagedPlayerState))
		{
			ActualDamage = 0.f;
		}
	}

	return ActualDamage;
}

void AHeliGameMode::ScoreHit(AController* Attacker, AController* VictimPlayer, const int Damage)
{
	AHeliPlayerState* AttackerPlayerState = Attacker ? Cast<AHeliPlayerState>(Attacker->PlayerState) : NULL;
	AHeliPlayerState* VictimPlayerState = VictimPlayer ? Cast<AHeliPlayerState>(VictimPlayer->PlayerState) : NULL;


	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->ScoreHit(Damage);
	}

}

bool AHeliGameMode::IsImmediatelyPlayerRestartAllowedAfterDeath()
{
	return true;
}

FString AHeliGameMode::GetGameModeName()
{
	return FString(TEXT("AHeliGameMode"));
}

void AHeliGameMode::CreateBotControllers()
{
	UWorld *World = GetWorld();
	int32 ExistingBots = 0;
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{
		AHeliAIController *AIC = Cast<AHeliAIController>(*It);
		if (AIC)
		{
			++ExistingBots;
		}
	}

	// Create any necessary AIControllers.  Hold off on Pawn creation until pawns are actually necessary or need recreating.
	int32 botNum = ExistingBots;
	for (int32 i = 0; i < MaxBots - ExistingBots; ++i)
	{
		CreateBot(botNum + i);
	}
}

AHeliAIController *AHeliGameMode::CreateBot(int32 botNum)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = nullptr;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = nullptr;

	UWorld *World = GetWorld();
	AHeliAIController *AIC = World->SpawnActor<AHeliAIController>(SpawnInfo);
	InitBot(AIC, botNum);

	return AIC;
}

void AHeliGameMode::StartBots()
{
	// checking number of existing human player.
	UWorld *World = GetWorld();
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{
		AHeliAIController *AIC = Cast<AHeliAIController>(*It);
		if (AIC)
		{
			RestartPlayer(AIC);
		}
	}
}

void AHeliGameMode::InitBot(AHeliAIController *AIC, int32 botNum)
{
	if (AIC)
	{
		AHeliPlayerState* playerState = Cast<AHeliPlayerState>(AIC->PlayerState);
		if (playerState)
		{
			FString BotName = FString::Printf(TEXT("Bot %d"), botNum);
			playerState->Server_SetPlayerName(BotName);
		}
	}
}

UClass *AHeliGameMode::GetDefaultPawnClassForController_Implementation(AController *InController)
{
	if (InController->IsA<AHeliAIController>())
	{
		return BotPawnClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}