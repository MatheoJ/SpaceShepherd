// PlayerShepherdComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerShepherdComponent.generated.h"

UENUM(BlueprintType)
enum class EShepherdMode : uint8
{
    Neutral     UMETA(DisplayName = "Neutral"),
    Attraction  UMETA(DisplayName = "Attraction"),
    Repulsion   UMETA(DisplayName = "Repulsion")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShepherdModeChanged, EShepherdMode, NewMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCowPickedUp, class ACowCharacter*, Cow);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCowThrown, class ACowCharacter*, Cow, float, ThrowPower);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCowDropped);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UPlayerShepherdComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPlayerShepherdComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ========== Shepherd Mode Properties ==========
    
    // Current shepherd mode
    UPROPERTY(BlueprintReadOnly, Category = "Shepherd")
    EShepherdMode CurrentMode = EShepherdMode::Neutral;
    
    // Mode change event
    UPROPERTY(BlueprintAssignable, Category = "Shepherd")
    FOnShepherdModeChanged OnModeChanged;
    
    // Visual feedback settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Visual")
    bool bShowModeIndicator = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Visual")
    float IndicatorRadius = 150.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Visual")
    FColor NeutralColor = FColor::White;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Visual")
    FColor AttractionColor = FColor::Green;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Visual")
    FColor RepulsionColor = FColor::Red;
    
    // ========== Carrying System Properties ==========
    
    // Carrying settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Carrying")
    float PickupRange = 200.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Carrying")
    float PickupAngle = 45.0f; // Cone angle in degrees
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Carrying")
    FVector CarryOffset = FVector(150.0f, 0.0f, 100.0f); // Offset from player
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Carrying")
    float CarryInterpSpeed = 10.0f; // How smoothly the cow moves to carry position
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Carrying")
    bool bUseCameraDirection = true; // Use camera look direction for carrying and throwing
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Carrying", meta = (EditCondition = "bUseCameraDirection"))
    float CarryPitchDamping = 0.5f; // How much to dampen camera pitch for carry position (0-1)
    
    // Throwing settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Throwing")
    float MinThrowSpeed = 500.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Throwing")
    float MaxThrowSpeed = 2000.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Throwing")
    float MaxChargeTime = 2.0f; // Maximum charge time in seconds
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Throwing")
    float ThrowUpwardAngle = 30.0f; // Upward angle for throw trajectory
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Throwing")
    bool bShowThrowTrajectory = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Throwing")
    int32 TrajectoryPoints = 30;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shepherd|Throwing")
    float TrajectoryTimeStep = 0.1f;
    
    // Current carrying state
    UPROPERTY(BlueprintReadOnly, Category = "Shepherd|Carrying")
    class ACowCharacter* CarriedCow = nullptr;
    
    UPROPERTY(BlueprintReadOnly, Category = "Shepherd|Carrying")
    bool bIsCarryingCow = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Shepherd|Throwing")
    bool bIsChargingThrow = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Shepherd|Throwing")
    float CurrentChargeTime = 0.0f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Shepherd|Throwing")
    float CurrentThrowPower = 0.0f; // 0 to 1
    
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Shepherd|Events")
    FOnCowPickedUp OnCowPickedUp;
    
    UPROPERTY(BlueprintAssignable, Category = "Shepherd|Events")
    FOnCowThrown OnCowThrown;
    
    UPROPERTY(BlueprintAssignable, Category = "Shepherd|Events")
    FOnCowDropped OnCowDropped;
    
    // ========== Mode Functions ==========
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd")
    void SetShepherdMode(EShepherdMode NewMode);
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd")
    void ToggleAttractionMode();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd")
    void ToggleRepulsionMode();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd")
    void SetNeutralMode();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd")
    EShepherdMode GetCurrentMode() const { return CurrentMode; }
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd")
    bool IsNotNeutral() const { return CurrentMode != EShepherdMode::Neutral; }
    
    // ========== Carrying Functions ==========
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Carrying")
    void TryPickupCow();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Carrying")
    void DropCow();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Carrying")
    bool CanPickupCow() const;
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Carrying")
    class ACowCharacter* GetCowInPickupRange() const;
    
    // ========== Throwing Functions ==========
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Throwing")
    void StartChargingThrow();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Throwing")
    void ReleaseThrow();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Throwing")
    void CancelThrow();
    
    UFUNCTION(BlueprintPure, Category = "Shepherd|Throwing")
    float GetCurrentThrowPower() const { return CurrentThrowPower; }
    
    UFUNCTION(BlueprintPure, Category = "Shepherd|Throwing")
    FVector CalculateThrowVelocity() const;
    
    // ========== Input Handling ==========
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Input")
    void HandleAttractionInput();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Input")
    void HandleRepulsionInput();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Input")
    void HandlePickupInput();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Input")
    void HandleThrowPressed();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Input")
    void HandleThrowReleased();

private:
    void UpdateNearbyCows();
    void DrawModeIndicator();
    void UpdateCarriedCow(float DeltaTime);
    void UpdateThrowCharge(float DeltaTime);
    void DrawThrowTrajectory();
    void ThrowCow();
    FVector GetCarryPosition() const;
    void DisableCowPhysics(class ACowCharacter* Cow);
    void EnableCowPhysics(class ACowCharacter* Cow);
    
    // Cache of nearby cows for efficient updates
    UPROPERTY()
    TArray<class ACowCharacter*> NearbyCows;
    
    float UpdateTimer = 0.0f;
    const float UpdateInterval = 0.2f; // Update nearby cows every 0.2 seconds
    
    // Trajectory preview points
    TArray<FVector> TrajectoryPointsCache;
};