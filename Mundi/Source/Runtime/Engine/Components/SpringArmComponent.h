#pragma once
#include "SceneComponent.h"

class USpringArmComponent : public USceneComponent
{
public:
	DECLARE_CLASS(USpringArmComponent, USceneComponent)
	GENERATED_REFLECTION_BODY()

	USpringArmComponent();
	virtual ~USpringArmComponent() override;

	DECLARE_DUPLICATE(USpringArmComponent)

protected:
	void TickComponent(float DeltaTime) override;

private:
	float TargetArmLength;
	bool bDoCollisionTest;
};