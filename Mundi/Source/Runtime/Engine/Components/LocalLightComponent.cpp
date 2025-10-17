﻿#include "pch.h"
#include "LocalLightComponent.h"

IMPLEMENT_CLASS(ULocalLightComponent)

ULocalLightComponent::ULocalLightComponent()
{
	AttenuationRadius = 1000.0f;
	FalloffExponent = 1.0f;
}

ULocalLightComponent::~ULocalLightComponent()
{
}

float ULocalLightComponent::GetAttenuationFactor(const FVector& WorldPosition) const
{
	FVector LightPosition = GetWorldLocation();
	float Distance = FVector::Distance(LightPosition, WorldPosition);

	if (Distance >= AttenuationRadius)
	{
		return 0.0f;
	}

	// 거리 기반 감쇠 계산
	float NormalizedDistance = Distance / AttenuationRadius;
	float Attenuation = 1.0f - pow(NormalizedDistance, FalloffExponent);

	return FMath::Max(0.0f, Attenuation);
}

void ULocalLightComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FJsonSerializer::ReadFloat(InOutHandle, "AttenuationRadius", AttenuationRadius, 1000.0f);
		FJsonSerializer::ReadFloat(InOutHandle, "FalloffExponent", FalloffExponent, 1.0f);
	}
	else
	{
		InOutHandle["AttenuationRadius"] = AttenuationRadius;
		InOutHandle["FalloffExponent"] = FalloffExponent;
	}
}

void ULocalLightComponent::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
}
