// SphericalSplineComponent.cpp
#include "SphericalSplineComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Kismet/KismetMathLibrary.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

USphericalSplineComponent::USphericalSplineComponent()
{
    // Setup default spline settings
    SetClosedLoop(true);
    bDrawDebug = true;
    bInputSplinePointsToConstructionScript = true;
    bShouldVisualizeScale = true;
    ScaleVisualizationWidth = 30.0f;
    
    // Set default editor colors
    EditorUnselectedSplineSegmentColor = FLinearColor(1.0f, 1.0f, 0.0f); // Yellow
    EditorSelectedSplineSegmentColor = FLinearColor(1.0f, 0.5f, 0.0f);   // Orange
}

void USphericalSplineComponent::OnComponentCreated()
{
    Super::OnComponentCreated();
    UpdateSplineInfo();
}

void USphericalSplineComponent::BeginPlay()
{
    Super::BeginPlay();
    UpdateSplineInfo();
}

#if WITH_EDITOR
void USphericalSplineComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (!PropertyChangedEvent.Property)
        return;
    
    FName PropertyName = PropertyChangedEvent.Property->GetFName();
    
    if (PropertyName == GET_MEMBER_NAME_CHECKED(USphericalSplineComponent, bAutoGenerateCircularPath))
    {
        if (bAutoGenerateCircularPath)
        {
            GenerateCircularPath();
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(USphericalSplineComponent, PlanetActor) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(USphericalSplineComponent, PlanetRadius) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(USphericalSplineComponent, HeightOffset))
    {
        if (bAutoSnapToSurface)
        {
            SnapAllPointsToSurface();
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(USphericalSplineComponent, CirclePointCount) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(USphericalSplineComponent, CircleRadius) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(USphericalSplineComponent, CircleNormalAxis) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(USphericalSplineComponent, CircleStartAngle))
    {
        if (bAutoGenerateCircularPath)
        {
            GenerateCircularPath();
        }
    }
    
    UpdateSplineInfo();
}

void USphericalSplineComponent::OnSplinePointMoved()
{
    if (bAutoSnapToSurface)
    {
        // Find which point was moved and snap it
        for (int32 i = 0; i < GetNumberOfSplinePoints(); i++)
        {
            SnapPointToSurface(i);
        }
        UpdateSplineInfo();
    }
}
#endif

void USphericalSplineComponent::GenerateCircularPath()
{
    if (!PlanetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("SphericalSplineComponent: No planet actor assigned!"));
        return;
    }
    
    // Clear existing points
    ClearSplinePoints();
    
    FVector PlanetCenter = GetPlanetCenter();
    
    // Normalize the normal axis
    FVector NormalAxis = CircleNormalAxis.GetSafeNormal();
    
    // Find two perpendicular vectors to create the circle plane
    FVector Right, Forward;
    
    if (FMath::Abs(NormalAxis.Z) < 0.9f)
    {
        Right = FVector::CrossProduct(NormalAxis, FVector::UpVector).GetSafeNormal();
    }
    else
    {
        Right = FVector::CrossProduct(NormalAxis, FVector::ForwardVector).GetSafeNormal();
    }
    Forward = FVector::CrossProduct(Right, NormalAxis).GetSafeNormal();
    
    // Apply start angle rotation
    if (!FMath::IsNearlyZero(CircleStartAngle))
    {
        FQuat StartRotation = FQuat(NormalAxis, FMath::DegreesToRadians(CircleStartAngle));
        Forward = StartRotation.RotateVector(Forward);
        Right = StartRotation.RotateVector(Right);
    }
    
    // Calculate the center point of the circular spline on the sphere surface
    // This is where we'll position the actor
    FVector CircleCenterOnSphere = PlanetCenter + NormalAxis * (PlanetRadius + HeightOffset);
    
    // Store points in world space first
    TArray<FVector> WorldSpacePoints;
    
    // Generate points in world space
    float AngleStep = 360.0f / CirclePointCount;
    
    for (int32 i = 0; i < CirclePointCount; i++)
    {
        float Angle = FMath::DegreesToRadians(AngleStep * i);
        
        // Create point on circle
        FVector LocalPoint = (Forward * FMath::Cos(Angle) + Right * FMath::Sin(Angle)) * CircleRadius;
        FVector WorldPoint = PlanetCenter + LocalPoint;
        
        // Snap to surface if needed
        if (bAutoSnapToSurface)
        {
            WorldPoint = ProjectPointToPlanetSurface(WorldPoint);
        }
        
        WorldSpacePoints.Add(WorldPoint);
    }
    
    // Move the owner actor to the center of the spline
    if (AActor* Owner = GetOwner())
    {
        // Calculate the actual center of the generated spline points
        FVector SplineCenter = FVector::ZeroVector;
        for (const FVector& Point : WorldSpacePoints)
        {
            SplineCenter += Point;
        }
        SplineCenter /= WorldSpacePoints.Num();
        
        // Set the actor's location to the spline center
        Owner->SetActorLocation(SplineCenter);
        
        // Now add the points as local positions relative to the new actor position
        for (const FVector& WorldPoint : WorldSpacePoints)
        {
            // Convert world position to local position relative to the actor
            FVector LocalPosition = Owner->GetActorTransform().InverseTransformPosition(WorldPoint);
            AddSplinePoint(LocalPosition, ESplineCoordinateSpace::Local);
        }
    }
    else
    {
        // Fallback: If no owner, add points in world space as before
        for (const FVector& WorldPoint : WorldSpacePoints)
        {
            AddSplineWorldPoint(WorldPoint);
        }
    }
    
    // Set normals for all points
    for (int32 i = 0; i < GetNumberOfSplinePoints(); i++)
    {
        SetSplinePointNormal(i);
    }
    
    // Ensure closed loop
    SetClosedLoop(true);
    
    // Update spline
    UpdateSpline();
    UpdateAllPointNormals();
    UpdateSplineInfo();
    
    // Log the actor position for debugging
    if (AActor* Owner = GetOwner())
    {
        UE_LOG(LogTemp, Log, TEXT("SphericalSplineComponent: Actor repositioned to spline center at %s"), 
               *Owner->GetActorLocation().ToString());
    }
}

