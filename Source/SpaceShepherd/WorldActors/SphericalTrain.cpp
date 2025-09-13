#include "SphericalTrain.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#endif

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
    
    // Make spline editable in editor
    PathSpline->bDrawDebug = true;
    PathSpline->bInputSplinePointsToConstructionScript = true;
    PathSpline->bShouldVisualizeScale = true;
    PathSpline->ScaleVisualizationWidth = 30.0f;

    // Create train mesh
    TrainMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrainMesh"));
    TrainMesh->SetupAttachment(RootComponent);
    TrainMesh->SetRelativeLocation(FVector(0, 0, 0));
    
    // Set default editor colors
    PathSpline->EditorUnselectedSplineSegmentColor = FLinearColor(1.0f, 1.0f, 0.0f); // Yellow
    PathSpline->EditorSelectedSplineSegmentColor = FLinearColor(1.0f, 0.5f, 0.0f); // Orange
}

void ASphericalTrain::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    bIsEditorPreview = true;
    
    // Update spline settings
    UpdateSplineSettings();
    
    // Auto-generate circular path if requested
    if (bGenerateCircularPath && PathSpline->GetNumberOfSplinePoints() < 2)
    {
        SetupSplineAroundPlanet();
    }
    
    // Auto-snap points to surface if enabled
    if (bAutoSnapToSurface && PlanetActor)
    {
        for (int32 i = 0; i < PathSpline->GetNumberOfSplinePoints(); i++)
        {
            FVector CurrentLocation = PathSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
            SnapSplinePointToPlanetSurface(i, CurrentLocation);
        }
        PathSpline->UpdateSpline();
    }
    
    // Update spline info
    SplineLength = PathSpline->GetSplineLength();
    SplinePointCount = PathSpline->GetNumberOfSplinePoints();
    
    // Preview train position in editor
    if (bShowTrainPreview)
    {
        PreviewTrainPosition();
    }
    
    // Handle action buttons
    if (bSnapAllPoints)
    {
        SnapAllPointsToSurface();
        bSnapAllPoints = false;
    }
    
    if (bClearSpline)
    {
        ClearSplinePoints();
        bClearSpline = false;
    }
}

#if WITH_EDITOR
void ASphericalTrain::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (!PropertyChangedEvent.Property)
        return;
    
    FName PropertyName = PropertyChangedEvent.Property->GetFName();
    
    // Handle property changes
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ASphericalTrain, PlanetActor) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ASphericalTrain, PlanetRadius) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ASphericalTrain, SplineHeightOffset))
    {
        if (bAutoSnapToSurface)
        {
            SnapAllPointsToSurface();
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASphericalTrain, bGenerateCircularPath))
    {
        if (bGenerateCircularPath)
        {
            GenerateCircularSpline();
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASphericalTrain, InitialPositionRatio))
    {
        PreviewTrainPosition();
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASphericalTrain, bClosedLoop))
    {
        PathSpline->SetClosedLoop(bClosedLoop);
        PathSpline->UpdateSpline();
        SplineLength = PathSpline->GetSplineLength();
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASphericalTrain, SplineTangentScale))
    {
        // Update all tangent scales
        for (int32 i = 0; i < PathSpline->GetNumberOfSplinePoints(); i++)
        {
            PathSpline->SetTangentAtSplinePoint(i, 
                PathSpline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local) * SplineTangentScale,
                ESplineCoordinateSpace::Local);
        }
        PathSpline->UpdateSpline();
    }
    
    // Update visualization
    UpdateSplineSettings();
}

void ASphericalTrain::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);
    
    if (bFinished && bAutoSnapToSurface)
    {
        SnapAllPointsToSurface();
    }
}
#endif

