// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "Helicopter.h"
#include "HeliGame.h"
#include "HeliMovementComponent.h"
#include "HeliPlayerController.h"
#include "Weapon.h"
#include "HeliDamageType.h"
#include "HeliHud.h"
#include "HeliGameMode.h"
#include "HeliProjectile.h"
#include "HeliPlayerState.h"
#include "HealthBarUserWidget.h"
#include "HeliGameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Curves/CurveFloat.h"
#include "Sound/SoundCue.h"
#include "Public/TimerManager.h"
#include "Public/Engine.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"





AHelicopter::AHelicopter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	bReplicateMovement = true;

	// Create static mesh component, this is the main mesh
	HeliMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeliMeshComponent"));
	RootComponent = HeliMeshComponent;

	// physics
	HeliMeshComponent->SetSimulatePhysics(true);
	HeliMeshComponent->SetLinearDamping(0.1f);
	HeliMeshComponent->SetAngularDamping(1.f);
	HeliMeshComponent->SetEnableGravity(true);

	// collisions
	HeliMeshComponent->SetCollisionProfileName("HeliMeshComponent");
	HeliMeshComponent->SetCollisionObjectType(COLLISION_HELICOPTER);
	HeliMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeliMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	HeliMeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	HeliMeshComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	HeliMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	HeliMeshComponent->SetCollisionResponseToChannel(COLLISION_HELICOPTER, ECR_Block);
	HeliMeshComponent->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);
	HeliMeshComponent->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	HeliMeshComponent->bGenerateOverlapEvents = false;
	HeliMeshComponent->SetNotifyRigidBodyCollision(true);
	OnCrashImpactDelegate.BindUFunction(this, "OnCrashImpact");
	HeliMeshComponent->OnComponentHit.Add(OnCrashImpactDelegate);

	// movement component
	HeliMovementComponent = CreateDefaultSubobject<UHeliMovementComponent>(TEXT("HeliMovementComponent"));
	HeliMovementComponent->SetUpdatedComponent(HeliMeshComponent);
	HeliMovementComponent->SetNetAddressable();
	HeliMovementComponent->SetIsReplicated(true);
	HeliMovementComponent->SetActive(false);

	MouseSensitivity = 1.f;
	KeyboardSensitivity = 1.f;
	InvertedAim = 1;


	// camera
	SpringArmFirstPerson = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmFirstPerson"));
	SpringArmFirstPerson->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	SpringArmFirstPerson->bEnableCameraLag = true;
	SpringArmFirstPerson->bEnableCameraRotationLag = true;
	SpringArmFirstPerson->CameraLagMaxDistance = 6.f;
	SpringArmFirstPerson->CameraLagSpeed = 60.f;
	SpringArmFirstPerson->CameraRotationLagSpeed = 60.f;

	SpringArmThirdPerson = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmThirdPerson"));
	SpringArmThirdPerson->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	SpringArmThirdPerson->bInheritPitch = false;
	SpringArmThirdPerson->bInheritYaw = true;
	SpringArmThirdPerson->bInheritRoll = false;
	SpringArmThirdPerson->bEnableCameraLag = true;
	SpringArmThirdPerson->bEnableCameraRotationLag = true;
	SpringArmThirdPerson->CameraLagMaxDistance = 1.f;
	SpringArmThirdPerson->CameraLagSpeed = 10.f;
	SpringArmThirdPerson->CameraRotationLagSpeed = 10.f;
	SpringArmThirdPerson->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->AttachToComponent(SpringArmFirstPerson, FAttachmentTransformRules::KeepRelativeTransform);



	// rotors animation and sound
	// min and max pitch for rotor sound
	MaxRotorPitch = 1.11f;
	MinRotorPitch = 0.88f;

	// max speed of the main and tail rotors
	RotorMaxSpeed = 50.f;
	// max time to wait to apply rotation onto the rotors
	MaxTimeRotorAnimation = 0.03f;
	// create static mesh component for the main rotor
	MainRotorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainRotorMeshComp"));
	MainRotorMeshComponent->AttachToComponent(HeliMeshComponent, FAttachmentTransformRules::KeepRelativeTransform, MainRotorAttachSocketName);
	MainRotorMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MainRotorMeshComponent->SetSimulatePhysics(false);


	TailRotorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailRotorMeshComp"));
	TailRotorMeshComponent->AttachToComponent(HeliMeshComponent, FAttachmentTransformRules::KeepRelativeTransform, TailRotorAttachSocketName);
	TailRotorMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TailRotorMeshComponent->SetSimulatePhysics(false);

	// weapon
	// naming primary and secondary weapon attach
	PrimaryWeaponAttachPoint = TEXT("PrimaryWeaponAttachPoint");

	CurrentWeapon = nullptr;


	// HUD Throttle
	// throttle power starts at 55% and minimum at 10%
	BaseThrottle = Throttle = 55;
	MinThrottle = 10;
	// update time for the throttle display info
	RefreshThrottleTime = 0.1f;
	// minimum throttle increasing and decreasing step
	MinThrottleIncreasingDecreasingStep = 5.f;
	isThrottleReleased = true;

	// Health and repairing settings
	MaxHealth = 100.f;
	HealthRegenRate = 0.5f;
	LastHealedTime = 0.0f;
	MinRestoreHealthValue = 1.f;
	RepairVelocityThreshould = 100.f;

	// crash impact settings
	RestoreControlsDelay = 2.f;
	CrashImpactDamageThreshold = 0.05f;

	// health bar
	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidgetComponent"));
	HealthBarWidgetComponent->AttachToComponent(HeliMeshComponent, FAttachmentTransformRules::KeepRelativeTransform, HealthBarSocketName);

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnDelay = 1.5f;
}

