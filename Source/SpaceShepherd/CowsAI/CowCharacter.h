// CowCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CowCharacter.generated.h"

UCLASS()
class ACowCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACowCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Boids AI Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UCowBoidsComponent* BoidsComponent;

	// Movement settings
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float WalkSpeed = 150.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float TurnRate = 180.0f;

public:
	// Player interaction states (for future use)
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	bool bIsAttractedToPlayer = false;
    
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	bool bIsRepulsedByPlayer = false;
    
	// Utility functions
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetPlayerAttraction(bool bAttracted);
    
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetPlayerRepulsion(bool bRepulsed);
};