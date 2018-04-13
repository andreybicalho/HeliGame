// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliBot.h"
#include "HeliGame.h"
#include "HeliProjectile.h"

#include "Curves/CurveFloat.h"
#include "Components/AudioComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

AHeliBot::AHeliBot(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// physics and collisions
	MainStaticMeshComponent->SetSimulatePhysics(false);
	MainStaticMeshComponent->SetLinearDamping(0.4f);
	MainStaticMeshComponent->SetAngularDamping(1.f);
	MainStaticMeshComponent->SetEnableGravity(true);

	MainStaticMeshComponent->SetCollisionProfileName("MainStaticMeshComponent");
	MainStaticMeshComponent->SetCollisionObjectType(COLLISION_HELICOPTER);
	MainStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MainStaticMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MainStaticMeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	MainStaticMeshComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	MainStaticMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	MainStaticMeshComponent->SetCollisionResponseToChannel(COLLISION_HELICOPTER, ECR_Block);
	MainStaticMeshComponent->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);
	MainStaticMeshComponent->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	MainStaticMeshComponent->bGenerateOverlapEvents = true;
	MainStaticMeshComponent->SetNotifyRigidBodyCollision(true);
	OnCrashImpactDelegate.BindUFunction(this, "OnCrashImpact");
	MainStaticMeshComponent->OnComponentHit.Add(OnCrashImpactDelegate);


	// movement component
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->SetUpdatedComponent(MainStaticMeshComponent);
	MovementComponent->SetNetAddressable();
	MovementComponent->SetIsReplicated(true);
	MovementComponent->SetActive(true);
}

// Called when the game starts or when spawned
void AHeliBot::BeginPlay()
{
	Super::BeginPlay();
	
	if ((MainAudioComponent == nullptr) || (MainAudioComponent && !MainAudioComponent->IsPlaying()))
	{
		MainAudioComponent = PlaySound(MainLoopSound);
	}

	EnableThirdPersonViewpoint();

	// setup health bar
	SetupPlayerInfoWidget();
}

void AHeliBot::OnCrashImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	AHeliProjectile* HeliProjectile = Cast<AHeliProjectile>(OtherActor);
	if (HeliProjectile)
	{
		return;
	}

	float Damage = ComputeCrashImpactDamage();

	if (Damage > 0.f)
	{
		Server_CrashImpactTakeDamage(Damage);
		//UE_LOG(LogTemp, Error, TEXT("Damage = %f"), Damage);
	}
}

float AHeliBot::ComputeCrashImpactDamage()
{
	float ActualDamage = 0.f;

	if (CrashImpactDamageCurve)
	{
		// get damage based on impact momentum
		float Speed = GetVelocity().Size() / 100.f;
		float Mass = MainStaticMeshComponent->GetMass();
		float ImpactMomentum = Speed * Mass;
		float BaseDamage = MaxHealth * CrashImpactDamageCurve->GetFloatValue(ImpactMomentum);

		// increase it with tilt/inclination angles		
		float TiltAngle = FMath::Abs(FVector::DotProduct(FVector::UpVector, MainStaticMeshComponent->GetRightVector()));
		float InclinationAngle = FMath::Abs(FVector::DotProduct(FVector::UpVector, MainStaticMeshComponent->GetForwardVector()));
		float TiltInclinationWeight = FMath::Clamp((TiltAngle + InclinationAngle), 0.f, 1.f);
		ActualDamage = BaseDamage * TiltInclinationWeight;

		//UE_LOG(LogTemp, Warning, TEXT("Inclination = %f   Tilt = %f   TiltInclinationWeight = %f   ImpactMomentum = %f   BaseDamage = %f   ActualDamage = %f"), InclinationAngle, TiltAngle, TiltInclinationWeight, ImpactMomentum, BaseDamage, ActualDamage);
	}


	return ActualDamage;
}

bool AHeliBot::Server_CrashImpactTakeDamage_Validate(float Damage)
{
	return true;
}

void AHeliBot::Server_CrashImpactTakeDamage_Implementation(float Damage)
{
	if (Damage > 0.f)
	{
		// decrease its health
		Health -= Damage;

		if (Health <= 0)
		{
			KilledBy(this);
		}
	}
}

void AHeliBot::OnDeath(float KillingDamage, FDamageEvent const &DamageEvent, APawn *PawnInstigator, AActor *DamageCauser)
{
	if (bIsDying)
	{
		return;
	}

	bIsDying = true;
	Health = 0.0f;
	//client will take authoritative control
	bTearOff = true;

	// turn sound off
	if (MainAudioComponent)
	{
		MainAudioComponent->Stop();
	}
	// hide mesh on game
	MainStaticMeshComponent->SetVisibility(false);
	//disable collisions on mesh
	MainStaticMeshComponent->SetSimulatePhysics(false);
	MainStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RemoveWeapons();
	RemoveHealthWidget();

	// detaching from controller will make game mode to call RestartPlayer
	DetachFromControllerPendingDestroy();

	// play sound and FX for death
	PlayHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);

	//GEngine->AddOnScreenDebugMessage(2, 15.0f, FColor::Green, FString::Printf(TEXT("%s"), *LogNetRole()) );
}
