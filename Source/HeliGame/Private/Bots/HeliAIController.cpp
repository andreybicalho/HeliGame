// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliAIController.h"
#include "HeliGame.h"
#include "HeliFighterVehicle.h"
#include "HeliBot.h"
#include "Weapon.h"
#include "HeliPlayerState.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetMathLibrary.h"

AHeliAIController::AHeliAIController(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    BlackboardComponent = ObjectInitializer.CreateDefaultSubobject<UBlackboardComponent>(this, TEXT("BlackBoardComponent"));

    BrainComponent = BehaviorComponent = ObjectInitializer.CreateDefaultSubobject<UBehaviorTreeComponent>(this, TEXT("BehaviorComponent"));

    bWantsPlayerState = true;
}

void AHeliAIController::Possess(APawn* InPawn)
{
	Super::Possess(InPawn);

	AHeliBot* Bot = Cast<AHeliBot>(InPawn);

	// start behavior
	if (Bot && Bot->BotBehavior)
	{
		if (Bot->BotBehavior->BlackboardAsset)
		{
			BlackboardComponent->InitializeBlackboard(*Bot->BotBehavior->BlackboardAsset);
		}

		// binds "Enemy" key from the blackboard to this int32 EnemyKeyID
		EnemyKeyID = BlackboardComponent->GetKeyID("Enemy");
		DestinationKeyID = BlackboardComponent->GetKeyID("Destination");

		BehaviorComponent->StartTree(*(Bot->BotBehavior));
	}
}

void AHeliAIController::UnPossess()
{
	Super::UnPossess();

	BehaviorComponent->StopTree();
}

void AHeliAIController::BeginInactiveState()
{
	Super::BeginInactiveState();

	AGameStateBase const* const GameState = GetWorld()->GetGameState();

	const float MinRespawnDelay = GameState ? GameState->GetPlayerRespawnDelay(this) : 1.0f;

	GetWorldTimerManager().SetTimer(TimerHandle_Respawn, this, &AHeliAIController::Respawn, MinRespawnDelay);
}

void AHeliAIController::Respawn()
{
	GetWorld()->GetAuthGameMode()->RestartPlayer(this);
}

void AHeliAIController::SetEnemy(class APawn *InPawn)
{
    if (BlackboardComponent)
    {
        BlackboardComponent->SetValue<UBlackboardKeyType_Object>(EnemyKeyID, InPawn);
        SetFocus(InPawn);
    }
}

AHeliFighterVehicle *AHeliAIController::GetEnemy() const
{
    if (BlackboardComponent)
    {
        return Cast<AHeliFighterVehicle>(BlackboardComponent->GetValue<UBlackboardKeyType_Object>(EnemyKeyID));
    }

    return nullptr;
}

void AHeliAIController::FindClosestEnemy()
{
	//UE_LOG(LogTemp, Display, TEXT("AHeliAIController::FindClosestEnemy"));

	APawn* MyBot = GetPawn();
	if (MyBot == nullptr)
	{
		return;
	}

	const FVector MyLoc = MyBot->GetActorLocation();
	float BestDistSq = MAX_FLT;
	AHeliFighterVehicle* BestPawn = nullptr;

	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		AHeliFighterVehicle* TestPawn = Cast<AHeliFighterVehicle>(*It);
		if (TestPawn && TestPawn->IsAlive() && TestPawn->IsEnemyFor(this))
		{
			const float DistSq = (TestPawn->GetActorLocation() - MyLoc).SizeSquared();
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestPawn = TestPawn;
			}
		}
	}

	if (BestPawn)
	{
		SetEnemy(BestPawn);
		//UE_LOG(LogTemp, Display, TEXT("AHeliAIController::FindClosestEnemy - Found %s"), BestPawn->GetPlayerName());
	}
}

bool AHeliAIController::FindClosestEnemyWithLOS(AHeliFighterVehicle *ExcludeEnemy)
{
	//UE_LOG(LogTemp, Display, TEXT("AHeliAIController::FindClosestEnemyWithLOS"));

    bool bGotEnemy = false;
    APawn *MyBot = GetPawn();
    if (MyBot != nullptr)
    {
        const FVector MyLoc = MyBot->GetActorLocation();
        float BestDistSq = MAX_FLT;
        AHeliFighterVehicle *BestPawn = nullptr;

        for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
        {
            AHeliFighterVehicle *TestPawn = Cast<AHeliFighterVehicle>(*It);
            if (TestPawn && TestPawn != ExcludeEnemy && TestPawn->IsAlive() && TestPawn->IsEnemyFor(this))
            {
                if (HasWeaponLOSToEnemy(TestPawn, true) == true)
                {
                    const float DistSq = (TestPawn->GetActorLocation() - MyLoc).SizeSquared();
                    if (DistSq < BestDistSq)
                    {
                        BestDistSq = DistSq;
                        BestPawn = TestPawn;
                    }
                }
            }
        }
        if (BestPawn)
        {
            SetEnemy(BestPawn);
            bGotEnemy = true;
			//UE_LOG(LogTemp, Display, TEXT("AHeliAIController::FindClosestEnemyWithLOS - Found %s"), BestPawn->GetPlayerName());
        }
    }
    return bGotEnemy;
}

