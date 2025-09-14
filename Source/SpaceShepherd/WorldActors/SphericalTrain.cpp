// SphericalTrain.cpp
#include "SphericalTrain.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Kismet/KismetMathLibrary.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

ASphericalTrain::ASphericalTrain()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create root component
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    RootComponent = RootSceneComponent;

    // Create spherical spline component
    PathSpline = CreateDefaultSubobject<USphericalSplineComponent>(TEXT("PathSpline"));
    PathSpline->SetupAttachment(RootComponent);
    
    // Create train mesh
    TrainMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrainMesh"));
    TrainMesh->SetupAttachment(RootComponent);
    TrainMesh->SetRelativeLocation(FVector(0, 0, 0));
}

void ASphericalTrain::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    bIsInEditor = true;
    
    // Update preview if enabled
    if (bShowTrainPreview && PathSpline)
    {
        PreviewTrainPosition();
    }
}

#if WITH_EDITOR
void ASphericalTrain::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (!PropertyChangedEvent.Property)
        return;
    
    FName PropertyName = PropertyChangedEvent.Property->GetFName();
    
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ASphericalTrain, InitialPositionRatio) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ASphericalTrain, bShowTrainPreview))
    {
        PreviewTrainPosition();
    }
}
#endif

void ASphericalTrain::BeginPlay()
{
    Super::BeginPlay();
    
    bIsInEditor = false;
    
    if (!PathSpline)
    {
        UE_LOG(LogTemp, Error, TEXT("SphericalTrain: No PathSpline component found!"));
        return;
    }
    
    // Set initial position
    float SplineLength = PathSpline->GetSplineLength();
    if (SplineLength > 0)
    {
        CurrentSplineDistance = SplineLength * FMath::Clamp(InitialPositionRatio, 0.0f, 1.0f);
        SetTrainPosition(CurrentSplineDistance);
        
        // Initialize rotation
        FVector InitialLocation = PathSpline->GetLocationAtDistanceAlongSpline(
            CurrentSplineDistance, ESplineCoordinateSpace::World);
        FVector InitialDirection = PathSpline->GetDirectionAtDistanceAlongSpline(
            CurrentSplineDistance, ESplineCoordinateSpace::World);
        
        CurrentRotation = CalculateTargetRotation(InitialLocation, InitialDirection);
        LastSplineDirection = InitialDirection;
        
        TrainMesh->SetWorldLocation(InitialLocation);
        TrainMesh->SetWorldRotation(CurrentRotation);
    }
    
    // Setup train cars mobility
    for (UStaticMeshComponent* Car : TrainCars)
    {
        if (Car)
        {
            Car->SetMobility(EComponentMobility::Movable);
        }
    }
}

void ASphericalTrain::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Update movement if not in editor
    if (!bIsInEditor && bIsMoving && PathSpline && PathSpline->GetSplineLength() > 0)
    {
        UpdateTrainMovement(DeltaTime);
        UpdateTrainOrientation(DeltaTime);
        UpdateTrainCars(DeltaTime);
    }
    
    // Preview in play mode if enabled
    if (bIsInEditor && bPreviewInPlayMode && PathSpline)
    {
        PreviewTrainPosition();
    }
    
    // Draw debug visualization
    if (bShowDebugInfo || bShowOrientationVectors || bShowMovementPath)
    {
        DrawDebugVisualization();
    }
}

void ASphericalTrain::UpdateTrainMovement(float DeltaTime)
{
    if (!PathSpline)
        return;
    
    float SplineLength = PathSpline->GetSplineLength();
    if (SplineLength <= 0.0f)
        return;
    
    // Calculate actual speed with direction
    CurrentSpeed = bReverseDirection ? -TrainSpeed : TrainSpeed;
    
    // Update distance along spline
    CurrentSplineDistance += CurrentSpeed * DeltaTime;
    
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
        if ((CurrentSplineDistance >= SplineLength && CurrentSpeed > 0) ||
            (CurrentSplineDistance <= 0.0f && CurrentSpeed < 0))
        {
            bIsMoving = false;
            CurrentSpeed = 0.0f;
        }
    }
    
    // Update position
    FVector NewLocation = PathSpline->GetLocationAtDistanceAlongSpline(
        CurrentSplineDistance, ESplineCoordinateSpace::World);
    
    if (!NewLocation.IsNearlyZero())
    {
        TrainMesh->SetWorldLocation(NewLocation);
    }
}

