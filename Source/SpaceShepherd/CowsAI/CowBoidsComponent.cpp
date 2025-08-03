// CowBoidsComponent.cpp
#include "CowBoidsComponent.h"
#include "CowCharacter.h"
#include "PlayerShepherdComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"
#include "AI/Navigation/NavigationTypes.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"

UCowBoidsComponent::UCowBoidsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    
    // Initialize current max speed to wander speed
    CurrentMaxSpeed = WanderSpeed;
    bIsAvoidingObstacle = false;
    bIsAvoidingCliff = false;
}

void UCowBoidsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter)
    {
        MovementComponent = OwnerCharacter->GetCharacterMovement();
        
        // Set initial movement speed
        if (MovementComponent)
        {
            MovementComponent->MaxWalkSpeed = WanderSpeed;
        }
        
        // Initialize wander target
        WanderTarget = FVector(FMath::RandRange(-1.0f, 1.0f), FMath::RandRange(-1.0f, 1.0f), 0.0f);
        WanderTarget.Normalize();
        WanderTarget *= WanderRadius;
    }
    
    // Set initial max speed
    CurrentMaxSpeed = WanderSpeed;
}

void UCowBoidsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!OwnerCharacter || !MovementComponent)
        return;
    
    // Update player detection
    UpdatePlayerDetection();
    
    // Update max speed based on current behavior
    UpdateMaxSpeed(DeltaTime);
    
    // Calculate steering force
    FVector SteeringForce = CalculateSteeringForce(DeltaTime);
    
    // Apply steering to velocity
    CurrentVelocity += SteeringForce * DeltaTime;
    CurrentVelocity = LimitVector(CurrentVelocity, CurrentMaxSpeed);
    
    // Apply velocity to movement
    if (CurrentVelocity.SizeSquared() > 0.1f)
    {
        MovementComponent->AddInputVector(CurrentVelocity.GetSafeNormal());
        
        // Optional: Rotate cow to face movement direction
        FRotator NewRotation = FRotationMatrix::MakeFromX(CurrentVelocity.GetSafeNormal()).Rotator();
        OwnerCharacter->SetActorRotation(FMath::RInterpTo(OwnerCharacter->GetActorRotation(), NewRotation, DeltaTime, 5.0f));
    }
    
    if (bDebugDraw)
    {
        DrawDebugInfo();
    }
}

void UCowBoidsComponent::UpdateMaxSpeed(float DeltaTime)
{
    ACowCharacter* CowChar = Cast<ACowCharacter>(OwnerCharacter);
    if (!CowChar || !MovementComponent)
    {
        CurrentMaxSpeed = WanderSpeed;
        if (MovementComponent)
        {
            MovementComponent->MaxWalkSpeed = WanderSpeed;
        }
        return;
    }
    
    // Determine current max speed based on behavior
    float TargetSpeed = WanderSpeed;
    
    if (bPlayerInRange && DetectedPlayer)
    {
        float DistanceToPlayer = FVector::Dist(OwnerCharacter->GetActorLocation(), DetectedPlayer->GetActorLocation());
        
        if (CowChar->bIsRepulsedByPlayer)
        {
            // Repulsion takes priority - cows run fastest when fleeing
            TargetSpeed = RepulsionSpeed;
        }
        else if (CowChar->bIsAttractedToPlayer)
        {
            // Slow down as we approach the player
            if (DistanceToPlayer <= AttractionStopDistance)
            {
                TargetSpeed = 0.0f; // Stop when close enough
            }
            else if (DistanceToPlayer <= AttractionSlowdownDistance)
            {
                // Gradual slowdown
                float SlowdownFactor = (DistanceToPlayer - AttractionStopDistance) / 
                                     (AttractionSlowdownDistance - AttractionStopDistance);
                TargetSpeed = AttractionSpeed * SlowdownFactor;
            }
            else
            {
                TargetSpeed = AttractionSpeed;
            }
        }
        else
        {
            // Default wander speed
            TargetSpeed = WanderSpeed;
        }
    }
    else
    {
        // No player interaction - use wander speed
        TargetSpeed = WanderSpeed;
    }
    
    // Update both our internal max speed and the movement component's walk speed
    CurrentMaxSpeed = TargetSpeed;
    
    // Apply speed to movement component with optional interpolation
    if (bSmoothSpeedTransitions)
    {
        float CurrentWalkSpeed = MovementComponent->MaxWalkSpeed;
        float NewWalkSpeed = FMath::FInterpTo(CurrentWalkSpeed, TargetSpeed, DeltaTime, SpeedTransitionRate);
        MovementComponent->MaxWalkSpeed = NewWalkSpeed;
    }
    else
    {
        MovementComponent->MaxWalkSpeed = TargetSpeed;
    }
}

