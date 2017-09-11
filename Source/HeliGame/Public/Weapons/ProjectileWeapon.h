// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Weapons/Weapon.h"
#include "ProjectileWeapon.generated.h"

USTRUCT()
struct FProjectileWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** projectile class */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class AHeliProjectile> ProjectileClass;

	/** life time */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float ProjectileLife;

	/** damage at impact point */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponStat")
	int32 ExplosionDamage;

	/** radius of damage */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponStat")
	float ExplosionRadius;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponStat")
	TSubclassOf<UDamageType> DamageType;

	/** weapon range */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponStat")
	float WeaponRange;
	

	/** defaults */
	FProjectileWeaponData()
	{
		ProjectileClass = NULL;
		ProjectileLife = 3.0f;
		ExplosionDamage = 10;
		ExplosionRadius = 300.0f;
		DamageType = UDamageType::StaticClass();
		WeaponRange = 30000.0f;
	}
};



/**
 * 
 */
UCLASS()
class HELIGAME_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Shotcounter;

public:
	// Sets default values for this actor's properties
	AProjectileWeapon(const FObjectInitializer& ObjectInitializer);

	/** apply config on projectile */
	void ApplyWeaponConfig(FProjectileWeaponData& Data);


protected:

	/** weapon config */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	FProjectileWeaponData ProjectileConfig;

	/** trail FX */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UParticleSystem* TrailFX;

	/** param name for beam target in trail FX */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	FName TrailTargetParam;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] weapon specific fire implementation */
	virtual void FireWeapon() override;

	/** spawn projectile on server */
	UFUNCTION(reliable, server, WithValidation)
	void ServerFireProjectile(FVector Origin, FVector_NetQuantizeNormal ShootDir);

private:
	FVector GetFirstPersonShootDirection(FVector& Origin);

	/** spawn trail effect */
	void SpawnTrailEffect(const FVector& Origin, const FVector& ShootDir);

	/* spawn trail effects in all clients */
	UFUNCTION(NetMulticast, Unreliable)
	void Client_SpawnTrailEffect(const FVector& Origin, const FVector& ShootDir);
};
