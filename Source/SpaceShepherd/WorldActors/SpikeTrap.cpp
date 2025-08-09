// SpikeTrap.cpp
#include "SpikeTrap.h"
#include "CowsAI/CowCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "CowsAI/CowCharacter.h"

ASpikeTrap::ASpikeTrap()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Configure base trap settings
    ActivationDelay = 1.0f; // Default 1 second delay
    CooldownDuration = 3.0f;
    bSingleUse = false;
    
    // Create spike mesh
    SpikeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpikeMesh"));
    SpikeMesh->SetupAttachment(Root);
    SpikeMesh->SetRelativeLocation(FVector(0, 0, -SpikeMaxHeight)); // Start retracted
    SpikeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    // Create warning indicator mesh
    WarningIndicatorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WarningIndicator"));
    WarningIndicatorMesh->SetupAttachment(Root);
    WarningIndicatorMesh->SetVisibility(false);
    WarningIndicatorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASpikeTrap::BeginPlay()
{
    Super::BeginPlay();
    
    // Create dynamic materials
    if (SpikeMesh && SpikeMesh->GetMaterial(0))
    {
        SpikeDynamicMaterial = SpikeMesh->CreateAndSetMaterialInstanceDynamic(0);
    }
    
    if (WarningIndicatorMesh && WarningIndicatorMesh->GetMaterial(0))
    {
        WarningDynamicMaterial = WarningIndicatorMesh->CreateAndSetMaterialInstanceDynamic(0);
    }
    
    // Initialize spike position
    CurrentSpikeHeight = 0.0f;
    TargetSpikeHeight = 0.0f;
    UpdateSpikePosition(0.0f);
}

void ASpikeTrap::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Update spike position if moving
    if (bSpikesExtending || bSpikesRetracting)
    {
        UpdateSpikePosition(DeltaTime);
    }
    
    // Update warning visuals
    if (bInWarningPhase)
    {
        UpdateWarningVisuals(DeltaTime);
    }
    
    // Debug visualization
    if (bShowDebugVisuals && CurrentState == ETrapState::Active)
    {
        // Show kill zone
        DrawDebugBox(GetWorld(), GetActorLocation() + FVector(0, 0, CurrentSpikeHeight/2), 
                    TriggerVolume->GetScaledBoxExtent(), GetActorQuat(), 
                    FColor::Red, false, -1, 0, 3.0f);
        
        // Show spike height
        DrawDebugLine(GetWorld(), GetActorLocation(), 
                     GetActorLocation() + FVector(0, 0, CurrentSpikeHeight), 
                     FColor::Red, false, -1, 0, 5.0f);
    }
}

void ASpikeTrap::OnTrigger(AActor* TriggeringActor)
{
    if (CurrentState != ETrapState::Armed)
        return;
    
    // Start warning phase if enabled
    if (bShowWarning && WarningDuration > 0.0f)
    {
        StartWarningPhase();
    }
    
    // Call base implementation
    Super::OnTrigger(TriggeringActor);
}

void ASpikeTrap::OnActivate()
{
    Super::OnActivate();
    
    // End warning phase
    EndWarningPhase();
    
    // Extend spikes
    ExtendSpikes();
    
    // Set timer for spike retraction
    if (SpikeActiveDuration > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(SpikeActiveTimerHandle, this, 
            &ASpikeTrap::OnSpikeActiveDurationComplete, SpikeActiveDuration, false);
    }
}

void ASpikeTrap::OnDeactivate()
{
    // Retract spikes
    RetractSpikes();
    
    // Clear killed cows list for next activation
    KilledCows.Empty();
    
    Super::OnDeactivate();
}

void ASpikeTrap::StartWarningPhase()
{
    bInWarningPhase = true;
    WarningPhaseTime = 0.0f;
    
    // Show warning indicator
    if (WarningIndicatorMesh)
    {
        WarningIndicatorMesh->SetVisibility(true);
    }
    
    // Play warning sound
    if (WarningSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, WarningSound, GetActorLocation());
    }
    
    // Set the actual activation delay to match warning duration
    if (WarningDuration > ActivationDelay)
    {
        ActivationDelay = WarningDuration;
    }
}

void ASpikeTrap::EndWarningPhase()
{
    bInWarningPhase = false;
    WarningPhaseTime = 0.0f;
    
    // Hide warning indicator
    if (WarningIndicatorMesh)
    {
        WarningIndicatorMesh->SetVisibility(false);
    }
}

