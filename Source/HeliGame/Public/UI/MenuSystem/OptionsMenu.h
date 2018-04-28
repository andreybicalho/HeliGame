// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MenuSystem/BaseMenuWidget.h"
#include "OptionsMenu.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API UOptionsMenu : public UBaseMenuWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta = (BindWidget))
	class UEditableText* CustomPlayerName;

	FScriptDelegate CustomPlayerNameOnTextCommitted_Delegate;

	UPROPERTY(meta = (BindWidget))
	class UButton* LowButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* MediumButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* HighButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* UltraButton;

	UPROPERTY(meta = (BindWidget))
	class UComboBoxString* ResolutionComboBox;
	
	FScriptDelegate ResolutionComboBoxOnSelectionChanged_Delegate;

	UPROPERTY(meta = (BindWidget))
	class UButton *FullscreenButton;

	UPROPERTY(meta = (BindWidget))
	class UButton *WindowedButton;

	UPROPERTY(meta = (BindWidget))
	class USpinBox* MouseSensitivity;

	FScriptDelegate MouseSensitivitySpinBoxOnValueChanged_Delegate;

	UPROPERTY(meta = (BindWidget))
	class USpinBox* KeyboardSensitivity;

	FScriptDelegate KeyboardSensitivitySpinBoxOnValueChanged_Delegate;

	UPROPERTY(meta = (BindWidget))
	class UButton* InvertedAimButton;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock *InvertedAimText;

	bool InvertedAim;

	UPROPERTY(meta = (BindWidget))
	class UButton* PilotAssistButton;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock *PilotAssistText;

	bool PilotAssist;

	UPROPERTY(meta = (BindWidget))
	class UButton* ApplySettingsButton;

	void InitializeResolutionComboBox();

	void InitializeAimMode();

	void InitializeMouseAndKeyboardSense();

	void InitializePilotAssist();

protected:
	bool Initialize() override;

private:
	UFUNCTION()
	void SetLowGraphics();

	UFUNCTION()
	void SetMediumGraphics();

	UFUNCTION()
	void SetHighGraphics();

	UFUNCTION()
	void SetUltraGraphics();

	UFUNCTION()
	void SetFullscreenMode();

	UFUNCTION()
	void SetWindowedMode();

	UFUNCTION() 
	void SetInvertedAim();

	UFUNCTION()
	void SetPilotAssist();

	UFUNCTION()
	void CustomPlayerName_OnTextCommitted();

	UFUNCTION()
	void ResolutionComboBox_OnSelectionChanged();

	UFUNCTION()
	void MouseSensitivitySpinBox_OnValueChanged();

	UFUNCTION()
	void KeyboardSensitivitySpinBox_OnValueChanged();

	UFUNCTION()
	void ApplySettings();
};
