// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "HeliFighterVehicle.generated.h"

class USoundCue;

USTRUCT()
struct FTakeHitData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float ActualDamage;

	UPROPERTY()
	UClass *DamageTypeClass;

	UPROPERTY()
	TWeakObjectPtr<class AHeliFighterVehicle> PawnInstigator;

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
	{
	}

	FDamageEvent &GetDamageEvent()
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

	void SetDamageEvent(const FDamageEvent &DamageEvent)
	{
		DamageEventClassID = DamageEvent.GetTypeID();
		switch (DamageEventClassID)
		{
		case FPointDamageEvent::ClassID:
			PointDamageEvent = *((FPointDamageEvent const *)(&DamageEvent));
			break;
		case FRadialDamageEvent::ClassID:
			RadialDamageEvent = *((FRadialDamageEvent const *)(&DamageEvent));
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








/*
* Base class for Fighter Vehicles, capable of handling damaging and weapon
*/
UCLASS()
class HELIGAME_API AHeliFighterVehicle : public APawn
{
	GENERATED_BODY()

protected:
	/* Take damage & handle death */
	virtual float TakeDamage(float Damage, struct FDamageEvent const &DamageEvent, class AController *EventInstigator, class AActor *DamageCauser) override;

public:
	// Called every frame
	// virtual void Tick(float DeltaTime) override;
	
	// Called to bind functionality to input	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** spawn inventory, setup initial variables */
	virtual void PostInitializeComponents() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

private:
	/** socket or bone name for attaching Primary weapon mesh */
 	UPROPERTY(Category = "Weapon", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
 	FName PrimaryWeaponAttachPoint;

protected:
	/** StaticMesh component that will be the visuals for our flying pawn and physics body */
	UPROPERTY(Category = "Mesh", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* MainStaticMeshComponent;

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

	UPROPERTY(Category = "HealthSettings", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FName HealthBarSocketName = TEXT("HealthBarSocket");

	UPROPERTY(Category = "HealthSettings", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent *HealthBarWidgetComponent;
	
	/*
	* Camera
	*/

	/** Spring arm that will offset the first person viewpoint for the camera */
	UPROPERTY(Category = "Camera", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent *SpringArmFirstPerson;
	
	/** Spring arm that will offset the third person viewpoint for the camera */
	UPROPERTY(Category = "Camera", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent *SpringArmThirdPerson;
	
	/** Camera component */
	UPROPERTY(Category = "Camera", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent *Camera;

	float SpawnDelay = 1.f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "HealthSettings", meta = (AllowPrivateAccess = "true"))
	float MaxHealth;

	// Current health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "HealthSettings", meta = (AllowPrivateAccess = "true"))
	float Health;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue *SoundTakeHit;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue *DeathExplosionSound;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UParticleSystem *DeathExplosionFX;

	/* Holds hit data to replicate hits and death to clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_LastTakeHitInfo)
	struct FTakeHitData LastTakeHitInfo;

	UFUNCTION()
	void OnRep_LastTakeHitInfo();

	bool bIsDying;

	virtual void PlayHit(float DamageTaken, struct FDamageEvent const &DamageEvent, APawn *PawnInstigator, AActor *DamageCauser, bool bKilled);

	void ReplicateHit(float DamageTaken, struct FDamageEvent const &DamageEvent, APawn *PawnInstigator, AActor *DamageCauser, bool bKilled);

	/** Kill this pawn */
	virtual bool CanDie(float KillingDamage, FDamageEvent const &DamageEvent, AController *Killer, AActor *DamageCauser) const;

	/** Returns True if the pawn can die in the current state */
	virtual bool Die(float KillingDamage, FDamageEvent const &DamageEvent, AController *Killer, AActor *DamageCauser);

	virtual void OnDeath(float KillingDamage, FDamageEvent const &DamageEvent, APawn *PawnInstigator, AActor *DamageCauser);



	/** Main loop sound cue */

	UPROPERTY(Category = "Sound", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	class USoundCue* MainLoopSound;

	class UAudioComponent* PlaySound(class USoundCue* Sound);

	UPROPERTY(BlueprintReadWrite, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* MainAudioComponent;

public:
	AHeliFighterVehicle(const FObjectInitializer &ObjectInitializer);
	
	/*
	* Weapon
	*/

	/** default weapon to spawn on begin play function */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeapon> DefaultPrimaryWeaponToSpawn;
	
	/** get Primary weapon attach point */
	FName GetPrimaryWeaponAttachPoint() const;
	
	/** get current weapon attach point */
	FName GetCurrentWeaponAttachPoint() const;
	bool bPrimaryWeaponEquiped;
	
	/** Current weapon */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Weapon)
	class AWeapon *CurrentWeapon;
	
	// actor can have a lot of weapons, but it can uses only one at a time
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AWeapon *GetCurrentWeaponEquiped();
	
	/* OnRep functions can use a parameter to hold the previous value of the variable. Very useful when you need to handle UnEquip etc. */
	UFUNCTION()
	void OnRep_Weapon(AWeapon *LastWeapon);
	
	/** set weapon */
	void SetWeapon(class AWeapon *NewWeapon, class AWeapon *LastWeapon);

  	/**
	* [server + local] equips weapon from inventory
	*
	* @param Weapon	Weapon to equip
	*/
  	void EquipWeapon(class AWeapon *NewWeapon);

	UFUNCTION(Reliable, Server, WithValidation)
	void Server_EquipWeapon(AWeapon *NewWeapon);
	
	// Weapon usage

	void OnStartFire();
	void OnStopFire();
	void OnReloadWeapon();

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
	virtual void KilledBy(class APawn *EventInstigator);
	
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
	
	void UpdatePlayerInfo(FName playerName, int32 teamNumber);
	
	void SetupPlayerInfoWidget();
	
	void RemoveHealthWidget();
	
	void EnableFirstPersonViewpoint();
	
	void EnableThirdPersonViewpoint();
};
