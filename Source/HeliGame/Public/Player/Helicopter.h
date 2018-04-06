// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "HeliFighterVehicle.h"
#include "Helicopter.generated.h"

class UStaticMeshComponent;
class UCurveFloat;
class UAudioComponent;

/**
 * 
 */
UCLASS()
class HELIGAME_API AHelicopter : public AHeliFighterVehicle
{
	GENERATED_BODY()

private:
	/** [server] perform PlayerState related setup */
	virtual void PossessedBy(class AController *C) override;

	/** [client] perform PlayerState related setup */
	virtual void OnRep_PlayerState() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	/** spawn inventory, setup initial variables */
	virtual void PostInitializeComponents() override;
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PawnClientRestart() override;

	virtual UPawnMovementComponent *GetMovementComponent() const override { return HeliMovementComponent; }

private:
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
	float CrashControlsOnImpactThreshold = 0.2f;

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
	UStaticMeshComponent* MainRotorMeshComponent = nullptr;

	UPROPERTY(Category = "Mesh", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TailRotorMeshComponent = nullptr;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings", meta = (AllowPrivateAccess = "true"))
	int32 InvertedAim = 1;


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
	
protected:
	void InitHelicopter();

	void OnDeath(float KillingDamage, FDamageEvent const &DamageEvent, APawn *PawnInstigator, AActor *DamageCauser) override;

public:
	AHelicopter(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Mesh")
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
	
	void OnStartRepair();
	void OnStopRepair();

	void DebugSomething();
	
	void ThrottleUpInput();
	void ThrottleDownInput();
	void ThrottleReleased();

	void SetMouseSensitivity(float inMouseSensitivity);
	void SetKeyboardSensitivity(float inKeyboardSensitivity);
	void SetInvertedAim(int32 inInvertedAim);

	void SetNetworkSmoothingFactor(float inNetworkSmoothingFactor);
};