void USphericalSplineComponent::SnapAllPointsToSurface()
{
    if (!PlanetActor) return;
    
    for (int32 i = 0; i < GetNumberOfSplinePoints(); i++)
    {
        SnapPointToSurface(i);
    }
    
    UpdateSpline();
    UpdateSplineInfo();
}

void USphericalSplineComponent::SnapPointToSurface(int32 PointIndex)
{
    if (!PlanetActor || PointIndex < 0 || PointIndex >= GetNumberOfSplinePoints())
        return;
    
    FVector CurrentLocation = GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
    FVector SurfacePoint = ProjectPointToPlanetSurface(CurrentLocation);
    
    // Update point location
    SetLocationAtSplinePoint(PointIndex, SurfacePoint, ESplineCoordinateSpace::World);
    
    // Update normal
    SetSplinePointNormal(PointIndex);
}

FVector USphericalSplineComponent::ProjectPointToPlanetSurface(const FVector& WorldPoint) const
{
    if (!PlanetActor)
        return WorldPoint;
    
    FVector PlanetCenter = GetPlanetCenter();
    
    // Get direction from planet center to point
    FVector DirectionFromCenter = (WorldPoint - PlanetCenter).GetSafeNormal();
    
    // Default to spherical projection
    FVector SurfacePoint = PlanetCenter + DirectionFromCenter * (PlanetRadius + HeightOffset);
    
    // Perform line trace for complex geometry if enabled
    if (bUseComplexCollisionForSnapping)
    {
        FHitResult HitResult;
        FVector TraceStart = PlanetCenter + DirectionFromCenter * (PlanetRadius * 2.0f);
        FVector TraceEnd = PlanetCenter;
        
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(GetOwner());
        
        if (GetWorld() && GetWorld()->LineTraceSingleByChannel(
            HitResult, TraceStart, TraceEnd, 
            ECollisionChannel::ECC_WorldStatic, QueryParams))
        {
            if (HitResult.GetActor() == PlanetActor)
            {
                SurfacePoint = HitResult.Location + HitResult.Normal * HeightOffset;
            }
        }
    }
    
    return SurfacePoint;
}

void USphericalSplineComponent::SetSplinePointNormal(int32 PointIndex)
{
    if (!PlanetActor || PointIndex < 0 || PointIndex >= GetNumberOfSplinePoints())
        return;
    
    FVector PointLocation = GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
    FVector PlanetCenter = GetPlanetCenter();
    FVector RadialDirection = (PointLocation - PlanetCenter).GetSafeNormal();
    
    SetUpVectorAtSplinePoint(PointIndex, RadialDirection, ESplineCoordinateSpace::World);
}

void USphericalSplineComponent::UpdateAllPointNormals()
{
    for (int32 i = 0; i < GetNumberOfSplinePoints(); i++)
    {
        SetSplinePointNormal(i);
    }
}

void USphericalSplineComponent::AddPointAtLocation(const FVector& WorldLocation, bool bSnapToSurface)
{
    FVector FinalLocation = WorldLocation;
    
    if (bSnapToSurface && PlanetActor)
    {
        FinalLocation = ProjectPointToPlanetSurface(WorldLocation);
    }
    
    int32 NewPointIndex = GetNumberOfSplinePoints();
    AddSplineWorldPoint(FinalLocation);
    
    if (PlanetActor)
    {
        SetSplinePointNormal(NewPointIndex);
    }
    
    UpdateSpline();
    UpdateSplineInfo();
}

