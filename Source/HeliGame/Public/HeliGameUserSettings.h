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
};