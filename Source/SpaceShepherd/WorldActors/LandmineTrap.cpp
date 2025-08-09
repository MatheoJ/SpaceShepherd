// LandmineTrap.cpp
#include "LandmineTrap.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Camera/CameraShakeBase.h"
#include "CowsAI/CowBoidsComponent.h"
#include "CowsAI/CowCharacter.h"
#include "Engine/OverlapResult.h"

ALandmineTrap::ALandmineTrap()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Configure base trap settings
    ActivationDelay = 0.1f; // Almost instant explosion after trigger
    CooldownDuration = 5.0f;
    bSingleUse = false; // Can be reused after cooldown
    
    // Create explosion radius component for visualization
    ExplosionRadiusComponent = CreateDefaultSubobject<USphereComponent>(TEXT("ExplosionRadius"));
    ExplosionRadiusComponent->SetupAttachment(Root);
    ExplosionRadiusComponent->SetSphereRadius(ExplosionRadius);
    ExplosionRadiusComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ExplosionRadiusComponent->SetHiddenInGame(false);
    
    // Create arming light
    ArmingLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ArmingLight"));
    ArmingLight->SetupAttachment(Root);
    ArmingLight->SetIntensity(1000.0f);
    ArmingLight->SetLightColor(FLinearColor::Red);
    ArmingLight->SetAttenuationRadius(200.0f);
    ArmingLight->SetVisibility(false);
    
    // Make trigger volume smaller for landmines
    if (TriggerVolume)
    {
        TriggerVolume->SetBoxExtent(FVector(50.0f, 50.0f, 20.0f));
    }
}

void ALandmineTrap::BeginPlay()
{
    Super::BeginPlay();
    
    // Start arming sequence if configured
    if (bStartArmedAfterDelay && ArmingDelay > 0.0f)
    {
        // Override the base armed state
        CurrentState = ETrapState::Idle;
        StartArmingSequence();
    }
    
    // Update explosion radius component
    if (ExplosionRadiusComponent)
    {
        ExplosionRadiusComponent->SetSphereRadius(ExplosionRadius);
    }
}

void ALandmineTrap::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Update arming indicators
    if (bIsArming)
    {
        UpdateArmingIndicators(DeltaTime);
    }
    
    // Debug visualization
    if (bShowDebugVisuals)
    {
        // Show explosion radius
        if (CurrentState == ETrapState::Armed || CurrentState == ETrapState::Triggered)
        {
            DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 24, 
                          FColor::Orange, false, -1, 0, 1.0f);
            
            // Show kill radius
            if (bKillCowsInCenter)
            {
                DrawDebugSphere(GetWorld(), GetActorLocation(), KillRadius, 16, 
                              FColor::Red, false, -1, 0, 2.0f);
            }
        }
    }
}

void ALandmineTrap::OnTrigger(AActor* TriggeringActor)
{
    if (CurrentState != ETrapState::Armed || bHasExploded)
        return;
    
    // Play click sound when triggered
    if (ClickSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ClickSound, GetActorLocation());
    }
    
    // Flash the light
    if (ArmingLight)
    {
        ArmingLight->SetLightColor(FLinearColor::White);
        ArmingLight->SetIntensity(5000.0f);
    }
    
    // Call base implementation
    Super::OnTrigger(TriggeringActor);
}

void ALandmineTrap::OnActivate()
{
    Super::OnActivate();
    
    // Explode!
    Explode();
}

void ALandmineTrap::OnDeactivate()
{
    // Reset explosion state
    bHasExploded = false;
    LaunchedCows.Empty();
    
    // Reset light
    if (ArmingLight)
    {
        ArmingLight->SetVisibility(false);
    }
    
    Super::OnDeactivate();
}

void ALandmineTrap::Explode()
{
    if (bHasExploded)
        return;
    
    bHasExploded = true;
    
    // Play explosion effects
    PlayExplosionEffects();
    
    // Apply explosion forces to nearby cows
    ApplyExplosionForces();
    
    // Broadcast explosion event
    OnMineExploded.Broadcast(this);
    
    // Hide the mine mesh (it exploded)
    if (TrapMesh)
    {
        TrapMesh->SetVisibility(false);
    }
    
    // Schedule deactivation
    FTimerHandle DeactivateTimer;
    GetWorld()->GetTimerManager().SetTimer(DeactivateTimer, [this]()
    {
        OnDeactivate();
        
        // Make mesh visible again after cooldown
        if (TrapMesh)
        {
            TrapMesh->SetVisibility(true);
        }
    }, 0.5f, false);
}

