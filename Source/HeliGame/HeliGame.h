// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

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
#define COLLISION_HELICOPTER	ECC_GameTraceChannel3

/** when you modify this, please note that this information can be saved with instances
* also DefaultEngine.ini [/Script/Engine.PhysicsSettings] should match with this list **/
#define SURFACE_DEFAULT				SurfaceType_Default
#define SURFACE_HELICOCKPIT			SurfaceType1
#define SURFACE_HELIFUSELAGE        SurfaceType2
#define SURFACE_HELITAIL			SurfaceType3

