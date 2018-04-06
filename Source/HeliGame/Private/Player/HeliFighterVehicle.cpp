// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliFighterVehicle.h"
#include "HeliPlayerController.h"
#include "Weapon.h"
#include "HeliDamageType.h"
#include "HealthBarUserWidget.h"
#include "HeliGameMode.h"
#include "HeliHud.h" // TODO(andrey): remover acoplamento do HUD, deixar hud somente nas classes derivadas desta

#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Sound/SoundCue.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AHeliFighterVehicle::AHeliFighterVehicle(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	// Create static mesh component, this is the main mesh
	MainStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainStaticMeshComponent"));
	RootComponent = MainStaticMeshComponent;

	// camera
	SpringArmFirstPerson = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmFirstPerson"));
	SpringArmFirstPerson->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	SpringArmFirstPerson->bEnableCameraLag = true;
	SpringArmFirstPerson->bEnableCameraRotationLag = true;
	SpringArmFirstPerson->CameraLagMaxDistance = 6.f;
	SpringArmFirstPerson->CameraLagSpeed = 60.f;
	SpringArmFirstPerson->CameraRotationLagSpeed = 60.f;

	SpringArmThirdPerson = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmThirdPerson"));
	SpringArmThirdPerson->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);	
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

	// weapon
	// naming primary and secondary weapon attach
	PrimaryWeaponAttachPoint = TEXT("PrimaryWeaponAttachPoint");

	CurrentWeapon = nullptr;

	// health bar
	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidgetComponent"));	
	HealthBarWidgetComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform, HealthBarSocketName);
	MaxHealth = 100.f;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnDelay = 1.0f;
}

/** spawn inventory, setup initial variables */
void AHeliFighterVehicle::PostInitializeComponents()
{
	//UE_LOG(LogTemp, Display, TEXT("AHeliFighterVehicle::PostInitializeComponents - %f"), GetWorld()->GetRealTimeSeconds());
	Super::PostInitializeComponents();

	if (HasAuthority())
	{
		// equip Primary weapon,
		SpawnDefaultPrimaryWeaponAndEquip();

		// set default health
		Health = MaxHealth;
	}

	// setup health bar
	SetupPlayerInfoWidget();
}

// Called to bind functionality to input
void AHeliFighterVehicle::SetupPlayerInputComponent(class UInputComponent *HeliInputComponent)
{
	Super::SetupPlayerInputComponent(HeliInputComponent);

	// Set up gameplay key bindings
	check(HeliInputComponent);

	HeliInputComponent->BindAction("Fire", IE_Pressed, this, &AHelicopter::OnStartFire);
	HeliInputComponent->BindAction("Fire", IE_Released, this, &AHelicopter::OnStopFire);
	HeliInputComponent->BindAction("Reload", IE_Pressed, this, &AHelicopter::OnReloadWeapon);
}

//							Replication List
void AHeliFighterVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AHeliFighterVehicle, LastTakeHitInfo, COND_Custom);

	// everyone
	DOREPLIFETIME(AHeliFighterVehicle, Health);
	DOREPLIFETIME(AHeliFighterVehicle, CurrentWeapon);
	DOREPLIFETIME(AHeliFighterVehicle, PlayerName);
	DOREPLIFETIME(AHeliFighterVehicle, TeamNumber);
}



void AHeliFighterVehicle::SpawnDefaultPrimaryWeaponAndEquip()
{
	// it is always safe certifying this will only execute for ROLE Authority
	if (Role < ROLE_Authority)
	{
		return;
	}

	if (GetCurrentWeaponEquiped() == nullptr)
	{
		AWeapon *PrimaryWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultPrimaryWeaponToSpawn);
		EquipWeapon(PrimaryWeapon);
	}
}

void AHeliFighterVehicle::OnReloadWeapon()
{
	AHeliPlayerController *MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->StartReload();
		}
	}
}

void AHeliFighterVehicle::OnStartFire()
{
	AHeliPlayerController *MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		StartWeaponFire();
	}
}

void AHeliFighterVehicle::OnStopFire()
{
	StopWeaponFire();
}

bool AHeliFighterVehicle::IsFirstPersonView()
{
	return SpringArmFirstPerson->IsActive();
}

void AHeliFighterVehicle::EnableFirstPersonViewpoint()
{
	if (!SpringArmFirstPerson->IsActive())
	{
		SpringArmThirdPerson->Deactivate();

		SpringArmFirstPerson->Activate(false);
		Camera->AttachToComponent(SpringArmFirstPerson, FAttachmentTransformRules::KeepRelativeTransform);
	}
}

