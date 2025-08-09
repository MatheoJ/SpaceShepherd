// BaseTrap.cpp
#include "BaseTrap.h"
#include "CowsAI/CowCharacter.h"
#include "SpaceShepherdCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"

ABaseTrap::ABaseTrap()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Create root component
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
    
    // Create mesh component
    TrapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrapMesh"));
    TrapMesh->SetupAttachment(Root);
    TrapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    // Create trigger volume
    TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
    TriggerVolume->SetupAttachment(Root);
    TriggerVolume->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));
    TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    
    // Initialize state
    CurrentState = ETrapState::Armed;
    TimeInCurrentState = 0.0f;
}

void ABaseTrap::BeginPlay()
{
    Super::BeginPlay();
    
    // Bind overlap events
    if (TriggerVolume)
    {
        TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ABaseTrap::OnTriggerBeginOverlap);
        TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &ABaseTrap::OnTriggerEndOverlap);
    }
    
    // Create dynamic material instance for visual feedback
    if (TrapMesh && TrapMesh->GetMaterial(0))
    {
        DynamicMaterial = TrapMesh->CreateAndSetMaterialInstanceDynamic(0);
    }
    
    // Set initial state
    if (bStartArmed)
    {
        ArmTrap();
    }
    else
    {
        DisarmTrap();
    }
    
    UpdateVisualState();
}

void ABaseTrap::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    UpdateStateTime(DeltaTime);
    
    if (bShowDebugVisuals)
    {
        FColor DebugColor = FColor::White;
        
        switch (CurrentState)
        {
            case ETrapState::Armed:
                DebugColor = FColor::Orange;
                break;
            case ETrapState::Triggered:
                DebugColor = FColor::Yellow;
                break;
            case ETrapState::Active:
                DebugColor = FColor::Red;
                break;
            case ETrapState::Cooldown:
                DebugColor = FColor::Blue;
                break;
            case ETrapState::Disabled:
                DebugColor = FColor::Black;
                break;
            default:
                DebugColor = FColor::White;
                break;
        }
        
        DrawDebugBox(GetWorld(), GetActorLocation(), TriggerVolume->GetScaledBoxExtent(), 
                    GetActorQuat(), DebugColor, false, -1, 0, 2.0f);
        
        // Draw state text
        FString StateText = UEnum::GetValueAsString(CurrentState);
        DrawDebugString(GetWorld(), GetActorLocation() + FVector(0, 0, 100), 
                       StateText, nullptr, DebugColor, 0.0f, true, 1.2f);
        
        // Draw timer if applicable
        if (CurrentState == ETrapState::Triggered || CurrentState == ETrapState::Cooldown)
        {
            float RemainingTime = 0.0f;
            
            if (CurrentState == ETrapState::Triggered && GetWorld()->GetTimerManager().IsTimerActive(ActivationTimerHandle))
            {
                RemainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(ActivationTimerHandle);
            }
            else if (CurrentState == ETrapState::Cooldown && GetWorld()->GetTimerManager().IsTimerActive(CooldownTimerHandle))
            {
                RemainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(CooldownTimerHandle);
            }
            
            FString TimerText = FString::Printf(TEXT("%.1fs"), RemainingTime);
            DrawDebugString(GetWorld(), GetActorLocation() + FVector(0, 0, 80), 
                          TimerText, nullptr, FColor::White, 0.0f, true, 1.0f);
        }
    }
}

void ABaseTrap::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !CanBeTriggerredBy(OtherActor))
        return;
    
    // Add to tracking list
    if (!ActorsInTrigger.Contains(OtherActor))
    {
        ActorsInTrigger.Add(OtherActor);
    }
    
    // Trigger the trap if it's armed
    if (CurrentState == ETrapState::Armed)
    {
        OnTrigger(OtherActor);
    }
}

void ABaseTrap::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    // Remove from tracking list
    ActorsInTrigger.Remove(OtherActor);
}

void ABaseTrap::ArmTrap()
{
    if (CurrentState == ETrapState::Disabled || CurrentState == ETrapState::Armed)
        return;
    
    SetTrapState(ETrapState::Armed);
    
    if (RearmSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, RearmSound, GetActorLocation());
    }
}