/*
6-DoF Physics based movements
*/

void AHelicopter::MousePitch(float Value)
{
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed() && Value != 0.f)
	{
		UHeliMovementComponent* MovementComponent = Cast<UHeliMovementComponent>(GetMovementComponent());
		if (MovementComponent)
		{
			MovementComponent->AddPitch(Value*MouseSensitivity*InvertedAim);
		}
	}
}

void AHelicopter::MouseYaw(float Value)
{
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed() && Value != 0.f)
	{
		UHeliMovementComponent* MovementComponent = Cast<UHeliMovementComponent>(GetMovementComponent());
		if (MovementComponent)
		{
			MovementComponent->AddYaw(Value*MouseSensitivity);
		}
	}
}

void AHelicopter::MouseRoll(float Value)
{
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed() && Value != 0.f)
	{
		UHeliMovementComponent* MovementComponent = Cast<UHeliMovementComponent>(GetMovementComponent());
		if (MovementComponent)
		{
			MovementComponent->AddRoll(Value*MouseSensitivity);
		}
	}
}

void AHelicopter::KeyboardPitch(float Value)
{
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed() && Value != 0.f)
	{
		UHeliMovementComponent* MovementComponent = Cast<UHeliMovementComponent>(GetMovementComponent());
		if (MovementComponent)
		{
			MovementComponent->AddPitch(Value*KeyboardSensitivity);
		}
	}
}

void AHelicopter::KeyboardYaw(float Value)
{
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed() && Value != 0.f)
	{
		UHeliMovementComponent* MovementComponent = Cast<UHeliMovementComponent>(GetMovementComponent());
		if (MovementComponent)
		{
			MovementComponent->AddYaw(Value*KeyboardSensitivity);
		}
	}
}

void AHelicopter::KeyboardRoll(float Value)
{
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed() && Value != 0.f)
	{
		UHeliMovementComponent* MovementComponent = Cast<UHeliMovementComponent>(GetMovementComponent());
		if (MovementComponent)
		{
			MovementComponent->AddRoll(Value*KeyboardSensitivity);
		}
	}
}

void AHelicopter::Thrust(float Value)
{
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed() && Value != 0.f)
	{
		UHeliMovementComponent* MovementComponent = Cast<UHeliMovementComponent>(GetMovementComponent());
		if (MovementComponent)
		{
			MovementComponent->AddThrust(Value);
		}
	}
}


void AHelicopter::EnableFirstPersonViewpoint()
{
	if (!SpringArmFirstPerson->IsActive()) {
		SpringArmThirdPerson->Deactivate();

		SpringArmFirstPerson->Activate(false);
		Camera->AttachToComponent(SpringArmFirstPerson, FAttachmentTransformRules::KeepRelativeTransform);
	}
}

void AHelicopter::EnableThirdPersonViewpoint()
{
	if (!SpringArmThirdPerson->IsActive()) {
		SpringArmFirstPerson->Deactivate();

		SpringArmThirdPerson->Activate(false);
		Camera->AttachToComponent(SpringArmThirdPerson, FAttachmentTransformRules::KeepRelativeTransform);
	}
}

void AHelicopter::SwitchCameraViewpoint()
{
	if (SpringArmFirstPerson->IsActive())
	{
		EnableThirdPersonViewpoint();
		DisableFirstPersonHud();
	}
	else {
		EnableFirstPersonViewpoint();
		EnableFirstPersonHud();
	}
}

UStaticMeshComponent * AHelicopter::GetHeliMeshComponent()
{
	return HeliMeshComponent;
}

void AHelicopter::SpawnDefaultPrimaryWeaponAndEquip()
{
	// it is always safe certifying this will only execute for ROLE Authority
	if (Role < ROLE_Authority)
	{
		return;
	}

	if (GetCurrentWeaponEquiped() == nullptr)
	{
		AWeapon* PrimaryWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultPrimaryWeaponToSpawn);
		EquipWeapon(PrimaryWeapon);
	}
}

void AHelicopter::DebugSomething()
{	
	UE_LOG(LogTemp, Warning, TEXT("AHelicopter::DebugSomething - %s - %s Team %d"), *GetName(), *PlayerName.ToString(), GetTeamNumber());
	if (HealthBarWidgetComponent)
	{
		UHealthBarUserWidget* HealthBarUserWidget = Cast<UHealthBarUserWidget>(HealthBarWidgetComponent->GetUserWidgetObject());
		if (HealthBarUserWidget)
		{
			HealthBarUserWidget->SetCurrentColor(FLinearColor(1.f, 1.f, 0.f, 0.4f));
		}
	}
	
}

void AHelicopter::OnReloadWeapon()
{
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->StartReload();
		}
	}
}