FVector UCowBoidsComponent::CalculateSteeringForce(float DeltaTime)
{
    FVector SteeringForce = FVector::ZeroVector;
    
    // Get cow character to check player interaction state
    ACowCharacter* CowChar = Cast<ACowCharacter>(OwnerCharacter);
    if (!CowChar)
        return SteeringForce;
    
    // 1. First priority: Obstacle and cliff avoidance (safety first!)
    FVector ObstacleAvoid = CalculateObstacleAvoidance();
    FVector CliffAvoid = CalculateCliffAvoidance();
    
    // Check if we're actively avoiding
    bIsAvoidingObstacle = !ObstacleAvoid.IsNearlyZero();
    bIsAvoidingCliff = !CliffAvoid.IsNearlyZero();
    
    // Apply safety forces with high priority
    if (bIsAvoidingObstacle || bIsAvoidingCliff)
    {
        SteeringForce += ObstacleAvoid * ObstacleAvoidanceWeight * SafetyPriorityMultiplier;
        SteeringForce += CliffAvoid * ObstacleAvoidanceWeight * SafetyPriorityMultiplier;
    }
    else
    {
        // Normal weights when not in danger
        SteeringForce += ObstacleAvoid * ObstacleAvoidanceWeight;
        SteeringForce += CliffAvoid * ObstacleAvoidanceWeight;
    }
    
    // 2. Separation from other cows
    FVector Separation = CalculateSeparation() * SeparationWeight;
    SteeringForce += Separation;
    
    // 3. Player interaction (reduced influence when avoiding obstacles)
    float PlayerInfluenceReduction = (bIsAvoidingObstacle || bIsAvoidingCliff) ? 0.2f : 1.0f;
    
    if (bPlayerInRange && DetectedPlayer)
    {
        float DistanceToPlayer = FVector::Dist(OwnerCharacter->GetActorLocation(), DetectedPlayer->GetActorLocation());
        
        if (CowChar->bIsAttractedToPlayer && DistanceToPlayer > AttractionStopDistance)
        {
            FVector PlayerForce = CalculatePlayerAttraction() * AttractionWeight * PlayerInfluenceReduction;
            SteeringForce += PlayerForce;
        }
        else if (CowChar->bIsRepulsedByPlayer)
        {
            FVector PlayerForce = CalculatePlayerRepulsion() * RepulsionWeight * PlayerInfluenceReduction;
            SteeringForce += PlayerForce;
        }
    }
    
    // 4. Wander behavior (only if not interacting with player and not avoiding obstacles)
    if (!bIsAvoidingObstacle && !bIsAvoidingCliff)
    {
        if (!bPlayerInRange || (!CowChar->bIsAttractedToPlayer && !CowChar->bIsRepulsedByPlayer))
        {
            FVector Wander = CalculateWander(DeltaTime);
            SteeringForce += Wander;
        }
    }
    
    // Limit steering force
    SteeringForce = LimitVector(SteeringForce, MaxSteerForce);
    
    return SteeringForce;
}

FVector UCowBoidsComponent::CalculateSeparation()
{
    FVector SeparationForce = FVector::ZeroVector;
    int32 Count = 0;
    
    TArray<AActor*> NearbyCows = GetNearbyCows();
    FVector MyLocation = OwnerCharacter->GetActorLocation();
    
    for (AActor* Cow : NearbyCows)
    {
        if (!Cow || Cow == OwnerCharacter)
            continue;
            
        FVector ToCow = MyLocation - Cow->GetActorLocation();
        float Distance = ToCow.Size();
        
        if (Distance > 0 && Distance < SeparationRadius)
        {
            // Stronger repulsion the closer they are
            ToCow.Normalize();
            ToCow *= (SeparationRadius - Distance) / SeparationRadius;
            SeparationForce += ToCow;
            Count++;
        }
    }
    
    if (Count > 0)
    {
        SeparationForce /= Count;
        SeparationForce.Normalize();
        SeparationForce *= CurrentMaxSpeed;
        SeparationForce -= CurrentVelocity;
    }
    
    return SeparationForce;
}

