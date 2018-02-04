// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MenuSystem/BaseMenuWidget.h"
#include "MainMenu.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API UMainMenu : public UBaseMenuWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta = (BindWidget))
	class UWidget* MainMenu;
	
	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;
	
	UPROPERTY(meta = (BindWidget))
	class UHostMenu* HostMenu;

	UPROPERTY(meta = (BindWidget))
	class UFindServersMenu* FindServersMenu;

	UPROPERTY(meta = (BindWidget))
	class UButton* ServersButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* OptionsButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* AboutButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* BackToMainMenuButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* QuitButton;


	UPROPERTY(meta = (BindWidget))
	class UWidgetSwitcher* MenuSwitcher;

	UFUNCTION()
	void OpenServersMenu();

	UFUNCTION()
	void OpenOptionsMenu();

	UFUNCTION()
	void OpenAboutMenu();

	UFUNCTION()
	void QuitGame();



	

protected:
	bool Initialize() override;

public:
	UMainMenu(const FObjectInitializer & ObjectInitializer);

	UFUNCTION()
	void OpenMainMenu();

	UFUNCTION()
	void OpenHostMenu();

	void SetAvailableServerList(TArray<FServerData> AvailableServersData);
};
