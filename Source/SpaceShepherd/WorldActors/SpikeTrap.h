// SpikeTrap.h
#pragma once

#include "CoreMinimal.h"
#include "BaseTrap.h"
#include "SpikeTrap.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCowKilledBySpikes, class ASpikeTrap*, Trap, class ACowCharacter*, KilledCow);

UCLASS()
class ASpikeTrap : public ABaseTrap
{
    GENERATED_BODY()

public:
    ASpikeTrap();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    // ========== Spike Configuration ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap")
    float SpikeDamage = 100.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap")
    bool bInstantKill = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap")
    float SpikeActiveDuration = 2.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap")
    float SpikeRetractSpeed = 200.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap")
    float SpikeExtendSpeed = 500.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap")
    float SpikeMaxHeight = 100.0f;
    
    // Warning phase before activation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap")
    bool bShowWarning = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap")
    float WarningDuration = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap")
    float WarningPulseSpeed = 5.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap")
    FLinearColor WarningColor = FLinearColor(1.0f, 0.3f, 0.0f);
    
    // ========== Components ==========
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* SpikeMesh;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* WarningIndicatorMesh;
    
    // ========== Effects ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap|Effects")
    class UParticleSystem* BloodSplatterEffect;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap|Effects")
    class UNiagaraSystem* BloodSplatterNiagaraEffect;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap|Audio")
    class USoundBase* SpikeExtendSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap|Audio")
    class USoundBase* SpikeRetractSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap|Audio")
    class USoundBase* ImpaleSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Trap|Audio")
    class USoundBase* WarningSound;
    
    // ========== Events ==========
    
    UPROPERTY(BlueprintAssignable, Category = "Spike Trap|Events")
    FOnCowKilledBySpikes OnCowKilled;
    
protected:
    // Override base trap functions
    virtual void OnTrigger(AActor* TriggeringActor) override;
    virtual void OnActivate() override;
    virtual void OnDeactivate() override;
    
    // Spike-specific functions
    void StartWarningPhase();
    void EndWarningPhase();
    void ExtendSpikes();
    void RetractSpikes();
    void KillCowsOnSpikes();
    void UpdateSpikePosition(float DeltaTime);
    void UpdateWarningVisuals(float DeltaTime);
    
    UFUNCTION()
    void OnSpikeActiveDurationComplete();
    
private:
    // Spike state
    float CurrentSpikeHeight = 0.0f;
    float TargetSpikeHeight = 0.0f;
    bool bSpikesExtending = false;
    bool bSpikesRetracting = false;
    bool bInWarningPhase = false;
    float WarningPhaseTime = 0.0f;
    
    // Tracking killed cows to avoid double-killing
    UPROPERTY()
    TArray<class ACowCharacter*> KilledCows;
    
    // Timer handles
    FTimerHandle SpikeActiveTimerHandle;
    FTimerHandle WarningTimerHandle;
    
    // Material instances for dynamic effects
    UPROPERTY()
    class UMaterialInstanceDynamic* SpikeDynamicMaterial;
    
    UPROPERTY()
    class UMaterialInstanceDynamic* WarningDynamicMaterial;
};