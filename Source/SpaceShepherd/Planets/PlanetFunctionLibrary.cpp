// Fill out your copyright notice in the Description page of Project Settings.


#include "Planets/PlanetFunctionLibrary.h"

float UPlanetFunctionLibrary::GravityFalloff(
	const FVector& CharacterPos,
	const FVector& PlanetCenter,
	float SurfaceRadius,
	float FalloffRadius)
{
	const float SurfaceRadiusSq = SurfaceRadius * SurfaceRadius;
	const float FalloffRadiusSq = FalloffRadius * FalloffRadius;
	const float DistanceToSurfaceSq = FVector::DistSquared(CharacterPos, PlanetCenter);

	if (DistanceToSurfaceSq >= FalloffRadiusSq)
		return 0.0f;

	if (DistanceToSurfaceSq <= SurfaceRadiusSq)
		return 1.0f;

	// physically accurate inverse square falloff
	const float InverseSqFalloff = SurfaceRadiusSq / DistanceToSurfaceSq;
	// linear falloff (inverse square a des asymptotes mais on veut avoir des boundaries 0 et 1 exact)
	const float LinearFalloff = 1.0f - (DistanceToSurfaceSq - SurfaceRadiusSq) / (FalloffRadiusSq - SurfaceRadiusSq);
	const float WeightedFalloff = FMath::Clamp(InverseSqFalloff * LinearFalloff, 0.0f, 1.0f);
	
	return WeightedFalloff;
}