void ASphericalTrain::BeginPlay()
{
    Super::BeginPlay();
    
    bIsEditorPreview = false;
    
    // Get initial spline length
    SplineLength = PathSpline->GetSplineLength();
    SplinePointCount = PathSpline->GetNumberOfSplinePoints();
    
    // Set initial position based on InitialPositionRatio
    CurrentSplineDistance = SplineLength * FMath::Clamp(InitialPositionRatio, 0.0f, 1.0f);
    
    // Initialize rotation
    if (SplineLength > 0)
    {
        FVector InitialLocation = PathSpline->GetLocationAtDistanceAlongSpline(CurrentSplineDistance, ESplineCoordinateSpace::World);
        FVector InitialDirection = PathSpline->GetDirectionAtDistanceAlongSpline(CurrentSplineDistance, ESplineCoordinateSpace::World);
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
    
    if (!bIsEditorPreview && bIsMoving && SplineLength > 0)
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

void ASphericalTrain::UpdateSplineSettings()
{
    if (!PathSpline) return;
    
    // Update spline visualization settings
    PathSpline->bDrawDebug = bShowSplineInEditor;
    PathSpline->SetClosedLoop(bClosedLoop);
    PathSpline->EditorUnselectedSplineSegmentColor = FLinearColor(SplineEditorColor);
    PathSpline->SetUnselectedSplineSegmentColor(SplineEditorColor);
    
    // Update spline
    PathSpline->UpdateSpline();
    SplineLength = PathSpline->GetSplineLength();
    SplinePointCount = PathSpline->GetNumberOfSplinePoints();
}

void ASphericalTrain::PreviewTrainPosition()
{
    if (!TrainMesh || SplineLength <= 0) return;
    
    // Calculate preview position
    float PreviewDistance = SplineLength * FMath::Clamp(InitialPositionRatio, 0.0f, 1.0f);
    
    // Get position and direction
    FVector PreviewLocation = PathSpline->GetLocationAtDistanceAlongSpline(PreviewDistance, ESplineCoordinateSpace::World);
    FVector PreviewDirection = PathSpline->GetDirectionAtDistanceAlongSpline(PreviewDistance, ESplineCoordinateSpace::World);
    
    // Calculate rotation
    FQuat PreviewRotation = CalculateTargetRotation(PreviewLocation, PreviewDirection);
    
    // Apply rotation offset
    if (!TrainRotationOffset.IsZero())
    {
        PreviewRotation = PreviewRotation * FQuat(TrainRotationOffset);
    }
    
    // Set preview transform
    TrainMesh->SetWorldLocation(PreviewLocation);
    TrainMesh->SetWorldRotation(PreviewRotation);
    
    // Update train cars preview
    for (int32 i = 0; i < TrainCars.Num(); i++)
    {
        if (!TrainCars[i]) continue;
        
        float CarDistance = PreviewDistance - (CarSeparationDistance * (i + 1));
        
        if (bLoopMovement)
        {
            CarDistance = FMath::Fmod(CarDistance + SplineLength * 2, SplineLength);
            if (CarDistance < 0) CarDistance += SplineLength;
        }
        else
        {
            CarDistance = FMath::Clamp(CarDistance, 0.0f, SplineLength);
        }
        
        FVector CarLocation = PathSpline->GetLocationAtDistanceAlongSpline(CarDistance, ESplineCoordinateSpace::World);
        FVector CarDirection = PathSpline->GetDirectionAtDistanceAlongSpline(CarDistance, ESplineCoordinateSpace::World);
        FQuat CarRotation = CalculateTargetRotation(CarLocation, CarDirection);
        
        if (!TrainRotationOffset.IsZero())
        {
            CarRotation = CarRotation * FQuat(TrainRotationOffset);
        }
        
        TrainCars[i]->SetWorldLocation(CarLocation);
        TrainCars[i]->SetWorldRotation(CarRotation);
    }
}

void ASphericalTrain::GenerateCircularSpline()
{
    if (!PlanetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("SphericalTrain: No planet actor assigned!"));
        return;
    }
    
    SetupSplineAroundPlanet();
    UpdateSplineSettings();
    PreviewTrainPosition();
}

void ASphericalTrain::ClearSplinePoints()
{
    PathSpline->ClearSplinePoints();
    PathSpline->UpdateSpline();
    SplineLength = 0;
    SplinePointCount = 0;
}

void ASphericalTrain::ReverseSplineDirection()
{
    TArray<FVector> Points;
    TArray<FVector> ArriveTangents;
    TArray<FVector> LeaveTangents;
    
    // Collect points in reverse order
    for (int32 i = PathSpline->GetNumberOfSplinePoints() - 1; i >= 0; i--)
    {
        Points.Add(PathSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
        
        // Swap and negate tangents
        FVector ArriveTan = PathSpline->GetArriveTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
        FVector LeaveTan = PathSpline->GetLeaveTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
        ArriveTangents.Add(-LeaveTan);
        LeaveTangents.Add(-ArriveTan);
    }
    
    // Clear and rebuild spline
    PathSpline->ClearSplinePoints();
    
    for (int32 i = 0; i < Points.Num(); i++)
    {
        PathSpline->AddSplineWorldPoint(Points[i]);
        PathSpline->SetTangentsAtSplinePoint(i, ArriveTangents[i], LeaveTangents[i], ESplineCoordinateSpace::World);
    }
    
    PathSpline->UpdateSpline();
    PreviewTrainPosition();
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
    
    // Rotate by start angle
    if (!FMath::IsNearlyZero(SplineStartAngle))
    {
        FQuat StartRotation = FQuat(SplineNormalAxis, FMath::DegreesToRadians(SplineStartAngle));
        Forward = StartRotation.RotateVector(Forward);
        Right = StartRotation.RotateVector(Right);
    }
    
    for (int32 i = 0; i < NumberOfSplinePoints; i++)
    {
        float Angle = FMath::DegreesToRadians(AngleStep * i);
        
        // Create point on circle
        FVector LocalPoint = (Forward * FMath::Cos(Angle) + Right * FMath::Sin(Angle)) * SplineRadius;
        FVector WorldPoint = PlanetCenter + LocalPoint;
        
        // Add point to spline
        PathSpline->AddSplineWorldPoint(WorldPoint);
        
        // Snap to planet surface and set proper normal
        SnapSplinePointToPlanetSurface(i, WorldPoint);
        
        // FIXED: Set the up vector (normal) to point radially outward from planet center
        FVector RadialDirection = (WorldPoint - PlanetCenter).GetSafeNormal();
        PathSpline->SetUpVectorAtSplinePoint(i, RadialDirection, ESplineCoordinateSpace::World);
    }
    
    // Set spline to closed loop
    PathSpline->SetClosedLoop(true);
    
    // Update spline with automatic tangents
    PathSpline->UpdateSpline();
    
    // FIXED: After updating spline, ensure all up vectors are still correct
    for (int32 i = 0; i < PathSpline->GetNumberOfSplinePoints(); i++)
    {
        FVector PointLocation = PathSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        FVector RadialDirection = (PointLocation - PlanetCenter).GetSafeNormal();
        PathSpline->SetUpVectorAtSplinePoint(i, RadialDirection, ESplineCoordinateSpace::World);
    }
    
    // Final spline update
    PathSpline->UpdateSpline();
    
    SplineLength = PathSpline->GetSplineLength();
    SplinePointCount = PathSpline->GetNumberOfSplinePoints();
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
    
    if (GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, 
                                             ECollisionChannel::ECC_WorldStatic, QueryParams))
    {
        if (HitResult.GetActor() == PlanetActor)
        {
            SurfacePoint = HitResult.Location + HitResult.Normal * SplineHeightOffset;
            // Use the hit normal if we have a complex surface
            DirectionFromCenter = HitResult.Normal;
        }
    }
    
    // Update spline point location
    PathSpline->SetLocationAtSplinePoint(PointIndex, SurfacePoint, ESplineCoordinateSpace::World);
    
    // FIXED: Set the up vector (normal) to point radially outward from planet center
    // This ensures the spline control point normal is directed from the center of the planet
    FVector RadialDirection = (SurfacePoint - PlanetCenter).GetSafeNormal();
    PathSpline->SetUpVectorAtSplinePoint(PointIndex, RadialDirection, ESplineCoordinateSpace::World);
}

void ASphericalTrain::SnapAllPointsToSurface()
{
    if (!PlanetActor) return;
    
    FVector PlanetCenter = GetPlanetCenter();
    
    for (int32 i = 0; i < PathSpline->GetNumberOfSplinePoints(); i++)
    {
        FVector CurrentLocation = PathSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        SnapSplinePointToPlanetSurface(i, CurrentLocation);
        
        // FIXED: Ensure up vector is set correctly after snapping
        FVector PointLocation = PathSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        FVector RadialDirection = (PointLocation - PlanetCenter).GetSafeNormal();
        PathSpline->SetUpVectorAtSplinePoint(i, RadialDirection, ESplineCoordinateSpace::World);
    }
    
    PathSpline->UpdateSpline();
    SplineLength = PathSpline->GetSplineLength();
    PreviewTrainPosition();
}

void ASphericalTrain::AddSplinePointAtActor()
{
    FVector ActorLocation = GetActorLocation();
    FVector PlanetCenter = GetPlanetCenter();
    
    if (PlanetActor)
    {
        // Snap to planet surface
        FVector Direction = (ActorLocation - PlanetCenter).GetSafeNormal();
        ActorLocation = PlanetCenter + Direction * (PlanetRadius + SplineHeightOffset);
    }
    
    // Add the point
    int32 NewPointIndex = PathSpline->GetNumberOfSplinePoints();
    PathSpline->AddSplineWorldPoint(ActorLocation);
    
    // FIXED: Set the up vector (normal) to point radially outward from planet center
    if (PlanetActor)
    {
        FVector RadialDirection = (ActorLocation - PlanetCenter).GetSafeNormal();
        PathSpline->SetUpVectorAtSplinePoint(NewPointIndex, RadialDirection, ESplineCoordinateSpace::World);
    }
    
    PathSpline->UpdateSpline();
    SplineLength = PathSpline->GetSplineLength();
    SplinePointCount = PathSpline->GetNumberOfSplinePoints();
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
        return;
    }
    
    // Calculate speed with direction
    float ActualSpeed = bReverseDirection ? -TrainSpeed : TrainSpeed;
    
    // Update distance along spline
    CurrentSplineDistance += ActualSpeed * DeltaTime;
    
    // Handle looping
    if (bLoopMovement)
    {
        CurrentSplineDistance = FMath::Fmod(CurrentSplineDistance, SplineLength);
        if (CurrentSplineDistance < 0.0f)
        {
            CurrentSplineDistance += SplineLength;
        }
    }
    else
    {
        CurrentSplineDistance = FMath::Clamp(CurrentSplineDistance, 0.0f, SplineLength);
        
        // Stop at ends if not looping
        if ((CurrentSplineDistance >= SplineLength && ActualSpeed > 0) ||
            (CurrentSplineDistance <= 0.0f && ActualSpeed < 0))
        {
            bIsMoving = false;
        }
    }
    
    // Get position from spline
    FVector NewLocation = PathSpline->GetLocationAtDistanceAlongSpline(CurrentSplineDistance, 
                                                                       ESplineCoordinateSpace::World);
    
    if (!NewLocation.IsNearlyZero())
    {
        TrainMesh->SetWorldLocation(NewLocation);
    }
}

