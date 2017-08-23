// Copyright 2016 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliGame.h"
#include "HeliMovementComponent.h"

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

	BaseThrust = 10000.f;

	MinimumTiltInclinationAcceleration = 10000.f;
}

/*
Controls
*/
void UHeliMovementComponent::AddPitch(float InPitch)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics())
	{
		const FVector AngularVelocity = BaseComp->GetRightVector() * InPitch;

		if (bUseAddTorque)
		{
			BaseComp->AddTorque(AngularVelocity, BoneName, bAccelChange);
		}
		else
		{
			BaseComp->SetPhysicsAngularVelocity(AngularVelocity, bAddToCurrent);
		}
	}
}

void UHeliMovementComponent::AddYaw(float InYaw)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics())
	{
		const FVector AngularVelocity = BaseComp->GetUpVector() * InYaw;

		if (bUseAddTorque)
		{
			BaseComp->AddTorque(AngularVelocity, BoneName, bAccelChange);
		}
		else
		{
			BaseComp->SetPhysicsAngularVelocity(AngularVelocity, bAddToCurrent);
		}
	}
}

void UHeliMovementComponent::AddRoll(float InRoll)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics())
	{
		const FVector AngularVelocity = BaseComp->GetForwardVector() * InRoll;

		if (bUseAddTorque)
		{
			BaseComp->AddTorque(AngularVelocity, BoneName, bAccelChange);
		}
		else
		{
			BaseComp->SetPhysicsAngularVelocity(AngularVelocity, bAddToCurrent);
		}
	}
}



/*
Thrust
*/

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



/*
Lift
*/
FVector UHeliMovementComponent::ComputeLift(UPrimitiveComponent* BaseComp)
{
	FVector LiftForce = FVector::ZeroVector;

	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		float GravityAcceleration = FMath::Abs(GetGravityZ());

		float InclinationAngle = FVector::DotProduct(FVector::UpVector, BaseComp->GetForwardVector());
		float TiltAngle = FVector::DotProduct(FVector::UpVector, BaseComp->GetRightVector());

		float InclinationWeight = 1 - FMath::Abs(InclinationAngle);
		float TiltWeight = 1 - FMath::Abs(TiltAngle);

		// weights gravity acceleration based on inclination and tilt
		float WeightedGravityAcceleration = GravityAcceleration * InclinationWeight * TiltWeight;

		// up momentum will produce zero net force against gravity force when stationary (F = m*a)
		FVector UpMomentum = BaseComp->GetUpVector() * (BaseComp->GetMass() * WeightedGravityAcceleration);

		// adds momentum for inclination and tilt (m = a * tanh(-angle))
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
		return BaseComp->GetPhysicsAngularVelocity();
	}

	return FVector::ZeroVector;
}

void UHeliMovementComponent::SetPhysicsLinearVelocity(FVector NewLinearVelocity)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		BaseComp->SetPhysicsLinearVelocity(NewLinearVelocity, false);
	}
}

void UHeliMovementComponent::SetPhysicsAngularVelocity(FVector NewAngularVelocity)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		BaseComp->SetPhysicsAngularVelocity(NewAngularVelocity, false);
	}
}


/*
Movement Replication (Client-side Authority)
*/
void UHeliMovementComponent::MovementReplication()
{
	if (GetPawnOwner())
	{
		if (GetPawnOwner()->IsLocallyControlled())
		{
			MovementReplication_Send();
		}
		else
		{
			MovementReplication_Receive();
		}
	}

}

void UHeliMovementComponent::MovementReplication_Send()
{
	// Don't replicate invalid data
	if (!GetPawnOwner() || (GetPawnOwner() && GetPawnOwner()->IsPendingKill()))
	{
		return;
	}

	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		Server_SendMovement(
			FMovementReplication(
				BaseComp->GetComponentLocation(),
				BaseComp->GetComponentRotation(),
				BaseComp->GetPhysicsLinearVelocity(),
				BaseComp->GetPhysicsAngularVelocity()
			)
		);
	}
}

void UHeliMovementComponent::MovementReplication_Receive()
{
	// Don't receive invalid or useless data
	if (!GetPawnOwner() || (GetPawnOwner() && GetPawnOwner()->IsPendingKill()))
	{
		return;
	}

	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		BaseComp->SetWorldLocation(Movement.Location);
		BaseComp->SetWorldRotation(Movement.Rotation.Quaternion());
		BaseComp->SetPhysicsLinearVelocity(Movement.LinearVelocity);
		BaseComp->SetPhysicsAngularVelocity(Movement.AngularVelocity);
	}
}

bool UHeliMovementComponent::Server_SendMovement_Validate(FMovementReplication NewMovement)
{
	return true;
}

void UHeliMovementComponent::Server_SendMovement_Implementation(FMovementReplication NewMovement)
{
	Movement = NewMovement;
}



/* overrides */
void UHeliMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("UHeliMovementComponent::TickComponent PawnOwner or UpdatedComponent not found!"));
		return;
	}

	// add lift
	if (bAddLift) {
		AddLift();
	}

	// replication
	// Don't need to replicate movement in single player games
	if (GEngine->GetNetMode(GetWorld()) != NM_Standalone)
	{
		MovementReplication();
	}
}

//							Replication List
void UHeliMovementComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UHeliMovementComponent, Movement, COND_SimulatedOnly);
}



