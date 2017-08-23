// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/HUD.h"
#include "HeliHud.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API AHeliHud : public AHUD
{
	GENERATED_BODY()		

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
		float AimDistanceForDeprojectionOfCrosshair;

public:
	AHeliHud();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Crosshair)
		float CrossSpread;

	// whether draw the crosshair
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Crosshair)
		bool CrosshairOn;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Crosshair)
		bool DotCrosshairOn;

	// crosshair color
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Crosshair)
		FColor CrosshairColor;

	// hit notify crosshair color
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Crosshair)
		FColor CrosshairHitNotifyColor;

	/** Notifies we have hit the enemy. */
	void NotifyEnemyHit();

	/***************************************************************************************
	*                                       Heli HUD Widget                                *
	****************************************************************************************/	
	// Reference UMG Asset in the Editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUDHeliInfo)
		TSubclassOf<class UUserWidget> HeliHudWidgetTemplate; // assigned in blueprint

	// heli hud widget
	//UUserWidget *HeliHudWidget; // that's ok, but
	TWeakObjectPtr<UUserWidget> HeliHudWidget; // smart pointers are safer

	/***************************************************************************************
	*                                      Scoreboard                                      *
	****************************************************************************************/
	// Reference UMG Asset in the Editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Scoreboard)
		TSubclassOf<class UUserWidget> ScoreboardWidgetTemplate; // assigned in blueprint

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InGameMenu)
		TSubclassOf<class UUserWidget> InGameMenuWidgetTemplate;

	TWeakObjectPtr<UUserWidget> ScoreboardWidget;

	TWeakObjectPtr<UUserWidget> InGameMenuWidget;

	void ShowScoreboard();

	void HideScoreboard();

	void ShowInGameMenu();

	/************************************************************************ /
	/*                                                                      */
	/************************************************************************/

	/** Get state of current match. */
	EHeliGameMatchState::Type GetMatchState() const;
	/**
	* Set state of current match.
	*
	* @param	NewState	The new match state.
	*/
	void SetMatchState(EHeliGameMatchState::Type NewState);

	/* Is the match over (IE Is the state Won or Lost). */
	bool IsMatchOver() const;

	void EnableFirstPersonHud();

	void DisableFirstPersonHud();

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;	

protected:
	/** Large font - used for ammo display etc. */
	UPROPERTY()
		UFont* BigFont;

	/** Normal font - used for death messages and such. */
	UPROPERTY()
		UFont* NormalFont;

	/************************************************************************ /
	/*                             Crosshair                                */
	/************************************************************************/
	/** texture for HUD elements. */
	UPROPERTY()
	class UTexture2D* HUDMainTexture;

	/** Crosshair icons (left, top, right, bottom and center). */
	UPROPERTY()
		FCanvasIcon Crosshair[5];

	FCanvasIcon CenterDotIcon;

	/** On crosshair indicator that we hit someone. */
	UPROPERTY()
		FCanvasIcon HitNotifyCrosshair;

	/** Draws weapon crosshair. */
	void DrawCrosshair();

	void DrawCenterDot();

	/** When we last time hit the enemy. */
	float LastEnemyHitTime;

	/** How long till enemy hit notify fades out completely. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
		float LastEnemyHitDisplayTime;


	/************************************************************************ /
	/*                                                                      */
	/************************************************************************/
	/** State of match. */
	EHeliGameMatchState::Type MatchState;


	/************************************************************************ /
	/*                    Round and Warmup Timers                           */
	/************************************************************************/
	FString GetTimeString(float TimeSeconds);

	void DrawWarmupMatchTimers();
public:
	void DebugInfo();

	FVector FindCrossHairPositionIn3DSpace();
};
