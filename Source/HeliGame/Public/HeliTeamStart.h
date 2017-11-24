// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

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
	UPROPERTY(EditInstanceOnly, Category = "Team")
	int32 SpawnTeam;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	bool isTaken;

};