void AHelicopter::OnStartFire()
{
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		StartWeaponFire();
	}
}

void AHelicopter::OnStopFire()
{
	StopWeaponFire();
}

void AHelicopter::ThrottleUpInput()
{
	UHeliMovementComponent* MovementComponent = Cast<UHeliMovementComponent>(GetMovementComponent());
	if (MovementComponent && !MovementComponent->IsActive())
	{
		return;
	}

	isThrottleReleased = false;

	GetWorld()->GetTimerManager().SetTimer(ThrottleDisplayTimerHandle, this, &AHelicopter::UpdatesThrottleForDisplayingAdd, RefreshThrottleTime, true);

	if (HeliAC)
	{
		HeliAC->SetPitchMultiplier(MaxRotorPitch);
	}
}

void AHelicopter::ThrottleDownInput()
{
	UHeliMovementComponent* MovementComponent = Cast<UHeliMovementComponent>(GetMovementComponent());
	if (MovementComponent && !MovementComponent->IsActive())
	{
		return;
	}

	isThrottleReleased = false;

	GetWorld()->GetTimerManager().SetTimer(ThrottleDisplayTimerHandle, this, &AHelicopter::UpdatesThrottleForDisplayingSub, RefreshThrottleTime, true);

	if (HeliAC)
	{
		HeliAC->SetPitchMultiplier(MinRotorPitch);
	}
}

void AHelicopter::ThrottleReleased()
{

	isThrottleReleased = true;

	GetWorldTimerManager().ClearTimer(ThrottleDisplayTimerHandle);

	if (Throttle > BaseThrottle)
	{
		GetWorld()->GetTimerManager().SetTimer(ThrottleDisplayTimerHandle, this, &AHelicopter::UpdatesThrottleForDisplayingSub, RefreshThrottleTime, true);
	}
	else if (Throttle < BaseThrottle)
	{
		GetWorld()->GetTimerManager().SetTimer(ThrottleDisplayTimerHandle, this, &AHelicopter::UpdatesThrottleForDisplayingAdd, RefreshThrottleTime, true);
	}

	if (HeliAC)
	{
		HeliAC->SetPitchMultiplier(1.0f);
	}
}


/***************************************************************************************
*                                Helicopter Animation and Sound                        *
****************************************************************************************/

/** updates Throttle property for displaying in hud, will be called by timer function*/
void AHelicopter::UpdatesThrottleForDisplayingAdd()
{
	Throttle = Throttle + MinThrottleIncreasingDecreasingStep;

	if ((Throttle == BaseThrottle && isThrottleReleased) || (Throttle == 100))
	{
		GetWorldTimerManager().ClearTimer(ThrottleDisplayTimerHandle);
	}
}

/** updates Throttle property for displaying in hud, will be called by timer function, subtracts throttle*/
void AHelicopter::UpdatesThrottleForDisplayingSub()
{
	Throttle = Throttle - MinThrottleIncreasingDecreasingStep;

	if ((Throttle == BaseThrottle && isThrottleReleased) || (Throttle == MinThrottle))
	{
		GetWorldTimerManager().ClearTimer(ThrottleDisplayTimerHandle);
	}
}

// play some sound of the heli
UAudioComponent*  AHelicopter::PlayHeliSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, this->GetRootComponent());
	}

	return AC;
}

float AHelicopter::GetThrottle()
{
	return Throttle;
}

// Animation for the main and tail rotors
void AHelicopter::ApplyRotationOnRotors()
{
	if (MainRotorMeshComponent && TailRotorMeshComponent) {
		// add yaw rotation for the main rotor
		MainRotorMeshComponent->AddLocalRotation(FRotator(0, RotorMaxSpeed, 0));

		// add pitch rotation for the tail rotor
		TailRotorMeshComponent->AddLocalRotation(FRotator(RotorMaxSpeed, 0, 0));
	}
}

void AHelicopter::DisableFirstPersonHud()
{
	// remove HeliHudWidget from viewport
	AHeliPlayerController* MyPc = Cast<AHeliPlayerController>(Controller);
	if (MyPc)
	{
		AHeliHud* MyHud = Cast<AHeliHud>(MyPc->GetHUD());
		if (MyHud)
		{
			MyHud->DisableFirstPersonHud();
		}
	}
}

void AHelicopter::EnableFirstPersonHud()
{
	AHeliPlayerController* MyPc = Cast<AHeliPlayerController>(Controller);
	if (MyPc)
	{
		AHeliHud* MyHud = Cast<AHeliHud>(MyPc->GetHUD());
		if (MyHud)
		{
			MyHud->EnableFirstPersonHud();
		}
	}

	if (SpringArmThirdPerson && SpringArmThirdPerson->IsActive())
	{
		SpringArmThirdPerson->Deactivate();
		EnableFirstPersonViewpoint();
	}
}

bool AHelicopter::IsFirstPersonView()
{
	return SpringArmFirstPerson->IsActive();
}

/***************************************************************************************
*                                       Heli weapon logic                              *
****************************************************************************************/
void AHelicopter::StartWeaponFire()
{
	if (!bWantsToFire)
	{
		bWantsToFire = true;
		if (CurrentWeapon)
		{
			CurrentWeapon->StartFire();
		}
	}
}

