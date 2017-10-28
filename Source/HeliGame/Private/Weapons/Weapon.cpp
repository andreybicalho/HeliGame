// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "Weapon.h"
#include "HeliGame.h"
#include "HeliPlayerController.h"
#include "Helicopter.h"
#include "Kismet/GameplayStatics.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Public/TimerManager.h"



// Sets default values
AWeapon::AWeapon(const FObjectInitializer& ObjectInitializer)
{

	Mesh1P = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh1P"));
	Mesh1P->bReceivesDecals = true;
	Mesh1P->CastShadow = true;
	Mesh1P->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh1P->SetVisibility(true, true);
	RootComponent = Mesh1P;

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bNetUseOwnerRelevancy = true;

	// an simple arrow to show direction on blueprint viewport
	WeaponArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("WeaponArrow"));
	WeaponArrow->AttachToComponent(Mesh1P, FAttachmentTransformRules::KeepRelativeTransform);

	bIsEquipped = false;
	bWantsToFire = false;
	bPendingReload = false;
	CurrentState = EWeaponState::Idle;

	CurrentAmmo = 0;
	CurrentAmmoInClip = 0;
	BurstCounter = 0;
	LastFireTime = 0.0f;
	ReloadDuration = 2.f;
}


/** perform initial setup */
void AWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();


	CurrentAmmoInClip = WeaponConfig.AmmoPerClip;
	CurrentAmmo = WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;

}

void AWeapon::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


//////////////////////////////////////////////////////////////////////////
// Reading data

/** get current ammo amount (total) */
int32 AWeapon::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

/** get current ammo amount (clip) */
int32 AWeapon::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

/** get clip size */
int32 AWeapon::GetAmmoPerClip() const
{
	return WeaponConfig.AmmoPerClip;
}

/** get max ammo amount */
int32 AWeapon::GetMaxAmmo() const
{
	return WeaponConfig.MaxAmmo;
}

/** get pawn owner */
AHelicopter* AWeapon::GetPawnOwner() const
{
	return MyPawn;
}

/** check if weapon has infinite ammo (include owner's cheats) */
bool AWeapon::HasInfiniteAmmo() const
{
	return WeaponConfig.bInfiniteAmmo;
}

/** check if weapon has infinite clip (include owner's cheats) */
bool AWeapon::HasInfiniteClip() const
{
	return WeaponConfig.bInfiniteClip;
}

/** set the weapon's owning pawn */
void AWeapon::SetOwningPawn(AHelicopter* NewOwner)
{
	if (MyPawn != NewOwner) {
		Instigator = NewOwner;
		MyPawn = NewOwner;

		// UE_LOG(LogHeliWeapon, Log, TEXT("AWeapon::SetOwningPawn(AHelicopter* NewOwner) --- > Weapon's owner was set, MyPawn will replicate"));

		// net owner for RPC calls
		SetOwner(NewOwner);
	}
}


/** weapon is being equipped by owner pawn */
void AWeapon::OnEquip()
{
	AttachMeshToPawn();

	DetermineWeaponState();
}

/** attaches weapon to pawn */
void AWeapon::AttachMeshToPawn()
{
	if (MyPawn && !bIsEquipped)
	{
		FName AttachPoint = MyPawn->GetCurrentWeaponAttachPoint();

		bool isAttachOk = Mesh1P->AttachToComponent(MyPawn->GetHeliMeshComponent(), FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
		// UE_LOG(LogHeliWeapon, Log, TEXT("AWeapon::AttachMeshToPawn(%s) ---> Weapon was Attached to HeliMesh."), *AttachPoint.ToString());

		bIsEquipped = true;
	}
}

/** unequip weapon from pawn */
void AWeapon::OnUnEquip()
{

	if (HasAuthority())
	{
		// UE_LOG(LogHeliWeapon, Log, TEXT("AWeapon::OnUnEquip() ---> Set owning pawn to NULL"));
		SetOwningPawn(NULL);
	}

	if (IsAttachedToPawn())
	{
		StopFire();
		DetachMeshFromPawn();
		bIsEquipped = false;


		if (bPendingReload)
		{
			bPendingReload = false;

			GetWorldTimerManager().ClearTimer(TimerHandle_ReloadWeapon);
		}

		DetermineWeaponState();
	}
}

/** detaches weapon from pawn */
void AWeapon::DetachMeshFromPawn()
{
	if (MyPawn && bIsEquipped)
	{
		// Mesh1P->DetachFromParent(); // 4.12 or older version
		Mesh1P->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		bIsEquipped = false;
	}
}


/** check if weapon is already attached */
bool AWeapon::IsAttachedToPawn() const
{
	return bIsEquipped;
}


EWeaponState::Type AWeapon::GetCurrentState() const
{
	return CurrentState;
}


//////////////////////////////////////////////////////////////////////////
// Replication & effects

void AWeapon::OnRep_MyPawn()
{
	if (MyPawn)
	{
		SetOwningPawn(MyPawn);
	}
}

void AWeapon::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}

