// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliMoveComp.h"
#include "HeliGame.h"
#include "HeliGameUserSettings.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Public/Engine.h"
#include "DrawDebugHelpers.h"

UHeliMoveComp::UHeliMoveComp(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);



	BoneName = NAME_None; // default is root

	bAccelChange = true;
	bUseAddTorque = true;

	bAddToCurrent = true;
	bUseAddForceForThrust = true;

	bAddLift = true;
	GravityWeight = 0.90f;

	BaseThrust = 10000.f;

	MinimumTiltInclinationAcceleration = 3000.f;

	MaximumAngularVelocity = 100.f;

	MaxInterpolationSpeed = 60.f;
	MinInterpolationSpeed = 1.f;
	CurrentInterpolationSpeed = MaxInterpolationSpeed;
	bUseInterpolationForMovementReplication = true;

	bAutoRollStabilization = false;
	AutoRollInterpSpeed = 1.f;

	bDrawRole = false;
}

/*
Controls
*/
void UHeliMoveComp::AddPitch(float InPitch)
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

void UHeliMoveComp::AddYaw(float InYaw)
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

void UHeliMoveComp::AddRoll(float InRoll)
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
		//UE_LOG(LogTemp, Display, TEXT("InRoll = %f - Angular Velocity Rolling: (%s) = %f"), InRoll, *BaseComp->GetPhysicsAngularVelocityInDegrees().ToString(), FMath::Abs(BaseComp->GetPhysicsAngularVelocityInDegrees().Size()) );
	}
}



/*
Thrust
*/

FVector UHeliMoveComp::ComputeThrust(UPrimitiveComponent* BaseComp, float InThrust)
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

void UHeliMoveComp::AddThrust(float InThrust)
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
FVector UHeliMoveComp::ComputeLift(UPrimitiveComponent* BaseComp)
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

void UHeliMoveComp::AddLift()
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics())
	{
		FVector Lift = ComputeLift(BaseComp);

		BaseComp->AddForce(Lift, BoneName, false);
	}
}

FVector UHeliMoveComp::GetPhysicsLinearVelocity()
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		return BaseComp->GetPhysicsLinearVelocity();
	}

	return FVector::ZeroVector;
}

FVector UHeliMoveComp::GetPhysicsAngularVelocity()
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		return BaseComp->GetPhysicsAngularVelocityInDegrees();
	}

	return FVector::ZeroVector;
}


void UHeliMoveComp::SetMovementState(const FMovementState& TargetMovementState)
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

void UHeliMoveComp::SetMovementStateSmoothly(const FMovementState& TargetMovementState, float DeltaTime)
{	
	if (TargetMovementState.Location.IsNearlyZero())
	{
		return;
	}

	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp)
	{
		// NOTE(andrey): should we use slerp or rinterpto for rotation interpolation?
		//Quat rotation = FQuat::Slerp(BaseComp->GetComponentRotation().Quaternion(), TargetPhysMovementState.Rotation.Quaternion(), 0.1f);
		FRotator rotation = FMath::RInterpTo(BaseComp->GetComponentRotation(), TargetMovementState.Rotation, DeltaTime, CurrentInterpolationSpeed);
		FVector location = FMath::VInterpTo(BaseComp->GetComponentLocation(), TargetMovementState.Location, DeltaTime, CurrentInterpolationSpeed);
		FVector linearVelocity = FMath::VInterpTo(BaseComp->GetPhysicsLinearVelocity(), TargetMovementState.LinearVelocity, DeltaTime, CurrentInterpolationSpeed);
		FVector angularVelocity = FMath::VInterpTo(BaseComp->GetPhysicsAngularVelocityInDegrees(), TargetMovementState.AngularVelocity, DeltaTime, CurrentInterpolationSpeed);

		//BaseComp->SetWorldRotation(rotation);
		BaseComp->SetWorldRotation(rotation.Quaternion());
		BaseComp->SetWorldLocation(location);
		BaseComp->SetPhysicsLinearVelocity(linearVelocity);
		BaseComp->SetPhysicsAngularVelocityInDegrees(angularVelocity);
	}
}

