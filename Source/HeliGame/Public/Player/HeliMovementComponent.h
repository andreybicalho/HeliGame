// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "GameFramework/PawnMovementComponent.h"
#include "HeliMovementComponent.generated.h"

class UPrimitiveComponent;

USTRUCT()
struct FReplicatedMovementState
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

	FReplicatedMovementState()
	{
		Location = LinearVelocity = AngularVelocity = FVector::ZeroVector;
		Rotation = FRotator::ZeroRotator;
	}
	FReplicatedMovementState(
		FVector_NetQuantize100 Loc,
		FRotator Rot,
		FVector_NetQuantize100 Vel,
		FVector_NetQuantize100 Angular
	)
		: Location(Loc)
		, Rotation(Rot)
		, LinearVelocity(Vel)
		, AngularVelocity(Angular)

	{}
};

/**
*
*/
UCLASS()
class HELIGAME_API UHeliMovementComponent : public UPawnMovementComponent
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

	UFUNCTION(Reliable, Server, WithValidation)
	void Server_UpdateMovementState(const FReplicatedMovementState& TargetMovementState);

	void SetMovementState(const FReplicatedMovementState& TargetMovementState);

public:
	UHeliMovementComponent(const FObjectInitializer& ObjectInitializer);

	void AddPitch(float InPitch);

	void AddYaw(float InYaw);

	void AddRoll(float InRoll);

	void AddThrust(float InThrust);

	FVector GetPhysicsLinearVelocity();

	FVector GetPhysicsAngularVelocity();

	/* overrides */
public:
	void InitializeComponent() override;

	void BeginPlay() override;

	// UActorComponent interface
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
