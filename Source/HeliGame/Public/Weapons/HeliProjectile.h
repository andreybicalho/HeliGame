// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ProjectileWeapon.h"
#include "HeliProjectile.generated.h"

UCLASS()
class HELIGAME_API AHeliProjectile : public AActor
{
	GENERATED_BODY()

	/** FX projectile, it will be it's representation in the world */
	UPROPERTY(Category = "Effects", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* ProjectileFX;

public:
	// Sets default values for this actor's properties
	AHeliProjectile();

	/** initial setup */
	virtual void PostInitializeComponents() override;

	/** setup velocity */
	void InitVelocity(FVector& ShootDirection);

	/** handle hit */
	UFUNCTION()
	void OnImpact(const FHitResult& HitResult);

private:
	/** movement component */
	UPROPERTY(VisibleDefaultsOnly, Category = "Projectile")
	UProjectileMovementComponent* MovementComp;

	/** collisions */
	UPROPERTY(VisibleDefaultsOnly, Category = "Projectile")
	USphereComponent* CollisionComp;

	/* Particle FX played when a surface is hit. */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class AImpactEffect> ImpactTemplate;

protected:
	/** controller that fired me (cache for damage calculations) */
	TWeakObjectPtr<AController> MyController;

	/** projectile data */
	struct FProjectileWeaponData WeaponConfig;

	/** did it explode? */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Exploded)
	bool bExploded;

	/** [client] explosion happened */
	UFUNCTION()
	void OnRep_Exploded();

	/** trigger explosion */
	void Explode(const FHitResult& Impact);

	/* spawn impact effects according to the surface that was hit */
	void SpawnImpactEffects(const FHitResult& Impact);

	/** shutdown projectile and prepare for destruction */
	void DisableAndDestroy();

	/** update velocity on client */
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;

	bool ShouldDealDamage(AActor* TestActor) const;

	void DealDamage(const FHitResult& Impact);

	/** find hit */
	FHitResult Trace(const FVector& TraceFrom, const FVector& TraceTo) const;

protected:
	/** Returns MovementComp subobject **/
	FORCEINLINE UProjectileMovementComponent* GetMovementComp() const { return MovementComp; }
	/** Returns CollisionComp subobject **/
	FORCEINLINE USphereComponent* GetCollisionComp() const { return CollisionComp; }
};