void AWeapon::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload(true);
	}
	else
	{
		StopReload();
	}
}

// weapon sound
UAudioComponent* AWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && MyPawn)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, MyPawn->GetRootComponent());
	}

	return AC;
}

// Cosmetics - FX of weapon firing effect, sound and camera shaking
void AWeapon::SimulateWeaponFire()
{
	// muzzle particle FX
	if (MuzzleFX)
	{
		{
			MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Mesh1P, WeaponConfig.MuzzleAttachPoint);
		}
	}

	// sound
	{
		FireAC = PlayWeaponSound(FireLoopSound);
	}

	// camera shake on firing
	AHeliPlayerController* heliPlayerController = (MyPawn != nullptr) ? Cast<AHeliPlayerController>(MyPawn->Controller) : nullptr;
	if (heliPlayerController && heliPlayerController->IsLocalController())
	{
		if (FireCameraShake && MyPawn->IsFirstPersonView())
		{
			heliPlayerController->ClientPlayCameraShake(FireCameraShake, 1);
		}
	}

}

void AWeapon::StopSimulatingWeaponFire()
{

	if (MuzzlePSC)
	{
		MuzzlePSC->DeactivateSystem();
		MuzzlePSC = nullptr;
	}

	if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = nullptr;

		PlayWeaponSound(FireFinishSound);
	}
}

// replication
void AWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, MyPawn);

	DOREPLIFETIME_CONDITION(AWeapon, CurrentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeapon, CurrentAmmoInClip, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AWeapon, bPendingReload, COND_SkipOwner);

}


//////////////////////////////////////////////////////////////////////////
// Control

/** check if weapon can fire */
bool AWeapon::CanFire() const
{
	bool bCanFire = MyPawn && MyPawn->CanFire();
	bool bStateOKToFire = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return ((bCanFire == true) && (bStateOKToFire == true));
}

/** check if weapon can be reloaded */
bool AWeapon::CanReload() const
{
	bool bCanReload = (!MyPawn || MyPawn->CanReload());
	bool bGotAmmo = (CurrentAmmoInClip < WeaponConfig.AmmoPerClip) && (CurrentAmmo - CurrentAmmoInClip > 0 || HasInfiniteClip());
	bool bStateOKToReload = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return ((bCanReload == true) && (bGotAmmo == true) && (bStateOKToReload == true));
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage

/** update weapon state */
void AWeapon::SetWeaponState(EWeaponState::Type NewState)
{
	const EWeaponState::Type PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

/** determine current weapon state */
void AWeapon::DetermineWeaponState()
{	
	EWeaponState::Type NewState = EWeaponState::Idle;

	if (bIsEquipped)
	{
		if (bPendingReload)
		{
			if (CanReload() == false)
			{
				NewState = CurrentState;
			}
			else
			{
				NewState = EWeaponState::Reloading;
			}
		}
		else if ((bPendingReload == false) && (bWantsToFire == true) && (CanFire() == true))
		{
			NewState = EWeaponState::Firing;			
		}
	}

	SetWeaponState(NewState);
}


void AWeapon::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
		LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AWeapon::HandleFiring, (LastFireTime + WeaponConfig.TimeBetweenShots - GameTime), false);
	}
	else
	{
		HandleFiring();
	}
}

void AWeapon::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;
}

//////////////////////////////////////////////////////////////////////////
// Input

