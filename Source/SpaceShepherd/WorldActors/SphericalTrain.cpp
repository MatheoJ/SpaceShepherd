#include "SphericalTrain.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

ASphericalTrain::ASphericalTrain()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create root component
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    RootComponent = RootSceneComponent;

    // Create spline component
    PathSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PathSpline"));
    PathSpline->SetupAttachment(RootComponent);
    PathSpline->SetClosedLoop(true);

    // Create train mesh
    TrainMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrainMesh"));
    TrainMesh->SetupAttachment(RootComponent);
    TrainMesh->SetRelativeLocation(FVector(0, 0, 0));
}

void ASphericalTrain::BeginPlay()
{
    Super::BeginPlay();
    
    // Setup the spline around the planet
    SetupSplineAroundPlanet();
    
    // Get initial spline length
    SplineLength = PathSpline->GetSplineLength();
    
    // Initialize rotation
    if (SplineLength > 0)
    {
        FVector InitialLocation = PathSpline->GetLocationAtDistanceAlongSpline(0, ESplineCoordinateSpace::World);
        FVector InitialDirection = PathSpline->GetDirectionAtDistanceAlongSpline(0, ESplineCoordinateSpace::World);
        CurrentRotation = CalculateTargetRotation(InitialLocation, InitialDirection);
        LastSplineDirection = InitialDirection;
        
        // Set initial position and rotation
        TrainMesh->SetWorldLocation(InitialLocation);
        TrainMesh->SetWorldRotation(CurrentRotation);
    }
    
    // Setup train cars
    for (int32 i = 0; i < TrainCars.Num(); i++)
    {
        if (TrainCars[i])
        {
            TrainCars[i]->SetMobility(EComponentMobility::Movable);
        }
    }
}

void ASphericalTrain::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (bIsMoving && SplineLength > 0)
    {
        UpdateTrainMovement(DeltaTime);
        UpdateTrainOrientationImproved(DeltaTime);
        UpdateTrainCarsImproved(DeltaTime);
    }
    
    if (bShowDebugInfo || bShowDebugSpline || bShowOrientationVectors)
    {
        DrawDebugVisualization();
    }
}

void ASphericalTrain::SetupSplineAroundPlanet()
{
    if (!PlanetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("SphericalTrain: No planet actor assigned!"));
        return;
    }
    
    // Clear existing spline points
    PathSpline->ClearSplinePoints();
    
    FVector PlanetCenter = GetPlanetCenter();
    
    // Create a circular path
    float AngleStep = 360.0f / NumberOfSplinePoints;
    
    // Find two perpendicular vectors to the normal axis
    FVector Right, Forward;
    SplineNormalAxis.Normalize();
    
    if (FMath::Abs(SplineNormalAxis.Z) < 0.9f)
    {
        Right = FVector::CrossProduct(SplineNormalAxis, FVector::UpVector).GetSafeNormal();
    }
    else
    {
        Right = FVector::CrossProduct(SplineNormalAxis, FVector::ForwardVector).GetSafeNormal();
    }
    Forward = FVector::CrossProduct(Right, SplineNormalAxis).GetSafeNormal();
    
    for (int32 i = 0; i < NumberOfSplinePoints; i++)
    {
        float Angle = FMath::DegreesToRadians(AngleStep * i);
        
        // Create point on circle
        FVector LocalPoint = (Forward * FMath::Cos(Angle) + Right * FMath::Sin(Angle)) * SplineRadius;
        FVector WorldPoint = PlanetCenter + LocalPoint;
        
        // Add point to spline
        PathSpline->AddSplineWorldPoint(WorldPoint);
        
        // Snap to planet surface
        SnapSplinePointToPlanetSurface(i, WorldPoint);
    }
    
    // Set spline to closed loop
    PathSpline->SetClosedLoop(true);
    
    // Update spline with automatic tangents
    PathSpline->UpdateSpline();
}

