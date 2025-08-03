// CowHerdingGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CowHerdingGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeUpdated, float, RemainingTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCowCountChanged, int32, CowCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameEnded, int32, FinalScore);

UCLASS()
class ACowHerdingGameMode : public AGameModeBase
{
    GENERATED_BODY()
    
public:
    ACowHerdingGameMode();
    
    // Game Settings
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings")
    float GameDuration = 120.0f; // 2 minutes by default
    
    // Current Game State
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    float RemainingTime;
    
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 CurrentCowsInVolume;
    
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    bool bGameActive;
    
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnTimeUpdated OnTimeUpdated;
    
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCowCountChanged OnCowCountChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnGameEnded OnGameEnded;
    
    // Game Control Functions
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void StartGame();
    
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void EndGame();
    
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void PauseGame();
    
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void ResumeGame();
    
    // Cow Management
    UFUNCTION(BlueprintCallable, Category = "Cow Management")
    void RegisterCowInVolume(AActor* Cow);
    
    UFUNCTION(BlueprintCallable, Category = "Cow Management")
    void UnregisterCowFromVolume(AActor* Cow);
    
    UFUNCTION(BlueprintCallable, Category = "Cow Management")
    int32 GetCurrentCowCount() const { return CurrentCowsInVolume; }
    
    UFUNCTION(BlueprintCallable, Category = "Game State")
    float GetRemainingTime() const { return RemainingTime; }
    
protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    
private:
    // Timer handle for game timer
    FTimerHandle GameTimerHandle;
    
    // Track unique cows in volume
    UPROPERTY()
    TSet<AActor*> CowsInVolume;
    
    // Update the timer
    void UpdateTimer();
};