void ASphericalTrain::UpdateTrainOrientationImproved(float DeltaTime)
{
    // Get current spline tangent
    FVector SplineTangent = PathSpline->GetDirectionAtDistanceAlongSpline(CurrentSplineDistance, 
                                                                           ESplineCoordinateSpace::World);
    SplineTangent.Normalize();
    
    // Reverse direction if needed
    if (bReverseDirection)
    {
        SplineTangent = -SplineTangent;
    }
    
    // Get train location
    FVector TrainLocation = TrainMesh->GetComponentLocation();
    
    // Calculate base rotation
    FQuat TargetRotation = CalculateTargetRotation(TrainLocation, SplineTangent);
    
    // Apply rotation offset
    if (!TrainRotationOffset.IsZero())
    {
        TargetRotation = TargetRotation * FQuat(TrainRotationOffset);
    }
    
    // Calculate and apply banking
    if (bBankOnCurves)
    {
        float TargetBankAngle = CalculateBankAngle(DeltaTime);
        CurrentBankAngle = FMath::FInterpTo(CurrentBankAngle, TargetBankAngle, DeltaTime, 5.0f);
        
        if (!FMath::IsNearlyZero(CurrentBankAngle))
        {
            FVector LocalForward = TargetRotation.GetForwardVector();
            FQuat BankRotation = FQuat(LocalForward, FMath::DegreesToRadians(CurrentBankAngle));
            TargetRotation = BankRotation * TargetRotation;
        }
    }
    
    // Apply rotation
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
    
    LastSplineDirection = SplineTangent;
}