void USphericalSplineComponent::ReverseSplineDirection()
{
    TArray<FVector> Points;
    TArray<FVector> ArriveTangents;
    TArray<FVector> LeaveTangents;
    
    // Collect points in reverse order
    for (int32 i = GetNumberOfSplinePoints() - 1; i >= 0; i--)
    {
        Points.Add(GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
        
        // Swap and negate tangents
        FVector ArriveTan = GetArriveTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
        FVector LeaveTan = GetLeaveTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
        ArriveTangents.Add(-LeaveTan);
        LeaveTangents.Add(-ArriveTan);
    }
    
    // Clear and rebuild spline
    ClearSplinePoints();
    
    for (int32 i = 0; i < Points.Num(); i++)
    {
        AddSplineWorldPoint(Points[i]);
        SetTangentsAtSplinePoint(i, ArriveTangents[i], LeaveTangents[i], ESplineCoordinateSpace::World);
    }
    
    UpdateAllPointNormals();
    UpdateSpline();
    UpdateSplineInfo();
}

void USphericalSplineComponent::ClearAllPoints()
{
    ClearSplinePoints();
    UpdateSpline();
    UpdateSplineInfo();
}

void USphericalSplineComponent::UpdateSplineInfo()
{
    TotalSplineLength = GetSplineLength();
    TotalPointCount = GetNumberOfSplinePoints();
}

FVector USphericalSplineComponent::GetPlanetCenter() const
{
    if (PlanetActor)
    {
        return PlanetActor->GetActorLocation();
    }
    return FVector::ZeroVector;
}

FVector USphericalSplineComponent::GetUpVectorAtDistance(float Distance) const
{
    if (!PlanetActor)
    {
        return GetUpVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
    }
    
    FVector Location = GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
    FVector PlanetCenter = GetPlanetCenter();
    return (Location - PlanetCenter).GetSafeNormal();
}

FVector USphericalSplineComponent::GetSphericalUpVector(int32 PointIndex) const
{
    if (!PlanetActor || PointIndex < 0 || PointIndex >= GetNumberOfSplinePoints())
    {
        return FVector::UpVector;
    }
    
    FVector Location = GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
    FVector PlanetCenter = GetPlanetCenter();
    return (Location - PlanetCenter).GetSafeNormal();
}

FQuat USphericalSplineComponent::GetOrientationAtDistance(float Distance) const
{
    FVector Location = GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
    FVector Tangent = GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
    FVector UpVector = GetUpVectorAtDistance(Distance);
    
    // Project tangent to be perpendicular to up vector
    Tangent = Tangent - (Tangent | UpVector) * UpVector;
    Tangent.Normalize();
    
    if (Tangent.IsNearlyZero(0.01f))
    {
        FVector ArbitraryVector = FMath::Abs(UpVector.Z) < 0.9f ? FVector::UpVector : FVector::RightVector;
        Tangent = FVector::CrossProduct(UpVector, ArbitraryVector).GetSafeNormal();
    }
    
    FVector RightVector = FVector::CrossProduct(Tangent, UpVector).GetSafeNormal();
    FVector ForwardVector = FVector::CrossProduct(UpVector, RightVector).GetSafeNormal();
    
    FRotator Rotation = UKismetMathLibrary::MakeRotFromXZ(ForwardVector, UpVector);
    return Rotation.Quaternion();
}

void USphericalSplineComponent::DrawDebugVisualization() const
{
    if (!bShowDebugSpheres || !GetWorld())
        return;
    
    for (int32 i = 0; i < GetNumberOfSplinePoints(); i++)
    {
        FVector PointLocation = GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        DrawDebugSphere(GetWorld(), PointLocation, DebugSphereRadius, 8, DebugSphereColor, false, 0.01f);
        
        // Draw point index
        DrawDebugString(GetWorld(), PointLocation + FVector(0, 0, DebugSphereRadius + 10), 
                       FString::FromInt(i), nullptr, FColor::White, 0.01f, true);
    }
}

// Editor Button Functions
void USphericalSplineComponent::Editor_GenerateCircularPath()
{
    GenerateCircularPath();
}

void USphericalSplineComponent::Editor_SnapAllToSurface()
{
    SnapAllPointsToSurface();
}

void USphericalSplineComponent::Editor_ClearSpline()
{
    ClearAllPoints();
}

void USphericalSplineComponent::Editor_ReverseDirection()
{
    ReverseSplineDirection();
}

void USphericalSplineComponent::Editor_AddPointAtOwner()
{
    if (GetOwner())
    {
        AddPointAtLocation(GetOwner()->GetActorLocation(), bAutoSnapToSurface);
    }
}