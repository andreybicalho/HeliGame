// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "Helicopter.h"
#include "HeliGame.h"
//#include "HeliMovementComponent.h"
#include "HeliMoveComp.h"
#include "HeliProjectile.h"
#include "HeliPlayerState.h"
#include "HeliGameUserSettings.h"

#include "Curves/CurveFloat.h"
#include "Sound/SoundCue.h"
#include "Public/TimerManager.h"
#include "Public/Engine.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"


AHelicopter::AHelicopter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{	
	bReplicateMovement = false; // disable movement replication since we are doing movement replication by ourselves

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
	HeliMovementComponent = CreateDefaultSubobject<UHeliMoveComp>(TEXT("HeliMovementComponent"));
	HeliMovementComponent->SetUpdatedComponent(MainStaticMeshComponent);
	HeliMovementComponent->SetNetAddressable();
	HeliMovementComponent->SetIsReplicated(true);
	HeliMovementComponent->SetActive(false);

	MouseSensitivity = 1.f;
	KeyboardSensitivity = 1.f;
	InvertedAim = 1;


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
	MainRotorMeshComponent->AttachToComponent(MainStaticMeshComponent, FAttachmentTransformRules::KeepRelativeTransform, MainRotorAttachSocketName);
	MainRotorMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MainRotorMeshComponent->SetSimulatePhysics(false);


	TailRotorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailRotorMeshComp"));
	TailRotorMeshComponent->AttachToComponent(MainStaticMeshComponent, FAttachmentTransformRules::KeepRelativeTransform, TailRotorAttachSocketName);
	TailRotorMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TailRotorMeshComponent->SetSimulatePhysics(false);


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
	CrashControlsOnImpactThreshold = 0.2f;	
}

/*
Overrides from APawn
*/

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

	// ThrotlleUp (HUD)
	HeliInputComponent->BindAction("ThrottleUp", IE_Pressed, this, &AHelicopter::ThrottleUpInput);
	HeliInputComponent->BindAction("ThrottleUp", IE_Released, this, &AHelicopter::ThrottleReleased);
	// ThrotlleDown (HUD)
	HeliInputComponent->BindAction("ThrottleDown", IE_Pressed, this, &AHelicopter::ThrottleDownInput);
	HeliInputComponent->BindAction("ThrottleDown", IE_Released, this, &AHelicopter::ThrottleReleased);

	// DebugSomething
	HeliInputComponent->BindAction("DebugSomething", IE_Pressed, this, &AHelicopter::DebugSomething);
}


void AHelicopter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// setup user settings
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		MouseSensitivity = heliGameUserSettings->GetMouseSensitivity();
		KeyboardSensitivity = heliGameUserSettings->GetKeyboardSensitivity();
		InvertedAim = heliGameUserSettings->GetInvertedAim();
	}
}

// Called when the game starts or when spawned
void AHelicopter::BeginPlay()
{
	//UE_LOG(LogTemp, Warning, TEXT("AHelicopter::BeginPlay ~ %s %s Role %d and RemoteRole %d - %d"), IsLocallyControlled() ? *FString::Printf(TEXT("Local")) : *FString::Printf(TEXT("Remote")), *GetName(), (int32)Role, (int32)GetRemoteRole(), GetWorld()->GetRealTimeSeconds());

	Super::BeginPlay();

	// Use a local timer handle as we don't need to store it for later but we don't need to look for something to clear
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AHelicopter::InitHelicopter, SpawnDelay, false);
}

/** Tell client that the Pawn is begin restarted. Calls Restart(). */
void AHelicopter::PawnClientRestart()
{
	//UE_LOG(LogTemp, Display, TEXT("AHelicopter::PawnClientRestart - %f"), GetWorld()->GetRealTimeSeconds());

	Super::PawnClientRestart();
}

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

/*
6-DoF Physics based movements
*/

