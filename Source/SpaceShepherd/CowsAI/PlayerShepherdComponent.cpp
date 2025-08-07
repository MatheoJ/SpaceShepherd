// PlayerShepherdComponent.cpp
#include "PlayerShepherdComponent.h"
#include "CowCharacter.h"
#include "CowBoidsComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

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
    
    // Update laser attraction if active
    UpdateLaserAttraction();
    
    // Update nearby cows periodically
    UpdateTimer += DeltaTime;
    if (UpdateTimer >= UpdateInterval)
    {
        UpdateTimer = 0.0f;
        UpdateNearbyCows();
    }
    
    // Update carried cow position
    if (bIsCarryingCow && CarriedCow)
    {
        UpdateCarriedCow(DeltaTime);
    }
    
    // Update throw charge
    if (bIsChargingThrow)
    {
        UpdateThrowCharge(DeltaTime);
    }
    
    // Draw visual feedback
    if (bShowModeIndicator)
    {
        DrawModeIndicator();
    }
    
    // Draw throw trajectory
    if (bIsCarryingCow && bIsChargingThrow && bShowThrowTrajectory)
    {
        DrawThrowTrajectory();
    }
}

// ========== Mode Management ==========

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

// ========== Carrying System ==========

void UPlayerShepherdComponent::TryPickupCow()
{
    if (bIsCarryingCow)
    {
        DropCow();
        return;
    }
    
    ACowCharacter* CowToPickup = GetCowInPickupRange();
    if (CowToPickup)
    {
        CarriedCow = CowToPickup;
        bIsCarryingCow = true;
        
        // Disable cow's physics and AI
        DisableCowPhysics(CarriedCow);
        
        // Disable the cow's boids behavior
        if (UCowBoidsComponent* BoidsComp = CarriedCow->FindComponentByClass<UCowBoidsComponent>())
        {
            BoidsComp->SetComponentTickEnabled(false);
        }
        
        OnCowPickedUp.Broadcast(CarriedCow);
    }
}

void UPlayerShepherdComponent::DropCow()
{
    if (!bIsCarryingCow || !CarriedCow)
        return;
    
    // Re-enable cow's physics and AI
    EnableCowPhysics(CarriedCow);
    
    // Re-enable the cow's boids behavior
    if (UCowBoidsComponent* BoidsComp = CarriedCow->FindComponentByClass<UCowBoidsComponent>())
    {
        BoidsComp->SetComponentTickEnabled(true);
    }
    
    // Reset carry state
    CarriedCow = nullptr;
    bIsCarryingCow = false;
    bIsChargingThrow = false;
    CurrentChargeTime = 0.0f;
    CurrentThrowPower = 0.0f;
    
    OnCowDropped.Broadcast();
}

bool UPlayerShepherdComponent::CanPickupCow() const
{
    return !bIsCarryingCow && GetCowInPickupRange() != nullptr;
}

ACowCharacter* UPlayerShepherdComponent::GetCowInPickupRange() const
{
    if (!GetOwner())
        return nullptr;
    
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
        return nullptr;
    
    FVector PlayerLocation = OwnerCharacter->GetActorLocation();
    FVector PlayerForward = OwnerCharacter->GetActorForwardVector();
    
    ACowCharacter* ClosestCow = nullptr;
    float ClosestDistance = PickupRange;
    
    // Find all cows in range
    for (TActorIterator<ACowCharacter> It(GetWorld()); It; ++It)
    {
        ACowCharacter* Cow = *It;
        if (!Cow)
            continue;
        
        FVector ToCow = Cow->GetActorLocation() - PlayerLocation;
        float Distance = ToCow.Size();
        
        // Check distance
        if (Distance > PickupRange)
            continue;
        
        // Check angle
        ToCow.Normalize();
        float DotProduct = FVector::DotProduct(PlayerForward, ToCow);
        float Angle = FMath::RadiansToDegrees(FMath::Acos(DotProduct));
        
        if (Angle <= PickupAngle && Distance < ClosestDistance)
        {
            ClosestCow = Cow;
            ClosestDistance = Distance;
        }
    }
    
    return ClosestCow;
}

// ========== Throwing System ==========

void UPlayerShepherdComponent::StartChargingThrow()
{
    if (!bIsCarryingCow || !CarriedCow)
        return;
    
    bIsChargingThrow = true;
    CurrentChargeTime = 0.0f;
    CurrentThrowPower = 0.0f;
}

