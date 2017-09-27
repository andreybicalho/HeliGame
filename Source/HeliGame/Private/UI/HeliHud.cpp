// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliHud.h"
#include "HeliGame.h"
#include "HeliPlayerController.h"
#include "Helicopter.h"
#include "HeliGameState.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/GameMode.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"


#define LOCTEXT_NAMESPACE "HeliGame.HUD.Menu"

namespace EHeliCrosshairDirection
{
	enum Type
	{
		Left = 0,
		Right = 1,
		Top = 2,
		Bottom = 3,
		Center = 4
	};
}


AHeliHud::AHeliHud()
{
	// Set the crosshair texture
	static ConstructorHelpers::FObjectFinder<UTexture2D> HUDMainTextureOb(TEXT("/Game/HeliBattle/UI/Textures/HUDMain"));
	HUDMainTexture = HUDMainTextureOb.Object;

	// getting drawings from texture for the crosshair
	Crosshair[EHeliCrosshairDirection::Left] = UCanvas::MakeIcon(HUDMainTexture, 43, 402, 25, 9); // left
	Crosshair[EHeliCrosshairDirection::Right] = UCanvas::MakeIcon(HUDMainTexture, 88, 402, 25, 9); // right
	Crosshair[EHeliCrosshairDirection::Top] = UCanvas::MakeIcon(HUDMainTexture, 74, 371, 9, 25); // top
	Crosshair[EHeliCrosshairDirection::Bottom] = UCanvas::MakeIcon(HUDMainTexture, 74, 415, 9, 25); // bottom
	Crosshair[EHeliCrosshairDirection::Center] = UCanvas::MakeIcon(HUDMainTexture, 75, 403, 7, 7); // center
	
	// getting drawings from texture for the crosshair notify
	HitNotifyCrosshair = UCanvas::MakeIcon(HUDMainTexture, 54, 453, 50, 50);

	// set the spread distance among primitive textures for the crosshair
	CrossSpread = 30.f;

	CrosshairOn = false;
	DotCrosshairOn = false;

	// set defaul color for the crosshair
	CrosshairColor = { 255, 255, 255, 192 };

	CrosshairHitNotifyColor = CrosshairColor;

	AimDistanceForDeprojectionOfCrosshair = 30000.f;

	LastEnemyHitDisplayTime = 0.2f;
	LastEnemyHitTime = -LastEnemyHitDisplayTime;

	static ConstructorHelpers::FObjectFinder<UTexture2D> HUDCenterDotObj(TEXT("/Game/HeliBattle/UI/Textures/T_CenterDot_M.T_CenterDot_M"));
	CenterDotIcon = UCanvas::MakeIcon(HUDCenterDotObj.Object);	


	static ConstructorHelpers::FObjectFinder<UFont> BigFontOb(TEXT("/Game/HeliBattle/UI/Roboto51"));
	static ConstructorHelpers::FObjectFinder<UFont> NormalFontOb(TEXT("/Game/HeliBattle/UI/Roboto18"));

	BigFont = BigFontOb.Object;
	NormalFont = NormalFontOb.Object;
}

void AHeliHud::BeginPlay()
{
	Super::BeginPlay();
	
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(GetOwningPlayerController());
	if (MyPC)
	{
		// enables input from player
		this->EnableInput(MyPC);
	}
	

}

void AHeliHud::EnableFirstPersonHud()
{
	CrosshairOn = true;
	DotCrosshairOn = false;	

	if (HeliHudWidget.IsValid())
	{
		if (!HeliHudWidget->IsInViewport())
		{
			HeliHudWidget->AddToViewport();
		}
	}
	else
	{
		AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(GetOwningPlayerController());
		if (HeliHudWidgetTemplate && MyPC)
		{
			HeliHudWidget = CreateWidget<UUserWidget>(MyPC, HeliHudWidgetTemplate);
			HeliHudWidget->AddToViewport();
		}

	}
}

void AHeliHud::DisableFirstPersonHud()
{
	CrosshairOn = false;
	DotCrosshairOn = false;	

	if (HeliHudWidget.IsValid() && HeliHudWidget->IsInViewport())
	{
		HeliHudWidget->RemoveFromViewport();
	}	
}

void AHeliHud::DrawHUD()
{
	Super::DrawHUD();	


	// crosshair
	if (CrosshairOn)
	{
		DrawCrosshair();
	}

	// if (DotCrosshairOn)
	// {
	// 	DrawCenterDot();
	// }

	// match and warmup timers
	DrawWarmupMatchTimers();


}


