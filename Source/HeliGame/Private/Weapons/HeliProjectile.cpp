// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliProjectile.h"
#include "HeliGame.h"
#include "ImpactEffect.h"
#include "HeliDamageType.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"
#include "Public/DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"


// Sets default values
AHeliProjectile::AHeliProjectile()
{
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->AlwaysLoadOnClient = true;
	CollisionComp->AlwaysLoadOnServer = true;
	CollisionComp->bTraceComplexOnMove = true;
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(COLLISION_PROJECTILE);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);	
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(COLLISION_CRASHBOX, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(COLLISION_HELICOPTER, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(COLLISION_HELI_ENV_BOX, ECR_Ignore);
	RootComponent = CollisionComp;	

	ProjectileFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ProjectileFX"));
	ProjectileFX->AttachToComponent(CollisionComp, FAttachmentTransformRules::KeepRelativeTransform);

	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->InitialSpeed = 10000.0f;
	MovementComp->MaxSpeed = 10000.0f;
	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->ProjectileGravityScale = 0.f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bReplicateMovement = true;

}

void AHeliProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	MovementComp->OnProjectileStop.AddDynamic(this, &AHeliProjectile::OnImpact);
	CollisionComp->MoveIgnoreActors.Add(Instigator);

	AProjectileWeapon* OwnerWeapon = Cast<AProjectileWeapon>(GetOwner());
	if (OwnerWeapon)
	{
		OwnerWeapon->ApplyWeaponConfig(WeaponConfig);
	}

	SetLifeSpan(WeaponConfig.ProjectileLife);
	MyController = GetInstigatorController();
}


void AHeliProjectile::InitVelocity(FVector& ShootDirection)
{
	if (MovementComp)
	{
		MovementComp->Velocity = ShootDirection * MovementComp->InitialSpeed;
	}
}

void AHeliProjectile::OnImpact(const FHitResult& HitResult)
{
	if (Role == ROLE_Authority && !bExploded)
	{	
		Explode(HitResult);

		DisableAndDestroy();
	}

	if(bShowImpactPoint)
	{ 
		DrawDebugSphere(
			GetWorld(),
			HitResult.Location,
			24,
			8,
			FColor(255, 0, 0),
			false,
			15.0f
		);
	}
}

void AHeliProjectile::SpawnImpactEffects(const FHitResult& Impact)
{
	if (ImpactTemplate && Impact.bBlockingHit)
	{
		FHitResult UseImpact = Impact;
		
		// trace again to get physical material
		if (Impact.PhysMaterial == nullptr)
		{
			const FVector StartTrace = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;
			const FVector EndTrace = Impact.ImpactPoint - Impact.ImpactNormal * 10.0f;
			FHitResult Hit = Trace(StartTrace, EndTrace);
			UseImpact = Hit;
		}

		FTransform const SpawnTransform(Impact.ImpactNormal.Rotation(), Impact.ImpactPoint);
		/* This function prepares an actor to spawn, but requires another call to finish the actual spawn progress. This allows manipulation of properties before entering into the level */
		AImpactEffect* EffectActor = GetWorld()->SpawnActorDeferred<AImpactEffect>(ImpactTemplate, SpawnTransform);
		if (EffectActor)
		{
			EffectActor->SurfaceHit = UseImpact;
			UGameplayStatics::FinishSpawningActor(EffectActor, FTransform(Impact.ImpactNormal.Rotation(), Impact.ImpactPoint));
		}
	}
}


void AHeliProjectile::Explode(const FHitResult& Impact)
{
	if (ProjectileFX)
	{
		ProjectileFX->SetVisibility(false);
		ProjectileFX->Deactivate();		
	}

	// effects and damage origin shouldn't be placed inside mesh at impact point
	const FVector NudgedImpactLocation = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;

	if (WeaponConfig.ExplosionDamage > 0 && WeaponConfig.ExplosionRadius > 0 && WeaponConfig.DamageType)
	{
		// we could apply radial damage if we wanted something like a rocket
		//UGameplayStatics::ApplyRadialDamage(this, WeaponConfig.ExplosionDamage, NudgedImpactLocation, WeaponConfig.ExplosionRadius, WeaponConfig.DamageType, TArray<AActor*>(), this, MyController.Get());		

		// ... or we apply damage as in instant weapon
		if (ShouldDealDamage(Impact.GetActor()))
		{
			DealDamage(Impact);
		}
	}

	// Impact FX
	SpawnImpactEffects(Impact);
	

	bExploded = true;
}

