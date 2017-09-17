// Copyright 2016 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliGame.h"
#include "HealthBarUserWidget.h"
#include "Helicopter.h"


UHealthBarUserWidget::UHealthBarUserWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}


void UHealthBarUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AHelicopter* MyPawn = Cast<AHelicopter>(GetOwningPlayerPawn());

	if ((MyPawn && OwningPawn.IsValid()) && (OwningPawn != MyPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("UHealthBarUserWidget::NativeTick ~ MyPawn %s - team %d VS OwningPawn %s - team %d"), *MyPawn->GetName(), MyPawn->GetTeamNumber(), *OwningPawn->GetName(), OwningPawn->GetTeamNumber());
	}

	if ((MyPawn && OwningPawn.IsValid()) && (OwningPawn == MyPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("UHealthBarUserWidget::NativeTick ~ MyPawn %s - team %d VS OwningPawn %s - team %d"), *MyPawn->GetName(), MyPawn->GetTeamNumber(), *OwningPawn->GetName(), OwningPawn->GetTeamNumber());
	}
}

void UHealthBarUserWidget::SetupCurrentColor()
{
	if(OwningPawn.IsValid())
	{				
		AHelicopter* MyPawn = Cast<AHelicopter>(GetOwningPlayerPawn());
		if(MyPawn)
		{ 			
			if(OwningPawn == MyPawn)
			{ 
				UE_LOG(LogTemp, Warning, TEXT("UHealthBarUserWidget::SetCurrentColor ~ MyPawn %s is the same OwningPawn %s, Team %d = %d"), *MyPawn->GetName(), *OwningPawn->GetName(), MyPawn->GetTeamNumber(), OwningPawn->GetTeamNumber());				
				CurrentColor = FLinearColor(0.f, 1.f, 0.f, 1.f);
			}
			else
			{ 
				UE_LOG(LogTemp, Warning, TEXT("UHealthBarUserWidget::SetCurrentColor ~ MyPawn %s team %d is different from OwningPawn %s team %d"), *MyPawn->GetName(), MyPawn->GetTeamNumber(), *OwningPawn->GetName(), OwningPawn->GetTeamNumber());
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
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not find MyPawn!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OwninPawn is NOT valid!"));
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