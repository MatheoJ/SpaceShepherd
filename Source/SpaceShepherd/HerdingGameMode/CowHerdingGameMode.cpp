// CowHerdingGameMode.cpp
#include "CowHerdingGameMode.h"
#include "CowHerdingHUD.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

ACowHerdingGameMode::ACowHerdingGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Set default HUD class (we'll create this next)
    HUDClass = ACowHerdingHUD::StaticClass();
    
    // Initialize variables
    RemainingTime = GameDuration;
    CurrentCowsInVolume = 0;
    bGameActive = false;
}

void ACowHerdingGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // Auto-start the game after a short delay
    FTimerHandle StartDelayHandle;
    GetWorldTimerManager().SetTimer(StartDelayHandle, this, &ACowHerdingGameMode::StartGame, 3.0f, false);
}

void ACowHerdingGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Manual timer update if needed (backup for timer)
    if (bGameActive && RemainingTime > 0)
    {
        // Timer is handled by FTimerHandle, but this is a safety check
    }
}

void ACowHerdingGameMode::StartGame()
{
    if (bGameActive)
    {
        return;
    }
    
    bGameActive = true;
    RemainingTime = GameDuration;
    CurrentCowsInVolume = 0;
    CowsInVolume.Empty();
    
    // Broadcast initial state
    OnTimeUpdated.Broadcast(RemainingTime);
    OnCowCountChanged.Broadcast(CurrentCowsInVolume);
    
    // Start the game timer
    GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ACowHerdingGameMode::UpdateTimer, 1.0f, true);
    
    UE_LOG(LogTemp, Warning, TEXT("Cow Herding Game Started! You have %.0f seconds!"), GameDuration);
}

void ACowHerdingGameMode::EndGame()
{
    if (!bGameActive)
    {
        return;
    }
    
    bGameActive = false;
    
    // Stop the timer
    GetWorldTimerManager().ClearTimer(GameTimerHandle);
    
    // Broadcast game ended with final score
    OnGameEnded.Broadcast(CurrentCowsInVolume);
    
    UE_LOG(LogTemp, Warning, TEXT("Game Ended! Final Score: %d cows"), CurrentCowsInVolume);
}

void ACowHerdingGameMode::PauseGame()
{
    if (bGameActive)
    {
        GetWorldTimerManager().PauseTimer(GameTimerHandle);
        UGameplayStatics::SetGamePaused(GetWorld(), true);
    }
}

void ACowHerdingGameMode::ResumeGame()
{
    if (bGameActive)
    {
        GetWorldTimerManager().UnPauseTimer(GameTimerHandle);
        UGameplayStatics::SetGamePaused(GetWorld(), false);
    }
}

void ACowHerdingGameMode::RegisterCowInVolume(AActor* Cow)
{
    if (!bGameActive || !Cow)
    {
        return;
    }
    
    // Check if this cow is already registered
    if (!CowsInVolume.Contains(Cow))
    {
        CowsInVolume.Add(Cow);
        CurrentCowsInVolume = CowsInVolume.Num();
        OnCowCountChanged.Broadcast(CurrentCowsInVolume);
        
        UE_LOG(LogTemp, Log, TEXT("Cow entered volume. Total: %d"), CurrentCowsInVolume);
    }
}

void ACowHerdingGameMode::UnregisterCowFromVolume(AActor* Cow)
{
    if (!bGameActive || !Cow)
    {
        return;
    }
    
    // Remove cow if it was in the set
    if (CowsInVolume.Remove(Cow) > 0)
    {
        CurrentCowsInVolume = CowsInVolume.Num();
        OnCowCountChanged.Broadcast(CurrentCowsInVolume);
        
        UE_LOG(LogTemp, Log, TEXT("Cow left volume. Total: %d"), CurrentCowsInVolume);
    }
}

void ACowHerdingGameMode::UpdateTimer()
{
    if (!bGameActive)
    {
        return;
    }
    
    RemainingTime -= 1.0f;
    OnTimeUpdated.Broadcast(RemainingTime);
    
    if (RemainingTime <= 0)
    {
        RemainingTime = 0;
        EndGame();
    }
}