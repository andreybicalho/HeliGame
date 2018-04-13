// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HeliAIController.generated.h"


class UBlackboardComponent;
class UBehaviorTreeComponent;
/**
 * 
 */
UCLASS(config = Game)
class HELIGAME_API AHeliAIController : public AAIController
{
	GENERATED_BODY()

public:

	// Begin AController interface
	virtual void GameHasEnded(class AActor* EndGameFocus = nullptr, bool bIsWinner = false) override;
	virtual void Possess(class APawn* InPawn) override;
	virtual void UnPossess() override;
	virtual void BeginInactiveState() override;
	// End APlayerController interface

	
private:
	UPROPERTY(transient)
	UBlackboardComponent* BlackboardComponent;

	/* Cached BT component */
	UPROPERTY(transient)
	UBehaviorTreeComponent* BehaviorComponent;

protected:	
	int32 EnemyKeyID;
	int32 DestinationKeyID;

	/** Handle for efficient management of Respawn timer */
	FTimerHandle TimerHandle_Respawn;

public:
	AHeliAIController(const FObjectInitializer &ObjectInitializer);

	FORCEINLINE UBlackboardComponent *GetBlackboardComponent() const { return BlackboardComponent; }
	FORCEINLINE UBehaviorTreeComponent *GetBehaviorComponent() const { return BehaviorComponent; }

	void SetEnemy(class APawn* InPawn);

	class AHeliFighterVehicle *GetEnemy() const;

	/* If there is line of sight to current enemy, start firing at them */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void ShootEnemy();

	/* Finds the closest enemy and sets them as current target */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void FindClosestEnemy();

	UFUNCTION(BlueprintCallable, Category = "Behavior")
	bool FindClosestEnemyWithLOS(class AHeliFighterVehicle *ExcludeEnemy);

	bool HasWeaponLOSToEnemy(AActor *InEnemyActor, const bool bAnyEnemy) const;
	
	void Respawn();

	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void LookAtDestination();

	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void LookAtEnemy();

	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void SmoothLookAtEnemy(float DeltaTime, float InterpSpeed);
};