void ASphericalTrain::SnapSplinePointToPlanetSurface(int32 PointIndex, const FVector& InitialPosition)
{
    FVector PlanetCenter = GetPlanetCenter();
    
    // Project point onto sphere surface
    FVector DirectionFromCenter = (InitialPosition - PlanetCenter).GetSafeNormal();
    FVector SurfacePoint = PlanetCenter + DirectionFromCenter * (PlanetRadius + SplineHeightOffset);
    
    // Perform a line trace to get exact surface if planet has complex geometry
    FHitResult HitResult;
    FVector TraceStart = PlanetCenter + DirectionFromCenter * (PlanetRadius * 2.0f);
    FVector TraceEnd = PlanetCenter;
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, 
                                             ECollisionChannel::ECC_WorldStatic, QueryParams))
    {
        if (HitResult.GetActor() == PlanetActor)
        {
            SurfacePoint = HitResult.Location + HitResult.Normal * SplineHeightOffset;
        }
    }
    
    // Update spline point location
    PathSpline->SetLocationAtSplinePoint(PointIndex, SurfacePoint, ESplineCoordinateSpace::World);
}

FVector ASphericalTrain::GetPlanetCenter() const
{
    if (PlanetActor)
    {
        return PlanetActor->GetActorLocation();
    }
    return FVector::ZeroVector;
}

void ASphericalTrain::UpdateTrainMovement(float DeltaTime)
{
    if (SplineLength <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("SphericalTrain: Invalid spline length"));
        return;
    }
    
    // Update distance along spline
    CurrentSplineDistance += TrainSpeed * DeltaTime;
    
    // Handle looping
    if (bLoopMovement)
    {
        // Use modulo for cleaner looping
        CurrentSplineDistance = FMath::Fmod(CurrentSplineDistance, SplineLength);
        if (CurrentSplineDistance < 0.0f)
        {
            CurrentSplineDistance += SplineLength;
        }
    }
    else
    {
        CurrentSplineDistance = FMath::Clamp(CurrentSplineDistance, 0.0f, SplineLength);
    }
    
    // Get position from spline
    FVector NewLocation = PathSpline->GetLocationAtDistanceAlongSpline(CurrentSplineDistance, 
                                                                       ESplineCoordinateSpace::World);
    
    // Validate the new location
    if (!NewLocation.IsNearlyZero())
    {
        TrainMesh->SetWorldLocation(NewLocation);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SphericalTrain: Invalid spline location at distance %.2f"), CurrentSplineDistance);
    }
}

void ASphericalTrain::UpdateTrainOrientationImproved(float DeltaTime)
{
    // Get current spline tangent (direction)
    FVector SplineTangent = PathSpline->GetDirectionAtDistanceAlongSpline(CurrentSplineDistance, 
                                                                           ESplineCoordinateSpace::World);
    SplineTangent.Normalize();
    
    // Get train location
    FVector TrainLocation = TrainMesh->GetComponentLocation();
    
    // Calculate base rotation aligned with planet surface and spline
    FQuat TargetRotation = CalculateTargetRotation(TrainLocation, SplineTangent);
    
    // Apply manual rotation offset if specified
    if (!TrainRotationOffset.IsZero())
    {
        TargetRotation = TargetRotation * FQuat(TrainRotationOffset);
    }
    
    // Calculate and apply banking if enabled
    if (bBankOnCurves)
    {
        float TargetBankAngle = CalculateBankAngle(DeltaTime);
        CurrentBankAngle = FMath::FInterpTo(CurrentBankAngle, TargetBankAngle, DeltaTime, 5.0f);
        
        if (!FMath::IsNearlyZero(CurrentBankAngle))
        {
            // Apply bank around the local forward axis
            FVector LocalForward = TargetRotation.GetForwardVector();
            FQuat BankRotation = FQuat(LocalForward, FMath::DegreesToRadians(CurrentBankAngle));
            TargetRotation = BankRotation * TargetRotation;
        }
    }
    
    // Apply rotation with smoothing if enabled
    if (bUseSmoothRotation)
    {
        CurrentRotation = FQuat::Slerp(CurrentRotation, TargetRotation, DeltaTime * RotationSmoothSpeed);
        TrainMesh->SetWorldRotation(CurrentRotation);
    }
    else
    {
        CurrentRotation = TargetRotation;
        TrainMesh->SetWorldRotation(TargetRotation);
    }
    
    // Store for next frame
    LastSplineDirection = SplineTangent;
}

