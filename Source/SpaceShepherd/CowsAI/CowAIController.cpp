// ===================================
// CowAIController.cpp
// ===================================

#include "CowAIController.h"
#include "CowCharacter.h"
#include "Navigation/CrowdFollowingComponent.h"

ACowAIController::ACowAIController()
{
	// Use basic movement without crowd manager (we handle avoidance in boids)
	bWantsPlayerState = false;
}

void ACowAIController::BeginPlay()
{
	Super::BeginPlay();
}

void ACowAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
    
	// Ensure we're possessing a cow
	ACowCharacter* Cow = Cast<ACowCharacter>(InPawn);
	if (Cow)
	{
		// Additional setup if needed
	}
}

void ACowAIController::OnUnPossess()
{
	Super::OnUnPossess();
}