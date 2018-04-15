// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

/** This weapon system is based on the ShooterGame and SurvivalGame
* ShooterGame doc: https://docs.unrealengine.com/latest/INT/Resources/SampleGames/ShooterGame/index.html
* SurvivalGame doc: https://wiki.unrealengine.com/Survival_sample_game
* Thank you guys you're awesome!
*/

#pragma once

#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class UStaticMeshComponent;
class UArrowComponent;
class AHeliFighterVehicle;
class UAudioComponent;
class USoundCue;
class UParticleSystemComponent;
class UCameraShake;

namespace EWeaponState
{
	enum Type
	{
		Idle,
		Firing,
		Reloading,
		Equipping,
	};
}

USTRUCT()
struct FWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** infinite ammo for reloads */
	UPROPERTY(EditDefaultsOnly, Category = "Ammo")
	bool bInfiniteAmmo;

	/** infinite ammo in clip, no reload required */
	UPROPERTY(EditDefaultsOnly, Category = "Ammo")
	bool bInfiniteClip;

	/** max ammo */
	UPROPERTY(EditDefaultsOnly, Category = "Ammo")
	int32 MaxAmmo = 400;

	/** clip size */
	UPROPERTY(EditDefaultsOnly, Category = "Ammo")
	int32 AmmoPerClip = 100;

	/** initial clips */
	UPROPERTY(EditDefaultsOnly, Category = "Ammo")
	int32 InitialClips;

	/** time between two consecutive shots */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponStat")
	float TimeBetweenShots = 0.05f;

	/** failsafe reload duration if weapon doesn't have any animation for it */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponStat")
	float NoAnimReloadDuration = 1.f;

	/** name of bone/socket for muzzle in weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	FName MuzzleAttachPoint;

	/** type of weapon in the game (primary or secondary) */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponStat")
	bool bPrimaryWeapon;


	/** defaults */
	FWeaponData()
	{
		bInfiniteAmmo = false;
		bInfiniteClip = false;
		MaxAmmo = 100;
		AmmoPerClip = 20;
		InitialClips = 4;
		TimeBetweenShots = 0.1f;
		NoAnimReloadDuration = 1.0f;
		MuzzleAttachPoint = TEXT("MuzzleAttachPoint");
		bPrimaryWeapon = true;

	}
};


UCLASS()
class HELIGAME_API AWeapon : public AActor
{
	GENERATED_BODY()

	/** arrow to show weapon directions on blueprint viewport*/
	UPROPERTY(Category = "Weapon", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* WeaponArrow;

	/** weapon mesh: 1st person view */
	UPROPERTY(Category = "Weapon", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* Mesh1P;

public:
	// Sets default values for this actor's properties
	AWeapon(const FObjectInitializer& ObjectInitializer);

	/** perform initial setup */
	virtual void PostInitializeComponents() override;

	virtual void Destroyed() override;

	//////////////////////////////////////////////////////////////////////////
	// Input

	/** [local + server] start weapon fire */
	void StartFire();

	/** [local + server] stop weapon fire */
	void StopFire();

	/** [all] start weapon reload */
	virtual void StartReload(bool bFromReplication = false);

	/** [local + server] interrupt weapon reload */
	void StopReload();

	/** [server] performs actual reload */
	void ReloadWeapon();

	/** trigger reload from server */
	UFUNCTION(reliable, client)
	void ClientStartReload();

	//////////////////////////////////////////////////////////////////////////
	// Control

	/** check if weapon can fire */
	bool CanFire() const;

	/** check if weapon can be reloaded */
	bool CanReload() const;


	//////////////////////////////////////////////////////////////////////////
	// Ammo

	/** consume a bullet */
	void UseAmmo();

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** weapon is being equipped by owner pawn */
	void OnEquip();

	/** check if weapon is already attached */
	bool IsAttachedToPawn() const;

	void OnUnEquip();

	//////////////////////////////////////////////////////////////////////////
	// Reading data

	/** get current weapon state */
	EWeaponState::Type GetCurrentState() const;

	/** get current ammo amount (total) */
	UFUNCTION(BlueprintCallable, Category = "WeaponConfig")
	int32 GetCurrentAmmo() const;

	/** get current ammo amount (clip) */
	UFUNCTION(BlueprintCallable, Category = "WeaponConfig")
	int32 GetCurrentAmmoInClip() const;

	/** get clip size */
	UFUNCTION(BlueprintCallable, Category = "WeaponConfig")
	int32 GetAmmoPerClip() const;

	/** get max ammo amount */
	int32 GetMaxAmmo() const;

	/** get pawn owner */
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
	class AHeliFighterVehicle* GetPawnOwner() const;

	/** check if weapon has infinite ammo (include owner's cheats) */
	UFUNCTION(BlueprintCallable, Category = "WeaponConfig")
	bool HasInfiniteAmmo() const;

	/** check if weapon has infinite clip (include owner's cheats) */
	UFUNCTION(BlueprintCallable, Category = "WeaponConfig")
	bool HasInfiniteClip() const;

	/** set the weapon's owning pawn */
	void SetOwningPawn (AHeliFighterVehicle* NewOwner);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	bool IsPrimaryWeapon();

protected:
	/** pawn owner */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_MyPawn)
	class AHeliFighterVehicle* MyPawn;

	/** weapon data */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	FWeaponData WeaponConfig;

	/** current weapon state */
	EWeaponState::Type CurrentState;

	/** reload duration time*/
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ReloadDuration;

	/** time of last successful weapon fire */
	float LastFireTime;

	/** is weapon currently equipped? */
	uint32 bIsEquipped : 1;

	/** is weapon fire active? */
	uint32 bWantsToFire : 1;

	/** is reloading? */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Reload)
	uint32 bPendingReload : 1;