void UPlayerShepherdComponent::ReleaseThrow()
{
    if (!bIsChargingThrow || !bIsCarryingCow || !CarriedCow)
        return;
    
    ThrowCow();
}

void UPlayerShepherdComponent::CancelThrow()
{
    if (!bIsChargingThrow)
        return;
    
    bIsChargingThrow = false;
    CurrentChargeTime = 0.0f;
    CurrentThrowPower = 0.0f;
}

FVector UPlayerShepherdComponent::CalculateThrowVelocity() const
{
    if (!GetOwner())
        return FVector::ZeroVector;
    
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
        return FVector::ZeroVector;
    
    FVector ThrowDirection;
    
    // Use camera direction if enabled and available
    if (bUseCameraDirection)
    {
        if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
        {
            // Get the control rotation (where the camera is looking)
            FRotator CameraRotation = PC->GetControlRotation();
            
            // Add upward angle to the camera direction
            CameraRotation.Pitch += ThrowUpwardAngle;
            
            // Convert to direction vector
            ThrowDirection = CameraRotation.Vector();
        }
        else
        {
            // Fallback to character forward if no player controller
            ThrowDirection = OwnerCharacter->GetActorForwardVector();
            FRotator ThrowRotation = ThrowDirection.Rotation();
            ThrowRotation.Pitch += ThrowUpwardAngle;
            ThrowDirection = ThrowRotation.Vector();
        }
    }
    else
    {
        // Use character forward direction
        ThrowDirection = OwnerCharacter->GetActorForwardVector();
        FRotator ThrowRotation = ThrowDirection.Rotation();
        ThrowRotation.Pitch += ThrowUpwardAngle;
        ThrowDirection = ThrowRotation.Vector();
    }
    
    // Calculate throw speed based on charge
    float ThrowSpeed = FMath::Lerp(MinThrowSpeed, MaxThrowSpeed, CurrentThrowPower);
    
    return ThrowDirection * ThrowSpeed;
}

// ========== Input Handlers ==========

void UPlayerShepherdComponent::HandleAttractionInput()
{
    ToggleAttractionMode();
}

void UPlayerShepherdComponent::HandleRepulsionInput()
{
    ToggleRepulsionMode();
}

void UPlayerShepherdComponent::HandlePickupInput()
{
    TryPickupCow();
}

void UPlayerShepherdComponent::HandleThrowPressed()
{
    StartChargingThrow();
}

void UPlayerShepherdComponent::HandleThrowReleased()
{
    ReleaseThrow();
}

// ========== Private Helper Functions ==========

void UPlayerShepherdComponent::UpdateNearbyCows()
{
    if (!GetOwner())
        return;
    
    // Clear previous cow states
    for (ACowCharacter* Cow : NearbyCows)
    {
        if (IsValid(Cow) && Cow != CarriedCow)
        {
            Cow->SetPlayerAttraction(false);
            Cow->SetPlayerRepulsion(false);
        }
    }
    
    NearbyCows.Empty();
    
    // Handle laser attraction mode separately - it doesn't depend on player distance
    if (CurrentMode == EShepherdMode::LaserAttraction && bIsLaserActive && bLaserHasValidHit)
    {
        // Find all cows within laser attraction radius
        for (TActorIterator<ACowCharacter> It(GetWorld()); It; ++It)
        {
            ACowCharacter* Cow = *It;
            if (!Cow || Cow == CarriedCow)
                continue;
            
            // Check if cow has boids component
            UCowBoidsComponent* BoidsComp = Cow->FindComponentByClass<UCowBoidsComponent>();
            if (!BoidsComp)
                continue;
            
            // Check distance from laser impact point to cow
            float DistanceToLaser = FVector::Dist(LaserImpactPoint, Cow->GetActorLocation());
            if (DistanceToLaser <= LaserAttractionRadius)
            {
                NearbyCows.Add(Cow);
                // Laser attraction uses the standard attraction flag
                // The boids component will detect it's laser mode through the shepherd component
                Cow->SetPlayerAttraction(true);
            }
        }
    }
    else
    {
        // Standard proximity-based modes (attraction/repulsion/neutral)
        for (TActorIterator<ACowCharacter> It(GetWorld()); It; ++It)
        {
            ACowCharacter* Cow = *It;
            if (!Cow || Cow == CarriedCow)
                continue;
            
            // Check if cow has boids component and is within its detection radius
            UCowBoidsComponent* BoidsComp = Cow->FindComponentByClass<UCowBoidsComponent>();
            if (BoidsComp)
            {
                // For normal modes, check player-to-cow distance
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
    
    // Debug logging
    if (CurrentMode == EShepherdMode::LaserAttraction && bIsLaserActive)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Laser Mode: Found %d cows near laser point"), NearbyCows.Num());
    }
}

void UPlayerShepherdComponent::UpdateCarriedCow(float DeltaTime)
{
    if (!CarriedCow || !GetOwner())
        return;
    
    // Calculate target position
    FVector TargetPosition = GetCarryPosition();
    
    // Smoothly move cow to carry position
    FVector CurrentPosition = CarriedCow->GetActorLocation();
    FVector NewPosition = FMath::VInterpTo(CurrentPosition, TargetPosition, DeltaTime, CarryInterpSpeed);
    
    CarriedCow->SetActorLocation(NewPosition);
    
    // Update cow rotation
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter)
    {
        FRotator TargetRotation;
        
        // Use camera rotation if enabled and available
        if (bUseCameraDirection)
        {
            if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
            {
                TargetRotation = PC->GetControlRotation();
                // Don't rotate cow up/down with camera pitch
                TargetRotation.Pitch = 0;
            }
            else
            {
                // Fallback to character rotation
                TargetRotation = OwnerCharacter->GetActorRotation();
            }
        }
        else
        {
            // Use character rotation
            TargetRotation = OwnerCharacter->GetActorRotation();
        }
        
        FRotator CurrentRotation = CarriedCow->GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, CarryInterpSpeed);
        CarriedCow->SetActorRotation(NewRotation);
    }
}