FQuat ASphericalTrain::CalculateTargetRotation(const FVector& Location, const FVector& SplineDirection) const
{
    FVector PlanetCenter = GetPlanetCenter();
    
    // Calculate up vector (normal to planet surface - away from center)
    FVector UpVector = (Location - PlanetCenter).GetSafeNormal();
    
    // FIXED: Ensure spline direction is truly tangent to the surface
    // Project the spline direction onto the plane tangent to the sphere at this point
    FVector TangentDirection = SplineDirection - (SplineDirection | UpVector) * UpVector;
    TangentDirection.Normalize();
    
    // Check if tangent direction is valid
    if (TangentDirection.IsNearlyZero(0.01f))
    {
        // Spline direction is parallel to up vector (shouldn't happen normally)
        // Create an arbitrary tangent perpendicular to up
        FVector ArbitraryVector = FMath::Abs(UpVector.Z) < 0.9f ? FVector::UpVector : FVector::RightVector;
        TangentDirection = FVector::CrossProduct(UpVector, ArbitraryVector).GetSafeNormal();
        
        UE_LOG(LogTemp, Warning, TEXT("SphericalTrain: Spline direction parallel to surface normal, using fallback"));
    }
    
    // FIXED: Calculate the right vector (perpendicular to both forward and up)
    FVector RightVector = FVector::CrossProduct(TangentDirection, UpVector).GetSafeNormal();
    
    // FIXED: Recalculate forward to ensure perfect orthogonality
    FVector ForwardVector = FVector::CrossProduct(UpVector, RightVector).GetSafeNormal();
    
    // Debug output to verify vectors
    if (bShowDebugInfo)
    {
        float ForwardDotUp = FVector::DotProduct(ForwardVector, UpVector);
        float RightDotUp = FVector::DotProduct(RightVector, UpVector);
        float ForwardDotRight = FVector::DotProduct(ForwardVector, RightVector);
        
        if (FMath::Abs(ForwardDotUp) > 0.01f || FMath::Abs(RightDotUp) > 0.01f || FMath::Abs(ForwardDotRight) > 0.01f)
        {
            UE_LOG(LogTemp, Warning, TEXT("Vectors not orthogonal! F·U=%.3f, R·U=%.3f, F·R=%.3f"), 
                   ForwardDotUp, RightDotUp, ForwardDotRight);
        }
    }
    
    // FIXED: Create rotation matrix properly
    // In Unreal Engine, when using MakeRotFromXZ:
    // X = Forward direction
    // Z = Up direction
    // Y = Right direction (calculated automatically)
    FRotator ResultRotator = UKismetMathLibrary::MakeRotFromXZ(ForwardVector, UpVector);
    
    return ResultRotator.Quaternion();
}

float ASphericalTrain::CalculateBankAngle(float DeltaTime) const
{
    if (DeltaTime <= 0.0f || TrainSpeed <= 0.0f) return 0.0f;
    
    // Sample the spline curvature by looking at direction change
    const float SampleDistance = 50.0f; // Sample ahead by this distance
    
    // Get current direction
    FVector CurrentDir = PathSpline->GetDirectionAtDistanceAlongSpline(CurrentSplineDistance, 
                                                                        ESplineCoordinateSpace::World);
    
    // Get direction a bit ahead
    float LookAheadDist = CurrentSplineDistance + SampleDistance;
    if (bLoopMovement && LookAheadDist > SplineLength)
    {
        LookAheadDist = FMath::Fmod(LookAheadDist, SplineLength);
    }
    else
    {
        LookAheadDist = FMath::Clamp(LookAheadDist, 0.0f, SplineLength);
    }
    
    FVector NextDir = PathSpline->GetDirectionAtDistanceAlongSpline(LookAheadDist, 
                                                                     ESplineCoordinateSpace::World);
    
    // Calculate the angular change
    float DotProduct = FVector::DotProduct(CurrentDir.GetSafeNormal(), NextDir.GetSafeNormal());
    DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
    float AngleChange = FMath::Acos(DotProduct);
    
    // Determine turn direction
    FVector CrossProduct = FVector::CrossProduct(CurrentDir, NextDir);
    FVector PlanetCenter = GetPlanetCenter();
    FVector TrainLocation = TrainMesh->GetComponentLocation();
    FVector UpVector = (TrainLocation - PlanetCenter).GetSafeNormal();
    
    float TurnDirection = FVector::DotProduct(CrossProduct, UpVector);
    
    // Calculate bank angle based on turn rate and speed
    float TurnSharpness = AngleChange / (SampleDistance / 1000.0f); // Convert to reasonable scale
    float SpeedFactor = FMath::Min(TrainSpeed / 500.0f, 1.0f); // Normalize speed influence
    float BankAngle = TurnSharpness * SpeedFactor * MaxBankAngle * -FMath::Sign(TurnDirection);
    
    return FMath::Clamp(BankAngle, -MaxBankAngle, MaxBankAngle);
}

