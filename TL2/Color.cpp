﻿#include "pch.h"
#include "Color.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

// ==============================================================================
// Operators
// ==============================================================================

inline FLinearColor& FLinearColor::operator=(const FVector& Other)
{
    // TODO: insert return statement here
	R = Other.X;
	G = Other.Y;
	B = Other.Z;
	A = 1.0f;
	return *this;
}

inline FLinearColor FLinearColor::operator+(const FLinearColor& Other) const
{
    return FLinearColor(R + Other.R, G + Other.G, B + Other.B, A + Other.A);
}

inline FLinearColor FLinearColor::operator+=(const FLinearColor& Other)
{
    R += Other.R;
    G += Other.G;
    B += Other.B;
    A += Other.A;
    return *this;
}

inline FLinearColor FLinearColor::operator-(const FLinearColor& Other) const
{
    return FLinearColor(R - Other.R, G - Other.G, B - Other.B, A - Other.A);
}

inline FLinearColor FLinearColor::operator-=(const FLinearColor& Other)
{
    R -= Other.R;
    G -= Other.G;
    B -= Other.B;
    A -= Other.A;
    return *this;
}

inline FLinearColor FLinearColor::operator*(const FLinearColor& Other) const
{
    return FLinearColor(R * Other.R, G * Other.G, B * Other.B, A * Other.A);
}

inline FLinearColor FLinearColor::operator*(float Scalar) const
{
    return FLinearColor(R * Scalar, G * Scalar, B * Scalar, A * Scalar);
}

inline FLinearColor& FLinearColor::operator*=(const FLinearColor& Other)
{
    R *= Other.R;
    G *= Other.G;
    B *= Other.B;
    A *= Other.A;
    return *this;
}

inline FLinearColor& FLinearColor::operator*=(float Scalar)
{
    R *= Scalar;
    G *= Scalar;
    B *= Scalar;
    A *= Scalar;
    return *this;
}

inline FLinearColor FLinearColor::operator/(const FLinearColor& Other) const
{
    return FLinearColor(R / Other.R, G / Other.G, B / Other.B, A / Other.A);
}

inline FLinearColor FLinearColor::operator/(float Scalar) const
{
    const float InvScalar = 1.0f / Scalar;
    return FLinearColor(R * InvScalar, G * InvScalar, B * InvScalar, A * InvScalar);
}

inline FLinearColor& FLinearColor::operator/=(const FLinearColor& Other)
{
    R /= Other.R;
    G /= Other.G;
    B /= Other.B;
    A /= Other.A;
    return *this;
}

inline FLinearColor& FLinearColor::operator/=(float Scalar)
{
    const float InvScalar = 1.0f / Scalar;
    R *= InvScalar;
    G *= InvScalar;
    B *= InvScalar;
    A *= InvScalar;
    return *this;
}

// ==============================================================================
// Member Methods
// ==============================================================================

bool FLinearColor::Equals(const FLinearColor& ColorB, float Tolerance) const
{
    return std::fabs(R - ColorB.R) <= Tolerance &&
        std::fabs(G - ColorB.G) <= Tolerance &&
        std::fabs(B - ColorB.B) <= Tolerance &&
        std::fabs(A - ColorB.A) <= Tolerance;
}

FLinearColor FLinearColor::GetClamped(float InMin, float InMax) const
{
    return FLinearColor(
        std::clamp(R, InMin, InMax),
        std::clamp(G, InMin, InMax),
        std::clamp(B, InMin, InMax),
        std::clamp(A, InMin, InMax)
    );
}

float FLinearColor::GetMax() const
{
    return std::max({ R, G, B, A });
}

float FLinearColor::GetMin() const
{
    return std::min({ R, G, B, A });
}

// ==============================================================================
// Static Utility Methods
// ==============================================================================

float FLinearColor::Dist(const FLinearColor& C1, const FLinearColor& C2)
{
    const float DR = C1.R - C2.R;
    const float DG = C1.G - C2.G;
    const float DB = C1.B - C2.B;
    const float DA = C1.A - C2.A;
    return std::sqrt(DR * DR + DG * DG + DB * DB + DA * DA);
}

float FLinearColor::DistSquared(const FLinearColor& C1, const FLinearColor& C2)
{
    const float DR = C1.R - C2.R;
    const float DG = C1.G - C2.G;
    const float DB = C1.B - C2.B;
    const float DA = C1.A - C2.A;
    return DR * DR + DG * DG + DB * DB + DA * DA;
}

FLinearColor FLinearColor::Lerp(const FLinearColor& C1, const FLinearColor& C2, float T)
{
    const float OneMinusT = 1.0f - T;
    return FLinearColor(
        C1.R * OneMinusT + C2.R * T,
        C1.G * OneMinusT + C2.G * T,
        C1.B * OneMinusT + C2.B * T,
        C1.A * OneMinusT + C2.A * T
    );
}

FLinearColor FLinearColor::MakeRandomColor()
{
    return FLinearColor(
        static_cast<float>(rand()) / RAND_MAX,
        static_cast<float>(rand()) / RAND_MAX,
        static_cast<float>(rand()) / RAND_MAX,
        1.0f  // Alpha는 1.0으로 고정
    );
}

FLinearColor FLinearColor::MakeRandomSeededColor(int32 Seed)
{
    srand(static_cast<unsigned int>(Seed));
    return FLinearColor(
        static_cast<float>(rand()) / RAND_MAX,
        static_cast<float>(rand()) / RAND_MAX,
        static_cast<float>(rand()) / RAND_MAX,
        1.0f  // Alpha는 1.0으로 고정
    );
}
