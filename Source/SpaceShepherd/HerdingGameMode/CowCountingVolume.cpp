// CowCountingVolume.cpp
#include "CowCountingVolume.h"
#include "CowHerdingGameMode.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

ACowCountingVolume::ACowCountingVolume()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // Create root component
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;
    
    // Create detection volume
    DetectionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DetectionVolume"));
    DetectionVolume->SetupAttachment(RootComponent);
    DetectionVolume->SetBoxExtent(VolumeSize);
    DetectionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DetectionVolume->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    DetectionVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    DetectionVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    
    // Set visualization
    DetectionVolume->SetHiddenInGame(false);
    DetectionVolume->SetLineThickness(2.0f);
}

void ACowCountingVolume::BeginPlay()
{
    Super::BeginPlay();
    
    // Get reference to game mode
    GameModeRef = Cast<ACowHerdingGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    
    if (!GameModeRef)
    {
        UE_LOG(LogTemp, Error, TEXT("CowCountingVolume: Could not find CowHerdingGameMode!"));
        return;
    }
    
    // Bind overlap events
    DetectionVolume->OnComponentBeginOverlap.AddDynamic(this, &ACowCountingVolume::OnVolumeBeginOverlap);
    DetectionVolume->OnComponentEndOverlap.AddDynamic(this, &ACowCountingVolume::OnVolumeEndOverlap);
    
    // Check for any cows already in the volume at start
    TArray<AActor*> OverlappingActors;
    DetectionVolume->GetOverlappingActors(OverlappingActors);
    
    for (AActor* Actor : OverlappingActors)
    {
        if (IsValidCow(Actor))
        {
            GameModeRef->RegisterCowInVolume(Actor);
        }
    }
}

void ACowCountingVolume::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    // Update volume size when changed in editor
    if (DetectionVolume)
    {
        DetectionVolume->SetBoxExtent(VolumeSize);
        DetectionVolume->ShapeColor = VolumeColor;
    }
}

void ACowCountingVolume::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, 
                                               AActor* OtherActor, 
                                               UPrimitiveComponent* OtherComp, 
                                               int32 OtherBodyIndex, 
                                               bool bFromSweep, 
                                               const FHitResult& SweepResult)
{
    if (!GameModeRef || !OtherActor)
    {
        return;
    }
    
    if (IsValidCow(OtherActor))
    {
        GameModeRef->RegisterCowInVolume(OtherActor);
        
        if (bShowDebugInfo)
        {
            DrawDebugSphere(GetWorld(), OtherActor->GetActorLocation(), 50.0f, 12, FColor::Green, false, 2.0f);
        }
    }
}

void ACowCountingVolume::OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, 
                                             AActor* OtherActor, 
                                             UPrimitiveComponent* OtherComp, 
                                             int32 OtherBodyIndex)
{
    if (!GameModeRef || !OtherActor)
    {
        return;
    }
    
    if (IsValidCow(OtherActor))
    {
        GameModeRef->UnregisterCowFromVolume(OtherActor);
        
        if (bShowDebugInfo)
        {
            DrawDebugSphere(GetWorld(), OtherActor->GetActorLocation(), 50.0f, 12, FColor::Red, false, 2.0f);
        }
    }
}

bool ACowCountingVolume::IsValidCow(AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }
    
    // Check if actor has the cow tag
    if (Actor->ActorHasTag(CowActorTag))
    {
        return true;
    }
    
    // Alternative: Check by class name (if your cow AI has a specific naming pattern)
    FString ActorName = Actor->GetClass()->GetName();
    if (ActorName.Contains("BP_COW_AI") || ActorName.Contains("Cow"))
    {
        return true;
    }
    
    return false;
}