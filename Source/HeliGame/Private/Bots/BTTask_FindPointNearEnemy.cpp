// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "BTTask_FindPointNearEnemy.h"
#include "HeliAIController.h"
#include "HeliFighterVehicle.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"

UBTTask_FindPointNearEnemy::UBTTask_FindPointNearEnemy(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
	MinimumDistanceFromEnemy = 10000.f;
}

EBTNodeResult::Type UBTTask_FindPointNearEnemy::ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory)
{
    AHeliAIController *myController = Cast<AHeliAIController>(OwnerComp.GetAIOwner());
    if (myController == nullptr)
    {
        return EBTNodeResult::Failed;
    }

    APawn* myBot = myController->GetPawn();
    AHeliFighterVehicle* enemy = myController->GetEnemy();

	FVector distanceFromEnemy = myBot->GetActorLocation() - enemy->GetActorLocation();
	FVector minimumDistance = (distanceFromEnemy).GetSafeNormal() * MinimumDistanceFromEnemy;
	
	FVector location = enemy->GetActorLocation() + minimumDistance;
	if (distanceFromEnemy.Size() < location.Size())
	{
		location = myBot->GetActorLocation();
	}



    if(myBot && enemy)
    {
        OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), location);
        return EBTNodeResult::Succeeded;
    }

	return EBTNodeResult::Failed;
}