// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

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
	struct FLinearColor OwnColor;

	UPROPERTY(EditDefaultsOnly, Category = "Health Display", meta = (AllowPrivateAccess = "true"))
	struct FLinearColor EnemyColor;

	UPROPERTY(EditDefaultsOnly, Category = "Health Display", meta = (AllowPrivateAccess = "true"))
	struct FLinearColor FriendColor;

	TWeakObjectPtr<class AHelicopter> OwningPawn;

	void SetupCurrentColor();

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthSettings", meta = (AllowPrivateAccess = "true"))
	struct FLinearColor CurrentColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthSettings", meta = (AllowPrivateAccess = "true"))
	FString PlayerName;	

	UFUNCTION(BlueprintPure, Category = "Display")
	float GetCurrentHealth();

	UFUNCTION(BlueprintPure, Category = "Display")
	FName GetPlayerName();


public:
	UHealthBarUserWidget(const FObjectInitializer& ObjectInitializer);

	void SetOwningPawn(TWeakObjectPtr<class AHelicopter> InOwningPawn);

	UFUNCTION(BlueprintCallable, Category = "Display")
	void SetupWidget();
};
