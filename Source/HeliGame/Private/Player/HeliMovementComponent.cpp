// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliMovementComponent.h"
#include "HeliGame.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Public/Engine.h"

UHeliMovementComponent::UHeliMovementComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);



	BoneName = NAME_None; // default is root

	bAccelChange = true;
	bUseAddTorque = true;

	bAddToCurrent = true;
	bUseAddForceForThrust = true;

	bAddLift = true;
	GravityWeight = 0.70f;

	BaseThrust = 10000.f;

	MinimumTiltInclinationAcceleration = 5000.f;

	MaximumAngularVelocity = 100.f;
}

void UHeliMovementComponent::SetMovementState(const FReplicatedMovementState& TargetMovementState)
{
	if (TargetMovementState.Location.IsNearlyZero())
	{
		return;
	}

	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp)
	{
		FRotator rotation = TargetMovementState.Rotation;
		FVector location = TargetMovementState.Location;
		FVector linearVelocity = TargetMovementState.LinearVelocity;
		FVector angularVelocity = TargetMovementState.AngularVelocity;

		BaseComp->SetWorldRotation(rotation.Quaternion());
		BaseComp->SetWorldLocation(location);
		BaseComp->SetPhysicsLinearVelocity(linearVelocity);
		BaseComp->SetPhysicsAngularVelocityInDegrees(angularVelocity);
	}
}

bool UHeliMovementComponent::Server_UpdateMovementState_Validate(const FReplicatedMovementState& TargetMovementState) { return true; }
void UHeliMovementComponent::Server_UpdateMovementState_Implementation(const FReplicatedMovementState& TargetMovementState)
{
	SetMovementState(TargetMovementState);
}

/*
Controls
*/

void UHeliMovementComponent::AddPitch(float InPitch)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics() && FMath::Abs(BaseComp->GetPhysicsAngularVelocityInDegrees().Size()) <= MaximumAngularVelocity)
	{
		const FVector AngularVelocity = BaseComp->GetRightVector() * InPitch;

		if (bUseAddTorque)
		{
			BaseComp->AddTorqueInRadians(AngularVelocity, BoneName, bAccelChange);
		}
		else
		{
			BaseComp->SetPhysicsAngularVelocityInDegrees(AngularVelocity, bAddToCurrent);
		}
	}
}

void UHeliMovementComponent::AddYaw(float InYaw)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics() && FMath::Abs(BaseComp->GetPhysicsAngularVelocityInDegrees().Size()) <= MaximumAngularVelocity)
	{
		const FVector AngularVelocity = BaseComp->GetUpVector() * InYaw;

		if (bUseAddTorque)
		{
			BaseComp->AddTorqueInRadians(AngularVelocity, BoneName, bAccelChange);
		}
		else
		{
			BaseComp->SetPhysicsAngularVelocityInDegrees(AngularVelocity, bAddToCurrent);
		}
	}
}

void UHeliMovementComponent::AddRoll(float InRoll)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics() && FMath::Abs(BaseComp->GetPhysicsAngularVelocityInDegrees().Size()) <= MaximumAngularVelocity)
	{
		const FVector AngularVelocity = BaseComp->GetForwardVector() * InRoll;

		if (bUseAddTorque)
		{
			BaseComp->AddTorqueInRadians(AngularVelocity, BoneName, bAccelChange);
		}
		else
		{
			BaseComp->SetPhysicsAngularVelocityInDegrees(AngularVelocity, bAddToCurrent);
		}
	}
}

void UHeliMovementComponent::AddThrust(float InThrust)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics())
	{
		FVector ThrustForce = ComputeThrust(BaseComp, InThrust);

		if (bUseAddForceForThrust)
		{
			BaseComp->AddForce(ThrustForce, BoneName, bAccelChange);
		}
		else
		{
			BaseComp->SetPhysicsLinearVelocity(ThrustForce, bAddToCurrent);
		}
	}
}