void AHelicopter::StopWeaponFire()
{
	if (bWantsToFire)
	{
		bWantsToFire = false;
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
	}
}

void AHelicopter::OnRep_Weapon(AWeapon* LastWeapon)
{
	SetWeapon(CurrentWeapon, LastWeapon);
}

void AHelicopter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
	{
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
	else
	{
		PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get(), LastTakeHitInfo.bKilled);
	}
}

bool AHelicopter::Server_EquipWeapon_Validate(AWeapon* NewWeapon)
{
	return true;
}

void AHelicopter::Server_EquipWeapon_Implementation(AWeapon* NewWeapon)
{
	EquipWeapon(NewWeapon);
}
/**
* [server + local] equips Primary weapon
*
* @param Weapon	Weapon to equip
*/
void AHelicopter::EquipWeapon(AWeapon* NewWeapon)
{
	if (NewWeapon)
	{
		if (Role == ROLE_Authority)
		{
			SetWeapon(NewWeapon, CurrentWeapon);
		}
		else
		{
			// UE_LOG(LogHeliWeapon, Log, TEXT("AHelicopter::EquipWeapon(AWeapon*) --- > Client will call the Server to equipe weapon"));
			Server_EquipWeapon(NewWeapon);
		}
	}
}

void AHelicopter::SetWeapon(class AWeapon* NewWeapon, class AWeapon* LastWeapon)
{
	AWeapon* LocalLastWeapon = nullptr;
	if (LastWeapon)
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}

	// CurrentWeapon will replicate
	CurrentWeapon = NewWeapon;
	// UE_LOG(LogHeliWeapon, Log, TEXT("AHelicopter::SetWeapon(...) --- > CurrentWeapon was set, CurrentWeapon will replicate."));

	if (NewWeapon)
	{
		NewWeapon->SetOwningPawn(this);	// Make sure weapon's MyPawn is pointing back to us. During replication, we can't guarantee APawn::CurrentWeapon will rep after AWeapon::MyPawn!
		NewWeapon->OnEquip();

		if (NewWeapon->IsPrimaryWeapon()) {
			bPrimaryWeaponEquiped = true;
		}
	}
}

bool AHelicopter::IsAlive() const
{
	return Health > 0;
}

/** check if pawn can fire weapon */
bool AHelicopter::CanFire() const
{
	return IsAlive();
}

/** check if pawn can reload weapon */
bool AHelicopter::CanReload() const
{
	return true;
}

/** remove all weapons attached to the pawn*/
void AHelicopter::RemoveWeapons()
{
	if (HasAuthority())
	{
		// primary weapon
		if (CurrentWeapon)
		{
			CurrentWeapon->OnUnEquip();
			CurrentWeapon->Destroy();
			bPrimaryWeaponEquiped = false;
		}
	}

}

/** get Primary weapon attach point */
FName AHelicopter::GetPrimaryWeaponAttachPoint() const
{
	return PrimaryWeaponAttachPoint;
}

FName AHelicopter::GetCurrentWeaponAttachPoint() const
{
	// TODO(andrey): logic to provide current weapon attach point, for now just return the primary weapon attach point
	return PrimaryWeaponAttachPoint;
}

AWeapon* AHelicopter::GetCurrentWeaponEquiped()
{
	return CurrentWeapon;
}

/************************************************************************/
/*                          Heli Damage & Death                         */
/************************************************************************/

void AHelicopter::Suicide()
{
	KilledBy(this);
}

void AHelicopter::KilledBy(APawn* EventInstigator)
{
	if (Role == ROLE_Authority && !bIsDying)
	{
		AController* Killer = NULL;
		if (EventInstigator != NULL)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = NULL;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, NULL);
	}
}

float AHelicopter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	if (Health <= 0.f)
	{
		return 0.f;
	}


	/* Modify damage based on game type rules */
	AHeliGameMode* MyGameMode = Cast<AHeliGameMode>(GetWorld()->GetAuthGameMode());
	Damage = MyGameMode ? MyGameMode->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : Damage;

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		// decrease its health
		Health -= ActualDamage;

		// score hit for the causer
		AController* const MySelf = Controller ? Controller : Cast<AController>(GetOwner());
		MyGameMode->ScoreHit(EventInstigator, MySelf, ActualDamage);


		if (Health <= 0)
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
		}
		else
		{
			/* Shorthand for - if x != null pick1 else pick2 */
			APawn* Pawn = EventInstigator ? EventInstigator->GetPawn() : nullptr;
			PlayHit(ActualDamage, DamageEvent, Pawn, DamageCauser, false);
		}
	}

	return ActualDamage;
}


bool AHelicopter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	/* Check if character is already dying, destroyed or if we have authority */
	if (bIsDying ||
		IsPendingKill() ||
		Role != ROLE_Authority ||
		GetWorld()->GetAuthGameMode<AGameMode>() == NULL ||
		GetWorld()->GetAuthGameMode<AGameMode>()->GetMatchState() == MatchState::LeavingMap)
	{
		return false;
	}

	return true;
}

bool AHelicopter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	Health = FMath::Min(0.0f, Health);

	/* Fallback to default DamageType if none is specified */
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	/* Notify the gamemode we got killed for scoring and game over state */
	AController* const KilledPlayer = Controller ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<AHeliGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);


	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, DamageCauser);
	return true;
}