void UHeliMoveComp::SendMovementState()
{
	if ((GetPawnOwner() && GetPawnOwner()->IsLocallyControlled())) // && GetPawnOwner()->Role >= ENetRole::ROLE_AutonomousProxy))
	{
		UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
		if (BaseComp && BaseComp->IsSimulatingPhysics())
		{
			if (BaseComp->GetPhysicsLinearVelocity().IsNearlyZero())
			{
				return;
			}

			/*
			 // TODO(andrey): don't try to replicate faster than server can handle, it will just consume resource. 
			 // recently I've notice a huge % amount of Server_SetMovementState calls while profiling									
			float serverDeltaTickRate = GEngine->NetClientTicksPerSecond;
			float differenceTime = GetWorld()->TimeSeconds - LastTimeReplicatedMovementWasSent;
			if (differenceTime < serverDeltaTickRate)
			{
				return;
			}
			*/						

			Server_SetMovementState(FMovementState(
				BaseComp->GetComponentLocation(),
				BaseComp->GetComponentRotation(),
				BaseComp->GetPhysicsLinearVelocity(),
				BaseComp->GetPhysicsAngularVelocityInDegrees(),
				GetWorld()->TimeSeconds
			));
		}
	}
}

bool UHeliMoveComp::Server_SetMovementState_Validate(const FMovementState& NewMovementState)
{
	return true;
}

void UHeliMoveComp::Server_SetMovementState_Implementation(const FMovementState& NewMovementState)
{
	ReplicatedMovementState = NewMovementState;
}

bool UHeliMoveComp::IsNetworkSmoothingFactorActive()
{
	return bUseInterpolationForMovementReplication;
}

void UHeliMoveComp::SetNetworkSmoothingFactor(float inNetworkSmoothingFactor)
{				
	if (inNetworkSmoothingFactor < 0.1f)
	{
		// turn interpolation off	
		CurrentInterpolationSpeed = MaxInterpolationSpeed;
		bUseInterpolationForMovementReplication = false;
		//UE_LOG(LogTemp, Display, TEXT("UHeliMoveComp::SetNetworkSmoothingFactor ~ Network Smoothing Factor Deactivated!"));
	}
	else if (inNetworkSmoothingFactor >= 100.f)
	{
		// minimum interpolation we allow
		CurrentInterpolationSpeed = MinInterpolationSpeed;
		bUseInterpolationForMovementReplication = true;
		//UE_LOG(LogTemp, Display, TEXT("UHeliMoveComp::SetNetworkSmoothingFactor ~ Network Smoothing Factor is 100%, CurrentInterpolationSpeed = %f"), MinInterpolationSpeed);
	}
	else
	{
		// normalize it between MinInterpolationSpeed and MaxInterpolationSpeed
		// ((limitMax - limitMin) * (valueIn - baseMin) / (baseMax - baseMin)) + limitMin;		
		float smoothFactorNormalized = ((MaxInterpolationSpeed - MinInterpolationSpeed) * (inNetworkSmoothingFactor - 0.f) / (100.f - 0.f)) + MinInterpolationSpeed;
		
		// smaller values means more interpolation speed, greater values means that interpolation should happen more smoothly (slow interp speed)
		CurrentInterpolationSpeed = MaxInterpolationSpeed - smoothFactorNormalized;
		bUseInterpolationForMovementReplication = true;

		//UE_LOG(LogTemp, Warning, TEXT("UHeliMoveComp::SetNetworkSmoothingFactor ~ inNetworkSmoothingFactor = %f, smoothFactorNormalized = %f, CurrentInterpolationSpeed = %f"), inNetworkSmoothingFactor, smoothFactorNormalized, CurrentInterpolationSpeed);
	}
}

