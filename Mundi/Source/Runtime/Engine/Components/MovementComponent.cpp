﻿#include "pch.h"
#include "MovementComponent.h"
#include "SceneComponent.h"
#include "ObjectFactory.h"

IMPLEMENT_CLASS(UMovementComponent)

UMovementComponent::UMovementComponent()
    : UpdatedComponent(nullptr)
    , Velocity(0.0f, 0.0f, 0.0f)
    , Acceleration(0.0f, 0.0f, 0.0f)
    , bUpdateOnlyIfRendered(false)
{
    // Movement component는 기본적으로 Tick 가능
    bCanEverTick = true;
    Owner = GetOwner();
}

UMovementComponent::~UMovementComponent()
{
}

void UMovementComponent::InitializeComponent()
{
    Super::InitializeComponent();
    
    // 자신을 소유한 액터의 루트 컴포넌트를 UpdatedComponent로 설정
    if (!UpdatedComponent && Owner)
    {
        AActor* OwnerActor = Cast<AActor>(Owner);
        if (OwnerActor)
        {
            UpdatedComponent = OwnerActor->GetRootComponent();
        }
    }
}

void UMovementComponent::TickComponent(float DeltaSeconds)
{
    Super::TickComponent(DeltaSeconds);

    if (!bIsActive || !bCanEverTick)
        return;

    // 매 프레임 호출, 파생 클래스에서 오버라이드하여 이동 로직 구현
}

void UMovementComponent::SetVelocity(const FVector& NewVelocity)
{
    Velocity = NewVelocity;
}

void UMovementComponent::SetAcceleration(const FVector& NewAcceleration)
{
    Acceleration = NewAcceleration;
}

void UMovementComponent::StopMovement()
{
    Velocity = FVector(0.0f, 0.0f, 0.0f);
    Acceleration = FVector(0.0f, 0.0f, 0.0f);
}

void UMovementComponent::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
    UpdatedComponent = NewUpdatedComponent;
}

void UMovementComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
    // MovementComponent has no sub-objects to duplicate
}
