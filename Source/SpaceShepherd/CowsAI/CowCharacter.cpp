// CowCharacter.cpp
#include "CowCharacter.h"
#include "CowBoidsComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AI/Navigation/NavigationTypes.h"

ACowCharacter::ACowCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // We'll handle rotation in boids
	GetCharacterMovement()->RotationRate = FRotator(0.0f, TurnRate, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 200.f;
    
	// Enable navigation
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bUseRVOAvoidance = false; // We handle avoidance in boids
    
	// Set collision
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    
	// Create Boids Component
	BoidsComponent = CreateDefaultSubobject<UCowBoidsComponent>(TEXT("BoidsComponent"));
    
	// Set default AI controller
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ACowCharacter::BeginPlay()
{
	Super::BeginPlay();
    
	// Configure movement component
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

void ACowCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACowCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// No input needed for AI cows
}

void ACowCharacter::SetPlayerAttraction(bool bAttracted)
{
	bIsAttractedToPlayer = bAttracted;
	bIsRepulsedByPlayer = false; // Can't be both
}

void ACowCharacter::SetPlayerRepulsion(bool bRepulsed)
{
	bIsRepulsedByPlayer = bRepulsed;
	bIsAttractedToPlayer = false; // Can't be both
}