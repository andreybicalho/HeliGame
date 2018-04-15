// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "BTTask_LookAtEnemy.h"
#include "HeliAIController.h"
#include "HeliFighterVehicle.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"

UBTTask_LookAtEnemy::UBTTask_LookAtEnemy(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
}

EBTNodeResult::Type UBTTask_LookAtEnemy::ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory)
{
	AHeliAIController *myController = Cast<AHeliAIController>(OwnerComp.GetAIOwner());
	if (myController == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	myController->LookAtEnemy();

	return EBTNodeResult::Succeeded;
}



