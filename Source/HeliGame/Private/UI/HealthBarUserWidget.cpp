// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HealthBarUserWidget.h"
#include "HeliGame.h"
#include "Helicopter.h"


UHealthBarUserWidget::UHealthBarUserWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{		
}

void UHealthBarUserWidget::SetupWidget()
{
	SetupCurrentColor();

	if (OwningPawn.IsValid())
	{
		PlayerName = OwningPawn->GetPlayerName().ToString();

		AHelicopter* myPawn = Cast<AHelicopter>(GetOwningPlayerPawn());
		if (myPawn && (myPawn != OwningPawn))
		{
			float distance = (myPawn->GetActorLocation() - OwningPawn->GetActorLocation()).Size();
			float scaleRation = 1.f / distance * 1000;

			FVector2D newSize = FVector2D(scaleRation, scaleRation);
			SetRenderScale(newSize);
		}
	}
}


void UHealthBarUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);	

	if(CurrentColor != FriendColor && CurrentColor != EnemyColor )
	{ 
		SetupWidget();
	}

	// scale with distance
	if (OwningPawn.IsValid())
	{
		AHelicopter* myPawn = Cast<AHelicopter>(GetOwningPlayerPawn());
		if (myPawn && (myPawn != OwningPawn))
		{
			float distance = (myPawn->GetActorLocation() - OwningPawn->GetActorLocation()).Size();
			float scaleRation = 1.f / distance * 1000;			
			
			FVector2D newSize = FVector2D(scaleRation, scaleRation);
			SetRenderScale(newSize);
		}
	}
}

void UHealthBarUserWidget::SetupCurrentColor()
{
	AHelicopter* MyPawn = Cast<AHelicopter>(GetOwningPlayerPawn());
	if (MyPawn && OwningPawn.IsValid())
	{
		if (OwningPawn == MyPawn)
		{
			CurrentColor = OwnColor;
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("UHealthBarUserWidget::SetCurrentColor ~ MyPawn %s team %d  Vs OwningPawn %s team %d"), *MyPawn->GetName(), MyPawn->GetTeamNumber(), *OwningPawn->GetName(), OwningPawn->GetTeamNumber());				
			//UE_LOG(LogTemp, Warning, TEXT("MyPawn LogNetRole:")); MyPawn->LogNetRole();
			//UE_LOG(LogTemp, Warning, TEXT("OwningPawn LogNetRole:")); OwningPawn->LogNetRole();

			if (MyPawn->GetTeamNumber() == OwningPawn->GetTeamNumber())
			{
				CurrentColor = FriendColor;
			}
			else
			{
				CurrentColor = EnemyColor;
			}
		}
	}
}

float UHealthBarUserWidget::GetCurrentHealth()
{
	if (OwningPawn.IsValid())
	{		
		return OwningPawn->GetHealthPercent();
	}

	return 0.f;	
}

void UHealthBarUserWidget::SetOwningPawn(TWeakObjectPtr<AHelicopter> InOwningPawn)
{
	OwningPawn = InOwningPawn;
}

FName UHealthBarUserWidget::GetPlayerName()
{
	if(OwningPawn.IsValid())
	{
		return OwningPawn->GetPlayerName();
	}

	return FName(TEXT("unknown"));
}