void AHelicopter::OnDeath(float KillingDamage, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser)
{
	if (bIsDying)
	{
		return;
	}

	bIsDying = true;
	Health = 0.0f;
	//client will take authoritative control
	bTearOff = true;
			
	// turn off rotors anim
	GetWorldTimerManager().ClearTimer(RotorAnimTimerHandle);
	// turn sound off
	if (HeliAC) {
		HeliAC->Stop();
	}
	// hide meshes on game
	HeliMeshComponent->SetVisibility(false);
	MainRotorMeshComponent->SetVisibility(false);
	TailRotorMeshComponent->SetVisibility(false);
	// disable movement component
	if (HeliMovementComponent)
	{
		HeliMovementComponent->SetActive(false);
	}
	//disable collisions on mesh
	HeliMeshComponent->SetSimulatePhysics(false);
	HeliMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);	

	RemoveWeapons();	
	RemoveHealthWidget();	

	// detaching from controller will make game mode to call RestartPlayer
	DetachFromControllerPendingDestroy();
	
	// play sound and FX for death
	PlayHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);

	//GEngine->AddOnScreenDebugMessage(2, 15.0f, FColor::Green, FString::Printf(TEXT("%s"), *LogNetRole()) );

	AHeliPlayerController* heliPlayerController = Cast<AHeliPlayerController>(Controller);
	if (heliPlayerController)
	{
		//GEngine->AddOnScreenDebugMessage(2, 15.0f, FColor::Red, FString::Printf(TEXT("%s"), *LogNetRole()));

		// remove inputs
		heliPlayerController->SetIgnoreMoveInput(true);
		// don't allow game actions
		heliPlayerController->SetAllowGameActions(false);
		// show scoreboard
		AHeliHud* HeliHUD = Cast<AHeliHud>(heliPlayerController->GetHUD());
		if (HeliHUD)
		{
			HeliHUD->DisableFirstPersonHud();
			HeliHUD->HideInGameOptionsMenu();
			HeliHUD->HideInGameMenu();

			HeliHUD->ShowScoreboard();
		}					
	}	
}

void AHelicopter::PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser, bool bKilled)
{
	if (Role == ROLE_Authority)
	{
		ReplicateHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser, bKilled);
	}


	if (GetNetMode() != NM_DedicatedServer)
	{
		if (bKilled && DeathExplosionSound && DeathExplosionFX)
		{
			// sound
			UGameplayStatics::SpawnSoundAttached(DeathExplosionSound, RootComponent, NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true);
			// FX
			UGameplayStatics::SpawnEmitterAtLocation(this, DeathExplosionFX, GetActorLocation(), GetActorRotation());
		}
		else if (SoundTakeHit)
		{
			// sound
			UGameplayStatics::SpawnSoundAttached(SoundTakeHit, RootComponent, NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true);
		}
	}

	// crosshair hit notify
	if (PawnInstigator && PawnInstigator != this && PawnInstigator->IsLocallyControlled())
	{
		AHeliPlayerController* InstigatorPC = Cast<AHeliPlayerController>(PawnInstigator->Controller);
		AHeliHud* InstigatorHUD = InstigatorPC ? Cast<AHeliHud>(InstigatorPC->GetHUD()) : NULL;
		if (InstigatorHUD)
		{
			InstigatorHUD->NotifyEnemyHit();
		}
	}

	// TODO(andrey): hud notify hit for taking damage
}


void AHelicopter::ReplicateHit(float DamageTaken, struct FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if (PawnInstigator == LastTakeHitInfo.PawnInstigator.Get() && LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass)
	{
		// Same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, ignore it
			return;
		}

		DamageTaken += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = DamageTaken;
	LastTakeHitInfo.PawnInstigator = Cast<AHelicopter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();
}

void AHelicopter::OnCrashImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	AHeliProjectile* HeliProjectile = Cast<AHeliProjectile>(OtherActor);
	if (HeliProjectile)
	{
		return;
	}

	// TODO(andrey): 
	// 1 - notify on HUD that controls are damaged
	// 2 - impact crash sound

	float Damage = ComputeCrashImpactDamage();

	if (Damage >= (MaxHealth * CrashImpactDamageThreshold))
	{
		CrashControls();

		Server_CrashImpactTakeDamage(Damage);

		//UE_LOG(LogTemp, Error, TEXT("Damage = %f"), Damage);
	}
}

void AHelicopter::CrashControls()
{
	UHeliMovementComponent* MovementComponent = Cast<UHeliMovementComponent>(GetMovementComponent());
	if (MovementComponent)
	{
		MovementComponent->SetActive(false);

		// restore controls back to normal after some time
		if (!TimerHandle_RestoreControls.IsValid())
		{
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_RestoreControls, this, &AHelicopter::RestoreControlsAfterCrashImpact, RestoreControlsDelay, false);
		}
	}

	if (HeliAC)
	{
		HeliAC->SetPitchMultiplier(MinRotorPitch);
	}
}