void ASphericalTrain::UpdateTrainOrientation(float DeltaTime)
{
    if (!PathSpline || !TrainMesh)
        return;
    
    // Get current spline tangent
    FVector SplineTangent = PathSpline->GetDirectionAtDistanceAlongSpline(
        CurrentSplineDistance, ESplineCoordinateSpace::World);
    SplineTangent.Normalize();
    
    // Reverse direction if needed
    if (bReverseDirection)
    {
        SplineTangent = -SplineTangent;
    }
    
    // Get train location
    FVector TrainLocation = TrainMesh->GetComponentLocation();
    
    // Calculate base rotation using spherical spline orientation
    FQuat TargetRotation = PathSpline->GetOrientationAtDistance(CurrentSplineDistance);
    
    // If we need to face the opposite direction
    if (bReverseDirection)
    {
        TargetRotation = TargetRotation * FQuat(FVector::UpVector, FMath::DegreesToRadians(180.0f));
    }
    
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
    
    // Apply rotation with smoothing
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

void ASphericalTrain::UpdateTrainCars(float DeltaTime)
{
    if (!PathSpline)
        return;
    
    float SplineLength = PathSpline->GetSplineLength();
    
    for (int32 i = 0; i < TrainCars.Num(); i++)
    {
        if (!TrainCars[i])
            continue;
        
        // Calculate car distance along spline
        float CarDistance = CurrentSplineDistance - 
            (CarSeparationDistance * (i + 1) * (bReverseDirection ? -1 : 1));
        
        // Handle looping
        if (bLoopMovement)
        {
            CarDistance = FMath::Fmod(CarDistance + SplineLength * 2, SplineLength);
            if (CarDistance < 0) 
                CarDistance += SplineLength;
        }
        else
        {
            CarDistance = FMath::Clamp(CarDistance, 0.0f, SplineLength);
        }
        
        // Get car position and orientation
        FVector CarLocation = PathSpline->GetLocationAtDistanceAlongSpline(
            CarDistance, ESplineCoordinateSpace::World);
        
        FQuat CarRotation = PathSpline->GetOrientationAtDistance(CarDistance);
        
        if (bReverseDirection)
        {
            CarRotation = CarRotation * FQuat(FVector::UpVector, FMath::DegreesToRadians(180.0f));
        }
        
        if (!TrainRotationOffset.IsZero())
        {
            CarRotation = CarRotation * FQuat(TrainRotationOffset);
        }
        
        // Apply position and rotation
        TrainCars[i]->SetWorldLocation(CarLocation);
        
        if (bUseSmoothRotation)
        {
            FQuat CurrentCarRotation = TrainCars[i]->GetComponentQuat();
            CarRotation = FQuat::Slerp(CurrentCarRotation, CarRotation, DeltaTime * RotationSmoothSpeed);
        }
        
        TrainCars[i]->SetWorldRotation(CarRotation);
    }
}

FQuat ASphericalTrain::CalculateTargetRotation(const FVector& Location, const FVector& Direction) const
{
    if (!PathSpline || !PathSpline->PlanetActor)
    {
        // Fallback to simple rotation
        return FRotationMatrix::MakeFromX(Direction).ToQuat();
    }
    
    FVector PlanetCenter = PathSpline->GetPlanetCenter();
    FVector UpVector = (Location - PlanetCenter).GetSafeNormal();
    
    // Project direction to be perpendicular to up vector
    FVector TangentDirection = Direction - (Direction | UpVector) * UpVector;
    TangentDirection.Normalize();
    
    if (TangentDirection.IsNearlyZero(0.01f))
    {
        FVector ArbitraryVector = FMath::Abs(UpVector.Z) < 0.9f ? 
            FVector::UpVector : FVector::RightVector;
        TangentDirection = FVector::CrossProduct(UpVector, ArbitraryVector).GetSafeNormal();
    }
    
    FVector RightVector = FVector::CrossProduct(TangentDirection, UpVector).GetSafeNormal();
    FVector ForwardVector = FVector::CrossProduct(UpVector, RightVector).GetSafeNormal();
    
    FRotator ResultRotator = UKismetMathLibrary::MakeRotFromXZ(ForwardVector, UpVector);
    return ResultRotator.Quaternion();
}

float ASphericalTrain::CalculateBankAngle(float DeltaTime) const
{
    if (!PathSpline || DeltaTime <= 0.0f || FMath::Abs(TrainSpeed) <= 0.0f)
        return 0.0f;
    
    const float SampleDistance = 50.0f;
    float SplineLength = PathSpline->GetSplineLength();
    
    // Get current and look-ahead directions
    FVector CurrentDir = PathSpline->GetDirectionAtDistanceAlongSpline(
        CurrentSplineDistance, ESplineCoordinateSpace::World);
    
    float LookAheadDist = CurrentSplineDistance + 
        (bReverseDirection ? -SampleDistance : SampleDistance);
    
    if (bLoopMovement)
    {
        LookAheadDist = FMath::Fmod(LookAheadDist + SplineLength * 2, SplineLength);
        if (LookAheadDist < 0) 
            LookAheadDist += SplineLength;
    }
    else
    {
        LookAheadDist = FMath::Clamp(LookAheadDist, 0.0f, SplineLength);
    }
    
    FVector NextDir = PathSpline->GetDirectionAtDistanceAlongSpline(
        LookAheadDist, ESplineCoordinateSpace::World);
    
    // Calculate turn sharpness
    float DotProduct = FVector::DotProduct(CurrentDir.GetSafeNormal(), NextDir.GetSafeNormal());
    DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
    float AngleChange = FMath::Acos(DotProduct);
    
    // Determine turn direction
    FVector CrossProduct = FVector::CrossProduct(CurrentDir, NextDir);
    FVector UpVector = PathSpline->GetUpVectorAtDistance(CurrentSplineDistance);
    float TurnDirection = FVector::DotProduct(CrossProduct, UpVector);
    
    // Calculate bank angle
    float TurnSharpness = AngleChange / (SampleDistance / 1000.0f);
    float SpeedFactor = FMath::Min(FMath::Abs(TrainSpeed) / 500.0f, 1.0f);
    float BankAngle = TurnSharpness * SpeedFactor * MaxBankAngle * -FMath::Sign(TurnDirection);
    
    if (bReverseDirection) 
        BankAngle = -BankAngle;
    
    return FMath::Clamp(BankAngle, -MaxBankAngle, MaxBankAngle);
}

void ASphericalTrain::PreviewTrainPosition()
{
    if (!TrainMesh || !PathSpline)
        return;
    
    float SplineLength = PathSpline->GetSplineLength();
    if (SplineLength <= 0.0f)
        return;
    
    // Calculate preview position
    float PreviewDistance = SplineLength * FMath::Clamp(InitialPositionRatio, 0.0f, 1.0f);
    SetTrainPosition(PreviewDistance);
}

void ASphericalTrain::SetTrainPosition(float Distance)
{
    if (!PathSpline || !TrainMesh)
        return;
    
    float SplineLength = PathSpline->GetSplineLength();
    if (SplineLength <= 0.0f)
        return;
    
    // Clamp distance
    Distance = FMath::Clamp(Distance, 0.0f, SplineLength);
    
    // Get position and rotation from spline
    FVector Location = PathSpline->GetLocationAtDistanceAlongSpline(
        Distance, ESplineCoordinateSpace::World);
    FQuat Rotation = PathSpline->GetOrientationAtDistance(Distance);
    
    // Apply rotation offset
    if (!TrainRotationOffset.IsZero())
    {
        Rotation = Rotation * FQuat(TrainRotationOffset);
    }
    
    // Set transform
    TrainMesh->SetWorldLocation(Location);
    TrainMesh->SetWorldRotation(Rotation);
    
    // Update train cars
    for (int32 i = 0; i < TrainCars.Num(); i++)
    {
        if (!TrainCars[i])
            continue;
        
        float CarDistance = Distance - (CarSeparationDistance * (i + 1));
        
        if (bLoopMovement)
        {
            CarDistance = FMath::Fmod(CarDistance + SplineLength * 2, SplineLength);
            if (CarDistance < 0) 
                CarDistance += SplineLength;
        }
        else
        {
            CarDistance = FMath::Clamp(CarDistance, 0.0f, SplineLength);
        }
        
        FVector CarLocation = PathSpline->GetLocationAtDistanceAlongSpline(
            CarDistance, ESplineCoordinateSpace::World);
        FQuat CarRotation = PathSpline->GetOrientationAtDistance(CarDistance);
        
        if (!TrainRotationOffset.IsZero())
        {
            CarRotation = CarRotation * FQuat(TrainRotationOffset);
        }
        
        TrainCars[i]->SetWorldLocation(CarLocation);
        TrainCars[i]->SetWorldRotation(CarRotation);
    }
}

void ASphericalTrain::DrawDebugVisualization() const
{
    if (bShowOrientationVectors)
        DrawDebugOrientationVectors();
    
    if (bShowMovementPath)
        DrawDebugMovementPath();
    
    if (bShowDebugInfo)
        DrawDebugInfo();
}

void ASphericalTrain::DrawDebugOrientationVectors() const
{
    if (!GetWorld() || !TrainMesh || !PathSpline)
        return;
    
    FVector TrainLocation = TrainMesh->GetComponentLocation();
    
    // Mesh vectors
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
    
    // Planet normal
    if (PathSpline->PlanetActor)
    {
        FVector PlanetCenter = PathSpline->GetPlanetCenter();
        FVector PlanetNormal = (TrainLocation - PlanetCenter).GetSafeNormal();
        DrawDebugLine(GetWorld(), TrainLocation, TrainLocation + PlanetNormal * 180, 
                     FColor::Cyan, false, 0.01f, 0, 3.0f);
        DrawDebugString(GetWorld(), TrainLocation + PlanetNormal * 190, TEXT("Planet Normal"), 
                       nullptr, FColor::Cyan, 0.01f, true);
    }
    
    // Spline direction
    FVector SplineTangent = PathSpline->GetDirectionAtDistanceAlongSpline(
        CurrentSplineDistance, ESplineCoordinateSpace::World);
    if (bReverseDirection) 
        SplineTangent = -SplineTangent;
    
    DrawDebugLine(GetWorld(), TrainLocation, TrainLocation + SplineTangent * 120, 
                 FColor::Yellow, false, 0.01f, 0, 3.0f);
    DrawDebugString(GetWorld(), TrainLocation + SplineTangent * 130, TEXT("Spline Dir"), 
                   nullptr, FColor::Yellow, 0.01f, true);
}

void ASphericalTrain::DrawDebugMovementPath() const
{
    if (!GetWorld() || !PathSpline)
        return;
    
    float SplineLength = PathSpline->GetSplineLength();
    const int32 Segments = 100;
    
    for (int32 i = 0; i < Segments; i++)
    {
        float Distance1 = (SplineLength / Segments) * i;
        float Distance2 = (SplineLength / Segments) * (i + 1);
        
        FVector Point1 = PathSpline->GetLocationAtDistanceAlongSpline(
            Distance1, ESplineCoordinateSpace::World);
        FVector Point2 = PathSpline->GetLocationAtDistanceAlongSpline(
            Distance2, ESplineCoordinateSpace::World);
        
        float Hue = (float)i / Segments * 360.0f;
        FLinearColor Color = FLinearColor::MakeFromHSV8(Hue, 255, 255);
        
        DrawDebugLine(GetWorld(), Point1, Point2, Color.ToFColor(true), false, 0.01f, 0, 2.0f);
    }
    
    // Draw current position
    FVector CurrentPos = PathSpline->GetLocationAtDistanceAlongSpline(
        CurrentSplineDistance, ESplineCoordinateSpace::World);
    DrawDebugSphere(GetWorld(), CurrentPos, 30.0f, 12, FColor::Green, false, 0.01f);
}

void ASphericalTrain::DrawDebugInfo() const
{
    if (!GetWorld() || !TrainMesh || !PathSpline)
        return;
    
    FVector TrainLocation = TrainMesh->GetComponentLocation();
    float SplineLength = PathSpline->GetSplineLength();
    
    FString DebugText = FString::Printf(
        TEXT("Speed: %.1f units/s %s\n")
        TEXT("Distance: %.1f / %.1f\n")
        TEXT("Progress: %.1f%%\n")
        TEXT("Bank Angle: %.1f°\n")
        TEXT("Moving: %s | Loop: %s"),
        CurrentSpeed,
        bReverseDirection ? TEXT("(Reverse)") : TEXT(""),
        CurrentSplineDistance, 
        SplineLength,
        GetCurrentProgress() * 100.0f,
        CurrentBankAngle,
        bIsMoving ? TEXT("YES") : TEXT("NO"),
        bLoopMovement ? TEXT("ON") : TEXT("OFF")
    );
    
    DrawDebugString(GetWorld(), TrainLocation + FVector(0, 0, 250), DebugText, 
                   nullptr, FColor::White, 0.01f, true, 1.2f);
}

// Public Blueprint Functions
void ASphericalTrain::StartMovement()
{
    bIsMoving = true;
}

void ASphericalTrain::StopMovement()
{
    bIsMoving = false;
    CurrentSpeed = 0.0f;
}

void ASphericalTrain::ToggleMovement()
{
    bIsMoving = !bIsMoving;
    if (!bIsMoving)
    {
        CurrentSpeed = 0.0f;
    }
}

void ASphericalTrain::SetSpeed(float NewSpeed)
{
    TrainSpeed = FMath::Clamp(NewSpeed, -2000.0f, 2000.0f);
}

void ASphericalTrain::ReverseDirection()
{
    bReverseDirection = !bReverseDirection;
}

void ASphericalTrain::SetPositionOnSpline(float NormalizedPosition)
{
    if (!PathSpline)
        return;
    
    float SplineLength = PathSpline->GetSplineLength();
    CurrentSplineDistance = SplineLength * FMath::Clamp(NormalizedPosition, 0.0f, 1.0f);
    SetTrainPosition(CurrentSplineDistance);
}

float ASphericalTrain::GetCurrentProgress() const
{
    if (!PathSpline)
        return 0.0f;
    
    float SplineLength = PathSpline->GetSplineLength();
    if (SplineLength <= 0.0f)
        return 0.0f;
    
    return CurrentSplineDistance / SplineLength;
}

FVector ASphericalTrain::GetCurrentLocation() const
{
    if (TrainMesh)
        return TrainMesh->GetComponentLocation();
    
    return FVector::ZeroVector;
}

FRotator ASphericalTrain::GetCurrentRotation() const
{
    if (TrainMesh)
        return TrainMesh->GetComponentRotation();
    
    return FRotator::ZeroRotator;
}

// Editor Functions
void ASphericalTrain::Editor_ResetToStart()
{
    InitialPositionRatio = 0.0f;
    PreviewTrainPosition();
}

void ASphericalTrain::Editor_PreviewAtMiddle()
{
    InitialPositionRatio = 0.5f;
    PreviewTrainPosition();
}

void ASphericalTrain::Editor_PreviewAtEnd()
{
    InitialPositionRatio = 1.0f;
    PreviewTrainPosition();
}