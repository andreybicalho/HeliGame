// Fill out your copyright notice in the Description page of Project Settings.

#include "HeliGame.h"
#include "ImpactEffect.h"

// Sets default values
AImpactEffect::AImpactEffect()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bAutoDestroyWhenFinished = true;

	DecalLifeSpan = 10.0f;
	DecalSize = 16.0f;

}

void AImpactEffect::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	/* Figure out what we hit (SurfaceHit is setting during actor instantiation in weapon class) */
	UPhysicalMaterial* HitPhysMat = SurfaceHit.PhysMaterial.Get();
	EPhysicalSurface HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitPhysMat);

	UParticleSystem* ImpactFX = GetImpactFX(HitSurfaceType);
	if (ImpactFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ImpactFX, GetActorLocation(), GetActorRotation());
	}

	USoundCue* ImpactSound = GetImpactSound(HitSurfaceType);
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	if (DecalMaterial)
	{
		FRotator RandomDecalRotation = SurfaceHit.ImpactNormal.Rotation();
		RandomDecalRotation.Roll = FMath::FRandRange(-180.0f, 180.0f);

		UGameplayStatics::SpawnDecalAttached(DecalMaterial, FVector(1.0f, DecalSize, DecalSize),
			SurfaceHit.Component.Get(), SurfaceHit.BoneName,
			SurfaceHit.ImpactPoint, RandomDecalRotation, EAttachLocation::KeepWorldPosition,
			DecalLifeSpan);
	}
}


UParticleSystem* AImpactEffect::GetImpactFX(EPhysicalSurface SurfaceType) const
{
	// TODO: different FX for different materials
	switch (SurfaceType)
	{
	case SURFACE_DEFAULT:
		return DefaultFX;
	case SURFACE_HELICOCKPIT:
		return HeliCockpitFX;
	case SURFACE_HELIFUSELAGE:
		return HeliFuselageFX;
	case SURFACE_HELITAIL:
		return HeliFuselageFX;
	default:
		return nullptr;
	}
}


USoundCue* AImpactEffect::GetImpactSound(EPhysicalSurface SurfaceType) const
{
	// TODO: different sounds for different materials
	switch (SurfaceType)
	{
	case SURFACE_DEFAULT:
		return DefaultSound;
	case SURFACE_HELICOCKPIT:
		return HeliFuselageSound;
	case SURFACE_HELIFUSELAGE:
		return HeliFuselageSound;
	case SURFACE_HELITAIL:
		return HeliFuselageSound;
	default:
		return nullptr;
	}
}