void ASphericalTrain::UpdateTrainCarsImproved(float DeltaTime)
{
    for (int32 i = 0; i < TrainCars.Num(); i++)
    {
        if (!TrainCars[i]) continue;
        
        // Calculate distance for this car
        float CarDistance = CurrentSplineDistance - (CarSeparationDistance * (i + 1));
        
        // Handle looping
        if (bLoopMovement)
        {
            CarDistance = FMath::Fmod(CarDistance, SplineLength);
            if (CarDistance < 0)
            {
                CarDistance += SplineLength;
            }
        }
        else
        {
            CarDistance = FMath::Clamp(CarDistance, 0.0f, SplineLength);
        }
        
        // Get position and direction for car
        FVector CarLocation = PathSpline->GetLocationAtDistanceAlongSpline(CarDistance, 
                                                                          ESplineCoordinateSpace::World);
        FVector CarDirection = PathSpline->GetDirectionAtDistanceAlongSpline(CarDistance, 
                                                                            ESplineCoordinateSpace::World);
        
        // Update car position
        TrainCars[i]->SetWorldLocation(CarLocation);
        
        // Calculate car rotation
        FQuat CarRotation = CalculateTargetRotation(CarLocation, CarDirection);
        
        // Apply rotation offset if needed
        if (!TrainRotationOffset.IsZero())
        {
            CarRotation = CarRotation * FQuat(TrainRotationOffset);
        }
        
        // Apply smooth rotation if enabled
        if (bUseSmoothRotation)
        {
            FQuat CurrentCarRotation = TrainCars[i]->GetComponentQuat();
            CarRotation = FQuat::Slerp(CurrentCarRotation, CarRotation, DeltaTime * RotationSmoothSpeed);
        }
        
        TrainCars[i]->SetWorldRotation(CarRotation);
    }
}