void ASpikeTrap::ExtendSpikes()
{
    bSpikesExtending = true;
    bSpikesRetracting = false;
    TargetSpikeHeight = SpikeMaxHeight;
    
    // Play extend sound
    if (SpikeExtendSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, SpikeExtendSound, GetActorLocation());
    }
}

void ASpikeTrap::RetractSpikes()
{
    bSpikesRetracting = true;
    bSpikesExtending = false;
    TargetSpikeHeight = 0.0f;
    
    // Play retract sound
    if (SpikeRetractSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, SpikeRetractSound, GetActorLocation());
    }
}

void ASpikeTrap::KillCowsOnSpikes()
{
    TArray<ACowCharacter*> CowsToKill = GetCowsInTrigger();
    
    for (ACowCharacter* Cow : CowsToKill)
    {
        if (!Cow || KilledCows.Contains(Cow))
            continue;
        
        // Mark as killed to avoid double-killing
        KilledCows.Add(Cow);
        
        // Play impale sound
        if (ImpaleSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, ImpaleSound, Cow->GetActorLocation());
        }
        
        // Spawn blood effect at cow location
        FVector EffectLocation = Cow->GetActorLocation();
        if (BloodSplatterNiagaraEffect)
        {
            /*UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BloodSplatterNiagaraEffect, 
                EffectLocation, FRotator::ZeroRotator);*/
        }
        else if (BloodSplatterEffect)
        {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BloodSplatterEffect, 
                EffectLocation, FRotator::ZeroRotator);
        }
        
        // Broadcast kill event
        OnCowKilled.Broadcast(this, Cow);
        
        // Destroy the cow
        Cow->Destroy();
        
        UE_LOG(LogTemp, Warning, TEXT("SpikeTrap killed cow: %s"), *Cow->GetName());
    }
}

void ASpikeTrap::UpdateSpikePosition(float DeltaTime)
{
    if (!SpikeMesh)
        return;
    
    // Calculate new spike height
    if (bSpikesExtending)
    {
        CurrentSpikeHeight = FMath::FInterpTo(CurrentSpikeHeight, TargetSpikeHeight, 
                                              DeltaTime, SpikeExtendSpeed / SpikeMaxHeight);
        
        // Check if fully extended
        if (FMath::IsNearlyEqual(CurrentSpikeHeight, TargetSpikeHeight, 1.0f))
        {
            CurrentSpikeHeight = TargetSpikeHeight;
            bSpikesExtending = false;
            
            // Kill any cows on the spikes
            KillCowsOnSpikes();
        }
    }
    else if (bSpikesRetracting)
    {
        CurrentSpikeHeight = FMath::FInterpTo(CurrentSpikeHeight, TargetSpikeHeight, 
                                              DeltaTime, SpikeRetractSpeed / SpikeMaxHeight);
        
        // Check if fully retracted
        if (FMath::IsNearlyEqual(CurrentSpikeHeight, TargetSpikeHeight, 1.0f))
        {
            CurrentSpikeHeight = TargetSpikeHeight;
            bSpikesRetracting = false;
        }
    }
    
    // Update spike mesh position
    FVector NewLocation = FVector(0, 0, CurrentSpikeHeight - SpikeMaxHeight);
    SpikeMesh->SetRelativeLocation(NewLocation);
    
    // Check for cows to kill while spikes are extended
    if (CurrentSpikeHeight > SpikeMaxHeight * 0.5f && CurrentState == ETrapState::Active)
    {
        KillCowsOnSpikes();
    }
}

void ASpikeTrap::UpdateWarningVisuals(float DeltaTime)
{
    if (!bInWarningPhase)
        return;
    
    WarningPhaseTime += DeltaTime;
    
    // Pulsing effect for warning
    float PulseAlpha = (FMath::Sin(WarningPhaseTime * WarningPulseSpeed) + 1.0f) * 0.5f;
    
    if (WarningDynamicMaterial)
    {
        FLinearColor PulsedColor = FLinearColor::LerpUsingHSV(
            FLinearColor(0.5f, 0.0f, 0.0f), 
            WarningColor, 
            PulseAlpha
        );
        
        WarningDynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), PulsedColor * 3.0f);
        WarningDynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), PulseAlpha);
    }
    
    // Also pulse the ground indicator if using the base mesh
    if (DynamicMaterial)
    {
        FLinearColor FlashColor = FLinearColor::LerpUsingHSV(ArmedColor, WarningColor, PulseAlpha);
        DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), FlashColor * 2.0f);
    }
}

void ASpikeTrap::OnSpikeActiveDurationComplete()
{
    // Start deactivation
    OnDeactivate();
}