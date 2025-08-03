// PlayerShepherdComponent.cpp
#include "PlayerShepherdComponent.h"
#include "CowCharacter.h"
#include "CowBoidsComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"

UPlayerShepherdComponent::UPlayerShepherdComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerShepherdComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UPlayerShepherdComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Update nearby cows periodically
    UpdateTimer += DeltaTime;
    if (UpdateTimer >= UpdateInterval)
    {
        UpdateTimer = 0.0f;
        UpdateNearbyCows();
    }
    
    // Draw visual feedback
    if (bShowModeIndicator)
    {
        DrawModeIndicator();
    }
}

void UPlayerShepherdComponent::SetShepherdMode(EShepherdMode NewMode)
{
    if (CurrentMode != NewMode)
    {
        CurrentMode = NewMode;
        
        // Update all nearby cows immediately
        UpdateNearbyCows();
        
        // Broadcast mode change event
        OnModeChanged.Broadcast(CurrentMode);
    }
}

void UPlayerShepherdComponent::ToggleAttractionMode()
{
    if (CurrentMode == EShepherdMode::Attraction)
    {
        SetShepherdMode(EShepherdMode::Neutral);
    }
    else
    {
        SetShepherdMode(EShepherdMode::Attraction);
    }
}

void UPlayerShepherdComponent::ToggleRepulsionMode()
{
    if (CurrentMode == EShepherdMode::Repulsion)
    {
        SetShepherdMode(EShepherdMode::Neutral);
    }
    else
    {
        SetShepherdMode(EShepherdMode::Repulsion);
    }
}

void UPlayerShepherdComponent::SetNeutralMode()
{
    SetShepherdMode(EShepherdMode::Neutral);
}

void UPlayerShepherdComponent::HandleAttractionInput()
{
    ToggleAttractionMode();
}

void UPlayerShepherdComponent::HandleRepulsionInput()
{
    ToggleRepulsionMode();
}

void UPlayerShepherdComponent::UpdateNearbyCows()
{
    if (!GetOwner())
        return;
        
    // Clear previous cow states
    for (ACowCharacter* Cow : NearbyCows)
    {
        if (IsValid(Cow))
        {
            Cow->SetPlayerAttraction(false);
            Cow->SetPlayerRepulsion(false);
        }
    }
    
    NearbyCows.Empty();
    
    // Find all cows in the world
    for (TActorIterator<ACowCharacter> It(GetWorld()); It; ++It)
    {
        ACowCharacter* Cow = *It;
        if (!Cow)
            continue;
            
        // Check if cow has boids component and is within its detection radius
        UCowBoidsComponent* BoidsComp = Cow->FindComponentByClass<UCowBoidsComponent>();
        if (BoidsComp)
        {
            float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Cow->GetActorLocation());
            if (Distance <= BoidsComp->PlayerDetectionRadius)
            {
                NearbyCows.Add(Cow);
                
                // Set cow state based on current mode
                switch (CurrentMode)
                {
                    case EShepherdMode::Attraction:
                        Cow->SetPlayerAttraction(true);
                        break;
                    case EShepherdMode::Repulsion:
                        Cow->SetPlayerRepulsion(true);
                        break;
                    case EShepherdMode::Neutral:
                    default:
                        // Already cleared above
                        break;
                }
            }
        }
    }
}

void UPlayerShepherdComponent::DrawModeIndicator()
{
    if (!GetOwner())
        return;
        
    FVector Location = GetOwner()->GetActorLocation();
    FColor DrawColor = NeutralColor;
    
    switch (CurrentMode)
    {
        case EShepherdMode::Attraction:
            DrawColor = AttractionColor;
            break;
        case EShepherdMode::Repulsion:
            DrawColor = RepulsionColor;
            break;
        default:
            break;
    }
    
    // Draw indicator circle around player
    if (CurrentMode != EShepherdMode::Neutral)
    {        
        // Draw pulsing effect
        float PulseScale = 1.0f + FMath::Sin(GetWorld()->GetTimeSeconds() * 3.0f) * 0.1f;
        DrawDebugSphere(GetWorld(), Location, IndicatorRadius * PulseScale /3.0f, 16, DrawColor, false, -1, 0, 1);
    }
}