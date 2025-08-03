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
    
    // Mode functions
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
    
    // Input handling (to be called from player controller or character)
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Input")
    void HandleAttractionInput();
    
    UFUNCTION(BlueprintCallable, Category = "Shepherd|Input")
    void HandleRepulsionInput();

private:
    void UpdateNearbyCows();
    void DrawModeIndicator();
    
    // Cache of nearby cows for efficient updates
    UPROPERTY()
    TArray<class ACowCharacter*> NearbyCows;
    
    float UpdateTimer = 0.0f;
    const float UpdateInterval = 0.2f; // Update nearby cows every 0.2 seconds
};