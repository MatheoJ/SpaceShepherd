// SpaceShepherdCharacter.cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceShepherdCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "CowsAI/PlayerShepherdComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

void ASpaceShepherdCharacter::OnGravityChanged()
{
}

ASpaceShepherdCharacter::ASpaceShepherdCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// Create Shepherd Component
	ShepherdComponent = CreateDefaultSubobject<UPlayerShepherdComponent>(TEXT("ShepherdComponent"));
}

void ASpaceShepherdCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) 
	{
		// ========== Movement Actions ==========
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASpaceShepherdCharacter::Move);
		
		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASpaceShepherdCharacter::Look);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ASpaceShepherdCharacter::Look);
		
		// ========== Shepherd Mode Actions ==========
		
		// Mode switching
		if (AttractionAction)
		{
			EnhancedInputComponent->BindAction(AttractionAction, ETriggerEvent::Started, this, &ASpaceShepherdCharacter::OnAttractionPressed);
		}
		
		if (RepulsionAction)
		{
			EnhancedInputComponent->BindAction(RepulsionAction, ETriggerEvent::Started, this, &ASpaceShepherdCharacter::OnRepulsionPressed);
		}
		
		if (NeutralAction)
		{
			EnhancedInputComponent->BindAction(NeutralAction, ETriggerEvent::Started, this, &ASpaceShepherdCharacter::OnNeutralPressed);
		}
		
		// ========== Cow Interaction Actions ==========
		
		// Pickup/Drop
		if (PickupAction)
		{
			EnhancedInputComponent->BindAction(PickupAction, ETriggerEvent::Started, this, &ASpaceShepherdCharacter::OnPickupPressed);
		}
		
		// Throw (Hold to charge)
		if (ThrowAction)
		{
			EnhancedInputComponent->BindAction(ThrowAction, ETriggerEvent::Started, this, &ASpaceShepherdCharacter::OnThrowPressed);
			EnhancedInputComponent->BindAction(ThrowAction, ETriggerEvent::Completed, this, &ASpaceShepherdCharacter::OnThrowReleased);
		}
		
		// ========== Laser Attraction (Right Click Hold) ==========
		
		if (LaserAction)
		{
			EnhancedInputComponent->BindAction(LaserAction, ETriggerEvent::Started, this, &ASpaceShepherdCharacter::OnLaserPressed);
			EnhancedInputComponent->BindAction(LaserAction, ETriggerEvent::Completed, this, &ASpaceShepherdCharacter::OnLaserReleased);
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ASpaceShepherdCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ASpaceShepherdCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ASpaceShepherdCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		FVector CameraForwardVector = FollowCamera->GetForwardVector();
		FVector CameraRightVector = FollowCamera->GetRightVector();

		FVector GravityDirection = GetCharacterMovement()->GetGravityDirection();

		CameraForwardVector = CameraForwardVector - FVector::DotProduct(CameraForwardVector, GravityDirection) * CameraForwardVector;
		CameraRightVector = CameraRightVector - FVector::DotProduct(CameraRightVector, GravityDirection) * CameraRightVector;
		//Normalize the vectors to ensure they are unit vectors
		CameraForwardVector.Normalize();
		CameraRightVector.Normalize();
		
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(CameraForwardVector, Forward);
		AddMovementInput(CameraRightVector, Right);
	}
}

void ASpaceShepherdCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ASpaceShepherdCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ASpaceShepherdCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void ASpaceShepherdCharacter::OnAttractionPressed()
{
	if (ShepherdComponent)
	{
		ShepherdComponent->HandleAttractionInput();
		UE_LOG(LogTemplateCharacter, Log, TEXT("Attraction mode toggled"));
	}
}

void ASpaceShepherdCharacter::OnRepulsionPressed()
{
	if (ShepherdComponent)
	{
		ShepherdComponent->HandleRepulsionInput();
		UE_LOG(LogTemplateCharacter, Log, TEXT("Repulsion mode toggled"));
	}
}

void ASpaceShepherdCharacter::OnNeutralPressed()
{
	if (ShepherdComponent)
	{
		ShepherdComponent->SetNeutralMode();
		UE_LOG(LogTemplateCharacter, Log, TEXT("Neutral mode set"));
	}
}

void ASpaceShepherdCharacter::OnPickupPressed()
{
	if (ShepherdComponent)
	{
		ShepherdComponent->HandlePickupInput();
		
		// Log pickup state
		if (ShepherdComponent->bIsCarryingCow)
		{
			UE_LOG(LogTemplateCharacter, Log, TEXT("Picked up cow"));
		}
		else
		{
			UE_LOG(LogTemplateCharacter, Log, TEXT("Dropped cow or no cow to pickup"));
		}
	}
}

void ASpaceShepherdCharacter::OnThrowPressed()
{
	if (ShepherdComponent)
	{
		ShepherdComponent->HandleThrowPressed();
		UE_LOG(LogTemplateCharacter, Log, TEXT("Started charging throw"));
	}
}

void ASpaceShepherdCharacter::OnThrowReleased()
{
	if (ShepherdComponent)
	{
		ShepherdComponent->HandleThrowReleased();
		UE_LOG(LogTemplateCharacter, Log, TEXT("Released throw with power: %.2f"), ShepherdComponent->GetCurrentThrowPower());
	}
}

void ASpaceShepherdCharacter::OnLaserPressed()
{
	if (ShepherdComponent)
	{
		ShepherdComponent->HandleLaserPressed();
		UE_LOG(LogTemplateCharacter, Log, TEXT("Laser attraction activated"));
	}
}

void ASpaceShepherdCharacter::OnLaserReleased()
{
	if (ShepherdComponent)
	{
		ShepherdComponent->HandleLaserReleased();
		UE_LOG(LogTemplateCharacter, Log, TEXT("Laser attraction deactivated"));
	}
}