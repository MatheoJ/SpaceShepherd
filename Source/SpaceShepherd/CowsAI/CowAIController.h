// CowAIController.h
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CowAIController.generated.h"

UCLASS()
class ACowAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACowAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;
};

