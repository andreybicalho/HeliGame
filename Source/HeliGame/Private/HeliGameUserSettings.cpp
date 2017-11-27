// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliGameUserSettings.h"

UHeliGameUserSettings::UHeliGameUserSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetToDefaults();
}


void UHeliGameUserSettings::SetMouseSensitivity(float inMouseSensitivity)
{
	MouseSensitiviy = inMouseSensitivity;
}

void UHeliGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	MouseSensitiviy = 1.f;
	KeyboardSensitivity = 1.f;
	InvertedAim = 1;
}

void UHeliGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);
}

float UHeliGameUserSettings::GetMouseSensitivity()
{
	return MouseSensitiviy;
}

void UHeliGameUserSettings::SetKeyboardSensitivity(float inKeyboardSensitivity)
{
	KeyboardSensitivity = inKeyboardSensitivity;
}

float UHeliGameUserSettings::GetKeyboardSensitivity()
{
	return KeyboardSensitivity;
}

void UHeliGameUserSettings::SetInvertedAim(int32 inInvertedAim)
{
	InvertedAim = FMath::Clamp(inInvertedAim, -1, 1);
}

int32 UHeliGameUserSettings::GetInvertedAim()
{
	return InvertedAim;
}


