// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "BaseMenuWidget.h"

void UBaseMenuWidget::Setup()
{
	this->AddToViewport();	

	APlayerController* PlayerController = this->GetOwningPlayer();
	if (!ensure(PlayerController != nullptr)) return;

	FInputModeUIOnly InputModeData;
	InputModeData.SetWidgetToFocus(this->TakeWidget());
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PlayerController->SetInputMode(InputModeData);

	PlayerController->bShowMouseCursor = true;
}

void UBaseMenuWidget::Teardown()
{
	this->RemoveFromViewport();

	APlayerController* PlayerController = this->GetOwningPlayer();
	if (!ensure(PlayerController != nullptr)) return;

	FInputModeGameOnly InputModeData;
	PlayerController->SetInputMode(InputModeData);

	PlayerController->bShowMouseCursor = false;
}

void UBaseMenuWidget::SetMenuInterface(IMenuInterface* MenuInterface)
{
	this->MenuInterface = MenuInterface;
}

IMenuInterface* UBaseMenuWidget::GetMenuInterface()
{
	return MenuInterface;
}


