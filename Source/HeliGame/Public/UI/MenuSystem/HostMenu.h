// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MenuSystem/BaseMenuWidget.h"

#include "HostMenu.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API UHostMenu : public UBaseMenuWidget
{
	GENERATED_BODY()	

	UPROPERTY(meta = (BindWidget))
	class UButton* HostServerButton;

	UPROPERTY(meta = (BindWidget))
	class UEditableText* ServerHostName;

	UPROPERTY(meta = (BindWidget))
	class UComboBoxString* GameModeComboBox;

	UPROPERTY(meta = (BindWidget))
	class UComboBoxString* MapComboBox;

	UPROPERTY(meta = (BindWidget))
	class UComboBoxString* NumberOfPlayersComboBox;

	UPROPERTY(meta = (BindWidget))
	class USpinBox* WarmupTimeSpinBox;

	UPROPERTY(meta = (BindWidget))
	class USpinBox* RoundTimeSpinBox;

	UPROPERTY(meta = (BindWidget))
	class USpinBox* TimeBetweenMatchesSpinBox;

	UPROPERTY(meta = (BindWidget))
	class UButton* ToggleLANButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* BackToMainHostMenuButton;

	UFUNCTION()
	void ToggleLan();

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* LanText;

	bool bIsLAN = true;

protected:
	bool Initialize() override;

public:
	UFUNCTION()
	void HostServer();

	UFUNCTION()
	void BackToMainMenu();
	
};
