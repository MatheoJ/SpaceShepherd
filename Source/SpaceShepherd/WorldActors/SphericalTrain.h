// SphericalTrain.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "SphericalSplineComponent.h"
#include "SphericalTrain.generated.h"

/**
 * A train actor that follows a spherical spline path
 * Uses SphericalSplineComponent for path management
 */
UCLASS()
class SPACESHEPHERD_API ASphericalTrain : public AActor
{
    GENERATED_BODY()
    
public:
    ASphericalTrain();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnConstruction(const FTransform& Transform) override;
    
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // ========== Components ==========
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootSceneComponent;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphericalSplineComponent* PathSpline;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* TrainMesh;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
    TArray<UStaticMeshComponent*> TrainCars;

    // ========== Movement Settings ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", 
        meta = (ClampMin = "-2000.0", ClampMax = "2000.0"))
    float TrainSpeed = 200.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bIsMoving = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bReverseDirection = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bLoopMovement = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", 
        meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float CarSeparationDistance = 150.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", 
        meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Start Position (0-1)"))
    float InitialPositionRatio = 0.0f;

    // ========== Orientation Settings ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
    bool bUseSmoothRotation = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation", 
        meta = (ClampMin = "0.1", ClampMax = "20.0", EditCondition = "bUseSmoothRotation"))
    float RotationSmoothSpeed = 10.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
    bool bBankOnCurves = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation", 
        meta = (ClampMin = "0.0", ClampMax = "45.0", EditCondition = "bBankOnCurves"))
    float MaxBankAngle = 20.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
    FRotator TrainRotationOffset = FRotator::ZeroRotator;

    // ========== Preview Settings ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    bool bShowTrainPreview = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    bool bPreviewInPlayMode = false;

    // ========== Debug Settings ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugInfo = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowOrientationVectors = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowMovementPath = false;

    // ========== Runtime Status (Read-Only) ==========
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Status")
    float CurrentSplineDistance = 0.0f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Status")
    float CurrentBankAngle = 0.0f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Status")
    float CurrentSpeed = 0.0f;

public:
    // ========== Public Blueprint Functions ==========
    UFUNCTION(BlueprintCallable, Category = "Train Control")
    void StartMovement();
    
    UFUNCTION(BlueprintCallable, Category = "Train Control")
    void StopMovement();
    
    UFUNCTION(BlueprintCallable, Category = "Train Control")
    void ToggleMovement();
    
    UFUNCTION(BlueprintCallable, Category = "Train Control")
    void SetSpeed(float NewSpeed);
    
    UFUNCTION(BlueprintCallable, Category = "Train Control")
    void ReverseDirection();
    
    UFUNCTION(BlueprintCallable, Category = "Train Control")
    void SetPositionOnSpline(float NormalizedPosition);
    
    UFUNCTION(BlueprintPure, Category = "Train Status")
    float GetCurrentProgress() const;
    
    UFUNCTION(BlueprintPure, Category = "Train Status")
    FVector GetCurrentLocation() const;
    
    UFUNCTION(BlueprintPure, Category = "Train Status")
    FRotator GetCurrentRotation() const;

    // Editor Functions
    UFUNCTION(CallInEditor, Category = "Train Actions", meta = (DisplayName = "Reset to Start"))
    void Editor_ResetToStart();
    
    UFUNCTION(CallInEditor, Category = "Train Actions", meta = (DisplayName = "Preview at Middle"))
    void Editor_PreviewAtMiddle();
    
    UFUNCTION(CallInEditor, Category = "Train Actions", meta = (DisplayName = "Preview at End"))
    void Editor_PreviewAtEnd();

private:
    // Movement Functions
    void UpdateTrainMovement(float DeltaTime);
    void UpdateTrainOrientation(float DeltaTime);
    void UpdateTrainCars(float DeltaTime);
    
    // Helper Functions
    FQuat CalculateTargetRotation(const FVector& Location, const FVector& Direction) const;
    float CalculateBankAngle(float DeltaTime) const;
    void PreviewTrainPosition();
    void SetTrainPosition(float Distance);
    
    // Debug Functions
    void DrawDebugVisualization() const;
    void DrawDebugOrientationVectors() const;
    void DrawDebugMovementPath() const;
    void DrawDebugInfo() const;
    
    // Cached values for smooth rotation
    FQuat CurrentRotation;
    FVector LastSplineDirection;
    
    // State flags
    bool bIsInEditor = false;
};