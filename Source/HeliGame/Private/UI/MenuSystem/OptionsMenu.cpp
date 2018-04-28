// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "OptionsMenu.h"
#include "HeliGameUserSettings.h"

#include "Components/EditableText.h"
#include "Components/SpinBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Public/Engine.h"
#include "Runtime/RHI/Public/DynamicRHI.h"

bool UOptionsMenu::Initialize()
{
	bool Success = Super::Initialize();
	if (!Success) return false;

	if (!ensure(CustomPlayerName != nullptr)) return false;
	CustomPlayerNameOnTextCommitted_Delegate.BindUFunction(this, "CustomPlayerName_OnTextCommitted");
	CustomPlayerName->OnTextCommitted.Add(CustomPlayerNameOnTextCommitted_Delegate);
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		CustomPlayerName->SetText(FText::FromString(heliGameUserSettings->GetPlayerName()));
	}

	if (!ensure(LowButton != nullptr)) return false;
	LowButton->OnClicked.AddDynamic(this, &UOptionsMenu::SetLowGraphics);

	if (!ensure(MediumButton != nullptr)) return false;
	MediumButton->OnClicked.AddDynamic(this, &UOptionsMenu::SetMediumGraphics);

	if (!ensure(HighButton != nullptr)) return false;
	HighButton->OnClicked.AddDynamic(this, &UOptionsMenu::SetHighGraphics);

	if (!ensure(UltraButton != nullptr)) return false;
	UltraButton->OnClicked.AddDynamic(this, &UOptionsMenu::SetUltraGraphics);

	if (!ensure(FullscreenButton != nullptr)) return false;
	FullscreenButton->OnClicked.AddDynamic(this, &UOptionsMenu::SetFullscreenMode);

	if (!ensure(WindowedButton != nullptr)) return false;
	WindowedButton->OnClicked.AddDynamic(this, &UOptionsMenu::SetWindowedMode);

	if (!ensure(MouseSensitivity != nullptr)) return false;
	MouseSensitivitySpinBoxOnValueChanged_Delegate.BindUFunction(this, "MouseSensitivitySpinBox_OnValueChanged");
	MouseSensitivity->OnValueChanged.Add(MouseSensitivitySpinBoxOnValueChanged_Delegate);

	if (!ensure(KeyboardSensitivity != nullptr)) return false;
	KeyboardSensitivitySpinBoxOnValueChanged_Delegate.BindUFunction(this, "KeyboardSensitivitySpinBox_OnValueChanged");
	KeyboardSensitivity->OnValueChanged.Add(KeyboardSensitivitySpinBoxOnValueChanged_Delegate);
	InitializeMouseAndKeyboardSense();

	if (!ensure(InvertedAimButton != nullptr)) return false;
	InvertedAimButton->OnClicked.AddDynamic(this, &UOptionsMenu::SetInvertedAim);
	if (!ensure(InvertedAimText != nullptr)) return false;
	InitializeAimMode();	

	if (!ensure(ApplySettingsButton != nullptr)) return false;
	ApplySettingsButton->OnClicked.AddDynamic(this, &UOptionsMenu::ApplySettings);

	if (!ensure(ResolutionComboBox != nullptr)) return false;
	ResolutionComboBoxOnSelectionChanged_Delegate.BindUFunction(this, "ResolutionComboBox_OnSelectionChanged");
	ResolutionComboBox->OnSelectionChanged.Add(ResolutionComboBoxOnSelectionChanged_Delegate);
	InitializeResolutionComboBox();

	if (!ensure(PilotAssistButton != nullptr)) return false;
	PilotAssistButton->OnClicked.AddDynamic(this, &UOptionsMenu::SetPilotAssist);
	if (!ensure(PilotAssistText != nullptr)) return false;
	InitializePilotAssist();

	return true;
}

void UOptionsMenu::InitializeResolutionComboBox()
{
	FScreenResolutionArray Resolutions;
	if (RHIGetAvailableResolutions(Resolutions, true))
	{
		for (const FScreenResolutionRHI& res : Resolutions)
		{
			FString resOption = FString::Printf(TEXT("%dx%d"), res.Width, res.Height);
			ResolutionComboBox->AddOption(resOption);
		}
	}

	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		FIntPoint lastConfirmedScreenResolution = heliGameUserSettings->GetScreenResolution();
		FString currentRes = FString::Printf(TEXT("%dx%d"), lastConfirmedScreenResolution.X, lastConfirmedScreenResolution.Y);
		ResolutionComboBox->SetSelectedOption(currentRes);
	}
}