void ABaseTrap::DisarmTrap()
{
    // Cancel any pending timers
    GetWorld()->GetTimerManager().ClearTimer(ActivationTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
    
    SetTrapState(ETrapState::Disabled);
}

void ABaseTrap::ResetTrap()
{
    // Clear all timers
    GetWorld()->GetTimerManager().ClearTimer(ActivationTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
    
    // Clear tracking
    ActorsInTrigger.Empty();
    LastTriggeringActor = nullptr;
    
    // Reset to armed state
    if (bStartArmed)
    {
        ArmTrap();
    }
    else
    {
        DisarmTrap();
    }
}

void ABaseTrap::SetTrapState(ETrapState NewState)
{
    if (CurrentState == NewState)
        return;
    
    ETrapState OldState = CurrentState;
    CurrentState = NewState;
    TimeInCurrentState = 0.0f;
    
    UpdateVisualState();
    
    // Handle state exit
    if (OldState == ETrapState::Active)
    {
        OnDeactivate();
    }
    
    // Handle state entry
    if (NewState == ETrapState::Active)
    {
        OnActivate();
    }
}

void ABaseTrap::OnTrigger(AActor* TriggeringActor)
{
    if (CurrentState != ETrapState::Armed)
        return;
    
    LastTriggeringActor = TriggeringActor;
    SetTrapState(ETrapState::Triggered);
    
    // Broadcast trigger event
    if (ACowCharacter* Cow = Cast<ACowCharacter>(TriggeringActor))
    {
        OnTrapTriggered.Broadcast(this, Cow);
    }
    
    PlayTriggerEffects();
    
    // Start activation timer
    if (ActivationDelay > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(ActivationTimerHandle, this, 
            &ABaseTrap::ActivateTrap, ActivationDelay, false);
    }
    else
    {
        ActivateTrap();
    }
}

void ABaseTrap::OnActivate()
{
    // Override in derived classes
    OnTrapActivated.Broadcast(this);
}

void ABaseTrap::OnDeactivate()
{
    // Override in derived classes
    OnTrapDeactivated.Broadcast(this);
    
    if (bSingleUse)
    {
        DisarmTrap();
    }
    else
    {
        StartCooldown();
    }
}

void ABaseTrap::OnCooldownComplete()
{
    // Rearm the trap
    ArmTrap();
}

bool ABaseTrap::CanBeTriggerredBy(AActor* Actor) const
{
    if (!Actor)
        return false;
    
    // Check if it's a cow
    if (bRequireCowToTrigger)
    {
        if (Cast<ACowCharacter>(Actor))
            return true;
    }
    
    // Check if it's a player
    if (bCanTriggerOnPlayer)
    {
        if (Cast<ASpaceShepherdCharacter>(Actor))
            return true;
    }
    
    return false;
}

void ABaseTrap::UpdateVisualState()
{
    if (!DynamicMaterial)
        return;
    
    FLinearColor StateColor = IdleColor;
    
    switch (CurrentState)
    {
        case ETrapState::Armed:
            StateColor = ArmedColor;
            break;
        case ETrapState::Triggered:
            StateColor = TriggeredColor;
            break;
        case ETrapState::Active:
            StateColor = ActiveColor;
            break;
        case ETrapState::Cooldown:
            StateColor = CooldownColor;
            break;
        default:
            StateColor = IdleColor;
            break;
    }
    
    DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), StateColor);
    DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), StateColor * 2.0f);
}

void ABaseTrap::PlayTriggerEffects()
{
    if (TriggerSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, TriggerSound, GetActorLocation());
    }
    
    if (TriggerNiagaraEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TriggerNiagaraEffect, 
            GetActorLocation(), GetActorRotation());
    }
    else if (TriggerEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TriggerEffect, 
            GetActorLocation(), GetActorRotation());
    }
}

void ABaseTrap::PlayActivateEffects()
{
    if (ActivateSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ActivateSound, GetActorLocation());
    }
    
    if (ActivateNiagaraEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ActivateNiagaraEffect, 
            GetActorLocation(), GetActorRotation());
    }
    else if (ActivateEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ActivateEffect, 
            GetActorLocation(), GetActorRotation());
    }
}

TArray<ACowCharacter*> ABaseTrap::GetCowsInTrigger() const
{
    TArray<ACowCharacter*> CowsInTrigger;
    
    for (AActor* Actor : ActorsInTrigger)
    {
        if (ACowCharacter* Cow = Cast<ACowCharacter>(Actor))
        {
            CowsInTrigger.Add(Cow);
        }
    }
    
    return CowsInTrigger;
}

TArray<AActor*> ABaseTrap::GetActorsInTrigger() const
{
    return ActorsInTrigger;
}

void ABaseTrap::ActivateTrap()
{
    SetTrapState(ETrapState::Active);
    PlayActivateEffects();
}

void ABaseTrap::StartCooldown()
{
    SetTrapState(ETrapState::Cooldown);
    
    if (CooldownDuration > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, 
            &ABaseTrap::OnCooldownComplete, CooldownDuration, false);
    }
    else
    {
        OnCooldownComplete();
    }
}

void ABaseTrap::UpdateStateTime(float DeltaTime)
{
    TimeInCurrentState += DeltaTime;
}