// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ImpactEffect.generated.h"

UCLASS()
class HELIGAME_API AImpactEffect : public AActor
{
	GENERATED_BODY()
	
protected:

	UParticleSystem* GetImpactFX(EPhysicalSurface SurfaceType) const;

	USoundCue* GetImpactSound(EPhysicalSurface SurfaceType) const;

public:
	// Sets default values for this actor's properties
	AImpactEffect();

	virtual void PostInitializeComponents() override;

	/* FX spawned on standard materials */
	UPROPERTY(EditDefaultsOnly)
		UParticleSystem* DefaultFX;

	UPROPERTY(EditDefaultsOnly)
		UParticleSystem* HeliFuselageFX;

	UPROPERTY(EditDefaultsOnly)
		UParticleSystem* HeliCockpitFX;

	UPROPERTY(EditDefaultsOnly)
		USoundCue* DefaultSound;

	UPROPERTY(EditDefaultsOnly)
		USoundCue* HeliFuselageSound;

	UPROPERTY(EditDefaultsOnly, Category = "Decal")
		UMaterial* DecalMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Decal")
		float DecalSize;

	UPROPERTY(EditDefaultsOnly, Category = "Decal")
		float DecalLifeSpan;

	FHitResult SurfaceHit;


};