/************************************************************************ /
/*                             Crosshair                                */
/************************************************************************/
void AHeliHud::DrawCrosshair()
{		
	/** UI scaling factor for other resolutions than Full HD. */
	float ScaleUI = Canvas->ClipY / 1080.0f;

	float CenterX = Canvas->ClipX / 2;
	float CenterY = Canvas->ClipY / 2;
	//Canvas->SetDrawColor(255, 255, 255, 192);
	Canvas->SetDrawColor(CrosshairColor);

	

	// center
	Canvas->DrawIcon(Crosshair[EHeliCrosshairDirection::Center],
		CenterX - (Crosshair[EHeliCrosshairDirection::Center]).UL*ScaleUI / 2.0f,
		CenterY - (Crosshair[EHeliCrosshairDirection::Center]).VL*ScaleUI / 2.0f, ScaleUI);


	// left
	Canvas->DrawIcon(Crosshair[EHeliCrosshairDirection::Left],
		CenterX - 1 - (Crosshair[EHeliCrosshairDirection::Left]).UL * ScaleUI - CrossSpread * ScaleUI,
		CenterY - (Crosshair[EHeliCrosshairDirection::Left]).VL*ScaleUI / 2.0f, ScaleUI);
	// right
	Canvas->DrawIcon(Crosshair[EHeliCrosshairDirection::Right],
		CenterX + CrossSpread * ScaleUI,
		CenterY - (Crosshair[EHeliCrosshairDirection::Right]).VL * ScaleUI / 2.0f, ScaleUI);


	// top
	Canvas->DrawIcon(Crosshair[EHeliCrosshairDirection::Top],
		CenterX - (Crosshair[EHeliCrosshairDirection::Top]).UL * ScaleUI / 2.0f,
		CenterY - 1 - (Crosshair[EHeliCrosshairDirection::Top]).VL * ScaleUI - CrossSpread * ScaleUI, ScaleUI);
	// bottom
	Canvas->DrawIcon(Crosshair[EHeliCrosshairDirection::Bottom],
		CenterX - (Crosshair[EHeliCrosshairDirection::Bottom]).UL * ScaleUI / 2.0f,
		CenterY + CrossSpread * ScaleUI, ScaleUI);

	


	// crosshair hit notify
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastEnemyHitTime >= 0 && CurrentTime - LastEnemyHitTime <= LastEnemyHitDisplayTime)
	{
		// fade out
		const float Alpha = FMath::Min(1.0f, 1 - (CurrentTime - LastEnemyHitTime) / LastEnemyHitDisplayTime);
		CrosshairHitNotifyColor.A = 255*Alpha;
		Canvas->SetDrawColor(CrosshairHitNotifyColor);

		// draw
		/** @param UL The width of the portion of the texture to be drawn(texels).
			@param VL The height of the portion of the texture to be drawn(texels).
		*/
		Canvas->DrawIcon(HitNotifyCrosshair,
				CenterX - HitNotifyCrosshair.UL*ScaleUI / 2.0f,
				CenterY - HitNotifyCrosshair.VL*ScaleUI / 2.0f, ScaleUI);
		
	}


	// TODO: notify hit points

	

	Canvas->SetDrawColor(CrosshairColor);
}


void AHeliHud::DrawCenterDot()
{
	/** UI scaling factor for other resolutions than Full HD. */
	float ScaleUI = Canvas->ClipY / 1080.0f;

	float CenterX = Canvas->ClipX / 2;
	float CenterY = Canvas->ClipY / 2;

	float CenterDotScale = 0.07f;

	Canvas->SetDrawColor(CrosshairColor);

	Canvas->DrawIcon(CenterDotIcon,
				CenterX - CenterDotIcon.UL*CenterDotScale / 2.0f,
				CenterY - CenterDotIcon.VL*CenterDotScale / 2.0f, CenterDotScale);


	// crosshair hit notify
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastEnemyHitTime >= 0 && CurrentTime - LastEnemyHitTime <= LastEnemyHitDisplayTime)
	{
		// fade out
		const float Alpha = FMath::Min(1.0f, 1 - (CurrentTime - LastEnemyHitTime) / LastEnemyHitDisplayTime);
		CrosshairHitNotifyColor.A = 255 * Alpha;
		Canvas->SetDrawColor(CrosshairHitNotifyColor);

		// draw
		/** @param UL The width of the portion of the texture to be drawn(texels).
		@param VL The height of the portion of the texture to be drawn(texels).
		*/
		Canvas->DrawIcon(HitNotifyCrosshair,
			CenterX - HitNotifyCrosshair.UL*ScaleUI / 2.0f,
			CenterY - HitNotifyCrosshair.VL*ScaleUI / 2.0f, ScaleUI);
	}
		
	
}

FVector AHeliHud::FindCrossHairPositionIn3DSpace()
{
	FVector WorldDirectionOfCrossHair2D = FVector::ZeroVector;

	FVector CrossHair3DPos = FVector::ZeroVector;

	// find center of the screen/Canvas
	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);

	Canvas->Deproject(Center, CrossHair3DPos, WorldDirectionOfCrossHair2D);

	//moves the cursor pos into full 3D, your chosen distance ahead of player
	CrossHair3DPos += WorldDirectionOfCrossHair2D * AimDistanceForDeprojectionOfCrosshair;

	return CrossHair3DPos;
}

void AHeliHud::NotifyEnemyHit()
{
	LastEnemyHitTime = GetWorld()->GetTimeSeconds();
}

