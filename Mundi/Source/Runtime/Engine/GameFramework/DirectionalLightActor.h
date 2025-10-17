﻿#pragma once
#include "Actor.h"

class UDirectionalLightComponent;

class ADirectionalLightActor : public AActor
{
public:
	DECLARE_CLASS(ADirectionalLightActor, AActor)

	ADirectionalLightActor();
protected:
	~ADirectionalLightActor() override;

public:
	UDirectionalLightComponent* GetLightComponent() const { return LightComponent; }

	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(ADirectionalLightActor)

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	UDirectionalLightComponent* LightComponent;
};
