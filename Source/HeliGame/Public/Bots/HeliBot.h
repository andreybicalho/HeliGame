// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HeliFighterVehicle.h"
#include "HeliBot.generated.h"

UCLASS()
class HELIGAME_API AHeliBot : public AHeliFighterVehicle
{
	GENERATED_BODY()

	UPROPERTY(Category = "MovementSettings", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UFloatingPawnMovement* MovementComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
  	AHeliBot(const FObjectInitializer &ObjectInitializer);
	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

private:
	/*
	*	Crash Impact
	*/

	FScriptDelegate OnCrashImpactDelegate;
	
	UFUNCTION()
	void OnCrashImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(Reliable, Server, WithValidation, Category = "CrashImpact")
	void Server_CrashImpactTakeDamage(float Damage);

	float ComputeCrashImpactDamage();

	/* crash impact damage curve */
	UPROPERTY(Category = "6DoF", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UCurveFloat* CrashImpactDamageCurve;

protected:
	void OnDeath(float KillingDamage, FDamageEvent const &DamageEvent, APawn *PawnInstigator, AActor *DamageCauser) override;

	void InitBot();

public:
	UPROPERTY(EditAnywhere, Category = "Behavior")
	class UBehaviorTree *BotBehavior;
};
