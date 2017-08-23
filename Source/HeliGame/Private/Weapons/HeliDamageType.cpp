// Fill out your copyright notice in the Description page of Project Settings.

#include "HeliGame.h"
#include "HeliDamageType.h"


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