void AHelicopter::MousePitch(float Value)
{
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed() && Value != 0.f)
	{
		UHeliMoveComp* MovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
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
		UHeliMoveComp* MovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
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
		UHeliMoveComp* MovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
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
		UHeliMoveComp* MovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
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
		UHeliMoveComp* MovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
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
		UHeliMoveComp* MovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
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
		UHeliMoveComp* MovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
		if (MovementComponent)
		{
			MovementComponent->AddThrust(Value);
		}
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

UStaticMeshComponent* AHelicopter::GetHeliMeshComponent()
{
	return MainStaticMeshComponent;
}

void AHelicopter::DebugSomething()
{	
	UE_LOG(LogTemp, Warning, TEXT("AHelicopter::DebugSomething - %s - %s Team %d"), *GetName(), *GetPlayerName().ToString(), GetTeamNumber());	
}


void AHelicopter::ThrottleUpInput()
{
	UHeliMoveComp* MovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
	if (MovementComponent && !MovementComponent->IsActive())
	{
		return;
	}

	isThrottleReleased = false;

	GetWorld()->GetTimerManager().SetTimer(ThrottleDisplayTimerHandle, this, &AHelicopter::UpdatesThrottleForDisplayingAdd, RefreshThrottleTime, true);

	if (MainAudioComponent)
	{
		MainAudioComponent->SetPitchMultiplier(MaxRotorPitch);
	}
}

void AHelicopter::ThrottleDownInput()
{
	UHeliMoveComp* MovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
	if (MovementComponent && !MovementComponent->IsActive())
	{
		return;
	}

	isThrottleReleased = false;

	GetWorld()->GetTimerManager().SetTimer(ThrottleDisplayTimerHandle, this, &AHelicopter::UpdatesThrottleForDisplayingSub, RefreshThrottleTime, true);

	if (MainAudioComponent)
	{
		MainAudioComponent->SetPitchMultiplier(MinRotorPitch);
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

	if (MainAudioComponent)
	{
		MainAudioComponent->SetPitchMultiplier(1.0f);
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



void AHelicopter::OnCrashImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
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

	if (Damage >= (MaxHealth * CrashControlsOnImpactThreshold))
	{
		CrashControls();
		
		// TODO(andrey): 
		// 1 - notify on HUD that controls are damaged
		// 2 - impact crash HARD sound		
	}
}

void AHelicopter::CrashControls()
{
	UHeliMoveComp* MovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
	if (MovementComponent)
	{
		MovementComponent->SetActive(false);

		// restore controls back to normal after some time
		if (!TimerHandle_RestoreControls.IsValid())
		{
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_RestoreControls, this, &AHelicopter::RestoreControlsAfterCrashImpact, RestoreControlsDelay, false);
		}
	}

	if (MainAudioComponent)
	{
		MainAudioComponent->SetPitchMultiplier(MinRotorPitch);
	}
}

void AHelicopter::RestoreControlsAfterCrashImpact()
{
	UHeliMoveComp* MovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
	if (MovementComponent)
	{
		MovementComponent->SetActive(true);

		GetWorldTimerManager().ClearTimer(TimerHandle_RestoreControls);
	}

	if (MainAudioComponent)
	{
		MainAudioComponent->SetPitchMultiplier(1.f);
	}
}

float AHelicopter::ComputeCrashImpactDamage()
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

void AHelicopter::SetNetworkSmoothingFactor(float inNetworkSmoothingFactor)
{
	UHeliMoveComp* heliMovementComponent = Cast<UHeliMoveComp>(GetMovementComponent());
	if (heliMovementComponent)
	{
		heliMovementComponent->SetNetworkSmoothingFactor(inNetworkSmoothingFactor);
	}
}

void AHelicopter::InitHelicopter() 
{
	// enable collision
	if (MainStaticMeshComponent)
	{
		MainStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		
		if (IsLocallyControlled())
		{
			MainStaticMeshComponent->SetSimulatePhysics(true);	
		}
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
	if ((MainAudioComponent == nullptr) || (MainAudioComponent && !MainAudioComponent->IsPlaying()))
		MainAudioComponent = PlaySound(MainLoopSound);

	EnableFirstPersonHud();

	AHeliPlayerController* heliPlayerController = Cast<AHeliPlayerController>(Controller);
	if (heliPlayerController)
	{
		// set inputs
		heliPlayerController->SetIgnoreMoveInput(false);

		// allow game actions
		heliPlayerController->SetAllowGameActions(true);

		// network smoothing factor
		SetNetworkSmoothingFactor(heliPlayerController->GetNetworkSmoothingFactor());
		

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

void AHelicopter::OnDeath(float KillingDamage, FDamageEvent const &DamageEvent, APawn *PawnInstigator, AActor *DamageCauser)
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
	if (MainAudioComponent)
	{
		MainAudioComponent->Stop();
	}
	// hide meshes on game
	MainStaticMeshComponent->SetVisibility(false);
	MainRotorMeshComponent->SetVisibility(false);
	TailRotorMeshComponent->SetVisibility(false);
	// disable movement component
	if (HeliMovementComponent)
	{
		HeliMovementComponent->SetActive(false);
	}
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

	AHeliPlayerController *heliPlayerController = Cast<AHeliPlayerController>(Controller);
	if (heliPlayerController)
	{
		//GEngine->AddOnScreenDebugMessage(2, 15.0f, FColor::Red, FString::Printf(TEXT("%s"), *LogNetRole()));

		// remove inputs
		heliPlayerController->SetIgnoreMoveInput(true);
		// don't allow game actions
		heliPlayerController->SetAllowGameActions(false);
		// show scoreboard
		AHeliHud *HeliHUD = Cast<AHeliHud>(heliPlayerController->GetHUD());
		if (HeliHUD)
		{
			HeliHUD->DisableFirstPersonHud();
			HeliHUD->HideInGameOptionsMenu();
			HeliHUD->HideInGameMenu();

			HeliHUD->ShowScoreboard();
		}
	}
}