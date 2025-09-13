// SphericalTrain.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "SphericalTrain.generated.h"

UCLASS()
class SPACESHEPHERD_API ASphericalTrain : public AActor
{
    GENERATED_BODY()
    
public:
    ASphericalTrain();

    // Editor functions
    virtual void OnConstruction(const FTransform& Transform) override;
    
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostEditMove(bool bFinished) override;
#endif

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // Core Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Train Components")
    USceneComponent* RootSceneComponent;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Train Components")
    USplineComponent* PathSpline;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Train Components")
    UStaticMeshComponent* TrainMesh;
    
    // Additional train cars (optional)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Train Components")
    TArray<UStaticMeshComponent*> TrainCars;

    // Planet reference
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Setup", meta = (DisplayName = "Planet Actor"))
    AActor* PlanetActor;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Setup", meta = (ClampMin = "100.0", ClampMax = "50000.0"))
    float PlanetRadius = 1000.0f;
    
    // Spline Generation Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Generation", meta = (DisplayName = "Generate Circle Path"))
    bool bGenerateCircularPath = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Generation", meta = (ClampMin = "4", ClampMax = "100", EditCondition = "bGenerateCircularPath"))
    int32 NumberOfSplinePoints = 20;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Generation", meta = (ClampMin = "0.0", ClampMax = "500.0"))
    float SplineHeightOffset = 50.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Generation", meta = (EditCondition = "bGenerateCircularPath"))
    float SplineRadius = 800.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Generation", meta = (EditCondition = "bGenerateCircularPath"))
    FVector SplineNormalAxis = FVector(0, 0, 1);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Generation", meta = (EditCondition = "bGenerateCircularPath"))
    float SplineStartAngle = 0.0f;
    
    // Spline Editing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Editing", meta = (DisplayName = "Auto-Snap to Planet Surface"))
    bool bAutoSnapToSurface = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Editing", meta = (DisplayName = "Snap All Points Now"))
    bool bSnapAllPoints = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Editing", meta = (DisplayName = "Clear Spline"))
    bool bClearSpline = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Editing")
    bool bClosedLoop = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Editing", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float SplineTangentScale = 1.0f;
    
    // Movement Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "-2000.0", ClampMax = "2000.0"))
    float TrainSpeed = 200.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bIsMoving = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bReverseDirection = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bLoopMovement = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float CarSeparationDistance = 150.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Start Position (0-1)"))
    float InitialPositionRatio = 0.0f;
    
    // Orientation Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
    bool bUseSmoothRotation = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation", meta = (ClampMin = "0.1", ClampMax = "20.0", EditCondition = "bUseSmoothRotation"))
    float RotationSmoothSpeed = 10.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
    bool bBankOnCurves = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation", meta = (ClampMin = "0.0", ClampMax = "45.0", EditCondition = "bBankOnCurves"))
    float MaxBankAngle = 20.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
    FRotator TrainRotationOffset = FRotator::ZeroRotator;
    
    // Runtime variables
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Status")
    float CurrentSplineDistance = 0.0f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Status")
    float SplineLength = 0.0f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Status")
    int32 SplinePointCount = 0;
    
    // Editor Visualization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    bool bShowSplineInEditor = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    bool bShowTrainPreview = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    FColor SplineEditorColor = FColor::Yellow;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization", meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float SplineEditorLineThickness = 2.0f;
    
    // Runtime Debug
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugSpline = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugInfo = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowOrientationVectors = false;

    // Editor functions
    UFUNCTION(CallInEditor, Category = "Spline Actions", meta = (DisplayName = "Generate Circular Path"))
    void GenerateCircularSpline();
    
    UFUNCTION(CallInEditor, Category = "Spline Actions", meta = (DisplayName = "Snap All Points to Surface"))
    void SnapAllPointsToSurface();
    
    UFUNCTION(CallInEditor, Category = "Spline Actions", meta = (DisplayName = "Clear Spline"))
    void ClearSplinePoints();
    
    UFUNCTION(CallInEditor, Category = "Spline Actions", meta = (DisplayName = "Reverse Spline Direction"))
    void ReverseSplineDirection();
    
    UFUNCTION(CallInEditor, Category = "Spline Actions", meta = (DisplayName = "Add Point at Actor Location"))
    void AddSplinePointAtActor();

private:
    // Setup functions
    void SetupSplineAroundPlanet();
    void SnapSplinePointToPlanetSurface(int32 PointIndex, const FVector& InitialPosition);
    FVector GetPlanetCenter() const;
    void UpdateSplineSettings();
    void PreviewTrainPosition();
    
    // Movement functions
    void UpdateTrainMovement(float DeltaTime);
    void UpdateTrainOrientationImproved(float DeltaTime);
    void UpdateTrainCarsImproved(float DeltaTime);
    
    // Helper functions
    FQuat CalculateTargetRotation(const FVector& Location, const FVector& SplineDirection) const;
    float CalculateBankAngle(float DeltaTime) const;
    
    // Debug
    void DrawDebugVisualization();
    
    // Cached values for smooth rotation
    FQuat CurrentRotation;
    FVector LastSplineDirection;
    float CurrentBankAngle = 0.0f;
    
    // Editor state
    bool bIsEditorPreview = false;
};