FVector UHeliMovementComponent::ComputeThrust(UPrimitiveComponent* BaseComp, float InThrust)
{
	FVector ThrustForce = FVector::ZeroVector;

	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		if (InThrust < 0.f)
		{
			ThrustForce = FVector::UpVector * InThrust * BaseThrust;
		}
		else
		{


			float InclinationAngle = FVector::DotProduct(FVector::UpVector, BaseComp->GetForwardVector());
			float ZWeight = 1.f - FMath::Abs(InclinationAngle);

			FVector UpForceCorrection = FVector(1.f, 1.f, ZWeight);

			ThrustForce = (BaseComp->GetUpVector() * UpForceCorrection) * BaseThrust * InThrust;
		}

	}

	return ThrustForce;
}

/*
Lift
*/
FVector UHeliMovementComponent::ComputeLift(UPrimitiveComponent* BaseComp)
{
	FVector LiftForce = FVector::ZeroVector;

	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		float GravityAcceleration = FMath::Abs(GetGravityZ()) * GravityWeight;

		float InclinationAngle = FVector::DotProduct(FVector::UpVector, BaseComp->GetForwardVector());
		float TiltAngle = FVector::DotProduct(FVector::UpVector, BaseComp->GetRightVector());

		float InclinationWeight = 1 - FMath::Abs(InclinationAngle);
		float TiltWeight = 1 - FMath::Abs(TiltAngle);

		// weights gravity acceleration based on inclination and tilt
		float WeightedGravityAcceleration = GravityAcceleration * InclinationWeight * TiltWeight;

		// up momentum will produce zero net force against gravity force when stationary (F = m*a)
		FVector UpMomentum = BaseComp->GetUpVector() * (BaseComp->GetMass() * WeightedGravityAcceleration);

		// adds momentum for inclination and tilt (m = a * tanh(-angle) a.k.a. hyperbolic tangent function) 
		float InclinationAcceleration = MinimumTiltInclinationAcceleration * FMath::Tan(-InclinationAngle);
		float TiltAcceleration = MinimumTiltInclinationAcceleration * FMath::Tan(-TiltAngle);

		FVector InclinationMomentum = BaseComp->GetForwardVector() * GravityAcceleration * InclinationAcceleration;
		FVector TiltMomentum = BaseComp->GetRightVector() * GravityAcceleration *  TiltAcceleration;

		// compute net lift force
		LiftForce = UpMomentum + InclinationMomentum + TiltMomentum;
	}

	return LiftForce;
}

void UHeliMovementComponent::AddLift()
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics())
	{
		FVector Lift = ComputeLift(BaseComp);

		BaseComp->AddForce(Lift, BoneName, false);
	}
}

FVector UHeliMovementComponent::GetPhysicsLinearVelocity()
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		return BaseComp->GetPhysicsLinearVelocity();
	}

	return FVector::ZeroVector;
}

FVector UHeliMovementComponent::GetPhysicsAngularVelocity()
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		return BaseComp->GetPhysicsAngularVelocityInDegrees();
	}

	return FVector::ZeroVector;
}

/* overrides */
void UHeliMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UHeliMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UHeliMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent)
	{
		//UE_LOG(LogTemp, Warning, TEXT("UHeliMoveComp::TickComponent PawnOwner or UpdatedComponent not found!"));
		return;
	}

	bool bLocalPlayerAuthority = GetPawnOwner() && GetPawnOwner()->IsLocallyControlled();// && GetPawnOwner()->Role >= ENetRole::ROLE_AutonomousProxy;


	if (bLocalPlayerAuthority)
	{
		// add lift
		if (bAddLift) {
			AddLift();
		}
		// replicates
		UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
		if (BaseComp && BaseComp->IsSimulatingPhysics())
		{
			Server_UpdateMovementState(FReplicatedMovementState(
				BaseComp->GetComponentLocation(),
				BaseComp->GetComponentRotation(),
				BaseComp->GetPhysicsLinearVelocity(),
				BaseComp->GetPhysicsAngularVelocityInDegrees()
			));
		}
	}

}

//							Replication List
void UHeliMovementComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