// [Previous implementation methods remain the same: CalculateTargetRotation, CalculateBankAngle, UpdateTrainCarsImproved, DrawDebugVisualization]
// These methods are identical to the previous version, so I'll include just the signatures for brevity

FQuat ASphericalTrain::CalculateTargetRotation(const FVector& Location, const FVector& SplineDirection) const
{
    FVector PlanetCenter = GetPlanetCenter();
    FVector UpVector = (Location - PlanetCenter).GetSafeNormal();
    FVector TangentDirection = SplineDirection - (SplineDirection | UpVector) * UpVector;
    TangentDirection.Normalize();
    
    if (TangentDirection.IsNearlyZero(0.01f))
    {
        FVector ArbitraryVector = FMath::Abs(UpVector.Z) < 0.9f ? FVector::UpVector : FVector::RightVector;
        TangentDirection = FVector::CrossProduct(UpVector, ArbitraryVector).GetSafeNormal();
    }
    
    FVector RightVector = FVector::CrossProduct(TangentDirection, UpVector).GetSafeNormal();
    FVector ForwardVector = FVector::CrossProduct(UpVector, RightVector).GetSafeNormal();
    
    FRotator ResultRotator = UKismetMathLibrary::MakeRotFromXZ(ForwardVector, UpVector);
    return ResultRotator.Quaternion();
}