void AHelicopter::RestoreControlsAfterCrashImpact()
{
	UHeliMovementComponent* MovementComponent = Cast<UHeliMovementComponent>(GetMovementComponent());
	if (MovementComponent)
	{
		MovementComponent->SetActive(true);

		GetWorldTimerManager().ClearTimer(TimerHandle_RestoreControls);
	}

	if (HeliAC)
	{
		HeliAC->SetPitchMultiplier(1.f);
	}
}

float AHelicopter::ComputeCrashImpactDamage()
{
	float ActualDamage = 0.f;

	if (CrashImpactDamageCurve)
	{
		// get damage based on impact momentum
		float Speed = GetVelocity().Size() / 100.f;
		float Mass = HeliMeshComponent->GetMass();
		float ImpactMomentum = Speed * Mass;
		float BaseDamage = MaxHealth * CrashImpactDamageCurve->GetFloatValue(ImpactMomentum);

		// increase it with tilt/inclination angles		
		float TiltAngle = FMath::Abs(FVector::DotProduct(FVector::UpVector, HeliMeshComponent->GetRightVector()));
		float InclinationAngle = FMath::Abs(FVector::DotProduct(FVector::UpVector, HeliMeshComponent->GetForwardVector()));
		float TiltInclinationWeight = FMath::Clamp((TiltAngle + InclinationAngle), 0.f, 1.f);
		ActualDamage = BaseDamage * TiltInclinationWeight;

		//UE_LOG(LogTemp, Warning, TEXT("Inclination = %f   Tilt = %f   TiltInclinationWeight = %f   ImpactMomentum = %f   BaseDamage = %f   ActualDamage = %f"), InclinationAngle, TiltAngle, TiltInclinationWeight, ImpactMomentum, BaseDamage, ActualDamage);
	}


	return ActualDamage;
}

bool AHelicopter::Server_CrashImpactTakeDamage_Validate(float Damage)
{
	return true;
}

void AHelicopter::Server_CrashImpactTakeDamage_Implementation(float Damage)
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


/***************************************************************************************
*                                       Health Regeneration                            *
****************************************************************************************/
void AHelicopter::HandleRepairing()
{
	if (IsLocallyControlled() && !(GetVelocity().GetAbsMax() > RepairVelocityThreshould))
	{
		// local client will notify server
		if (Role < ROLE_Authority)
		{
			Server_AddHealth(MinRestoreHealthValue);
		}
		else
		{
			Health = (Health + MinRestoreHealthValue) <= MaxHealth ? (Health + MinRestoreHealthValue) : MaxHealth;
		}


		GetWorldTimerManager().SetTimer(TimerHandle_RestoreHealth, this, &AHelicopter::HandleRepairing, HealthRegenRate, false);
	}

	LastHealedTime = GetWorld()->GetTimeSeconds();
}

bool AHelicopter::Server_AddHealth_Validate(float Value)
{
	return true;
}

void AHelicopter::Server_AddHealth_Implementation(float Value)
{
	Health = (Health + MinRestoreHealthValue) <= MaxHealth ? (Health + MinRestoreHealthValue) : MaxHealth;
}

void AHelicopter::OnStartRepair()
{
	const float GameTime = GetWorld()->GetTimeSeconds();

	if (LastHealedTime > 0 && HealthRegenRate > 0.0f &&
		LastHealedTime + HealthRegenRate > GameTime)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_RestoreHealth, this, &AHelicopter::HandleRepairing, (LastHealedTime + HealthRegenRate - GameTime), false);
	}
	else
	{
		HandleRepairing();
	}
}

void AHelicopter::OnStopRepair()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_RestoreHealth);
}

float AHelicopter::GetHealthPercent()
{
	return Health / MaxHealth;
}

void AHelicopter::SetupPlayerInfoWidget()
{	
	if (HealthBarWidgetComponent)
	{				
		// only remote clients shows the health bar and player name
		if (!IsLocallyControlled())
		{
			UHealthBarUserWidget* HealthBarUserWidget = Cast<UHealthBarUserWidget>(HealthBarWidgetComponent->GetUserWidgetObject());
			if (HealthBarUserWidget)
			{
				HealthBarUserWidget->SetOwningPawn(this);
				HealthBarUserWidget->SetupWidget();
			}
		} 
		else
		{
			RemoveHealthWidget();			
		}
	}
}

FName AHelicopter::GetPlayerName()
{
	return PlayerName;
}

void AHelicopter::SetPlayerName(FName NewPlayerName)
{
	PlayerName = NewPlayerName;
}

int32 AHelicopter::GetTeamNumber()
{
	return TeamNumber;
}

void AHelicopter::SetTeamNumber(int32 NewTeamNumber)
{
	TeamNumber = NewTeamNumber;	
}

void AHelicopter::SetPlayerInfo(FName NewPlayerName, int32 NewTeamNumber)
{
	if (HasAuthority())
	{
		PlayerName = NewPlayerName;
		TeamNumber = NewTeamNumber;
	}
}

void AHelicopter::OnRep_PlayerInfo()
{
	//UE_LOG(LogTemp, Display, TEXT("AHelicopter::OnRep_PlayerInfo ~ %s Team %d"), *PlayerName.ToString(), GetTeamNumber());

	SetupPlayerInfoWidget();
}