/** [local + server] start weapon fire */
void AWeapon::StartFire()
{	
	if (Role < ROLE_Authority)
	{
		ServerStartFire();		
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

void AWeapon::StopFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

void AWeapon::StartReload(bool bFromReplication)
{
	if (!bFromReplication && Role < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		GetWorldTimerManager().SetTimer(TimerHandle_StopReload, this, &AWeapon::StopReload, ReloadDuration, false);
		if (Role == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_ReloadWeapon, this, &AWeapon::ReloadWeapon, FMath::Max(0.1f, ReloadDuration - 0.1f), false);
		}

		PlayWeaponSound(ReloadSound);
	}
}

void AWeapon::StopReload()
{
	if (CurrentState == EWeaponState::Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
	}
}

bool AWeapon::ServerStartFire_Validate()
{
	return true;
}

void AWeapon::ServerStartFire_Implementation()
{
	StartFire();
}

bool AWeapon::ServerStopFire_Validate()
{
	return true;
}

void AWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

bool AWeapon::ServerStartReload_Validate()
{
	return true;
}

void AWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

bool AWeapon::ServerStopReload_Validate()
{
	return true;
}

void AWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

void AWeapon::ClientStartReload_Implementation()
{
	StartReload();
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage

FHitResult AWeapon::WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const
{
	FCollisionQueryParams TraceParams(TEXT("WeaponTrace"), true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;
	
	// actors to ignore
	TArray<TWeakObjectPtr<AActor>> IgnoredActors;
	IgnoredActors.Add(Cast<AActor>(this));
	IgnoredActors.Add(GetPawnOwner());
	TraceParams.AddIgnoredActors(IgnoredActors);

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceFrom, TraceTo, COLLISION_WEAPON, TraceParams);

	return Hit;
}

void AWeapon::UseAmmo()
{
	if (!HasInfiniteAmmo())
	{
		CurrentAmmoInClip--;
	}

	if (!HasInfiniteAmmo() && !HasInfiniteClip())
	{
		CurrentAmmo--;
	}
}

void AWeapon::HandleFiring()
{
	if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			FireWeapon();

			UseAmmo();

			// update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (MyPawn && MyPawn->IsLocallyControlled())
	{
		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		// local client will notify server
		if (Role < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		// reload after firing last round
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReload();
		}

		// setup refire timer
		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AWeapon::HandleFiring, WeaponConfig.TimeBetweenShots, false);
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}

bool AWeapon::ServerHandleFiring_Validate()
{
	return true;
}

void AWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

	HandleFiring();

	if (bShouldUpdateAmmo)
	{
		// update ammo
		UseAmmo();

		// update firing FX on remote clients
		BurstCounter++;
	}
}


void AWeapon::ReloadWeapon()
{
	int32 ClipDelta = FMath::Min(WeaponConfig.AmmoPerClip - CurrentAmmoInClip, CurrentAmmo - CurrentAmmoInClip);

	if (HasInfiniteClip())
	{
		ClipDelta = WeaponConfig.AmmoPerClip - CurrentAmmoInClip;
	}

	if (ClipDelta > 0)
	{
		CurrentAmmoInClip += ClipDelta;
	}

	if (HasInfiniteClip())
	{
		CurrentAmmo = FMath::Max(CurrentAmmoInClip, CurrentAmmo);
	}

	// TODO(andrey): play reload sound
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

FVector AWeapon::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	AHeliPlayerController* PC = MyPawn ? Cast<AHeliPlayerController>(MyPawn->Controller) : nullptr;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		// use player's camera
		FRotator UnusedRot;
		PC->GetPlayerViewPoint(OutStartTrace, UnusedRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * ((MyPawn->GetActorLocation() - OutStartTrace) | AimDir);		
	}

	return OutStartTrace;
}

FVector AWeapon::GetAimFromViewpoint() const
{
	AHeliPlayerController* const PlayerController = MyPawn ? Cast<AHeliPlayerController>(MyPawn->GetController()) : nullptr;
	FVector FinalAim = FVector::ZeroVector;
	
	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}

	return FinalAim;
}

void AWeapon::GetAimViewpoint(FVector& out_Location, FVector& out_Rotation) const
{
	AHeliPlayerController* const PlayerController = MyPawn ? Cast<AHeliPlayerController>(MyPawn->GetController()) : nullptr;
	FVector FinalAim = FVector::ZeroVector;

	if (PlayerController)
	{
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(out_Location, CamRot);
		out_Rotation = CamRot.Vector();
	}

}




/** get the muzzle location of the weapon */
FVector AWeapon::GetMuzzleLocation() const
{
	return Mesh1P->GetSocketLocation(WeaponConfig.MuzzleAttachPoint);
}

/** get direction of weapon's muzzle */
FVector AWeapon::GetMuzzleDirection() const
{
	return Mesh1P->GetSocketRotation(WeaponConfig.MuzzleAttachPoint).Vector();
}

bool AWeapon::IsPrimaryWeapon() 
{
	return WeaponConfig.bPrimaryWeapon;
}