void UPlayerShepherdComponent::UpdateThrowCharge(float DeltaTime)
{
    CurrentChargeTime += DeltaTime;
    CurrentChargeTime = FMath::Min(CurrentChargeTime, MaxChargeTime);
    CurrentThrowPower = CurrentChargeTime / MaxChargeTime;
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
        case EShepherdMode::LaserAttraction:
            DrawColor = LaserColor;
            break;
        default:
            break;
    }
    
    // Draw indicator circle around player (except for laser mode)
    if (CurrentMode != EShepherdMode::Neutral && CurrentMode != EShepherdMode::LaserAttraction)
    {
        // Draw pulsing effect
        float PulseScale = 1.0f + FMath::Sin(GetWorld()->GetTimeSeconds() * 3.0f) * 0.1f;
        DrawDebugSphere(GetWorld(), Location, IndicatorRadius * PulseScale / 3.0f, 16, DrawColor, false, -1, 0, 1);
    }
    
    // Draw laser mode indicator
    if (CurrentMode == EShepherdMode::LaserAttraction && bIsLaserActive)
    {
        // Small indicator around player showing laser mode is active
        DrawDebugSphere(GetWorld(), Location, 30.0f, 8, LaserColor, false, -1, 0, 1);
    }
    
    // Draw pickup indicator
    if (!bIsCarryingCow)
    {
        ACowCharacter* CowInRange = GetCowInPickupRange();
        if (CowInRange)
        {
            // Highlight the cow that can be picked up
            DrawDebugSphere(GetWorld(), CowInRange->GetActorLocation(), 50.0f, 8, FColor::Cyan, false, -1, 0, 2);
        }
    }
    
    // Draw throw power indicator
    if (bIsCarryingCow && bIsChargingThrow)
    {
        FVector BarStart = Location + FVector(0, 0, 200);
        FVector BarEnd = BarStart + FVector(100 * CurrentThrowPower, 0, 0);
        FColor PowerColor = FColor::MakeRedToGreenColorFromScalar(CurrentThrowPower);
        DrawDebugLine(GetWorld(), BarStart, BarEnd, PowerColor, false, -1, 0, 10);
    }
}

void UPlayerShepherdComponent::DrawThrowTrajectory()
{
    if (!CarriedCow || !GetOwner())
        return;
    
    FVector StartLocation = CarriedCow->GetActorLocation();
    FVector InitialVelocity = CalculateThrowVelocity();
    
    TrajectoryPointsCache.Empty();
    
    // Simulate trajectory
    for (int32 i = 0; i < TrajectoryPoints; i++)
    {
        float Time = i * TrajectoryTimeStep;
        
        // Physics equation: s = ut + 0.5at^2
        FVector Point = StartLocation + InitialVelocity * Time;
        Point.Z += 0.5f * GetWorld()->GetGravityZ() * Time * Time;
        
        TrajectoryPointsCache.Add(Point);
        
        // Draw point
        DrawDebugSphere(GetWorld(), Point, 5.0f, 4, FColor::Yellow, false, -1, 0, 1);
        
        // Draw line between points
        if (i > 0)
        {
            DrawDebugLine(GetWorld(), TrajectoryPointsCache[i-1], Point, FColor::Yellow, false, -1, 0, 2);
        }
    }
}

