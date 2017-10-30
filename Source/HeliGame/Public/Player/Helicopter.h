// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Helicopter.generated.h"

class UStaticMeshComponent;
class UCurveFloat;
class USoundCue;
class UAudioComponent;

USTRUCT()
struct FTakeHitData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float ActualDamage;

	UPROPERTY()
	UClass* DamageTypeClass;

	UPROPERTY()
	TWeakObjectPtr<class AHelicopter> PawnInstigator;

	UPROPERTY()
	TWeakObjectPtr<class AActor> DamageCauser;

	UPROPERTY()
	uint8 DamageEventClassID;

	UPROPERTY()
	bool bKilled;

private:

	UPROPERTY()
	uint8 EnsureReplicationByte;

	UPROPERTY()
	FDamageEvent GeneralDamageEvent;

	UPROPERTY()
	FPointDamageEvent PointDamageEvent;

	UPROPERTY()
	FRadialDamageEvent RadialDamageEvent;

public:
	FTakeHitData()
		: ActualDamage(0),
		DamageTypeClass(nullptr),
		PawnInstigator(nullptr),
		DamageCauser(nullptr),
		DamageEventClassID(0),
		bKilled(false),
		EnsureReplicationByte(0)
	{}

	FDamageEvent& GetDamageEvent()
	{
		switch (DamageEventClassID)
		{
		case FPointDamageEvent::ClassID:
			if (PointDamageEvent.DamageTypeClass == nullptr)
			{
				PointDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return PointDamageEvent;

		case FRadialDamageEvent::ClassID:
			if (RadialDamageEvent.DamageTypeClass == nullptr)
			{
				RadialDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return RadialDamageEvent;

		default:
			if (GeneralDamageEvent.DamageTypeClass == nullptr)
			{
				GeneralDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return GeneralDamageEvent;
		}
	}


	void SetDamageEvent(const FDamageEvent& DamageEvent)
	{
		DamageEventClassID = DamageEvent.GetTypeID();
		switch (DamageEventClassID)
		{
		case FPointDamageEvent::ClassID:
			PointDamageEvent = *((FPointDamageEvent const*)(&DamageEvent));
			break;
		case FRadialDamageEvent::ClassID:
			RadialDamageEvent = *((FRadialDamageEvent const*)(&DamageEvent));
			break;
		default:
			GeneralDamageEvent = DamageEvent;
		}
	}


	void EnsureReplication()
	{
		EnsureReplicationByte++;
	}
};

/**
 * 
 */
UCLASS()
class HELIGAME_API AHelicopter : public APawn
{
	GENERATED_BODY()
	
	/*
	*	Main Helicopter Mesh
	*/

	/** StaticMesh component that will be the visuals for our flying pawn */
	UPROPERTY(Category = "Mesh", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* HeliMeshComponent;


	/*
	*	Crash Impact
	*/

	FScriptDelegate OnCrashImpactDelegate;

	// AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep
	UFUNCTION()
	void OnCrashImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// Server should apply damage
	UFUNCTION(Reliable, Server, WithValidation, Category = "CrashImpact")
	void Server_CrashImpactTakeDamage(float Damage);

	/* Time handler to restore controls after crashing */
	FTimerHandle TimerHandle_RestoreControls;

	void RestoreControlsAfterCrashImpact();

	UPROPERTY(EditDefaultsOnly, Category = "CrashImpact", meta = (AllowPrivateAccess = "true"))
	float RestoreControlsDelay = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "CrashImpact", meta = (AllowPrivateAccess = "true"))
	float CrashImpactDamageThreshold = 0.1f;

	void CrashControls();

	float ComputeCrashImpactDamage();

	/* crash impact damage curve */
	UPROPERTY(Category = "6DoF", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* CrashImpactDamageCurve;

	/*
	*	Rotors mesh and Animation
	*/

	// apply rotation on the rotors meshes
	void ApplyRotationOnRotors();

	/* Handle to manage the timer for the rotor animation*/
	FTimerHandle RotorAnimTimerHandle;

	// max speed of the main and tail rotors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RotorAnimation", meta = (AllowPrivateAccess = "true"))
	float RotorMaxSpeed;

	// max time to wait to apply rotation onto the rotors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RotorAnimation", meta = (AllowPrivateAccess = "true"))
	float MaxTimeRotorAnimation;

	UPROPERTY(Category = "Mesh", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* MainRotorMeshComponent = nullptr;

	UPROPERTY(Category = "Mesh", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* TailRotorMeshComponent = nullptr;

	UPROPERTY(Category = "Mesh", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FName MainRotorAttachSocketName = TEXT("MainRotorSocket");

	UPROPERTY(Category = "Mesh", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FName TailRotorAttachSocketName = TEXT("TailRotorSocket");

	/*
		Movement Component
	*/

	UPROPERTY(Category = "MovementSettings", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UPawnMovementComponent* HeliMovementComponent = nullptr;

	// mouse sensitivity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings", meta = (AllowPrivateAccess = "true"))
	float MouseSensitivity = 1.f;

	// keyboard sensitivity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings", meta = (AllowPrivateAccess = "true"))
	float KeyboardSensitivity = 1.f;


	/*
		Camera
	*/

	/** Spring arm that will offset the first person viewpoint for the camera */
	UPROPERTY(Category = "Camera", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArmFirstPerson;

	/** Spring arm that will offset the third person viewpoint for the camera */
	UPROPERTY(Category = "Camera", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArmThirdPerson;

	/** Camera component */
	UPROPERTY(Category = "Camera", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* Camera;


	/*
	*	Weapons
	*/
	
	/** socket or bone name for attaching Primary weapon mesh */
	UPROPERTY(Category = "Weapon", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FName PrimaryWeaponAttachPoint;


	/*
	*	HUD
	*/

	/** float property to display throttle power in hud */
	float Throttle;

	// for using in widget blueprints
	UFUNCTION(BlueprintCallable, Category = "HUDInfo")
	float GetThrottle();

	// base throttle value to display in hud
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUDInfo", meta = (AllowPrivateAccess = "true"))
	float BaseThrottle;

	// minimum throttle value to display in hud
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUDInfo", meta = (AllowPrivateAccess = "true"))
	float MinThrottle;

	// update time for the throttle display info
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUDInfo", meta = (AllowPrivateAccess = "true"))
	float RefreshThrottleTime;

	// minimum throttle increasing and decreasing step
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUDInfo", meta = (AllowPrivateAccess = "true"))
	float MinThrottleIncreasingDecreasingStep;

	/* Handle to manage the timer for the hud display throttle info */
	FTimerHandle ThrottleDisplayTimerHandle;

	bool isThrottleReleased;

	/** updates Throttle property for displaying in hud, will be called by timer function, adds throttle*/
	void UpdatesThrottleForDisplayingAdd();
	/** updates Throttle property for displaying in hud, will be called by timer function, subtracts throttle*/
	void UpdatesThrottleForDisplayingSub();

	void DisableFirstPersonHud();

	void EnableFirstPersonHud();

	/*
	* Player Info (HUD)
	*/

	UPROPERTY(ReplicatedUsing = OnRep_PlayerInfo, Transient)
	int32 TeamNumber = -1;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerInfo, Transient)
	FName PlayerName = FName(TEXT("unknown"));

	UFUNCTION()
	void OnRep_PlayerInfo();

	FTimerHandle TimerHandle_PlayerState;	

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UpdatePlayerInfo(FName NewPlayerName, int32 NewTeamNumber);

	/*
	*	Sounds
	*/
	
	/** Main rotor loop sound cue */
	UPROPERTY(Category = "HelicopterSound", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	USoundCue* MainRotorLoopSound;
	
	/** Play Heli sounds */
	UAudioComponent*  PlayHeliSound(USoundCue* Sound);
	
	// heli audio component
	UPROPERTY(BlueprintReadWrite, Category = "HelicopterSound", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* HeliAC;

	// max pitch for rotor sound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HelicopterSound", meta = (AllowPrivateAccess = "true"))
	float MaxRotorPitch;
	// min pitch for rotor sound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HelicopterSound", meta = (AllowPrivateAccess = "true"))
	float MinRotorPitch;


	/*
		Health Regen
	*/
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "HealthSettings", meta = (AllowPrivateAccess = "true"))
	float MaxHealth;

	UFUNCTION(Reliable, Server, WithValidation, BlueprintCallable, Category = "RepairSettings")
	void Server_AddHealth(float Value);

	void HandleRepairing();

	UPROPERTY(EditDefaultsOnly, Category = "HealthSettings", meta = (AllowPrivateAccess = "true"))
	float MinRestoreHealthValue;

	FTimerHandle TimerHandle_RestoreHealth;

	UPROPERTY(EditDefaultsOnly, Category = "HealthSettings", meta = (AllowPrivateAccess = "true"))
	float HealthRegenRate;

	float LastHealedTime;

	UPROPERTY(EditDefaultsOnly, Category = "HealthSettings", meta = (AllowPrivateAccess = "true"))
	float RepairVelocityThreshould;		

	UPROPERTY(Category = "HealthSettings", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FName HealthBarSocketName = TEXT("HealthBarSocket");

	UPROPERTY(Category = "HealthSettings", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* HealthBarWidgetComponent;	
	
protected:
	/*
	*	Weapons
	*/

	/** current firing state */
	uint8 bWantsToFire : 1;

	/** [server] remove all weapons and destroy them */
	void RemoveWeapons();

	/* [server] spawn default primary weapon and equip it */
	void SpawnDefaultPrimaryWeaponAndEquip();

	/*
	*	Damaging & Death
	*/

	// Current health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "HealthSettings", meta = (AllowPrivateAccess = "true"))
	float Health;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* SoundTakeHit;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* DeathExplosionSound;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UParticleSystem* DeathExplosionFX;

	/* Holds hit data to replicate hits and death to clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_LastTakeHitInfo)
	struct FTakeHitData LastTakeHitInfo;

	UFUNCTION()
	void OnRep_LastTakeHitInfo();

	bool bIsDying;

	virtual void PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser, bool bKilled);

	void ReplicateHit(float DamageTaken, struct FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser, bool bKilled);



	/** Kill this pawn */
	virtual bool CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const;

	/** Returns True if the pawn can die in the current state */
	virtual bool Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser);

	virtual void OnDeath(float KillingDamage, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser);

public:
	AHelicopter(const FObjectInitializer& ObjectInitializer);

	UStaticMeshComponent* GetHeliMeshComponent();



	/*
	*	Action Inputs
	*/	
	/* controls */
	void MousePitch(float Value);
	void MouseYaw(float Value);
	void MouseRoll(float Value);
	void KeyboardPitch(float Value);
	void KeyboardYaw(float Value);
	void KeyboardRoll(float Value);
	void Thrust(float Value);
	
	void SwitchCameraViewpoint();
	void EnableFirstPersonViewpoint();
	void EnableThirdPersonViewpoint();
	
	void OnStartRepair();
	void OnStopRepair();

	void DebugSomething();

	void OnStartFire();
	void OnStopFire();

	void OnReloadWeapon();
	
	void ThrottleUpInput();
	void ThrottleDownInput();
	void ThrottleReleased();

	void SetMouseSensitivity(float inMouseSensitivity);
	void SetKeyboardSensitivity(float inKeyboardSensitivity);



	/*
	*       Weapons
	*/

	/** default weapon to spawn on begin play function */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeapon>  DefaultPrimaryWeaponToSpawn;	

	/** get Primary weapon attach point */
	FName GetPrimaryWeaponAttachPoint() const;

	/** get current weapon attach point */
	FName GetCurrentWeaponAttachPoint() const;

	bool bPrimaryWeaponEquiped;

	/** Current weapon */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Weapon)
	class AWeapon* CurrentWeapon;

	// actor can have a lot of weapons, but it can uses only one at a time
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AWeapon* GetCurrentWeaponEquiped();

	/* OnRep functions can use a parameter to hold the previous value of the variable. Very useful when you need to handle UnEquip etc. */
	UFUNCTION()
	void OnRep_Weapon(AWeapon* LastWeapon);

	/** set weapon */
	void SetWeapon(class AWeapon* NewWeapon, class AWeapon* LastWeapon);

	/**
	* [server + local] equips weapon from inventory
	*
	* @param Weapon	Weapon to equip
	*/
	void EquipWeapon(class AWeapon* NewWeapon);

	UFUNCTION(Reliable, Server, WithValidation)
	void Server_EquipWeapon(AWeapon* NewWeapon);

	// Weapon usage

	/** [local] starts weapon fire */
	void StartWeaponFire();

	/** [local] stops weapon fire */
	void StopWeaponFire();

	/** check if pawn can fire weapon */
	bool CanFire() const;

	/** check if pawn can reload weapon */
	bool CanReload() const;

	/** check if pawn is still alive */
	bool IsAlive() const;

	/* check if pawn is in first person view */
	bool IsFirstPersonView();
	
	/** Kill this pawn */
	virtual void KilledBy(class APawn* EventInstigator);

	/** Pawn suicide */
	virtual void Suicide();	


	/*
	* Player Info (HUD)
	*/

	void SetTeamNumber(int32 NewTeamNumber);

	int32 GetTeamNumber();

	FName GetPlayerName();

	void SetPlayerName(FName NewPlayerName);

	float GetHealthPercent();

	void SetPlayerInfo(FName NewPlayerName, int32 NewTeamNumber);

	void SetPlayerInfoFromPlayerState();

	void UpdatePlayerInfo(FName playerName, int32 teamNumber);

	void SetupPlayerInfoWidget();

	void RemoveHealthWidget();


	
/* overrides */
private:
	/** [server] perform PlayerState related setup */
	virtual void PossessedBy(class AController* C) override;

	/** [client] perform PlayerState related setup */
	virtual void OnRep_PlayerState() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/* Take damage & handle death */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

public:
	//virtual void Tick(float DeltaTime) override;	

	/** spawn inventory, setup initial variables */
	virtual void PostInitializeComponents() override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* HeliInputComponent) override;

	virtual UPawnMovementComponent* GetMovementComponent() const override { return HeliMovementComponent; }
	
	virtual void PawnClientRestart() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// TODO(andrey): remove
	void LogNetRole();
};