bool AHeliAIController::HasWeaponLOSToEnemy(AActor *InEnemyActor, const bool bAnyEnemy) const
{	
	AHeliBot* MyBot = Cast<AHeliBot>(GetPawn());

	bool bHasLOS = false;
	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(AIWeaponLosTrace), true, GetPawn());
	TraceParams.bTraceAsyncScene = true;

	TraceParams.bReturnPhysicalMaterial = true;	
	FVector StartLocation = MyBot->GetActorLocation();	
	StartLocation.Z += GetPawn()->BaseEyeHeight; //look from eyes
	
	FHitResult Hit(ForceInit);
	const FVector EndLocation = InEnemyActor->GetActorLocation();
	GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, COLLISION_WEAPON, TraceParams);
	if (Hit.bBlockingHit == true)
	{
		// Theres a blocking hit - check if its our enemy actor
		AActor* HitActor = Hit.GetActor();
		if (Hit.GetActor() != nullptr)
		{
			if (HitActor == InEnemyActor)
			{
				bHasLOS = true;
			}
			else if (bAnyEnemy == true)
			{
				// Its not our actor, maybe its still an enemy ?
				AHeliFighterVehicle* HitVehicle = Cast<AHeliFighterVehicle>(HitActor);
				if (HitVehicle != nullptr)
				{
					AHeliPlayerState* HitPlayerState = Cast<AHeliPlayerState>(HitVehicle->PlayerState);
					AHeliPlayerState* MyPlayerState = Cast<AHeliPlayerState>(PlayerState);
					if ((HitPlayerState != nullptr) && (MyPlayerState != nullptr))
					{
						if (HitPlayerState->GetTeamNumber() != MyPlayerState->GetTeamNumber())
						{
							bHasLOS = true;
						}
					}
				}
			}
		}
	}

	return bHasLOS;
}

void AHeliAIController::GameHasEnded(AActor *EndGameFocus, bool bIsWinner)
{
	// Stop the behaviour tree/logic
	BehaviorComponent->StopTree();

	// Stop any movement we already have
	StopMovement();

	// Cancel the repsawn timer
	GetWorldTimerManager().ClearTimer(TimerHandle_Respawn);

	// Clear any enemy
	SetEnemy(nullptr);

	// Finally stop firing
    AHeliBot *MyBot = Cast<AHeliBot>(GetPawn());
    AWeapon *MyWeapon = MyBot ? MyBot->GetCurrentWeaponEquiped() : nullptr;
    if (MyWeapon)
    {		
	    MyBot->StopWeaponFire();	
	}
}

void AHeliAIController::ShootEnemy()
{
	AHeliBot* MyBot = Cast<AHeliBot>(GetPawn());
    AWeapon *MyWeapon = MyBot ? MyBot->GetCurrentWeaponEquiped() : nullptr;
    if (MyWeapon == nullptr)
	{
		return;
	}

	bool bCanShoot = false;
	AHeliFighterVehicle* Enemy = GetEnemy();
	if ( Enemy && ( Enemy->IsAlive() )&& (MyWeapon->GetCurrentAmmo() > 0) && ( MyWeapon->CanFire() == true ) )
	{
		if (LineOfSightTo(Enemy, MyBot->GetActorLocation()))
		{
			bCanShoot = true;

			// aim at the enemy
			FRotator BotRot = UKismetMathLibrary::FindLookAtRotation(MyBot->GetActorLocation(), Enemy->GetActorLocation());
			MyBot->SetActorRotation(BotRot);
		}
	}

	if (bCanShoot)
	{
		MyBot->StartWeaponFire();
	}
	else
	{
		MyBot->StopWeaponFire();
	}
}

void AHeliAIController::LookAtDestination()
{
	AHeliBot* MyBot = Cast<AHeliBot>(GetPawn());	

	if (BlackboardComponent && MyBot)
	{
		FVector Destination = BlackboardComponent->GetValue<UBlackboardKeyType_Vector>(DestinationKeyID);

		if (!Destination.IsNearlyZero())
		{
			FRotator BotRot = UKismetMathLibrary::FindLookAtRotation(MyBot->GetActorLocation(), Destination);
			MyBot->SetActorRotation(BotRot);
		}		
	}
}

void AHeliAIController::LookAtEnemy()
{
	AHeliBot* myBot = Cast<AHeliBot>(GetPawn());
	AHeliFighterVehicle* enemy = GetEnemy();

	if (enemy && myBot)
	{		
		FRotator botRot = UKismetMathLibrary::FindLookAtRotation(myBot->GetActorLocation(), enemy->GetActorLocation());
		myBot->SetActorRotation(botRot);
	}
}

void AHeliAIController::SmoothLookAtEnemy(float DeltaTime, float InterpSpeed)
{
	AHeliBot* myBot = Cast<AHeliBot>(GetPawn());
	AHeliFighterVehicle* enemy = GetEnemy();

	if (enemy && myBot)
	{
		FRotator botRotationTarget = UKismetMathLibrary::FindLookAtRotation(myBot->GetActorLocation(), enemy->GetActorLocation());
		FRotator rotation = FMath::RInterpTo(myBot->GetActorRotation(), botRotationTarget, DeltaTime, InterpSpeed);
		myBot->SetActorRotation(rotation);
	}
	
}