void UPlayerShepherdComponent::ThrowCow()
{
    if (!CarriedCow)
        return;
    
    // Calculate throw velocity
    FVector ThrowVelocity = CalculateThrowVelocity();
    
    // Re-enable physics
    EnableCowPhysics(CarriedCow);
    
    // Apply throw velocity
    if (UCharacterMovementComponent* MovementComp = CarriedCow->GetCharacterMovement())
    {
        MovementComp->Velocity = ThrowVelocity;
        MovementComp->SetMovementMode(MOVE_Falling);
        
        // Add some rotation for visual effect
        CarriedCow->GetCapsuleComponent()->SetPhysicsAngularVelocityInDegrees(FVector(0, 360, 0));
    }
    
    // Re-enable the cow's boids behavior (with a delay to let it land)
    if (UCowBoidsComponent* BoidsComp = CarriedCow->FindComponentByClass<UCowBoidsComponent>())
    {
        FTimerHandle EnableBoidsTimer;
        GetWorld()->GetTimerManager().SetTimer(EnableBoidsTimer, [BoidsComp]()
        {
            BoidsComp->SetComponentTickEnabled(true);
        }, 2.0f, false);
    }
    
    // Broadcast throw event
    OnCowThrown.Broadcast(CarriedCow, CurrentThrowPower);
    
    // Reset state
    CarriedCow = nullptr;
    bIsCarryingCow = false;
    bIsChargingThrow = false;
    CurrentChargeTime = 0.0f;
    CurrentThrowPower = 0.0f;
}

FVector UPlayerShepherdComponent::GetCarryPosition() const
{
    if (!GetOwner())
        return FVector::ZeroVector;
    
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
        return FVector::ZeroVector;
    
    FVector Position = OwnerCharacter->GetActorLocation();
    FVector ForwardVector;
    FVector RightVector;
    
    // Use camera direction if enabled
    if (bUseCameraDirection)
    {
        if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
        {
            FRotator CameraRotation = PC->GetControlRotation();
            // Dampen pitch to keep carry position more level
            CameraRotation.Pitch = FMath::Clamp(CameraRotation.Pitch, -30.0f, 30.0f) * CarryPitchDamping;
            
            ForwardVector = CameraRotation.Vector();
            RightVector = FRotationMatrix(CameraRotation).GetScaledAxis(EAxis::Y);
        }
        else
        {
            // Fallback to character orientation
            ForwardVector = OwnerCharacter->GetActorForwardVector();
            RightVector = OwnerCharacter->GetActorRightVector();
        }
    }
    else
    {
        // Use character orientation
        ForwardVector = OwnerCharacter->GetActorForwardVector();
        RightVector = OwnerCharacter->GetActorRightVector();
    }
    
    // Calculate position based on direction vectors
    Position += ForwardVector * CarryOffset.X;
    Position += RightVector * CarryOffset.Y;
    Position += FVector::UpVector * CarryOffset.Z;
    
    return Position;
}

