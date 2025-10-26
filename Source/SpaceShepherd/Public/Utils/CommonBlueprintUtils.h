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

	// Trivial conversions, for clearer intent (so you dont need to add a comment)
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert m ➞ cm"))
	static float MetersToCentimeters(float Meters)
	{
		return Meters * 100.0f;
	}

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert cm ➞ m"))
	static float CentimetersToMeters(float Centimeters)
	{
		return Centimeters / 100.0f;
	}
};