void ASphericalTrain::DrawDebugVisualization()
{
    if (!GetWorld()) return;
    
    if (bShowDebugSpline)
    {
        // Draw spline path with gradient color
        const int32 Segments = 100;
        for (int32 i = 0; i < Segments; i++)
        {
            float Distance1 = (SplineLength / Segments) * i;
            float Distance2 = (SplineLength / Segments) * (i + 1);
            
            FVector Point1 = PathSpline->GetLocationAtDistanceAlongSpline(Distance1, 
                                                                         ESplineCoordinateSpace::World);
            FVector Point2 = PathSpline->GetLocationAtDistanceAlongSpline(Distance2, 
                                                                         ESplineCoordinateSpace::World);
            
            // Color gradient to show direction
            float Hue = (float)i / Segments * 360.0f;
            FLinearColor Color = FLinearColor::MakeFromHSV8(Hue, 255, 255);
            
            DrawDebugLine(GetWorld(), Point1, Point2, Color.ToFColor(true), false, 0.01f, 0, 2.0f);
        }
        
        // Draw spline control points
        for (int32 i = 0; i < PathSpline->GetNumberOfSplinePoints(); i++)
        {
            FVector PointLocation = PathSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
            DrawDebugSphere(GetWorld(), PointLocation, 20.0f, 8, FColor::Red, false, 0.01f);
            
            // Draw point index
            DrawDebugString(GetWorld(), PointLocation + FVector(0, 0, 30), FString::FromInt(i), 
                          nullptr, FColor::White, 0.01f, true);
        }
        
        // Draw current position on spline
        FVector CurrentPos = PathSpline->GetLocationAtDistanceAlongSpline(CurrentSplineDistance, 
                                                                         ESplineCoordinateSpace::World);
        DrawDebugSphere(GetWorld(), CurrentPos, 30.0f, 12, FColor::Green, false, 0.01f);
    }
    
    if (bShowOrientationVectors)
    {
        FVector TrainLocation = TrainMesh->GetComponentLocation();
        FVector PlanetCenter = GetPlanetCenter();
        
        // Get actual mesh orientation
        FVector MeshForward = TrainMesh->GetForwardVector();
        FVector MeshRight = TrainMesh->GetRightVector();
        FVector MeshUp = TrainMesh->GetUpVector();
        
        // Draw mesh coordinate axes with labels
        // Forward - Red
        DrawDebugDirectionalArrow(GetWorld(), TrainLocation, TrainLocation + MeshForward * 150, 
                                 30.0f, FColor::Red, false, 0.01f, 0, 5.0f);
        DrawDebugString(GetWorld(), TrainLocation + MeshForward * 160, TEXT("Forward"), 
                       nullptr, FColor::Red, 0.01f, true);
        
        // Right - Green
        DrawDebugDirectionalArrow(GetWorld(), TrainLocation, TrainLocation + MeshRight * 100, 
                                 20.0f, FColor::Green, false, 0.01f, 0, 5.0f);
        DrawDebugString(GetWorld(), TrainLocation + MeshRight * 110, TEXT("Right"), 
                       nullptr, FColor::Green, 0.01f, true);
        
        // Up - Blue
        DrawDebugDirectionalArrow(GetWorld(), TrainLocation, TrainLocation + MeshUp * 200, 
                                 40.0f, FColor::Blue, false, 0.01f, 0, 5.0f);
        DrawDebugString(GetWorld(), TrainLocation + MeshUp * 210, TEXT("Up"), 
                       nullptr, FColor::Blue, 0.01f, true);
        
        // Planet normal (should align with Up) - Cyan
        FVector PlanetNormal = (TrainLocation - PlanetCenter).GetSafeNormal();
        DrawDebugLine(GetWorld(), TrainLocation, TrainLocation + PlanetNormal * 180, 
                     FColor::Cyan, false, 0.01f, 0, 3.0f);
        DrawDebugString(GetWorld(), TrainLocation + PlanetNormal * 190, TEXT("Planet Normal"), 
                       nullptr, FColor::Cyan, 0.01f, true);
        
        // Spline tangent - Yellow
        FVector SplineTangent = PathSpline->GetDirectionAtDistanceAlongSpline(CurrentSplineDistance, 
                                                                              ESplineCoordinateSpace::World);
        DrawDebugLine(GetWorld(), TrainLocation, TrainLocation + SplineTangent * 120, 
                     FColor::Yellow, false, 0.01f, 0, 3.0f);
        DrawDebugString(GetWorld(), TrainLocation + SplineTangent * 130, TEXT("Spline Dir"), 
                       nullptr, FColor::Yellow, 0.01f, true);
        
        // Draw to planet center
        DrawDebugLine(GetWorld(), TrainLocation, PlanetCenter, 
                     FColor::Magenta, false, 0.01f, 0, 1.0f);
    }
    
    if (bShowDebugInfo)
    {
        FVector TrainLocation = TrainMesh->GetComponentLocation();
        
        // Create detailed debug info
        FString DebugText = FString::Printf(
            TEXT("Speed: %.1f units/s\n")
            TEXT("Distance: %.1f / %.1f\n")
            TEXT("Progress: %.1f%%\n")
            TEXT("Bank Angle: %.1f°\n")
            TEXT("Smooth Rot: %s\n")
            TEXT("Loop Mode: %s"),
            TrainSpeed, 
            CurrentSplineDistance, 
            SplineLength,
            (CurrentSplineDistance / SplineLength) * 100.0f,
            CurrentBankAngle,
            bUseSmoothRotation ? TEXT("ON") : TEXT("OFF"),
            bLoopMovement ? TEXT("ON") : TEXT("OFF")
        );
        
        DrawDebugString(GetWorld(), TrainLocation + FVector(0, 0, 250), DebugText, 
                       nullptr, FColor::White, 0.01f, true, 1.2f);
        
        // Show rotation values
        FRotator CurrentRot = CurrentRotation.Rotator();
        FString RotText = FString::Printf(TEXT("Rotation: P=%.1f Y=%.1f R=%.1f"), 
                                         CurrentRot.Pitch, CurrentRot.Yaw, CurrentRot.Roll);
        DrawDebugString(GetWorld(), TrainLocation + FVector(0, 0, 100), RotText, 
                       nullptr, FColor::Yellow, 0.01f, true);
    }
}