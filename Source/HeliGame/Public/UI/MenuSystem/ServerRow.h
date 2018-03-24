// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ServerRow.generated.h"

/**
 * 
 */
UCLASS()
class HELIGAME_API UServerRow : public UUserWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ServerName;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MapName;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* GameModeName;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* NumberOfPlayersFraction;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Ping;



	uint32 Index;

	UPROPERTY(meta = (BindWidget))
	class UButton* RowButton;

	UFUNCTION()
	void OnClicked();

	UPROPERTY()
	class UFindServersMenu* Parent;

public:
	void Setup(class UFindServersMenu* Parent, uint32 Index);

	void SetServerName(FString InServerName);

	void SetMapName(FString InMapName);

	void SetGameModeName(FString InGameModeName);

	void SetNumberOfPlayersFraction(FString InNumberOfPlayersFraction);

	void SetPing(FString InPing);
		
	UPROPERTY(BlueprintReadOnly)
	bool Selected = false;
};
