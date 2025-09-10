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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Setup")
    AActor* PlanetActor;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Setup")
    float PlanetRadius = 1000.0f;
    
    // Spline Setup
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Setup")
    int32 NumberOfSplinePoints = 20;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Setup")
    float SplineHeightOffset = 50.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Setup")
    float SplineRadius = 800.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Setup")
    FVector SplineNormalAxis = FVector(0, 0, 1);
    
    // Movement Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float TrainSpeed = 200.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bIsMoving = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bLoopMovement = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float CarSeparationDistance = 150.0f;
    
    // Orientation Settings - NEW/IMPROVED
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
    bool bUseSmoothRotation = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation", meta = (ClampMin = "0.1", ClampMax = "20.0"))
    float RotationSmoothSpeed = 10.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
    bool bBankOnCurves = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation", meta = (ClampMin = "0.0", ClampMax = "45.0"))
    float MaxBankAngle = 20.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
    FRotator TrainRotationOffset = FRotator::ZeroRotator; // Manual adjustment if needed
    
    // Runtime variables
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    float CurrentSplineDistance = 0.0f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    float SplineLength = 0.0f;
    
    // Debug
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugSpline = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugInfo = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowOrientationVectors = false;

private:
    // Setup functions
    void SetupSplineAroundPlanet();
    void SnapSplinePointToPlanetSurface(int32 PointIndex, const FVector& InitialPosition);
    FVector GetPlanetCenter() const;
    
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
};
/*// Custom Movement Component for more advanced features (optional)
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPACESHEPHERD_API USphericalMovementComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USphericalMovementComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
                               FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    AActor* PlanetActor;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float GravityStrength = 980.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bAlignToPlanetSurface = true;
    
    void SetPlanet(AActor* NewPlanet);
    FVector GetGravityDirection(const FVector& Location) const;
    FRotator AlignRotationToPlanet(const FVector& Location, const FRotator& CurrentRotation) const;
};*/