void AHeliFighterVehicle::EnableThirdPersonViewpoint()
{
	if (!SpringArmThirdPerson->IsActive())
	{
		SpringArmFirstPerson->Deactivate();

		SpringArmThirdPerson->Activate(false);
		Camera->AttachToComponent(SpringArmThirdPerson, FAttachmentTransformRules::KeepRelativeTransform);
	}
}

/***************************************************************************************
*                                       Heli weapon logic                              *
****************************************************************************************/
void AHeliFighterVehicle::StartWeaponFire()
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

void AHeliFighterVehicle::StopWeaponFire()
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

void AHeliFighterVehicle::OnRep_Weapon(AWeapon *LastWeapon)
{
	SetWeapon(CurrentWeapon, LastWeapon);
}

void AHeliFighterVehicle::OnRep_LastTakeHitInfo()
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

bool AHeliFighterVehicle::Server_EquipWeapon_Validate(AWeapon *NewWeapon)
{
	return true;
}

void AHeliFighterVehicle::Server_EquipWeapon_Implementation(AWeapon *NewWeapon)
{
	EquipWeapon(NewWeapon);
}
/**
* [server + local] equips Primary weapon
*
* @param Weapon	Weapon to equip
*/
void AHeliFighterVehicle::EquipWeapon(AWeapon *NewWeapon)
{
	if (NewWeapon)
	{
		if (Role == ROLE_Authority)
		{
			SetWeapon(NewWeapon, CurrentWeapon);
		}
		else
		{
			// UE_LOG(LogHeliWeapon, Log, TEXT("AHeliFighterVehicle::EquipWeapon(AWeapon*) --- > Client will call the Server to equipe weapon"));
			Server_EquipWeapon(NewWeapon);
		}
	}
}

void AHeliFighterVehicle::SetWeapon(class AWeapon *NewWeapon, class AWeapon *LastWeapon)
{
	AWeapon *LocalLastWeapon = nullptr;
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
	// UE_LOG(LogHeliWeapon, Log, TEXT("AHeliFighterVehicle::SetWeapon(...) --- > CurrentWeapon was set, CurrentWeapon will replicate."));

	if (NewWeapon)
	{
		NewWeapon->SetOwningPawn(this); // Make sure weapon's MyPawn is pointing back to us. During replication, we can't guarantee APawn::CurrentWeapon will rep after AWeapon::MyPawn!
		NewWeapon->OnEquip();

		if (NewWeapon->IsPrimaryWeapon())
		{
			bPrimaryWeaponEquiped = true;
		}
	}
}

bool AHeliFighterVehicle::IsAlive() const
{
	return Health > 0;
}

/** check if pawn can fire weapon */
bool AHeliFighterVehicle::CanFire() const
{
	return IsAlive();
}

/** check if pawn can reload weapon */
bool AHeliFighterVehicle::CanReload() const
{
	return true;
}

/** remove all weapons attached to the pawn*/
void AHeliFighterVehicle::RemoveWeapons()
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
FName AHeliFighterVehicle::GetPrimaryWeaponAttachPoint() const
{
	return PrimaryWeaponAttachPoint;
}

FName AHeliFighterVehicle::GetCurrentWeaponAttachPoint() const
{
	return PrimaryWeaponAttachPoint;
}

AWeapon *AHeliFighterVehicle::GetCurrentWeaponEquiped()
{
	return CurrentWeapon;
}

/************************************************************************/
/*                          Heli Damage & Death                         */
/************************************************************************/

void AHeliFighterVehicle::Suicide()
{
	KilledBy(this);
}

void AHeliFighterVehicle::KilledBy(APawn *EventInstigator)
{
	if (Role == ROLE_Authority && !bIsDying)
	{
		AController *Killer = nullptr;
		if (EventInstigator != nullptr)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = nullptr;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, nullptr);
	}
}

float AHeliFighterVehicle::TakeDamage(float Damage, struct FDamageEvent const &DamageEvent, class AController *EventInstigator, class AActor *DamageCauser)
{
	//UE_LOG(LogTemp, Display, TEXT("AHeliFighterVehicle::TakeDamage ~ Health = %f, Damage Taken = %f"), Health, Damage);
	if (Health <= 0.f)
	{
		return 0.f;
	}

	/* Modify damage based on game type rules */
	AHeliGameMode *MyGameMode = Cast<AHeliGameMode>(GetWorld()->GetAuthGameMode());
	Damage = MyGameMode ? MyGameMode->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : Damage;

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		// decrease its health
		Health -= ActualDamage;

		// score hit for the causer
		AController *const MySelf = Controller ? Controller : Cast<AController>(GetOwner());
		MyGameMode->ScoreHit(EventInstigator, MySelf, ActualDamage);

		if (Health <= 0)
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
		}
		else
		{
			/* Shorthand for - if x != null pick1 else pick2 */
			APawn *Pawn = EventInstigator ? EventInstigator->GetPawn() : nullptr;
			PlayHit(ActualDamage, DamageEvent, Pawn, DamageCauser, false);
		}
	}

	return ActualDamage;
}