float ASphericalTrain::CalculateBankAngle(float DeltaTime) const
{
    if (DeltaTime <= 0.0f || FMath::Abs(TrainSpeed) <= 0.0f) return 0.0f;
    
    const float SampleDistance = 50.0f;
    FVector CurrentDir = PathSpline->GetDirectionAtDistanceAlongSpline(CurrentSplineDistance, 
                                                                        ESplineCoordinateSpace::World);
    
    float LookAheadDist = CurrentSplineDistance + (bReverseDirection ? -SampleDistance : SampleDistance);
    if (bLoopMovement)
    {
        LookAheadDist = FMath::Fmod(LookAheadDist + SplineLength * 2, SplineLength);
        if (LookAheadDist < 0) LookAheadDist += SplineLength;
    }
    else
    {
        LookAheadDist = FMath::Clamp(LookAheadDist, 0.0f, SplineLength);
    }
    
    FVector NextDir = PathSpline->GetDirectionAtDistanceAlongSpline(LookAheadDist, 
                                                                     ESplineCoordinateSpace::World);
    
    float DotProduct = FVector::DotProduct(CurrentDir.GetSafeNormal(), NextDir.GetSafeNormal());
    DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
    float AngleChange = FMath::Acos(DotProduct);
    
    FVector CrossProduct = FVector::CrossProduct(CurrentDir, NextDir);
    FVector PlanetCenter = GetPlanetCenter();
    FVector TrainLocation = TrainMesh->GetComponentLocation();
    FVector UpVector = (TrainLocation - PlanetCenter).GetSafeNormal();
    
    float TurnDirection = FVector::DotProduct(CrossProduct, UpVector);
    float TurnSharpness = AngleChange / (SampleDistance / 1000.0f);
    float SpeedFactor = FMath::Min(FMath::Abs(TrainSpeed) / 500.0f, 1.0f);
    float BankAngle = TurnSharpness * SpeedFactor * MaxBankAngle * -FMath::Sign(TurnDirection);
    
    if (bReverseDirection) BankAngle = -BankAngle;
    
    return FMath::Clamp(BankAngle, -MaxBankAngle, MaxBankAngle);
}