void AHeliProjectile::DisableAndDestroy()
{
	// TODO: should turn some audio off????


	MovementComp->StopMovementImmediately();

	// give clients some time to show explosion
	SetLifeSpan(2.0f);
}


bool AHeliProjectile::ShouldDealDamage(AActor* TestActor) const
{
	// If we are an actor on the server, or the local client has authoritative control over actor, we should register damage.
	if (TestActor)
	{
		if (GetNetMode() != NM_Client ||
			TestActor->Role == ROLE_Authority ||
			TestActor->bTearOff)
		{
			return true;
		}
	}

	return false;
}

void AHeliProjectile::DealDamage(const FHitResult& Impact)
{
	float ActualHitDamage = 1.f;

	/* Handle special damage location on the helicopter body (types are setup in the Physics Asset of the helicopter */
	UHeliDamageType* DmgType = Cast<UHeliDamageType>(WeaponConfig.DamageType->GetDefaultObject());	

	// re-trace to get physics material for damage type information only
	FVector ProjDirection = GetActorForwardVector();
	const FVector StartTrace = GetActorLocation() - ProjDirection * 100;
	const FVector EndTrace = GetActorLocation() + ProjDirection * 100;
	FCollisionQueryParams TraceParams(FName(TEXT("ProjClient")), true, Instigator);
	TraceParams.bReturnPhysicalMaterial = true;
	
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, COLLISION_PROJECTILE, TraceParams);

	UPhysicalMaterial* PhysMat = HitResult.PhysMaterial.Get();
	if (PhysMat && DmgType)
	{
		if (PhysMat->SurfaceType == SURFACE_HELICOCKPIT)
		{
			ActualHitDamage *= DmgType->GetCockpitDamageModifier();
		}
		else if (PhysMat->SurfaceType == SURFACE_HELIFUSELAGE)
		{
			ActualHitDamage *= DmgType->GetFuselageDamageModifier();
		}
		else if (PhysMat->SurfaceType == SURFACE_HELITAIL)
		{
			ActualHitDamage *= DmgType->GetTailDamageModifier();
		}
	}

	FPointDamageEvent PointDmg;
	PointDmg.DamageTypeClass = WeaponConfig.DamageType;
	// PointDmg.HitInfo = Impact;
	PointDmg.HitInfo = HitResult;
	// directions is it's own rotation
	PointDmg.ShotDirection = this->GetActorRotation().Vector();
	PointDmg.Damage = ActualHitDamage;

	Impact.GetActor()->TakeDamage(PointDmg.Damage, PointDmg, MyController.Get(), this);
}

FHitResult AHeliProjectile::Trace(const FVector& StartTrace, const FVector& EndTrace) const
{
	static FName TraceTag = FName(TEXT("HeliProjectileTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(TraceTag, true, this);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult OutHit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(OutHit, StartTrace, EndTrace, COLLISION_PROJECTILE, TraceParams);

	return OutHit;
}


///CODE_SNIPPET_START: AActor::GetActorLocation AActor::GetActorRotation
void AHeliProjectile::OnRep_Exploded()
{
	FVector ProjDirection = GetActorForwardVector();

	const FVector StartTrace = GetActorLocation() - ProjDirection * 200;
	const FVector EndTrace = GetActorLocation() + ProjDirection * 150;
	FHitResult Impact;
	
	FCollisionQueryParams TraceParams(FName(TEXT("ProjClient")), true, Instigator);
	TraceParams.bReturnPhysicalMaterial = true;

	if (!GetWorld()->LineTraceSingleByChannel(Impact, StartTrace, EndTrace, COLLISION_PROJECTILE, TraceParams))
	{
		// failsafe
		Impact.ImpactPoint = GetActorLocation();
		Impact.ImpactNormal = -ProjDirection;
	}

	Explode(Impact);
}
///CODE_SNIPPET_END

void AHeliProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (MovementComp)
	{
		MovementComp->Velocity = NewVelocity;
	}
}

void AHeliProjectile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeliProjectile, bExploded);
}