FVector UCowBoidsComponent::CalculateWander(float DeltaTime)
{
    // Add random jitter to wander target
    WanderTarget += FVector(
        FMath::RandRange(-1.0f, 1.0f) * WanderJitter,
        FMath::RandRange(-1.0f, 1.0f) * WanderJitter,
        0.0f
    );
    
    // Keep wander target on circle
    WanderTarget.Normalize();
    WanderTarget *= WanderRadius;
    
    // Calculate wander force
    FVector TargetLocal = WanderTarget + FVector(WanderDistance, 0, 0);
    FVector TargetWorld = OwnerCharacter->GetActorTransform().TransformPosition(TargetLocal);
    
    FVector DesiredVelocity = TargetWorld - OwnerCharacter->GetActorLocation();
    DesiredVelocity.Z = 0; // Keep on ground
    DesiredVelocity.Normalize();
    DesiredVelocity *= WanderSpeed; // Use wander speed specifically
    
    return DesiredVelocity - CurrentVelocity;
}

FVector UCowBoidsComponent::CalculateObstacleAvoidance()
{
    FVector AvoidanceForce = FVector::ZeroVector;
    FVector Forward = CurrentVelocity.GetSafeNormal();
    
    if (Forward.IsNearlyZero())
        Forward = OwnerCharacter->GetActorForwardVector();
    
    // Check multiple rays for better obstacle detection
    TArray<FVector> RayDirections = {
        Forward,
        (Forward + OwnerCharacter->GetActorRightVector() * 0.5f).GetSafeNormal(),
        (Forward - OwnerCharacter->GetActorRightVector() * 0.5f).GetSafeNormal()
    };
    
    float ClosestObstacleDistance = WallAvoidanceDistance;
    FVector BestAvoidanceDirection = FVector::ZeroVector;
    
    for (const FVector& Direction : RayDirections)
    {
        FVector Start = OwnerCharacter->GetActorLocation() + FVector(0, 0, 50);
        FVector End = Start + Direction * WallAvoidanceDistance;
        
        FHitResult Hit;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(OwnerCharacter);
        
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, QueryParams))
        {
            float Distance = Hit.Distance;
            if (Distance < ClosestObstacleDistance)
            {
                ClosestObstacleDistance = Distance;
                
                // Calculate avoidance direction perpendicular to hit normal
                FVector Right = FVector::CrossProduct(Hit.Normal, FVector::UpVector).GetSafeNormal();
                
                // Choose direction that aligns better with current movement
                if (FVector::DotProduct(Right, CurrentVelocity) < 0)
                    Right *= -1;
                
                // Blend between normal and tangent based on distance
                float NormalInfluence = 1.0f - (Distance / WallAvoidanceDistance);
                BestAvoidanceDirection = (Hit.Normal * NormalInfluence + Right * (1.0f - NormalInfluence)).GetSafeNormal();
            }
        }
    }
    
    if (!BestAvoidanceDirection.IsNearlyZero())
    {
        // Stronger avoidance the closer we are
        float AvoidanceStrength = 1.0f - (ClosestObstacleDistance / WallAvoidanceDistance);
        AvoidanceForce = BestAvoidanceDirection * CurrentMaxSpeed * AvoidanceStrength;
        AvoidanceForce -= CurrentVelocity;
    }
    
    return AvoidanceForce;
}

FVector UCowBoidsComponent::CalculateCliffAvoidance()
{
    FVector AvoidanceForce = FVector::ZeroVector;
    FVector Forward = CurrentVelocity.GetSafeNormal();
    
    if (Forward.IsNearlyZero())
        Forward = OwnerCharacter->GetActorForwardVector();
    
    // Check for ground ahead
    if (!IsGroundAhead(Forward, CliffAvoidanceDistance))
    {
        // Turn away from cliff
        FVector Right = FVector::CrossProduct(Forward, FVector::UpVector);
        
        // Check which side has ground
        bool RightHasGround = IsGroundAhead(Right, CliffAvoidanceDistance * 0.5f);
        bool LeftHasGround = IsGroundAhead(-Right, CliffAvoidanceDistance * 0.5f);
        
        if (RightHasGround && !LeftHasGround)
            AvoidanceForce = Right;
        else if (LeftHasGround && !RightHasGround)
            AvoidanceForce = -Right;
        else
            AvoidanceForce = -Forward; // Back away
            
        AvoidanceForce.Normalize();
        AvoidanceForce *= CurrentMaxSpeed;
        AvoidanceForce -= CurrentVelocity;
    }
    
    return AvoidanceForce;
}

