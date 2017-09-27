// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "GameFramework/DamageType.h"
#include "HeliDamageType.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API UHeliDamageType : public UDamageType
{
	GENERATED_BODY()
	
	/* Damage modifier for cockpit damage */
	UPROPERTY(EditDefaultsOnly)
		float CockpitDmgModifier;

	/* Damage modifier for Fuselage damage */
	UPROPERTY(EditDefaultsOnly)
		float FuselageDmgModifier;

	/* Damage modifier for tail damage */
	UPROPERTY(EditDefaultsOnly)
		float TailDmgModifier;

public:
	UHeliDamageType();

	float GetCockpitDamageModifier();

	float GetFuselageDamageModifier();
	
	float GetTailDamageModifier();


	
	
};
