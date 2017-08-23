// Fill out your copyright notice in the Description page of Project Settings.

#include "HeliGame.h"
#include "ProjectileWeapon.h"
#include "HeliProjectile.h"
#include "Helicopter.h"

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
	FVector ShootDir = GetActorForwardVector();

	if(MyPawn->IsFirstPersonView())
	{
		ShootDir = GetFirstPersonShootDirection(Origin);
	}
	
	//UE_LOG(LogHeliWeapon, Log, TEXT("Impact.ImpactPoint = %s"), *Impact.ImpactPoint.ToString());
	//UE_LOG(LogHeliWeapon, Log, TEXT("Impact.Location = %s"), *Impact.Location.ToString());

	// debug line - from weapon location to aim point
	/*DrawDebugLine(
		GetWorld(),
		Origin,
		ShootDir * ProjectileAdjustRange,
		FColor(0, 255, 0),
		true, -1, 0,
		12.333
	);*/

	// Server spawn the projectile
	ServerFireProjectile(Origin, ShootDir);	
}

FVector AProjectileWeapon::GetFirstPersonShootDirection(FVector& Origin)
{
	FVector ShootDir = GetAdjustedAim();

	// trace from camera to check what's under crosshair
	const float ProjectileAdjustRange = ProjectileConfig.WeaponRange;
	const FVector StartTrace = GetCameraDamageStartLocation(ShootDir);
	const FVector EndTrace = StartTrace + ShootDir * ProjectileAdjustRange;
	FHitResult Impact = WeaponTrace(StartTrace, EndTrace);

	// and adjust directions to hit that actor
	if (Impact.bBlockingHit)
	{
		const FVector AdjustedDir = (Impact.ImpactPoint - Origin).GetSafeNormal();
		bool bWeaponPenetration = false;

		const float DirectionDot = FVector::DotProduct(AdjustedDir, ShootDir);
		if (DirectionDot < 0.0f)
		{
			// shooting backwards = weapon is penetrating
			bWeaponPenetration = true;
		}
		else if (DirectionDot < 0.5f)
		{
			// check for weapon penetration if angle difference is big enough
			// raycast along weapon mesh to check if there's blocking hit

			FVector MuzzleStartTrace = Origin - GetMuzzleDirection() * 150.0f;
			FVector MuzzleEndTrace = Origin;
			FHitResult MuzzleImpact = WeaponTrace(MuzzleStartTrace, MuzzleEndTrace);

			if (MuzzleImpact.bBlockingHit)
			{
				bWeaponPenetration = true;
			}
		}

		if (bWeaponPenetration)
		{
			// spawn at crosshair position
			//Origin = Impact.ImpactPoint - ShootDir * 10.0f;
		}
		else
		{
			// adjust direction to hit
			ShootDir = AdjustedDir;
		}
	}

	// debug line - from eye to aim point
	/*DrawDebugLine(
		GetWorld(),
		StartTrace,
		EndTrace,
		FColor(255, 0, 0),
		true, -1, 0,
		12.333
		);*/

	return ShootDir;
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
		Projectile->InitVelocity(ShootDir);

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