void ALandmineTrap::ApplyExplosionForces()
{
    FVector ExplosionLocation = GetActorLocation();
    
    // Find all actors in explosion radius
    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    
    GetWorld()->OverlapMultiByObjectType(
        OverlapResults,
        ExplosionLocation,
        FQuat::Identity,
        FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
        FCollisionShape::MakeSphere(ExplosionRadius),
        QueryParams
    );
    
    // Process each actor
    for (const FOverlapResult& Result : OverlapResults)
    {
        ACowCharacter* Cow = Cast<ACowCharacter>(Result.GetActor());
        if (!Cow || LaunchedCows.Contains(Cow))
            continue;
        
        float DistanceFromCenter = FVector::Dist(ExplosionLocation, Cow->GetActorLocation());
        
        // Check if cow should be killed (too close to center)
        if (bKillCowsInCenter && DistanceFromCenter <= KillRadius)
        {
            // Play death effect
            if (ExplosionNiagaraEffect)
            {
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionNiagaraEffect, 
                    Cow->GetActorLocation(), FRotator::ZeroRotator, FVector(0.5f));
            }
            
            // Destroy the cow
            Cow->Destroy();
            
            UE_LOG(LogTemp, Warning, TEXT("Landmine killed cow (too close): %s"), *Cow->GetName());
        }
        else
        {
            // Launch the cow
            LaunchCow(Cow);
        }
    }
}

void ALandmineTrap::LaunchCow(ACowCharacter* Cow)
{
    if (!Cow || LaunchedCows.Contains(Cow))
        return;
    
    // Mark as launched
    LaunchedCows.Add(Cow);
    
    // Calculate launch velocity
    FVector LaunchVelocity = CalculateLaunchVelocity(Cow->GetActorLocation());
    
    // Disable boids temporarily
    if (UCowBoidsComponent* BoidsComp = Cow->FindComponentByClass<UCowBoidsComponent>())
    {
        BoidsComp->SetComponentTickEnabled(false);
        
        // Re-enable after a delay
        FTimerHandle ReenableTimer;
        GetWorld()->GetTimerManager().SetTimer(ReenableTimer, [BoidsComp]()
        {
            if (IsValid(BoidsComp))
            {
                BoidsComp->SetComponentTickEnabled(true);
            }
        }, 3.0f, false);
    }
    
    // Apply launch velocity
    if (UCharacterMovementComponent* MovementComp = Cow->GetCharacterMovement())
    {
        // Set to falling mode
        MovementComp->SetMovementMode(MOVE_Falling);
        
        // Apply the launch velocity
        MovementComp->Velocity = LaunchVelocity;
        
        // Add random spin if configured
        if (bAddRandomSpin && Cow->GetCapsuleComponent())
        {
            FVector RandomSpin = FVector(
                FMath::RandRange(-MaxSpinRate, MaxSpinRate),
                FMath::RandRange(-MaxSpinRate, MaxSpinRate),
                FMath::RandRange(-MaxSpinRate, MaxSpinRate)
            );
            
            Cow->GetCapsuleComponent()->SetPhysicsAngularVelocityInDegrees(RandomSpin);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Landmine launched cow: %s with velocity: %s"), 
               *Cow->GetName(), *LaunchVelocity.ToString());
    }
    
    // Broadcast launch event
    OnCowLaunched.Broadcast(this, Cow, LaunchVelocity);
}

FVector ALandmineTrap::CalculateLaunchVelocity(const FVector& CowLocation) const
{
    FVector ExplosionLocation = GetActorLocation();
    FVector Direction = CowLocation - ExplosionLocation;
    float Distance = Direction.Size();
    
    // Normalize direction
    if (Distance > 0.0f)
    {
        Direction /= Distance;
    }
    else
    {
        // Cow is directly on mine, launch straight up
        Direction = FVector::UpVector;
        Distance = 1.0f;
    }
    
    // Calculate force falloff based on distance
    float DistanceFactor = FMath::Clamp(1.0f - (Distance / ExplosionRadius), 0.0f, 1.0f);
    DistanceFactor = FMath::Pow(DistanceFactor, 0.5f); // Soften the falloff curve
    
    // Calculate launch speed
    float LaunchSpeed = FMath::Lerp(MinLaunchSpeed, MaxLaunchSpeed, DistanceFactor);
    
    // Separate horizontal and vertical components
    FVector HorizontalDirection = FVector(Direction.X, Direction.Y, 0.0f).GetSafeNormal();
    float VerticalComponent = FMath::Max(0.3f, DistanceFactor); // Always some upward force
    
    // Build final velocity
    FVector LaunchVelocity = HorizontalDirection * LaunchSpeed * HorizontalLaunchMultiplier;
    LaunchVelocity.Z = LaunchSpeed * VerticalComponent * VerticalLaunchMultiplier;
    
    return LaunchVelocity;
}