FString UHeliMoveComp::GetRoleAsString(ENetRole inRole)
{
	switch (inRole)
	{
	case ROLE_MAX:
		return FString(TEXT("ROLE_MAX"));
	case ROLE_Authority:
		return FString(TEXT("ROLE_Authority"));
	case ROLE_AutonomousProxy:
		return FString(TEXT("ROLE_AutonomousProxy"));
	case ROLE_SimulatedProxy:
		return FString(TEXT("ROLE_SimulatedProxy"));
	case ROLE_None:
		return FString(TEXT("ROLE_None"));
	default:
		return "Error";
	}	
}


void UHeliMoveComp::AutoRollStabilization(float deltaTime)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		FRotator CurrentRotation = BaseComp->GetComponentRotation();

		float TargetRoll = FMath::FInterpTo(CurrentRotation.Roll, 0.f, deltaTime, AutoRollInterpSpeed);

		FRotator TargetRotation = FRotator(CurrentRotation.Pitch, CurrentRotation.Yaw, TargetRoll);

		BaseComp->SetAllPhysicsRotation(TargetRotation);
	}	
}

void UHeliMoveComp::SetAutoRollStabilization(bool bNewAutoRollStabilization)
{
	bAutoRollStabilization = bNewAutoRollStabilization;
}

bool UHeliMoveComp::IsAutoRollingStabilization()
{
	return bAutoRollStabilization;
}

/* overrides */
void UHeliMoveComp::InitializeComponent()
{
	Super::InitializeComponent();

	// if this is a remote proxies get network smoothing factor
	UHeliGameUserSettings* heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings && GetPawnOwner() && !GetPawnOwner()->IsLocallyControlled() && GetPawnOwner()->Role == ENetRole::ROLE_SimulatedProxy)
	{		
		SetNetworkSmoothingFactor(heliGameUserSettings->GetNetworkSmoothingFactor());
	}

	if (heliGameUserSettings)
	{
		SetAutoRollStabilization(heliGameUserSettings->GetPilotAssist() > 0 ? true : false);
	}
}

void UHeliMoveComp::BeginPlay()
{
	Super::BeginPlay();	
}

void UHeliMoveComp::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent)
	{
		//UE_LOG(LogTemp, Warning, TEXT("UHeliMoveComp::TickComponent PawnOwner or UpdatedComponent not found!"));
		return;
	}	

	bool bLocalPlayerAuthority = GetPawnOwner() && GetPawnOwner()->IsLocallyControlled() && GetPawnOwner()->Role >= ENetRole::ROLE_AutonomousProxy;

	// add lift only to pawns that simulate physics
	if (bAddLift && bLocalPlayerAuthority) {
		AddLift();
	}

	// apply received replicated movement state for whatever pawn that is NOT locally controlled
	if (GetPawnOwner() && !GetPawnOwner()->IsLocallyControlled())
	{
		if (bUseInterpolationForMovementReplication)
		{
			SetMovementStateSmoothly(ReplicatedMovementState, DeltaTime);
		}
		else
		{
			SetMovementState(ReplicatedMovementState);
		}
	}

	// send movement state
	// Don't need to replicate movement in single player games
	if (GEngine->GetNetMode(GetWorld()) != NM_Standalone)
	{
		SendMovementState();
	}

	if (bAutoRollStabilization)
	{
		AutoRollStabilization(DeltaTime);
	}


	if (bDrawRole)
	{
		DrawDebugString(GetWorld(), FVector(0, 0, 200), GetRoleAsString(GetPawnOwner()->Role), GetPawnOwner(), FColor::White, DeltaTime);
	}

}

//							Replication List
void UHeliMoveComp::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(UHeliMoveComp, ReplicatedMovementState);

	DOREPLIFETIME_CONDITION(UHeliMoveComp, ReplicatedMovementState, COND_SimulatedOnly);
}






