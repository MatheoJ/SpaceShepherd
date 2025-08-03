// Copyright Epic Games, Inc. All Rights Reserved.


#include "SpaceShepherdPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "HerdingGameMode/CowHerdingGameMode.h"
#include "Kismet/GameplayStatics.h"

void ASpaceShepherdPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Add Input Mapping Contexts
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
}

void ASpaceShepherdPlayerController::BeginPlay()
{
	Super::BeginPlay();
    
	GameModeRef = Cast<ACowHerdingGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
}

void ASpaceShepherdPlayerController::RestartGame()
{
	if (GameModeRef)
	{
		// End current game if active
		if (GameModeRef->bGameActive)
		{
			GameModeRef->EndGame();
		}
        
		// Start new game
		GameModeRef->StartGame();
	}
}

void ASpaceShepherdPlayerController::PauseGame()
{
	if (GameModeRef && GameModeRef->bGameActive)
	{
		if (UGameplayStatics::IsGamePaused(GetWorld()))
		{
			GameModeRef->ResumeGame();
		}
		else
		{
			GameModeRef->PauseGame();
		}
	}
}