FVector UCowBoidsComponent::CalculatePlayerAttraction()
{
    if (!DetectedPlayer)
        return FVector::ZeroVector;
        
    FVector ToPlayer = DetectedPlayer->GetActorLocation() - OwnerCharacter->GetActorLocation();
    ToPlayer.Z = 0; // Keep on ground
    
    float Distance = ToPlayer.Size();
    
    // Stop if we're close enough
    if (Distance <= AttractionStopDistance)
    {
        // Apply braking force
        return -CurrentVelocity * 2.0f;
    }
    
    if (Distance > 0)
    {
        ToPlayer.Normalize();
        
        // Calculate desired speed based on distance
        float DesiredSpeed = AttractionSpeed;
        if (Distance < AttractionSlowdownDistance)
        {
            // Slow down as we approach
            float SlowdownFactor = (Distance - AttractionStopDistance) / 
                                 (AttractionSlowdownDistance - AttractionStopDistance);
            DesiredSpeed *= SlowdownFactor;
        }
        
        ToPlayer *= DesiredSpeed;
        
        return ToPlayer - CurrentVelocity;
    }
    
    return FVector::ZeroVector;
}

FVector UCowBoidsComponent::CalculatePlayerRepulsion()
{
    if (!DetectedPlayer)
        return FVector::ZeroVector;
        
    FVector AwayFromPlayer = OwnerCharacter->GetActorLocation() - DetectedPlayer->GetActorLocation();
    AwayFromPlayer.Z = 0; // Keep on ground
    
    float Distance = AwayFromPlayer.Size();
    if (Distance > 0)
    {
        AwayFromPlayer.Normalize();
        
        // Flee at repulsion speed
        // Stronger urgency when closer
        float UrgencyFactor = 1.0f - FMath::Clamp(Distance / PlayerDetectionRadius, 0.0f, 0.8f);
        UrgencyFactor = FMath::Pow(UrgencyFactor, 1.5f); // Increase urgency curve
        
        AwayFromPlayer *= RepulsionSpeed;
        
        return AwayFromPlayer - CurrentVelocity;
    }
    
    return FVector::ZeroVector;
}

TArray<AActor*> UCowBoidsComponent::GetNearbyCows()
{
    TArray<AActor*> FoundCows;
    
    if (!CowClass)
        CowClass = OwnerCharacter->GetClass();
    
    UWorld* World = GetWorld();
    if (!World)
        return FoundCows;
    
    // Find all cows within perception radius
    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    
    World->OverlapMultiByObjectType(
        OverlapResults,
        OwnerCharacter->GetActorLocation(),
        FQuat::Identity,
        FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
        FCollisionShape::MakeSphere(PerceptionRadius),
        QueryParams
    );
    
    for (const FOverlapResult& Result : OverlapResults)
    {
        if (Result.GetActor() && Result.GetActor()->IsA(CowClass))
        {
            FoundCows.Add(Result.GetActor());
        }
    }
    
    return FoundCows;
}

void UCowBoidsComponent::UpdatePlayerDetection()
{
    bPlayerInRange = false;
    DetectedPlayer = nullptr;
    
    // Find all actors with PlayerShepherdComponent
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor)
            continue;
            
        UPlayerShepherdComponent* ShepherdComp = Actor->FindComponentByClass<UPlayerShepherdComponent>();
        if (ShepherdComp && ShepherdComp->IsNotNeutral())
        {
            float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Actor->GetActorLocation());
            if (Distance <= PlayerDetectionRadius)
            {
                bPlayerInRange = true;
                DetectedPlayer = Actor;
                break; // Assume only one player for now
            }
        }
    }
}

bool UCowBoidsComponent::IsGroundAhead(FVector Direction, float Distance)
{
    FVector Start = OwnerCharacter->GetActorLocation();
    FVector End = Start + Direction * Distance;
    
    // Cast ray downward from the end position
    FVector DownStart = End + FVector(0, 0, 100);
    FVector DownEnd = End - FVector(0, 0, 500);
    
    FHitResult Hit;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    
    return GetWorld()->LineTraceSingleByChannel(
        Hit,
        DownStart,
        DownEnd,
        ECC_WorldStatic,
        QueryParams
    );
}

