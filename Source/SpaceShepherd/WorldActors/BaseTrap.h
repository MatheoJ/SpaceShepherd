// BaseTrap.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseTrap.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTrapTriggered, class ABaseTrap*, Trap, class ACowCharacter*, TriggeringCow);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrapActivated, class ABaseTrap*, Trap);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrapDeactivated, class ABaseTrap*, Trap);

UENUM(BlueprintType)
enum class ETrapState : uint8
{
    Idle        UMETA(DisplayName = "Idle"),
    Armed       UMETA(DisplayName = "Armed"),
    Triggered   UMETA(DisplayName = "Triggered"),
    Active      UMETA(DisplayName = "Active"),
    Cooldown    UMETA(DisplayName = "Cooldown"),
    Disabled    UMETA(DisplayName = "Disabled")
};

UCLASS(Abstract)
class ABaseTrap : public AActor
{
    GENERATED_BODY()

public:
    ABaseTrap();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // ========== Components ==========
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USceneComponent* Root;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* TrapMesh;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* TriggerVolume;
    
    // ========== Trap Configuration ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
    ETrapState CurrentState = ETrapState::Armed;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
    bool bStartArmed = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
    bool bSingleUse = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
    float CooldownDuration = 3.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
    float ActivationDelay = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
    bool bRequireCowToTrigger = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
    bool bCanTriggerOnPlayer = false;
    
    // ========== Visual Feedback ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Visual")
    bool bShowDebugVisuals = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Visual")
    FLinearColor IdleColor = FLinearColor(0.5f, 0.5f, 0.5f);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Visual")
    FLinearColor ArmedColor = FLinearColor(1.0f, 0.5f, 0.0f);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Visual")
    FLinearColor TriggeredColor = FLinearColor(1.0f, 1.0f, 0.0f);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Visual")
    FLinearColor ActiveColor = FLinearColor(1.0f, 0.0f, 0.0f);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Visual")
    FLinearColor CooldownColor = FLinearColor(0.0f, 0.0f, 1.0f);
    
    // ========== Audio ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Audio")
    class USoundBase* TriggerSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Audio")
    class USoundBase* ActivateSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Audio")
    class USoundBase* RearmSound;
    
    // ========== Effects ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Effects")
    class UParticleSystem* TriggerEffect;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Effects")
    class UParticleSystem* ActivateEffect;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Effects")
    class UNiagaraSystem* TriggerNiagaraEffect;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Effects")
    class UNiagaraSystem* ActivateNiagaraEffect;
    
    // ========== Events ==========
    
    UPROPERTY(BlueprintAssignable, Category = "Trap|Events")
    FOnTrapTriggered OnTrapTriggered;
    
    UPROPERTY(BlueprintAssignable, Category = "Trap|Events")
    FOnTrapActivated OnTrapActivated;
    
    UPROPERTY(BlueprintAssignable, Category = "Trap|Events")
    FOnTrapDeactivated OnTrapDeactivated;
    
    // ========== Public Functions ==========
    
    UFUNCTION(BlueprintCallable, Category = "Trap")
    virtual void ArmTrap();
    
    UFUNCTION(BlueprintCallable, Category = "Trap")
    virtual void DisarmTrap();
    
    UFUNCTION(BlueprintCallable, Category = "Trap")
    virtual void ResetTrap();
    
    UFUNCTION(BlueprintCallable, Category = "Trap")
    virtual void SetTrapState(ETrapState NewState);
    
    UFUNCTION(BlueprintPure, Category = "Trap")
    bool IsArmed() const { return CurrentState == ETrapState::Armed; }
    
    UFUNCTION(BlueprintPure, Category = "Trap")
    bool IsActive() const { return CurrentState == ETrapState::Active; }
    
    UFUNCTION(BlueprintPure, Category = "Trap")
    bool CanTrigger() const { return CurrentState == ETrapState::Armed; }
    
protected:
    // ========== Protected Functions ==========
    
    UFUNCTION()
    virtual void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    
    UFUNCTION()
    virtual void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
    
    // Virtual functions for derived classes to implement
    virtual void OnTrigger(AActor* TriggeringActor);
    virtual void OnActivate();
    virtual void OnDeactivate();
    virtual void OnCooldownComplete();
    
    // Helper functions
    virtual bool CanBeTriggerredBy(AActor* Actor) const;
    virtual void UpdateVisualState();
    virtual void PlayTriggerEffects();
    virtual void PlayActivateEffects();
    
    // Get all actors currently in the trigger volume
    UFUNCTION(BlueprintCallable, Category = "Trap")
    TArray<class ACowCharacter*> GetCowsInTrigger() const;
    
    UFUNCTION(BlueprintCallable, Category = "Trap")
    TArray<AActor*> GetActorsInTrigger() const;
    
    // Timer handles
    FTimerHandle ActivationTimerHandle;
    FTimerHandle CooldownTimerHandle;
    FTimerHandle StateUpdateTimerHandle;
    
    // Tracking
    UPROPERTY(BlueprintReadOnly, Category = "Trap")
    TArray<AActor*> ActorsInTrigger;
    
    UPROPERTY(BlueprintReadOnly, Category = "Trap")
    AActor* LastTriggeringActor;
    
    UPROPERTY(BlueprintReadOnly, Category = "Trap")
    float TimeInCurrentState = 0.0f;
    
private:
    void ActivateTrap();
    void StartCooldown();
    void UpdateStateTime(float DeltaTime);

protected:
    // Material instance for dynamic color changes
    UPROPERTY()
    class UMaterialInstanceDynamic* DynamicMaterial;
};