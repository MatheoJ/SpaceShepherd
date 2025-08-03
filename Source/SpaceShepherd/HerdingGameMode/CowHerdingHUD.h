// CowHerdingHUD.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CowHerdingHUD.generated.h"

UCLASS()
class ACowHerdingHUD : public AHUD
{
	GENERATED_BODY()
    
public:
	ACowHerdingHUD();
    
	// HUD Display Settings
	UPROPERTY(EditDefaultsOnly, Category = "HUD Settings")
	FLinearColor TimerColor = FLinearColor::White;
    
	UPROPERTY(EditDefaultsOnly, Category = "HUD Settings")
	FLinearColor CowCountColor = FLinearColor::Green;
    
	UPROPERTY(EditDefaultsOnly, Category = "HUD Settings")
	FLinearColor GameOverColor = FLinearColor::Red;
    
	UPROPERTY(EditDefaultsOnly, Category = "HUD Settings")
	float HUDScale = 1.0f;
    
	// Font settings
	UPROPERTY(EditDefaultsOnly, Category = "HUD Settings")
	class UFont* HUDFont;
    
protected:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;
    
private:
	// Cached references
	class ACowHerdingGameMode* GameModeRef;
    
	// HUD State
	float CurrentTime;
	int32 CurrentCowCount;
	bool bIsGameOver;
	int32 FinalScore;
    
	// Event handlers
	UFUNCTION()
	void OnTimeUpdated(float RemainingTime);
    
	UFUNCTION()
	void OnCowCountChanged(int32 CowCount);
    
	UFUNCTION()
	void OnGameEnded(int32 Score);
    
	// Drawing functions
	void DrawTimer();
	void DrawCowCount();
	void DrawGameOver();
	void DrawText(const FString& Text, FLinearColor Color, float X, float Y, float Scale = 1.0f);
};