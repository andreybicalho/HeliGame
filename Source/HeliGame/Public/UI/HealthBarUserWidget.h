// Copyright 2016 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBarUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API UHealthBarUserWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Health Display", meta = (AllowPrivateAccess = "true"))
	struct FLinearColor EnemyColor;

	UPROPERTY(EditDefaultsOnly, Category = "Health Display", meta = (AllowPrivateAccess = "true"))
	struct FLinearColor FriendColor;

	TWeakObjectPtr<class AHelicopter> OwningPawn;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthSettings", meta = (AllowPrivateAccess = "true"))
	struct FLinearColor CurrentColor;

	UFUNCTION(BlueprintCallable, Category = "Display")
	void SetupCurrentColor();

	UFUNCTION(BlueprintPure, Category = "Display")
	float GetCurrentHealth();

public:
	UHealthBarUserWidget(const FObjectInitializer& ObjectInitializer);

	void SetOwningPawn(TWeakObjectPtr<class AHelicopter> InOwningPawn);	
};
