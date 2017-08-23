// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"
#include "Runtime/UMG/Public/UMG.h"

namespace EHeliGameMatchState
{
	enum Type
	{
		Warmup,
		Playing,
		Won,
		Lost,
	};
}

// need loging?
//DECLARE_LOG_CATEGORY_EXTERN(LogHeli, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogHeliWeapon, Log, All);

/** when you modify this, please note that this information can be saved with instances
* also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list **/
#define COLLISION_PROJECTILE	ECC_GameTraceChannel1
#define COLLISION_WEAPON		ECC_GameTraceChannel2
#define COLLISION_CRASHBOX		ECC_GameTraceChannel3
#define COLLISION_HELICOPTER	ECC_GameTraceChannel4
#define COLLISION_HELI_ENV_BOX 	ECC_GameTraceChannel5

/** when you modify this, please note that this information can be saved with instances
* also DefaultEngine.ini [/Script/Engine.PhysicsSettings] should match with this list **/
#define SURFACE_DEFAULT				SurfaceType_Default
#define SURFACE_HELICOCKPIT			SurfaceType1
#define SURFACE_HELIFUSELAGE        SurfaceType2
#define SURFACE_HELITAIL			SurfaceType3

