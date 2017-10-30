// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliTeamStart.h"
#include "Components/CapsuleComponent.h"
#include "HeliGame.h"

AHeliTeamStart::AHeliTeamStart(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Overlap);
		GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_HELICOPTER, ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);		
	}
}