bool AHeliFighterVehicle::CanDie(float KillingDamage, FDamageEvent const &DamageEvent, AController *Killer, AActor *DamageCauser) const
{
	/* Check if character is already dying, destroyed or if we have authority */
	if (bIsDying ||
		IsPendingKill() ||
		Role != ROLE_Authority ||
		GetWorld()->GetAuthGameMode<AGameMode>() == nullptr ||
		GetWorld()->GetAuthGameMode<AGameMode>()->GetMatchState() == MatchState::LeavingMap)
	{
		return false;
	}

	return true;
}

bool AHeliFighterVehicle::Die(float KillingDamage, FDamageEvent const &DamageEvent, AController *Killer, AActor *DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	Health = FMath::Min(0.0f, Health);

	/* Fallback to default DamageType if none is specified */
	UDamageType const *const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	/* Notify the gamemode we got killed for scoring and game over state */
	AController *const KilledPlayer = Controller ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<AHeliGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : nullptr, DamageCauser);
	return true;
}

void AHeliFighterVehicle::OnDeath(float KillingDamage, FDamageEvent const &DamageEvent, APawn *PawnInstigator, AActor *DamageCauser)
{
}


void AHeliFighterVehicle::PlayHit(float DamageTaken, struct FDamageEvent const &DamageEvent, APawn *PawnInstigator, AActor *DamageCauser, bool bKilled)
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
		AHeliPlayerController *InstigatorPC = Cast<AHeliPlayerController>(PawnInstigator->Controller);
		AHeliHud *InstigatorHUD = InstigatorPC ? Cast<AHeliHud>(InstigatorPC->GetHUD()) : NULL;
		if (InstigatorHUD)
		{
			InstigatorHUD->NotifyEnemyHit();
		}
	}

	// TODO(andrey): hud notify hit for taking damage
}

void AHeliFighterVehicle::ReplicateHit(float DamageTaken, struct FDamageEvent const &DamageEvent, APawn *PawnInstigator, AActor *DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const &LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
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
	LastTakeHitInfo.PawnInstigator = Cast<AHeliFighterVehicle>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();
}

float AHeliFighterVehicle::GetHealthPercent()
{
	return Health / MaxHealth;
}

void AHeliFighterVehicle::SetupPlayerInfoWidget()
{
	if (HealthBarWidgetComponent)
	{
		// only remote clients, or bots, shows the health bar and player name
		if (!IsLocallyControlled() || !IsPlayerControlled())
		{
			UHealthBarUserWidget *HealthBarUserWidget = Cast<UHealthBarUserWidget>(HealthBarWidgetComponent->GetUserWidgetObject());
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

FName AHeliFighterVehicle::GetPlayerName()
{
	return PlayerName;
}

void AHeliFighterVehicle::SetPlayerName(FName NewPlayerName)
{
	PlayerName = NewPlayerName;
}

int32 AHeliFighterVehicle::GetTeamNumber()
{
	return TeamNumber;
}

void AHeliFighterVehicle::SetTeamNumber(int32 NewTeamNumber)
{
	TeamNumber = NewTeamNumber;
}

void AHeliFighterVehicle::SetPlayerInfo(FName NewPlayerName, int32 NewTeamNumber)
{
	if (HasAuthority())
	{
		PlayerName = NewPlayerName;
		TeamNumber = NewTeamNumber;
	}
}

void AHeliFighterVehicle::OnRep_PlayerInfo()
{
	//UE_LOG(LogTemp, Display, TEXT("AHeliFighterVehicle::OnRep_PlayerInfo ~ %s Team %d"), *PlayerName.ToString(), GetTeamNumber());

	SetupPlayerInfoWidget();
}

void AHeliFighterVehicle::UpdatePlayerInfo(FName playerName, int32 teamNumber)
{
	Server_UpdatePlayerInfo(playerName, teamNumber);
}

bool AHeliFighterVehicle::Server_UpdatePlayerInfo_Validate(FName NewPlayerName, int32 NewTeamNumber)
{
	return true;
}

void AHeliFighterVehicle::Server_UpdatePlayerInfo_Implementation(FName NewPlayerName, int32 NewTeamNumber)
{
	SetPlayerInfo(NewPlayerName, NewTeamNumber);
}

void AHeliFighterVehicle::RemoveHealthWidget()
{
	if (HealthBarWidgetComponent)
	{
		UHealthBarUserWidget *healthBarUserWidget = Cast<UHealthBarUserWidget>(HealthBarWidgetComponent->GetUserWidgetObject());
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

UAudioComponent* AHeliFighterVehicle::PlaySound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, this->GetRootComponent());
	}

	return AC;
}