	/** weapon is refiring */
	uint32 bRefiring;

	/** current total ammo */
	UPROPERTY(Transient, Replicated)
	int32 CurrentAmmo;

	/** current ammo - inside clip */
	UPROPERTY(Transient, Replicated)
	int32 CurrentAmmoInClip;

	/** burst counter, used for replicating fire events to remote clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_BurstCounter)
	int32 BurstCounter;

	/** Handle for efficient management of StopReload timer */
	FTimerHandle TimerHandle_StopReload;

	/** Handle for efficient management of ReloadWeapon timer */
	FTimerHandle TimerHandle_ReloadWeapon;

	/** Handle for efficient management of HandleFiring timer */
	FTimerHandle TimerHandle_HandleFiring;

	/** FX for muzzle flash */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UParticleSystem* MuzzleFX;

	/** spawned component for muzzle FX */
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSC;

	//////////////////////////////////////////////////////////////////////////
	// Input - server side

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartFire();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopFire();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartReload();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopReload();

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] weapon fire */
	/* With PURE_VIRTUAL we skip implementing the function in AWeapon.cpp and can do this in AInstantWeapon.cpp instead */
	virtual void FireWeapon() PURE_VIRTUAL(AWeapon::FireWeapon, );

	/** [server] fire & update ammo */
	UFUNCTION(reliable, server, WithValidation)
	void ServerHandleFiring();

	/** [local + server] handle weapon fire */
	void HandleFiring();

	/** [local + server] firing started */
	void OnBurstStarted();

	/** [local + server] firing finished */
	void OnBurstFinished();

	/** update weapon state */
	void SetWeaponState(EWeaponState::Type NewState);

	/** determine current weapon state */
	void DetermineWeaponState();

	// weapon sound
	/** play weapon sounds */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound);

	/** firing audio */
	UPROPERTY(Transient)
	UAudioComponent* FireAC;

	/** looped fire sound */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* FireLoopSound;

	/** finished burst sound */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* FireFinishSound;

	/** reload sound */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* ReloadSound;

	// camera shaking
	/** camera shake on firing */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UCameraShake> FireCameraShake;

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** attaches weapon mesh to pawn's slot */
	void AttachMeshToPawn();

	/** detaches weapon mesh from pawn */
	void DetachMeshFromPawn();


	//////////////////////////////////////////////////////////////////////////
	// Weapon usage helpers

	/** find hit */
	FHitResult WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const;

	/** Get the crosshair location in 3D space, basically it means where the crosshair is on, or where the player is aiming */
	FVector GetCrossHairLocationIn3DSpace();

	/** Get the aim of the weapon, allowing for adjustments to be made by the weapon */
	virtual FVector GetAimFromViewpoint() const;

	void GetAimViewpoint(FVector& out_Location, FVector& out_Rotation) const;

	/** get the originating location for camera damage */
	FVector GetCameraDamageStartLocation(const FVector& AimDir) const;

	/** get the muzzle location of the weapon */
	FVector GetMuzzleLocation() const;

	/** get direction of weapon's muzzle */
	FVector GetMuzzleDirection() const;

	//////////////////////////////////////////////////////////////////////////
	// Replication & effects

	UFUNCTION()
	void OnRep_MyPawn();

	UFUNCTION()
	void OnRep_BurstCounter();

	UFUNCTION()
	void OnRep_Reload();

	/** Called in network play to do the cosmetic fx for firing */
	void SimulateWeaponFire();

	/** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	void StopSimulatingWeaponFire();

protected:
	/** Returns Mesh1P subobject **/
	FORCEINLINE UStaticMeshComponent* GetMesh1P() const { return Mesh1P; }

	FORCEINLINE UArrowComponent* GetWeaponArrow() const { return WeaponArrow; }
};
