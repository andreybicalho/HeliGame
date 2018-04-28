// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "HeliGameUserSettings.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API UHeliGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

	UPROPERTY(config)
	float MouseSensitiviy = 1.f;

	UPROPERTY(config)
	float KeyboardSensitivity = 1.f;

	UPROPERTY(config)
	int32 InvertedAim = 1;
	
	UPROPERTY(config)
	int32 PilotAssist = -1;

	UPROPERTY(config)
	float NetworkSmoothingFactor = 1000;

	UPROPERTY(config)
	FString PlayerName;

	// interface UGameUserSettings
	virtual void SetToDefaults() override;

public:
	UHeliGameUserSettings(const FObjectInitializer& ObjectInitializer);

	/** Applies all current user settings to the game and saves to permanent storage (e.g. file), optionally checking for command line overrides. */
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

	void SetMouseSensitivity(float inMouseSensitivity);

	float GetMouseSensitivity();

	void SetKeyboardSensitivity(float inKeyboardSensitivity);

	float GetKeyboardSensitivity();

	void SetInvertedAim(int32 inInvertedAim);

	int32 GetInvertedAim();

	void SetPilotAssist(int32 bInPilotAssist);

	int32 GetPilotAssist();

	void SetNetworkSmoothingFactor(float inNetworkSmoothingFactor);

	float GetNetworkSmoothingFactor();

	FString GetPlayerName();

	void SetPlayerName(const FString& PlayerName);
};
