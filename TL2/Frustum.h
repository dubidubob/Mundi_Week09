﻿#pragma once
#include "Vector.h"
#include "AABoundingBoxComponent.h"
#include "CameraComponent.h"


class UCameraComponent;

struct Plane
{
    // unit vector
    FVector4 Normal = { 0.f, 1.f, 0.f , 0.f };

    // 평면이 원점에서 법선 N 방향으로 얼마만큼 떨어져 있는지
    float Distance = 0.f;
};

struct Frustum
{
    Plane TopFace;
    Plane BottomFace;
    Plane RightFace;
    Plane LeftFace;
    Plane NearFace;
    Plane FarFace;
};

// A 32-byte aligned bounding box structure suitable for AVX operations.
struct FAlignedBound
{
    FVector4 Min;
    FVector4 Max;
};

Frustum CreateFrustumFromCamera(const UCameraComponent& Camera, float OverrideAspect = -1.0f);
bool IsAABBVisible(const Frustum& Frustum, const FBound& Bound);

// AVX-optimized culling for 8 AABBs
// Processes 8 AABBs against the frustum.
// Returns an 8-bit mask: bit i is set if box i is visible.
uint8_t AreAABBsVisible_8_AVX(const Frustum& Frustum, const FAlignedBound Bounds[8]);

bool Intersects(const Plane& P, const FVector4& Center, const FVector4& Extents);