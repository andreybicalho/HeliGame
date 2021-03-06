// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "HeliMoveComp.generated.h"

class UPrimitiveComponent;

USTRUCT()
struct FMovementState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector_NetQuantize100 Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector_NetQuantize100 LinearVelocity;

	UPROPERTY()
	FVector_NetQuantize100 AngularVelocity;

	UPROPERTY()
	float Timestamp;

	FMovementState()
	{
		Location = LinearVelocity = AngularVelocity = FVector::ZeroVector;
		Rotation = FRotator::ZeroRotator;
		Timestamp = 1;
	}
	FMovementState(
		FVector_NetQuantize100 Loc,
		FRotator Rot,
		FVector_NetQuantize100 Vel,
		FVector_NetQuantize100 Angular,
		float ReplicationTimeInSeconds
	)
		: Location(Loc)
		, Rotation(Rot)
		, LinearVelocity(Vel)
		, AngularVelocity(Angular)
		, Timestamp(ReplicationTimeInSeconds)

	{}
};

/**
 * 
 */
UCLASS()
class HELIGAME_API UHeliMoveComp : public UPawnMovementComponent
{
	GENERATED_BODY()
	
	/* Maximum angular velocity the body can get */
	UPROPERTY(Category = "6DoFPhysics", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float MaximumAngularVelocity;

	/* whether use AddTorque or SetPhysics */
	UPROPERTY(Category = "6DoFPhysics", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	bool bUseAddTorque;

	/* If a SkeletalMeshComponent, name of body to apply torque to. 'None' indicates root body. */
	UPROPERTY(Category = "6DoFPhysics", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	FName BoneName;

	/*If true, Torque is taken as a change in angular acceleration instead of a physical torque(i.e.mass will have no affect). */
	UPROPERTY(Category = "6DoFPhysics", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	bool bAccelChange;

	/* adds velocity to current when using "SetPhysics" */
	UPROPERTY(Category = "6DoFPhysics", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	bool bAddToCurrent;

	/* whether use AddForce or SetPhysics */
	UPROPERTY(Category = "6DoFPhysics", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	bool bUseAddForceForThrust;

	/* Minimum acceleration to apply when inclination and tilt increases */
	UPROPERTY(Category = "6DoFPhysics", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float MinimumTiltInclinationAcceleration;

	/*
		Lift
	*/

	/** Whether to apply lift. Lift will be computed based on gravity and actor inclination regarded the actor UpVector. */
	UPROPERTY(Category = "6DoFPhysics", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	bool bAddLift;

	/** Percentage of the gravityZ used for lift */
	UPROPERTY(Category = "6DoFPhysics", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float GravityWeight;

	FVector ComputeLift(UPrimitiveComponent* BaseComp);

	void AddLift();


	/*
		Thrust
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings", meta = (AllowPrivateAccess = "true"))
	float BaseThrust = 1.f;

	FVector ComputeThrust(UPrimitiveComponent* BaseComp, float InThrust);

	/*
		Movement Replication
	*/
	
	void SendMovementState();

	UFUNCTION(Unreliable, Server, WithValidation)
	void Server_SetMovementState(const FMovementState& NewMovementState);

	UPROPERTY(Transient, Replicated)
	struct FMovementState ReplicatedMovementState;

	void SetMovementState(const FMovementState& TargetMovementState);	
	
	void SetMovementStateSmoothly(const FMovementState& TargetMovementState, float DeltaTime);

	/* controls whether use or not interpolation for movement replication. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings|Replication", meta = (AllowPrivateAccess = "true"))
	bool bUseInterpolationForMovementReplication;

	/* controls how fast actual movement data will be interpolated with server's data. Greater values means that it will interpolate faster. */
	float CurrentInterpolationSpeed;

	/* Greater values means that it will interpolate faster, so much greater values will have the same effect as NO interpolation at all... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings|Replication", meta = (AllowPrivateAccess = "true"))
	float MaxInterpolationSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings|Replication", meta = (AllowPrivateAccess = "true"))
	float MinInterpolationSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Helpers|Replication", meta = (AllowPrivateAccess = "true"))
	bool bDrawRole;

	FString GetRoleAsString(ENetRole inRole);	

	/* whether to use auto roll stabilization or not */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings", meta = (AllowPrivateAccess = "true"))
	bool bAutoRollStabilization;

	/* controls how fast 'auto roll' will reach the stabilized rotation */
	UPROPERTY(Category = "MovementSettings", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float AutoRollInterpSpeed;

	void AutoRollStabilization(float deltaTime);

public:
	UHeliMoveComp(const FObjectInitializer& ObjectInitializer);

	void AddPitch(float InPitch);

	void AddYaw(float InYaw);

	void AddRoll(float InRoll);

	void AddThrust(float InThrust);

	FVector GetPhysicsLinearVelocity();

	FVector GetPhysicsAngularVelocity();

	void SetNetworkSmoothingFactor(float inNetworkSmoothingFactor);

	bool IsNetworkSmoothingFactorActive();

	void SetAutoRollStabilization(bool bNewAutoRollStabilization);

	bool IsAutoRollingStabilization();

	/* overrides */
public:
	void InitializeComponent() override;

	void BeginPlay() override;

	// UActorComponent interface
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
