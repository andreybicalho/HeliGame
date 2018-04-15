// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace
{
	const FString GameVersionName("BirdsOfWar_v0.3.0");
}

const static FName SERVER_NAME_SETTINGS_KEY = TEXT("ServerName");
const static FName GAME_VERSION_SETTINGS_KEY = TEXT("GameVersion");

// need loging?
//DECLARE_LOG_CATEGORY_EXTERN(LogHeli, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogHeliWeapon, Log, All);

/** when you modify this, please note that this information can be saved with instances
* also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list **/
#define COLLISION_PROJECTILE	ECC_GameTraceChannel1
#define COLLISION_WEAPON		ECC_GameTraceChannel2
#define COLLISION_HELICOPTER	ECC_GameTraceChannel3

/** when you modify this, please note that this information can be saved with instances
* also DefaultEngine.ini [/Script/Engine.PhysicsSettings] should match with this list **/
#define SURFACE_DEFAULT				SurfaceType_Default
#define SURFACE_HELICOCKPIT			SurfaceType1
#define SURFACE_HELIFUSELAGE        SurfaceType2
#define SURFACE_HELITAIL			SurfaceType3
#define SURFACE_EXPLOSIVE			SurfaceType4