void UPlayerShepherdComponent::DisableCowPhysics(ACowCharacter* Cow)
{
    if (!Cow)
        return;
    
    // Disable collision
    if (UCapsuleComponent* Capsule = Cow->GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    // Stop movement
    if (UCharacterMovementComponent* MovementComp = Cow->GetCharacterMovement())
    {
        MovementComp->StopMovementImmediately();
        MovementComp->SetMovementMode(MOVE_None);
        MovementComp->DisableMovement();
    }
}

void UPlayerShepherdComponent::EnableCowPhysics(ACowCharacter* Cow)
{
    if (!Cow)
        return;
    
    // Re-enable collision
    if (UCapsuleComponent* Capsule = Cow->GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    
    // Re-enable movement
    if (UCharacterMovementComponent* MovementComp = Cow->GetCharacterMovement())
    {
        MovementComp->SetMovementMode(MOVE_Walking);
        MovementComp->SetMovementMode(MOVE_NavWalking);
    }
}

void UPlayerShepherdComponent::HandleLaserPressed()
{
    StartLaserAttraction();
}

void UPlayerShepherdComponent::HandleLaserReleased()
{
    StopLaserAttraction();
}

void UPlayerShepherdComponent::StartLaserAttraction()
{
    if (bIsLaserActive)
        return;
    
    bIsLaserActive = true;
    
    // Set mode to laser attraction
    SetShepherdMode(EShepherdMode::LaserAttraction);
}

void UPlayerShepherdComponent::StopLaserAttraction()
{
    if (!bIsLaserActive)
        return;
    
    bIsLaserActive = false;
    bLaserHasValidHit = false;
    LaserImpactPoint = FVector::ZeroVector;
    
    // Return to neutral mode
    SetShepherdMode(EShepherdMode::Neutral);
}

void UPlayerShepherdComponent::UpdateLaserAttraction()
{
    if (!bIsLaserActive)
        return;
    
    // Perform the laser trace
    PerformLaserTrace();
    
    // Draw the laser visual
    DrawLaser();
}

void UPlayerShepherdComponent::PerformLaserTrace()
{
    if (!GetOwner())
        return;
    
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
        return;
    
    // Get start position and direction for the laser
    FVector StartPosition = GetLaserStartPosition();
    FVector LaserDirection = GetLaserDirection();
    FVector EndPosition = StartPosition + (LaserDirection * LaserMaxRange);
    
    // Setup collision query params
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    QueryParams.bTraceComplex = false;
    
    // Perform the line trace
    bLaserHasValidHit = GetWorld()->LineTraceSingleByChannel(
        LaserHitResult,
        StartPosition,
        EndPosition,
        ECC_Visibility,
        QueryParams
    );
    
    if (bLaserHasValidHit)
    {
        LaserImpactPoint = LaserHitResult.ImpactPoint;
    }
    else
    {
        // If no hit, set impact point at max range
        LaserImpactPoint = EndPosition;
    }
}

FVector UPlayerShepherdComponent::GetLaserStartPosition() const
{
    if (!GetOwner())
        return FVector::ZeroVector;
    
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
        return FVector::ZeroVector;
    
    // Start from character's location with some height offset
    FVector StartPos = OwnerCharacter->GetActorLocation();
    
    // Offset slightly forward and up to avoid starting inside the character
    StartPos += OwnerCharacter->GetActorForwardVector() * 50.0f;
    StartPos += FVector(0, 0, 50.0f); // Height offset
    
    return StartPos;
}

FVector UPlayerShepherdComponent::GetLaserDirection() const
{
    if (!GetOwner())
        return FVector::ForwardVector;
    
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
        return FVector::ForwardVector;
    
    // Get the camera direction for third person aiming
    if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        // Get camera location and rotation
        FVector CameraLocation;
        FRotator CameraRotation;
        PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
        
        // Return the camera's forward vector
        return CameraRotation.Vector();
    }
    
    // Fallback to character forward
    return OwnerCharacter->GetActorForwardVector();
}

void UPlayerShepherdComponent::DrawLaser()
{
    if (!bIsLaserActive || !GetOwner())
        return;
    
    FVector StartPosition = GetLaserStartPosition();
    FVector EndPosition = LaserImpactPoint;
    
    // Draw the main laser beam
    DrawDebugLine(
        GetWorld(),
        StartPosition,
        EndPosition,
        LaserColor,
        false,
        -1,
        0,
        LaserThickness
    );
    
    // Draw impact point indicator if we hit something
    if (bLaserHasValidHit && bShowLaserImpactPoint)
    {
        // Draw impact sphere
        DrawDebugSphere(
            GetWorld(),
            LaserImpactPoint,
            LaserImpactSphereSize,
            12,
            LaserColor,
            false,
            -1,
            0,
            2.0f
        );
        
        // Draw attraction radius at impact point
        DrawDebugSphere(
            GetWorld(),
            LaserImpactPoint,
            LaserAttractionRadius,
            24,
            FColor(LaserColor.R, LaserColor.G, LaserColor.B, 64), // Semi-transparent
            false,
            -1,
            0,
            1.0f
        );
        
        // Add pulsing effect
        float PulseScale = 1.0f + FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f) * 0.1f;
        DrawDebugSphere(
            GetWorld(),
            LaserImpactPoint,
            LaserImpactSphereSize * PulseScale,
            8,
            FColor::White,
            false,
            -1,
            0,
            1.0f
        );
    }
}
