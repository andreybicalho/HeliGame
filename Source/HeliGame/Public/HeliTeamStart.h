// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerStart.h"
#include "HeliTeamStart.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliTeamStart : public APlayerStart
{
	GENERATED_BODY()
	
public:
	AHeliTeamStart(const FObjectInitializer& ObjectInitializer);

	/** Which team can start at this point */
	UPROPERTY(EditInstanceOnly, Category = Team)
	int32 SpawnTeam;
	
	UPROPERTY(EditInstanceOnly, Category = Team)
	bool isTaken;
};