void ASphericalTrain::UpdateTrainCarsImproved(float DeltaTime)
{
    for (int32 i = 0; i < TrainCars.Num(); i++)
    {
        if (!TrainCars[i]) continue;
        
        float CarDistance = CurrentSplineDistance - (CarSeparationDistance * (i + 1) * (bReverseDirection ? -1 : 1));
        
        if (bLoopMovement)
        {
            CarDistance = FMath::Fmod(CarDistance + SplineLength * 2, SplineLength);
            if (CarDistance < 0) CarDistance += SplineLength;
        }
        else
        {
            CarDistance = FMath::Clamp(CarDistance, 0.0f, SplineLength);
        }
        
        FVector CarLocation = PathSpline->GetLocationAtDistanceAlongSpline(CarDistance, 
                                                                          ESplineCoordinateSpace::World);
        FVector CarDirection = PathSpline->GetDirectionAtDistanceAlongSpline(CarDistance, 
                                                                            ESplineCoordinateSpace::World);
        
        if (bReverseDirection) CarDirection = -CarDirection;
        
        TrainCars[i]->SetWorldLocation(CarLocation);
        
        FQuat CarRotation = CalculateTargetRotation(CarLocation, CarDirection);
        
        if (!TrainRotationOffset.IsZero())
        {
            CarRotation = CarRotation * FQuat(TrainRotationOffset);
        }
        
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
    // [Same implementation as before - draws debug lines, spheres, and text]
    // Keeping this method identical to the previous version for consistency
    if (!GetWorld()) return;
    
    if (bShowDebugSpline)
    {
        const int32 Segments = 100;
        for (int32 i = 0; i < Segments; i++)
        {
            float Distance1 = (SplineLength / Segments) * i;
            float Distance2 = (SplineLength / Segments) * (i + 1);
            
            FVector Point1 = PathSpline->GetLocationAtDistanceAlongSpline(Distance1, ESplineCoordinateSpace::World);
            FVector Point2 = PathSpline->GetLocationAtDistanceAlongSpline(Distance2, ESplineCoordinateSpace::World);
            
            float Hue = (float)i / Segments * 360.0f;
            FLinearColor Color = FLinearColor::MakeFromHSV8(Hue, 255, 255);
            
            DrawDebugLine(GetWorld(), Point1, Point2, Color.ToFColor(true), false, 0.01f, 0, 2.0f);
        }
        
        for (int32 i = 0; i < PathSpline->GetNumberOfSplinePoints(); i++)
        {
            FVector PointLocation = PathSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
            DrawDebugSphere(GetWorld(), PointLocation, 20.0f, 8, FColor::Red, false, 0.01f);
            DrawDebugString(GetWorld(), PointLocation + FVector(0, 0, 30), FString::FromInt(i), 
                          nullptr, FColor::White, 0.01f, true);
        }
        
        FVector CurrentPos = PathSpline->GetLocationAtDistanceAlongSpline(CurrentSplineDistance, 
                                                                         ESplineCoordinateSpace::World);
        DrawDebugSphere(GetWorld(), CurrentPos, 30.0f, 12, FColor::Green, false, 0.01f);
    }
    
    if (bShowOrientationVectors)
    {
        FVector TrainLocation = TrainMesh->GetComponentLocation();
        FVector PlanetCenter = GetPlanetCenter();
        
        FVector MeshForward = TrainMesh->GetForwardVector();
        FVector MeshRight = TrainMesh->GetRightVector();
        FVector MeshUp = TrainMesh->GetUpVector();
        
        DrawDebugDirectionalArrow(GetWorld(), TrainLocation, TrainLocation + MeshForward * 150, 
                                 30.0f, FColor::Red, false, 0.01f, 0, 5.0f);
        DrawDebugString(GetWorld(), TrainLocation + MeshForward * 160, TEXT("Forward"), 
                       nullptr, FColor::Red, 0.01f, true);
        
        DrawDebugDirectionalArrow(GetWorld(), TrainLocation, TrainLocation + MeshRight * 100, 
                                 20.0f, FColor::Green, false, 0.01f, 0, 5.0f);
        DrawDebugString(GetWorld(), TrainLocation + MeshRight * 110, TEXT("Right"), 
                       nullptr, FColor::Green, 0.01f, true);
        
        DrawDebugDirectionalArrow(GetWorld(), TrainLocation, TrainLocation + MeshUp * 200, 
                                 40.0f, FColor::Blue, false, 0.01f, 0, 5.0f);
        DrawDebugString(GetWorld(), TrainLocation + MeshUp * 210, TEXT("Up"), 
                       nullptr, FColor::Blue, 0.01f, true);
        
        FVector PlanetNormal = (TrainLocation - PlanetCenter).GetSafeNormal();
        DrawDebugLine(GetWorld(), TrainLocation, TrainLocation + PlanetNormal * 180, 
                     FColor::Cyan, false, 0.01f, 0, 3.0f);
        DrawDebugString(GetWorld(), TrainLocation + PlanetNormal * 190, TEXT("Planet Normal"), 
                       nullptr, FColor::Cyan, 0.01f, true);
        
        FVector SplineTangent = PathSpline->GetDirectionAtDistanceAlongSpline(CurrentSplineDistance, 
                                                                              ESplineCoordinateSpace::World);
        if (bReverseDirection) SplineTangent = -SplineTangent;
        DrawDebugLine(GetWorld(), TrainLocation, TrainLocation + SplineTangent * 120, 
                     FColor::Yellow, false, 0.01f, 0, 3.0f);
        DrawDebugString(GetWorld(), TrainLocation + SplineTangent * 130, TEXT("Spline Dir"), 
                       nullptr, FColor::Yellow, 0.01f, true);
        
        DrawDebugLine(GetWorld(), TrainLocation, PlanetCenter, 
                     FColor::Magenta, false, 0.01f, 0, 1.0f);
    }
    
    if (bShowDebugInfo)
    {
        FVector TrainLocation = TrainMesh->GetComponentLocation();
        
        FString DebugText = FString::Printf(
            TEXT("Speed: %.1f units/s %s\n")
            TEXT("Distance: %.1f / %.1f\n")
            TEXT("Progress: %.1f%%\n")
            TEXT("Bank Angle: %.1f°\n")
            TEXT("Spline Points: %d\n")
            TEXT("Moving: %s | Loop: %s"),
            TrainSpeed,
            bReverseDirection ? TEXT("(Reverse)") : TEXT(""),
            CurrentSplineDistance, 
            SplineLength,
            (CurrentSplineDistance / SplineLength) * 100.0f,
            CurrentBankAngle,
            SplinePointCount,
            bIsMoving ? TEXT("YES") : TEXT("NO"),
            bLoopMovement ? TEXT("ON") : TEXT("OFF")
        );
        
        DrawDebugString(GetWorld(), TrainLocation + FVector(0, 0, 250), DebugText, 
                       nullptr, FColor::White, 0.01f, true, 1.2f);
        
        FRotator CurrentRot = CurrentRotation.Rotator();
        FString RotText = FString::Printf(TEXT("Rotation: P=%.1f Y=%.1f R=%.1f"), 
                                         CurrentRot.Pitch, CurrentRot.Yaw, CurrentRot.Roll);
        DrawDebugString(GetWorld(), TrainLocation + FVector(0, 0, 100), RotText, 
                       nullptr, FColor::Yellow, 0.01f, true);
    }
}