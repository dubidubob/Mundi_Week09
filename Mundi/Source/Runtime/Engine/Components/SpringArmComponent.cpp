#include "pch.h"
#include "SpringArmComponent.h"

IMPLEMENT_CLASS(USpringArmComponent)
	BEGIN_PROPERTIES(USpringArmComponent)
	ADD_PROPERTY(float, TargetArmLength, "Target Arm Length", true, "카메라 arm 길이")
	ADD_PROPERTY(bool, bDoCollisionTest, "Do Collision Test", true, "충돌 테스트")
END_PROPERTIES()

USpringArmComponent::USpringArmComponent()
	: TargetArmLength(10.0f)
	, bDoCollisionTest(true)
{
	bCanEverTick = true;
	SetWorldLocation(FVector(1.0f, 1.0f, 1.0f));
}

USpringArmComponent::~USpringArmComponent()
{

}

void USpringArmComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	if (!GetAttachParent())	return;

	// Get parent component's transform
	const FTransform& parentTransform = GetAttachParent()->GetWorldTransform();
	const FVector SocketLocation = parentTransform.Translation;
	const FQuat SocketRotation = parentTransform.Rotation;

	const FQuat LocalRotation = GetRelativeRotation();
	const FQuat FinalRotation = SocketRotation * LocalRotation;

	const FVector ArmDirection = SocketRotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
	const FVector UpDirection = SocketRotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));

	float ActualArmLength = TargetArmLength;
	float CameraHeightOffset = 5.0f;

	FVector DesiredLocation = SocketLocation + ArmDirection * ActualArmLength + UpDirection * CameraHeightOffset;

	SetWorldTransform(FTransform(DesiredLocation, SocketRotation, FVector(1.0f, 1.0f, 1.0f)));
}