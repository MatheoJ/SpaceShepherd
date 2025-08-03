// CowCountingVolume.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "CowCountingVolume.generated.h"

UCLASS()
class ACowCountingVolume : public AActor
{
	GENERATED_BODY()
    
public:    
	ACowCountingVolume();
    
	// The tag that cow actors should have to be counted
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cow Detection")
	FName CowActorTag = "Cow";
    
	// Visual representation in editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Settings")
	FVector VolumeSize = FVector(1000.0f, 1000.0f, 500.0f);
    
	// Debug visualization
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugInfo = true;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FColor VolumeColor = FColor::Green;
    
protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
    
	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
							  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
							  bool bFromSweep, const FHitResult& SweepResult);
    
	UFUNCTION()
	void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
							UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
    
private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UBoxComponent* DetectionVolume;
    
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USceneComponent* RootSceneComponent;
    
	class ACowHerdingGameMode* GameModeRef;
    
	bool IsValidCow(AActor* Actor) const;
};