void AHelicopter::UpdatePlayerInfo(FName playerName, int32 teamNumber)
{
	Server_UpdatePlayerInfo(playerName, teamNumber);	
}

bool AHelicopter::Server_UpdatePlayerInfo_Validate(FName NewPlayerName, int32 NewTeamNumber)
{
	return true;
}

void AHelicopter::Server_UpdatePlayerInfo_Implementation(FName NewPlayerName, int32 NewTeamNumber)
{
	SetPlayerInfo(NewPlayerName, NewTeamNumber);
}

void AHelicopter::RemoveHealthWidget()
{	
	if (HealthBarWidgetComponent)
	{
		UHealthBarUserWidget* healthBarUserWidget = Cast<UHealthBarUserWidget>(HealthBarWidgetComponent->GetUserWidgetObject());
		if (healthBarUserWidget)
		{
			healthBarUserWidget->SetOwningPawn(nullptr);
			healthBarUserWidget->Destruct();
		}

		// FIXME(andrey): is it the right way of removing widgets that were not directly added to UMG
		HealthBarWidgetComponent->SetWidget(nullptr);
		HealthBarWidgetComponent->SetActive(false);
	}
}

void AHelicopter::SetMouseSensitivity(float inMouseSensitivity)
{
	MouseSensitivity = inMouseSensitivity;
}

void AHelicopter::SetKeyboardSensitivity(float inKeyboardSensitivity)
{
	KeyboardSensitivity = inKeyboardSensitivity;
}

void AHelicopter::SetInvertedAim(int32 inInvertedAim)
{
	InvertedAim = FMath::Clamp(inInvertedAim, -1, 1);
}



/*
	Overrides from APawn
*/

/**
* Called when this Pawn is possessed. Only called on the server (or in standalone).
* @param C The controller possessing this pawn
*/
void AHelicopter::PossessedBy(class AController* InController)
{
	//UE_LOG(LogTemp, Display, TEXT("AHelicopter::PossessedBy ~ %s %s Role %d and RemoteRole %d"), InController->IsLocalPlayerController() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *InController->GetName(), (int32)InController->Role, (int32)InController->GetRemoteRole());

	Super::PossessedBy(InController);

	// [server] as soon as PlayerState is assigned, set team colors of this pawn for local player
}

void AHelicopter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [client] as soon as PlayerState is assigned, set team colors of this pawn for local player
	if (PlayerState)
	{
		AHeliPlayerState* heliPlayerState = Cast<AHeliPlayerState>(PlayerState);
		if (heliPlayerState)
		{			
			//UE_LOG(LogTemp, Warning, TEXT("AHelicopter::OnRep_PlayerState ~ %s %s Role %d and RemoteRole %d - Team %d"), IsLocallyControlled() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *GetName(), (int32)Role, (int32)GetRemoteRole(), heliPlayerState->GetTeamNumber());			
			SetupPlayerInfoWidget();
		}
	}
}

void AHelicopter::InitHelicopter() 
{
	// enable collision
	if (HeliMeshComponent && !HeliMeshComponent->IsCollisionEnabled())
	{
		HeliMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeliMeshComponent->SetSimulatePhysics(true);
		
	}
	
	// enable movements
	if (HeliMovementComponent && !HeliMovementComponent->IsActive())
	{
		HeliMovementComponent->SetActive(true);
	}

	// set timer for rotors animation
	if (!RotorAnimTimerHandle.IsValid())
		GetWorld()->GetTimerManager().SetTimer(RotorAnimTimerHandle, this, &AHelicopter::ApplyRotationOnRotors, MaxTimeRotorAnimation, true);

	// start sound
	if ((HeliAC == nullptr) || (HeliAC && !HeliAC->IsPlaying()))
		HeliAC = PlayHeliSound(MainRotorLoopSound);

	EnableFirstPersonHud();

	AHeliPlayerController* heliPlayerController = Cast<AHeliPlayerController>(Controller);
	if (heliPlayerController)
	{
		// set inputs
		heliPlayerController->SetIgnoreMoveInput(false);

		// allow game actions
		heliPlayerController->SetAllowGameActions(true);

		// hide scoreboard
		AHeliHud* heliHud = Cast<AHeliHud>(heliPlayerController->GetHUD());
		if (heliHud)
		{
			heliHud->HideScoreboard();
		}

		//UE_LOG(LogTemp, Warning, TEXT("AHelicopter::InitHelicopter ~ %s %s Role %d and RemoteRole %d with spawn location: %s"), heliPlayerController->IsLocalPlayerController() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *heliPlayerController->GetName(), (int32)heliPlayerController->Role, (int32)heliPlayerController->GetRemoteRole(), *heliPlayerController->GetSpawnLocation().ToCompactString());
	}

	// setup health bar
	SetupPlayerInfoWidget();
}

