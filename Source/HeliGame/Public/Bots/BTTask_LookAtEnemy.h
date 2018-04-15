// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_LookAtEnemy.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API UBTTask_LookAtEnemy : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTTask_LookAtEnemy(const FObjectInitializer &ObjectInitializer);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	
};
