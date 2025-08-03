// CowBoidsComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CowBoidsComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UCowBoidsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCowBoidsComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Boids Parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Movement")
    float WanderSpeed = 150.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Movement")
    float MaxSteerForce = 150.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Movement")
    float WanderRadius = 100.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Movement")
    float WanderDistance = 200.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Movement")
    float WanderJitter = 40.0f;

    // Avoidance Parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Avoidance")
    float SeparationRadius = 150.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Avoidance")
    float SeparationWeight = 2.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Avoidance")
    float ObstacleAvoidanceWeight = 3.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Avoidance")
    float CliffAvoidanceDistance = 300.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Avoidance")
    float WallAvoidanceDistance = 200.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Avoidance")
    float SafetyPriorityMultiplier = 3.0f;

    // Perception
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Perception")
    float PerceptionRadius = 500.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Perception")
    TSubclassOf<AActor> CowClass;

    // Player Interaction
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Player")
    float PlayerDetectionRadius = 1900.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Player")
    float AttractionWeight = 1.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Player")
    float RepulsionWeight = 2.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Player")
    float AttractionSpeed = 250.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Player")
    float RepulsionSpeed = 350.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Player")
    float AttractionStopDistance = 200.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Player")
    float AttractionSlowdownDistance = 400.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Player")
    bool bSmoothSpeedTransitions = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Player", meta = (EditCondition = "bSmoothSpeedTransitions"))
    float SpeedTransitionRate = 2.0f;

private:
    // Internal state
    FVector CurrentVelocity;
    FVector WanderTarget;
    float CurrentMaxSpeed;
    class ACharacter* OwnerCharacter;
    class UCharacterMovementComponent* MovementComponent;
    class AActor* DetectedPlayer;
    bool bPlayerInRange;
    bool bIsAvoidingObstacle;
    bool bIsAvoidingCliff;

    // Core boids functions
    FVector CalculateSteeringForce(float DeltaTime);
    FVector CalculateSeparation();
    FVector CalculateWander(float DeltaTime);
    FVector CalculateObstacleAvoidance();
    FVector CalculateCliffAvoidance();
    FVector CalculatePlayerAttraction();
    FVector CalculatePlayerRepulsion();
    
    // Helper functions
    TArray<AActor*> GetNearbyCows();
    void UpdatePlayerDetection();
    void UpdateMaxSpeed(float DeltaTime);
    bool IsGroundAhead(FVector Direction, float Distance);
    bool IsObstacleAhead(FVector Direction, float Distance);
    FVector LimitVector(FVector Vector, float MaxMagnitude);
    
    // Debug
    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDebugDraw = false;
    
    void DrawDebugInfo();
};