void UOptionsMenu::InitializeAimMode()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		if (heliGameUserSettings->GetInvertedAim() > 0)
		{
			InvertedAimText->SetText(FText::FromString(FString(TEXT("Normal Aim"))));
			InvertedAim = false;
		}
		else
		{
			InvertedAimText->SetText(FText::FromString(FString(TEXT("Inverted Aim"))));
			InvertedAim = true;
		}

	}
}

void UOptionsMenu::InitializeMouseAndKeyboardSense()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		MouseSensitivity->SetValue(heliGameUserSettings->GetMouseSensitivity());
		KeyboardSensitivity->SetValue(heliGameUserSettings->GetKeyboardSensitivity());
	}
}

void UOptionsMenu::InitializePilotAssist()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		if (heliGameUserSettings->GetPilotAssist() > 0)
		{
			PilotAssistText->SetText(FText::FromString(FString(TEXT("Pilot Assist ON"))));
			PilotAssist = true;
		}
		else
		{
			PilotAssistText->SetText(FText::FromString(FString(TEXT("Pilot Assist OFF"))));
			PilotAssist = false;
		}
	}
}

void UOptionsMenu::CustomPlayerName_OnTextCommitted()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetPlayerName(CustomPlayerName->GetText().ToString());
	}
}

void UOptionsMenu::ResolutionComboBox_OnSelectionChanged()
{
	FString width, height;
	if (ResolutionComboBox->GetSelectedOption().Split("x", &width, &height))
	{
		UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
		if (heliGameUserSettings)
		{
			heliGameUserSettings->SetScreenResolution(FIntPoint(FCString::Atoi(*width), FCString::Atoi(*height)));
		}
	}
}

void UOptionsMenu::MouseSensitivitySpinBox_OnValueChanged()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetMouseSensitivity(MouseSensitivity->GetValue());
	}
}

void UOptionsMenu::KeyboardSensitivitySpinBox_OnValueChanged()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetKeyboardSensitivity(KeyboardSensitivity->GetValue());
	}
}

void UOptionsMenu::SetInvertedAim()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (!heliGameUserSettings)
	{
		return;
	}

	if(InvertedAim)
	{
		InvertedAim = false;
		InvertedAimText->SetText(FText::FromString(FString(TEXT("Normal Aim"))));
		heliGameUserSettings->SetInvertedAim(1);
	}
	else 
	{
		InvertedAim = true;
		InvertedAimText->SetText(FText::FromString(FString(TEXT("Inverted Aim"))));
		heliGameUserSettings->SetInvertedAim(-1);
	}
}

void UOptionsMenu::SetPilotAssist()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (!heliGameUserSettings)
	{
		return;
	}

	if (PilotAssist)
	{
		PilotAssist = false;
		PilotAssistText->SetText(FText::FromString(FString(TEXT("Pilot Assist OFF"))));
		heliGameUserSettings->SetPilotAssist(-1);
	}
	else
	{
		PilotAssist = true;
		PilotAssistText->SetText(FText::FromString(FString(TEXT("Pilot Assist ON"))));
		heliGameUserSettings->SetPilotAssist(1);
	}
}

void UOptionsMenu::SetLowGraphics()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetOverallScalabilityLevel(0);
	}
}

void UOptionsMenu::SetMediumGraphics()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetOverallScalabilityLevel(1);
	}
}

void UOptionsMenu::SetHighGraphics()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetOverallScalabilityLevel(2);
	}
}

void UOptionsMenu::SetUltraGraphics()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetOverallScalabilityLevel(3);
	}
}

void UOptionsMenu::SetFullscreenMode()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetFullscreenMode(EWindowMode::Fullscreen);
	}
}

void UOptionsMenu::SetWindowedMode()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		heliGameUserSettings->SetFullscreenMode(EWindowMode::Windowed);
	}
}

void UOptionsMenu::ApplySettings()
{
	UHeliGameUserSettings *heliGameUserSettings = Cast<UHeliGameUserSettings>(GEngine->GetGameUserSettings());
	if (heliGameUserSettings)
	{
		heliGameUserSettings->ApplySettings(false);
	}
}