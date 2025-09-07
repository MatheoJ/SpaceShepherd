// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlanetFunctionLibrary.generated.h"

UCLASS()
class SPACESHEPHERD_API UPlanetFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static float GravityFalloff(const FVector& CharacterPos, const FVector& PlanetCenter, float SurfaceRadius, float FalloffRadius);
};
