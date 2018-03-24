// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MenuInterface.h"
#include "BaseMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API UBaseMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void Setup();
	void Teardown();	
	
	void SetMenuInterface(IMenuInterface* MenuInterface);

	IMenuInterface* GetMenuInterface();
protected:
	IMenuInterface* MenuInterface;
};