bool UCowBoidsComponent::IsObstacleAhead(FVector Direction, float Distance)
{
    FVector Start = OwnerCharacter->GetActorLocation() + FVector(0, 0, 50); // Raise a bit
    FVector End = Start + Direction * Distance;
    
    FHitResult Hit;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    
    return GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_WorldStatic,
        QueryParams
    );
}

FVector UCowBoidsComponent::LimitVector(FVector Vector, float MaxMagnitude)
{
    if (Vector.SizeSquared() > MaxMagnitude * MaxMagnitude)
    {
        Vector.Normalize();
        Vector *= MaxMagnitude;
    }
    return Vector;
}

void UCowBoidsComponent::DrawDebugInfo()
{
    if (!OwnerCharacter)
        return;
        
    FVector Location = OwnerCharacter->GetActorLocation();
    
    // Draw velocity
    DrawDebugLine(GetWorld(), Location, Location + CurrentVelocity, FColor::Green, false, -1, 0, 2);
    
    // Draw perception radius
    DrawDebugSphere(GetWorld(), Location, PerceptionRadius, 16, FColor::Yellow, false, -1, 0, 1);
    
    // Draw separation radius
    DrawDebugSphere(GetWorld(), Location, SeparationRadius, 12, FColor::Red, false, -1, 0, 1);
    
    // Draw player detection radius
    DrawDebugSphere(GetWorld(), Location, PlayerDetectionRadius, 20, FColor::Cyan, false, -1, 0, 1);
    
    // Draw wander circle
    FVector WanderCenter = Location + OwnerCharacter->GetActorForwardVector() * WanderDistance;
    DrawDebugSphere(GetWorld(), WanderCenter, WanderRadius, 8, FColor::Blue, false, -1, 0, 1);
    
    // Draw attraction stop distance if attracted
    ACowCharacter* CowChar = Cast<ACowCharacter>(OwnerCharacter);
    if (CowChar && CowChar->bIsAttractedToPlayer && DetectedPlayer)
    {
        DrawDebugSphere(GetWorld(), DetectedPlayer->GetActorLocation(), AttractionStopDistance, 12, FColor::Green, false, -1, 0, 0.5f);
        DrawDebugSphere(GetWorld(), DetectedPlayer->GetActorLocation(), AttractionSlowdownDistance, 12, FColor::Yellow, false, -1, 0, 0.5f);
    }
    
    // Draw current speed info
    FString SpeedInfo = FString::Printf(TEXT("Speed: %.1f / %.1f (Walk: %.1f)"), 
        CurrentVelocity.Size(), 
        CurrentMaxSpeed,
        MovementComponent ? MovementComponent->MaxWalkSpeed : 0.0f);
    DrawDebugString(GetWorld(), Location + FVector(0, 0, 100), SpeedInfo, nullptr, FColor::White, 0.0f, true);
    
    // Draw line to player if detected
    if (bPlayerInRange && DetectedPlayer)
    {
        FColor LineColor = FColor::White;
        FString BehaviorText = TEXT("Neutral");
        
        if (CowChar)
        {
            if (CowChar->bIsAttractedToPlayer)
            {
                LineColor = FColor::Green;
                float Distance = FVector::Dist(Location, DetectedPlayer->GetActorLocation());
                if (Distance <= AttractionStopDistance)
                    BehaviorText = TEXT("Attracted (Stopped)");
                else if (Distance <= AttractionSlowdownDistance)
                    BehaviorText = TEXT("Attracted (Slowing)");
                else
                    BehaviorText = TEXT("Attracted");
            }
            else if (CowChar->bIsRepulsedByPlayer)
            {
                LineColor = FColor::Red;
                BehaviorText = TEXT("Repulsed");
            }
        }
        
        DrawDebugLine(GetWorld(), Location, DetectedPlayer->GetActorLocation(), LineColor, false, -1, 0, 2);
        DrawDebugString(GetWorld(), Location + FVector(0, 0, 150), BehaviorText, nullptr, LineColor, 0.0f, true);
    }
    
    // Show avoidance status
    if (bIsAvoidingObstacle || bIsAvoidingCliff)
    {
        FString AvoidanceText = TEXT("");
        if (bIsAvoidingObstacle) AvoidanceText += TEXT("Avoiding Wall ");
        if (bIsAvoidingCliff) AvoidanceText += TEXT("Avoiding Cliff");
        DrawDebugString(GetWorld(), Location + FVector(0, 0, 200), AvoidanceText, nullptr, FColor::Orange, 0.0f, true);
    }
}