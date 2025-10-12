﻿#include "pch.h"
#include "AABoundingBoxComponent.h"
#include "StaticMeshActor.h"
#include "StaticMeshComponent.h"
#include "ObjectFactory.h"
#include "BillboardComponent.h"
AStaticMeshActor::AStaticMeshActor()
{
    Name = "Static Mesh Actor";
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
    
    // 루트 교체
    USceneComponent* TempRootComponent = RootComponent;
    RootComponent = StaticMeshComponent;
    RemoveOwnedComponent(TempRootComponent);
}

void AStaticMeshActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    //AddActorWorldLocation(FVector(3.0f, 0.0f, 0.0f) * DeltaTime);
    if (bIsPicked)
    {
        //CollisionComponent->SetFromVertices(StaticMeshComponent->GetStaticMesh()->GetStaticMeshAsset()->Vertices);
    }
}

AStaticMeshActor::~AStaticMeshActor()
{
    if (StaticMeshComponent)
    {
        ObjectFactory::DeleteObject(StaticMeshComponent);
    }
    StaticMeshComponent = nullptr;
}

FAABB AStaticMeshActor::GetBounds() const
{
    if (StaticMeshComponent)
    {
        return StaticMeshComponent->GetWorldAABB();
    }

    return FAABB();
}

void AStaticMeshActor::SetStaticMeshComponent(UStaticMeshComponent* InStaticMeshComponent)
{
    StaticMeshComponent = InStaticMeshComponent;
}

void AStaticMeshActor::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    for (UActorComponent* Component : OwnedComponents)
    {
        if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Component))
        {
            StaticMeshComponent = StaticMeshComp;
            break;
        }
    }
}