void ALandmineTrap::PlayExplosionEffects()
{
    FVector ExplosionLocation = GetActorLocation();
    
    // Play explosion sound
    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ExplosionLocation);
    }
    
    // Spawn explosion particle effect
    if (ExplosionNiagaraEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionNiagaraEffect, 
            ExplosionLocation, FRotator::ZeroRotator);
    }
    else if (ExplosionEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, 
            ExplosionLocation, FRotator::ZeroRotator);
    }
    
    // Spawn smoke effect
    if (SmokeEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SmokeEffect, 
            ExplosionLocation, FRotator::ZeroRotator, FVector(1.5f));
    }
    
    // Camera shake
    if (ExplosionCameraShake)
    {
        UGameplayStatics::PlayWorldCameraShake(GetWorld(), ExplosionCameraShake, 
            ExplosionLocation, 0.0f, CameraShakeRadius);
    }
    
    // Flash light effect
    if (ArmingLight)
    {
        ArmingLight->SetIntensity(10000.0f);
        ArmingLight->SetLightColor(FLinearColor(1.0f, 0.5f, 0.0f));
        ArmingLight->SetAttenuationRadius(ExplosionRadius * 2.0f);
        
        // Fade out the light
        FTimerHandle LightFadeTimer;
        GetWorld()->GetTimerManager().SetTimer(LightFadeTimer, [this]()
        {
            if (ArmingLight)
            {
                ArmingLight->SetVisibility(false);
            }
        }, 0.2f, false);
    }
}

void ALandmineTrap::StartArmingSequence()
{
    bIsArming = true;
    ArmingTimeElapsed = 0.0f;
    CurrentBeepInterval = BeepInterval;
    
    // Play arming sound
    if (ArmingSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ArmingSound, GetActorLocation());
    }
    
    // Show arming light
    if (ArmingLight && bShowArmingIndicator)
    {
        ArmingLight->SetVisibility(true);
    }
    
    // Start beeping
    PlayBeep();
    
    // Set timer for arming completion
    GetWorld()->GetTimerManager().SetTimer(ArmingTimerHandle, this, 
        &ALandmineTrap::OnArmingComplete, ArmingDelay, false);
}

void ALandmineTrap::UpdateArmingIndicators(float DeltaTime)
{
    if (!bIsArming)
        return;
    
    ArmingTimeElapsed += DeltaTime;
    TimeSinceLastBeep += DeltaTime;
    
    // Accelerate beeping as arming progresses
    float Progress = ArmingTimeElapsed / ArmingDelay;
    CurrentBeepInterval = BeepInterval * FMath::Pow(BeepAcceleration, Progress * 10.0f);
    
    // Check if it's time to beep
    if (TimeSinceLastBeep >= CurrentBeepInterval)
    {
        PlayBeep();
        TimeSinceLastBeep = 0.0f;
    }
    
    // Update light intensity (pulsing)
    if (ArmingLight)
    {
        float Intensity = 1000.0f + 500.0f * FMath::Sin(GetWorld()->GetTimeSeconds() * 10.0f * (1.0f + Progress));
        ArmingLight->SetIntensity(Intensity);
    }
}

void ALandmineTrap::OnArmingComplete()
{
    bIsArming = false;
    ArmingTimeElapsed = 0.0f;
    
    // Arm the trap
    ArmTrap();
    
    // Change light to armed state
    if (ArmingLight)
    {
        ArmingLight->SetLightColor(FLinearColor::Green);
        ArmingLight->SetIntensity(500.0f);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Landmine armed and ready!"));
}

void ALandmineTrap::PlayBeep()
{
    if (BeepSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, BeepSound, GetActorLocation());
    }
    
    // Flash the light
    if (ArmingLight)
    {
        ArmingLight->SetIntensity(2000.0f);
        
        // Dim back down after a moment
        FTimerHandle DimTimer;
        GetWorld()->GetTimerManager().SetTimer(DimTimer, [this]()
        {
            if (ArmingLight)
            {
                ArmingLight->SetIntensity(1000.0f);
            }
        }, 0.1f, false);
    }
}