/************************************************************************ /
/*                 Scoreboard                                           */
/************************************************************************/

void AHeliHud::ShowScoreboard()
{
	if (ScoreboardWidget.IsValid())
	{
		if (!ScoreboardWidget->IsInViewport())
		{
			ScoreboardWidget->AddToViewport();
		}
	}
	else
	{
		AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(GetOwningPlayerController());
		if (ScoreboardWidgetTemplate && MyPC)
		{			
			ScoreboardWidget = CreateWidget<UUserWidget>(MyPC, ScoreboardWidgetTemplate);
			ScoreboardWidget->AddToViewport();
		}

	}
}

void AHeliHud::HideScoreboard()
{
	if (ScoreboardWidget.IsValid())
	{
		if (ScoreboardWidget->IsInViewport())
		{
			ScoreboardWidget->RemoveFromViewport();
		}
	}
}

void AHeliHud::ShowInGameMenu()
{
	AHeliPlayerController* MyPC = Cast<AHeliPlayerController>(GetOwningPlayerController());
	
	if (InGameMenuWidget.IsValid())
	{		
		if (!InGameMenuWidget->IsInViewport() && MyPC)
		{
			InGameMenuWidget->AddToViewport();
			InGameMenuWidget->SetUserFocus(MyPC);

			MyPC->SetIgnoreLookInput(true);
			MyPC->SetIgnoreMoveInput(true);
			MyPC->SetAllowGameActions(false);
			MyPC->bShowMouseCursor = true;
		}
	}
	else
	{
		if (InGameMenuWidgetTemplate && MyPC)
		{			
			InGameMenuWidget = CreateWidget<UUserWidget>(MyPC, InGameMenuWidgetTemplate);
			InGameMenuWidget->AddToViewport();
			InGameMenuWidget->SetUserFocus(MyPC);

			MyPC->SetIgnoreLookInput(true);
			MyPC->SetIgnoreMoveInput(true);
			MyPC->SetAllowGameActions(false);
			MyPC->bShowMouseCursor = true;
		}
	}
}

/************************************************************************ /
/*                                                                      */
/************************************************************************/
EHeliGameMatchState::Type AHeliHud::GetMatchState() const
{
	return MatchState;
}

void AHeliHud::SetMatchState(EHeliGameMatchState::Type NewState)
{
	MatchState = NewState;
}

bool AHeliHud::IsMatchOver() const
{
	return GetMatchState() == EHeliGameMatchState::Lost || GetMatchState() == EHeliGameMatchState::Won;
}


/************************************************************************ /
/*                    Round and Warmup Timers                           */
/************************************************************************/
FString AHeliHud::GetTimeString(float TimeSeconds)
{
	// only minutes and seconds are relevant
	const int32 TotalSeconds = FMath::Max(0, FMath::TruncToInt(TimeSeconds) % 3600);
	const int32 NumMinutes = TotalSeconds / 60;
	const int32 NumSeconds = TotalSeconds % 60;

	const FString TimeDesc = FString::Printf(TEXT("%02d:%02d"), NumMinutes, NumSeconds);
	return TimeDesc;
}

void AHeliHud::DrawWarmupMatchTimers()
{
	AHeliGameState* const MyGameState = Cast<AHeliGameState>(GetWorld()->GetGameState());

	// match timer
	if (MyGameState && MyGameState->RemainingTime > 0)
	{
		FColor HUDDark = FColor(110, 124, 131, 255);
		FColor HUDLight = FColor(175, 202, 213, 255);
		/** FontRenderInfo enabling casting shadow.s */
		FFontRenderInfo ShadowedFont;
		ShadowedFont.bEnableShadow = true;
		float ScaleUI = Canvas->ClipY / 1080.0f;

		FCanvasTextItem TextItem(FVector2D::ZeroVector, FText::GetEmpty(), BigFont, HUDDark);
		TextItem.EnableShadow(FLinearColor::Black);
		
		
		float TextScale = 0.57f;
		FString Text;
		TextItem.FontRenderInfo = ShadowedFont;
		TextItem.Scale = FVector2D(TextScale*ScaleUI, TextScale*ScaleUI);
		// warmup timer
		if (MyGameState->GetMatchState() == MatchState::WaitingToStart)
		{
			TextItem.Scale = FVector2D(ScaleUI, ScaleUI);
			Text = LOCTEXT("WarmupString", "MATCH STARTS IN: ").ToString() + FString::FromInt(MyGameState->RemainingTime);
			TextItem.SetColor(HUDLight);
			TextItem.Text = FText::FromString(Text);

			float SizeX, SizeY;
			Canvas->StrLen(TextItem.Font, TextItem.Text.ToString(), SizeX, SizeY);
			float Y = (Canvas->ClipY / 4.0)* ScaleUI;
			float CanvasCentre = Canvas->ClipX / 2.0f;
			float X = CanvasCentre - (SizeX * TextItem.Scale.X) / 2.0f;
			Canvas->DrawItem(TextItem, X, Y);
		}
	}
}

#undef LOCTEXT_NAMESPACE