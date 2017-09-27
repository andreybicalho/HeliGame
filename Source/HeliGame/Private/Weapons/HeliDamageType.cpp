// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliDamageType.h"
#include "HeliGame.h"


UHeliDamageType::UHeliDamageType()
{
	/* We apply this modifier based on the physics material setup to the cockpit of the enemy PhysAsset */
	CockpitDmgModifier = 3.0f;

	FuselageDmgModifier = 2.0f;

	TailDmgModifier = 1.0f;
}

float UHeliDamageType::GetCockpitDamageModifier()
{
	return CockpitDmgModifier;
}

float UHeliDamageType::GetFuselageDamageModifier()
{
	return FuselageDmgModifier;
}

float UHeliDamageType::GetTailDamageModifier()
{
	return TailDmgModifier;
}



