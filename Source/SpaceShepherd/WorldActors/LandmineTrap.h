// LandmineTrap.h
#pragma once

#include "CoreMinimal.h"
#include "BaseTrap.h"
#include "LandmineTrap.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCowLaunchedByMine, class ALandmineTrap*, Mine, class ACowCharacter*, LaunchedCow, FVector, LaunchVelocity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMineExploded, class ALandmineTrap*, Mine);

UCLASS()
class ALandmineTrap : public ABaseTrap
{
    GENERATED_BODY()

public:
    ALandmineTrap();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    // ========== Explosion Configuration ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine")
    float ExplosionRadius = 500.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine")
    float ExplosionDamage = 50.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine")
    bool bKillCowsInCenter = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine")
    float KillRadius = 100.0f;
    
    // Launch physics
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Physics")
    float MinLaunchSpeed = 800.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Physics")
    float MaxLaunchSpeed = 1500.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Physics")
    float VerticalLaunchMultiplier = 1.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Physics")
    float HorizontalLaunchMultiplier = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Physics")
    bool bAddRandomSpin = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Physics")
    float MaxSpinRate = 720.0f;
    
    // Arming settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine")
    float ArmingDelay = 2.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine")
    bool bStartArmedAfterDelay = true;
    
    // Warning indicators
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine")
    bool bShowArmingIndicator = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine")
    float BeepInterval = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine")
    float BeepAcceleration = 0.9f;
    
    // ========== Components ==========
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USphereComponent* ExplosionRadiusComponent;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPointLightComponent* ArmingLight;
    
    // ========== Effects ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Effects")
    class UParticleSystem* ExplosionEffect;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Effects")
    class UNiagaraSystem* ExplosionNiagaraEffect;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Effects")
    class UParticleSystem* SmokeEffect;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Effects")
    TSubclassOf<class UCameraShakeBase> ExplosionCameraShake;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Effects")
    float CameraShakeRadius = 1000.0f;
    
    // Audio
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Audio")
    class USoundBase* ExplosionSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Audio")
    class USoundBase* BeepSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Audio")
    class USoundBase* ArmingSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landmine|Audio")
    class USoundBase* ClickSound;
    
    // ========== Events ==========
    
    UPROPERTY(BlueprintAssignable, Category = "Landmine|Events")
    FOnCowLaunchedByMine OnCowLaunched;
    
    UPROPERTY(BlueprintAssignable, Category = "Landmine|Events")
    FOnMineExploded OnMineExploded;
    
protected:
    // Override base trap functions
    virtual void OnTrigger(AActor* TriggeringActor) override;
    virtual void OnActivate() override;
    virtual void OnDeactivate() override;
    
    // Landmine-specific functions
    void Explode();
    void LaunchCow(class ACowCharacter* Cow);
    void ApplyExplosionForces();
    FVector CalculateLaunchVelocity(const FVector& CowLocation) const;
    void PlayExplosionEffects();
    void StartArmingSequence();
    void UpdateArmingIndicators(float DeltaTime);
    
    UFUNCTION()
    void OnArmingComplete();
    
    UFUNCTION()
    void PlayBeep();
    
private:
    // State tracking
    bool bIsArming = false;
    bool bHasExploded = false;
    float ArmingTimeElapsed = 0.0f;
    float CurrentBeepInterval = 1.0f;
    float TimeSinceLastBeep = 0.0f;
    
    // Cows affected by this explosion (to avoid double-launching)
    UPROPERTY()
    TArray<class ACowCharacter*> LaunchedCows;
    
    // Timer handles
    FTimerHandle ArmingTimerHandle;
    FTimerHandle BeepTimerHandle;
};