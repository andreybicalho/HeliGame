// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MenuSystem/BaseMenuWidget.h"
#include "FindServersMenu.generated.h"

class UButton;

/**
 * 
 */
UCLASS()
class HELIGAME_API UFindServersMenu : public UBaseMenuWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta = (BindWidget))
	UButton* FindServersButton;
	
	UPROPERTY(meta = (BindWidget))
	UButton* JoinServerButton;

	UPROPERTY(meta = (BindWidget))
	UButton* ToggleLANButton;

	UPROPERTY(meta = (BindWidget))
	class UPanelWidget* ServerList;

	UFUNCTION()
	void ToggleLan();

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* LanText;

	bool bIsLAN = true;

	int32 SelectedServerIndex;

	TArray<FServerData> AvailableServers;

	TSubclassOf<class UUserWidget> ServerRowClass;

protected:
	bool Initialize() override;

	void RefreshServerList();
public:
	UFindServersMenu(const FObjectInitializer & ObjectInitializer);

	UFUNCTION()
	void FindServers();

	UFUNCTION()
	void JoinServer();

	void SetSelectServerIndex(int32 ServerIndex);
	
	void SetAvailableServerList(TArray<FServerData> AvailableServersData);

};
