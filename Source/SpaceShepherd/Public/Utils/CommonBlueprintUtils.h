// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CommonBlueprintUtils.generated.h"

/**
* Common routines exposed to Blueprint not tied to a specific game module
*/
UCLASS()
class SPACESHEPHERD_API UCommonBlueprintUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	static bool IsDevelopmentOrEditorBuild()
	{
#if WITH_EDITOR || !UE_BUILD_SHIPPING
		return true;
#else
		return false;
#endif
	}
};

