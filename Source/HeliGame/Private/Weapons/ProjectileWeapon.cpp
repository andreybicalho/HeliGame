// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "ProjectileWeapon.h"
#include "HeliGame.h"
#include "HeliProjectile.h"
#include "Helicopter.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Public/DrawDebugHelpers.h"

//////////////////////////////////////////////////////////////////////////
// Weapon usage

AProjectileWeapon::AProjectileWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TrailTargetParam = TEXT("ShockBeamEnd");

	Shotcounter = 0;
}

void AProjectileWeapon::FireWeapon()
{		
	FVector Origin = GetMuzzleLocation();
	FVector ShootDir = GetWeaponArrow()->GetForwardVector();//GetActorForwardVector();

	if(MyPawn->IsFirstPersonView())
	{
		ShootDir = GetAdjustedShootDirectionForFirstPersonView();
	}

	if (bShowShotDirection)
	{
		DrawDebugLine(
			GetWorld(),
			GetMuzzleLocation(),
			GetMuzzleLocation() + (ShootDir * ProjectileConfig.WeaponRange),
			FColor(0, 0, 255),
			false,
			15.f,
			0,
			12.333
		);
	}

	// Server spawn the projectile
	ServerFireProjectile(Origin, ShootDir);
}

FVector AProjectileWeapon::GetAdjustedShootDirectionForFirstPersonView()
{
	FVector StartTrace;
	FVector TraceDirection;
	GetAimViewpoint(StartTrace, TraceDirection);
	FVector EndTrace = StartTrace + (TraceDirection * ProjectileConfig.WeaponRange);
	FHitResult Impact = WeaponTrace(StartTrace, EndTrace);

	FVector AdjustedDir = GetActorForwardVector();

	// and adjust directions to hit that actor
	if (Impact.bBlockingHit)
	{
		AdjustedDir = (Impact.Location - GetMuzzleLocation()).GetSafeNormal();
	}

	return AdjustedDir;
}

bool AProjectileWeapon::ServerFireProjectile_Validate(FVector Origin, FVector_NetQuantizeNormal ShootDir)
{
	return true;
}

void AProjectileWeapon::ServerFireProjectile_Implementation(FVector Origin, FVector_NetQuantizeNormal ShootDir)
{
	FTransform SpawnTM(ShootDir.Rotation(), Origin);
	AHeliProjectile* Projectile = Cast<AHeliProjectile>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, ProjectileConfig.ProjectileClass, SpawnTM));
	if (Projectile)
	{
		Projectile->Instigator = Instigator;
		Projectile->SetOwner(this);

		FVector heliVelocity = GetPawnOwner()->GetVelocity();
		Projectile->InitVelocity(ShootDir, heliVelocity);

		UGameplayStatics::FinishSpawningActor(Projectile, SpawnTM);
	}

	// spawn trail FX in all the remote clients
	Client_SpawnTrailEffect(Origin, ShootDir);

}

void AProjectileWeapon::ApplyWeaponConfig(FProjectileWeaponData& Data)
{
	Data = ProjectileConfig;
}

void AProjectileWeapon::SpawnTrailEffect(const FVector& Origin, const FVector& ShootDir)
{
	Shotcounter++;
	// do not spawn trail FX for every single shot
	if((Shotcounter % 2 == 0))
	{
		return;
	}

	if (TrailFX)
	{
		UParticleSystemComponent* TrailPSC = UGameplayStatics::SpawnEmitterAtLocation(this, TrailFX, Origin);
		if (TrailPSC)
		{
			const FVector EndPoint = Origin + ShootDir * ProjectileConfig.WeaponRange;
			TrailPSC->SetVectorParameter(TrailTargetParam, EndPoint);
		}
	}
}


void AProjectileWeapon::Client_SpawnTrailEffect_Implementation(const FVector& Origin, const FVector& ShootDir)
{
	SpawnTrailEffect(Origin, ShootDir);
}