// Called to bind functionality to input
void AHelicopter::SetupPlayerInputComponent(class UInputComponent* HeliInputComponent)
{
	Super::SetupPlayerInputComponent(HeliInputComponent);

	// Set up gameplay key bindings
	check(HeliInputComponent);

	// axis binding

	InputComponent->BindAxis("Pitch", this, &AHelicopter::MousePitch);
	InputComponent->BindAxis("Yaw", this, &AHelicopter::MouseYaw);
	InputComponent->BindAxis("Roll", this, &AHelicopter::MouseRoll);

	InputComponent->BindAxis("PitchKB", this, &AHelicopter::KeyboardPitch);
	InputComponent->BindAxis("YawKB", this, &AHelicopter::KeyboardYaw);
	InputComponent->BindAxis("RollKB", this, &AHelicopter::KeyboardRoll);

	InputComponent->BindAxis("Thrust", this, &AHelicopter::Thrust);

	// actions binding

	// Switch camera view point
	InputComponent->BindAction("SwitchCam", IE_Pressed, this, &AHelicopter::SwitchCameraViewpoint);

	// Repair
	InputComponent->BindAction("Repair", IE_Pressed, this, &AHelicopter::OnStartRepair);
	InputComponent->BindAction("Repair", IE_Released, this, &AHelicopter::OnStopRepair);

	// Fire
	HeliInputComponent->BindAction("Fire", IE_Pressed, this, &AHelicopter::OnStartFire);
	HeliInputComponent->BindAction("Fire", IE_Released, this, &AHelicopter::OnStopFire);

	HeliInputComponent->BindAction("Reload", IE_Pressed, this, &AHelicopter::OnReloadWeapon);

	// ThrotlleUp (HUD)
	HeliInputComponent->BindAction("ThrottleUp", IE_Pressed, this, &AHelicopter::ThrottleUpInput);
	HeliInputComponent->BindAction("ThrottleUp", IE_Released, this, &AHelicopter::ThrottleReleased);
	// ThrotlleDown (HUD)
	HeliInputComponent->BindAction("ThrottleDown", IE_Pressed, this, &AHelicopter::ThrottleDownInput);
	HeliInputComponent->BindAction("ThrottleDown", IE_Released, this, &AHelicopter::ThrottleReleased);

	// DebugSomething
	HeliInputComponent->BindAction("DebugSomething", IE_Pressed, this, &AHelicopter::DebugSomething);
}


/** spawn inventory, setup initial variables */
void AHelicopter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (HasAuthority())
	{
		// equip Primary weapon, 
		SpawnDefaultPrimaryWeaponAndEquip();

		// set default health
		Health = MaxHealth;		
	}

	// setup user settings
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		MouseSensitivity = heliGameUserSettings->GetMouseSensitivity();
		KeyboardSensitivity = heliGameUserSettings->GetKeyboardSensitivity();
		InvertedAim = heliGameUserSettings->GetInvertedAim();
	}

	// setup health bar
	SetupPlayerInfoWidget();
}

// Called when the game starts or when spawned
void AHelicopter::BeginPlay()
{
	//UE_LOG(LogTemp, Display, TEXT("AHelicopter::BeginPlay - %f"), GetWorld()->GetRealTimeSeconds());

	Super::BeginPlay();	

	if (HeliMovementComponent)
	{
		HeliMovementComponent->SetActive(false);
	}
}

/** Tell client that the Pawn is begin restarted. Calls Restart(). */
void AHelicopter::PawnClientRestart()
{
	//UE_LOG(LogTemp, Display, TEXT("AHelicopter::PawnClientRestart - %f"), GetWorld()->GetRealTimeSeconds());

	Super::PawnClientRestart();
	// Use a local timer handle as we don't need to store it for later but we don't need to look for something to clear
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AHelicopter::InitHelicopter, SpawnDelay, false);
}

//							Replication List
void AHelicopter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AHelicopter, LastTakeHitInfo, COND_Custom);


	// everyone
	DOREPLIFETIME(AHelicopter, Health);
	DOREPLIFETIME(AHelicopter, CurrentWeapon);
	DOREPLIFETIME(AHelicopter, PlayerName);
	DOREPLIFETIME(AHelicopter, TeamNumber);
}

FString AHelicopter::LogNetRole()
{
	FString roles = FString::Printf(TEXT("Role: "));

	switch(Role)
	{
	case ROLE_MAX:
		roles += FString(TEXT("ROLE_MAX"));
		break;
	case ROLE_Authority:
		roles += FString(TEXT("ROLE_Authority"));
		break;
	case ROLE_AutonomousProxy:
		roles += FString(TEXT("ROLE_AutonomousProxy"));
		break;
	case ROLE_SimulatedProxy:
		roles += FString(TEXT("ROLE_SimulatedProxy"));
		break;
	case ROLE_None:
		roles += FString(TEXT("ROLE_None"));
		break;
	default:
		break;
	}

	roles += FString(TEXT(", RemoteRole: "));
	switch (GetRemoteRole())
	{
	case ROLE_MAX:
		roles += FString(TEXT("ROLE_MAX"));
		break;
	case ROLE_Authority:
		roles += FString(TEXT("ROLE_Authority"));
		break;
	case ROLE_AutonomousProxy:
		roles += FString(TEXT("ROLE_AutonomousProxy"));
		break;
	case ROLE_SimulatedProxy:
		roles += FString(TEXT("ROLE_SimulatedProxy"));
		break;
	case ROLE_None:
		roles += FString(TEXT("ROLE_None"));
		break;
	default:
		break;
	}

	UE_LOG(LogTemp, Warning, TEXT("%s %s"), *GetName(), *roles);
	return roles;
}