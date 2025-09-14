// SphericalSplineComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "SphericalSplineComponent.generated.h"

/**
 * A spline component designed to work on spherical surfaces (planets)
 * Handles spline generation, editing, and planet surface snapping
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPACESHEPHERD_API USphericalSplineComponent : public USplineComponent
{
    GENERATED_BODY()

public:
    USphericalSplineComponent();

    // Planet Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Configuration")
    AActor* PlanetActor;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Configuration", 
        meta = (ClampMin = "100.0", ClampMax = "50000.0"))
    float PlanetRadius = 1000.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Configuration",
        meta = (ClampMin = "0.0", ClampMax = "500.0"))
    float HeightOffset = 0.0f;

    // Circular Path Generation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Generation")
    bool bAutoGenerateCircularPath = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Generation",
        meta = (ClampMin = "4", ClampMax = "100", EditCondition = "bAutoGenerateCircularPath"))
    int32 CirclePointCount = 20;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Generation",
        meta = (EditCondition = "bAutoGenerateCircularPath"))
    float CircleRadius = 800.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Generation",
        meta = (EditCondition = "bAutoGenerateCircularPath"))
    FVector CircleNormalAxis = FVector(0, 0, 1);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Generation",
        meta = (EditCondition = "bAutoGenerateCircularPath"))
    float CircleStartAngle = 0.0f;

    // Surface Snapping
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface Snapping")
    bool bAutoSnapToSurface = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface Snapping")
    bool bUseComplexCollisionForSnapping = true;

    // Visualization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    bool bShowDebugSpheres = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    FColor DebugSphereColor = FColor::Red;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    float DebugSphereRadius = 20.0f;

    // Runtime Info
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Info")
    float TotalSplineLength = 0.0f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Info")
    int32 TotalPointCount = 0;

public:
    // Core Functions
    UFUNCTION(BlueprintCallable, Category = "Spherical Spline")
    void GenerateCircularPath();
    
    UFUNCTION(BlueprintCallable, Category = "Spherical Spline")
    void SnapAllPointsToSurface();
    
    UFUNCTION(BlueprintCallable, Category = "Spherical Spline")
    void SnapPointToSurface(int32 PointIndex);
    
    UFUNCTION(BlueprintCallable, Category = "Spherical Spline")
    void AddPointAtLocation(const FVector& WorldLocation, bool bSnapToSurface = true);
    
    UFUNCTION(BlueprintCallable, Category = "Spherical Spline")
    void ReverseSplineDirection();
    
    UFUNCTION(BlueprintCallable, Category = "Spherical Spline")
    void ClearAllPoints();
    
    UFUNCTION(BlueprintCallable, Category = "Spherical Spline")
    void UpdateSplineInfo();

    // Utility Functions
    UFUNCTION(BlueprintPure, Category = "Spherical Spline")
    FVector GetPlanetCenter() const;
    
    UFUNCTION(BlueprintPure, Category = "Spherical Spline")
    FVector GetUpVectorAtDistance(float Distance) const;
    
    UFUNCTION(BlueprintPure, Category = "Spherical Spline")
    FVector GetSphericalUpVector(int32 PointIndex) const;
    
    UFUNCTION(BlueprintPure, Category = "Spherical Spline")
    FQuat GetOrientationAtDistance(float Distance) const;

    // Editor Functions
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    void OnSplinePointMoved();
#endif

    // Editor Action Buttons
    UFUNCTION(CallInEditor, Category = "Spherical Spline Actions", meta = (DisplayName = "Generate Circular Path"))
    void Editor_GenerateCircularPath();
    
    UFUNCTION(CallInEditor, Category = "Spherical Spline Actions", meta = (DisplayName = "Snap All to Surface"))
    void Editor_SnapAllToSurface();
    
    UFUNCTION(CallInEditor, Category = "Spherical Spline Actions", meta = (DisplayName = "Clear Spline"))
    void Editor_ClearSpline();
    
    UFUNCTION(CallInEditor, Category = "Spherical Spline Actions", meta = (DisplayName = "Reverse Direction"))
    void Editor_ReverseDirection();
    
    UFUNCTION(CallInEditor, Category = "Spherical Spline Actions", meta = (DisplayName = "Add Point at Owner"))
    void Editor_AddPointAtOwner();

protected:
    virtual void BeginPlay() override;
    virtual void OnComponentCreated() override;
    
private:
    FVector ProjectPointToPlanetSurface(const FVector& WorldPoint) const;
    void SetSplinePointNormal(int32 PointIndex);
    void UpdateAllPointNormals();
    void DrawDebugVisualization() const;
};