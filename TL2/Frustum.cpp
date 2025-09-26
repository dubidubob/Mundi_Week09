﻿#include "pch.h"

#include "Vector.h"
#include "Frustum.h"
#include "AABoundingBoxComponent.h"
#include "CameraComponent.h"


inline FVector4 MakePoint4(const FVector& P) { return FVector4(P.X, P.Y, P.Z, 1.0f); }
inline FVector4 MakeDir4(const FVector& D) { return FVector4(D.X, D.Y, D.Z, 0.0f); }

inline float     Dot3(const FVector4& A, const FVector4& B)
{
    return A.X * B.X + A.Y * B.Y + A.Z * B.Z; // W는 무시
}

inline FVector4  Cross3(const FVector4& A, const FVector4& B)
{
    return FVector4(
        A.Y * B.Z - A.Z * B.Y,
        A.Z * B.X - A.X * B.Z,
        A.X * B.Y - A.Y * B.X,
        0.0f // 방향 벡터
    );
}
inline float     Length3(const FVector4& V)
{
    return std::sqrt(Dot3(V, V));
}
inline FVector4  Normalize3(const FVector4& V)
{
    const float L = Length3(V);
    if (L > 0.0f) return FVector4(V.X / L, V.Y / L, V.Z / L, 0.0f);
    return FVector4(0, 0, 0, 0);
}
// ------------------------------------------------------------
// 절두체(Frustum)/평면 유틸
//  - 평면 식:  dot(N, X) - D = 0
//  - 본 코드는 "절두체 안쪽이 >= 0" 이 되도록 법선을 맞춘다(가시성 테스트에 유리).
//  - 좌표계가 LH여도 외적(Cross)은 RH 정의를 사용한다.
//    => 법선의 "방향(안/밖)"은 외적의 피연산자 "순서"로 결정한다.
// ------------------------------------------------------------

namespace
{
    // 점(Point)과 법선(Normal)로 평면 생성
    //  - 입력 Normal은 정규화되지 않았을 수 있으므로, 반드시 정규화
    //  - Distance = dot(Normal, Point)  (평면 상의 임의의 점과 법선의 내적)
    Plane MakePlane(const FVector4& Point, const FVector4& Normal)
    {
        const FVector4 UnitNormal = Normalize3(Normal); // 길이 1로 보정
        return Plane
        {
            UnitNormal,
            Dot3(UnitNormal, Point)
        };
    }
    // (선택) 모든 평면을 "안쪽을 향하도록" 보정
    //  - 수치 불안정, 축/부호 실수, 미러링(det<0) 등에도 항상 안쪽 ≥ 0을 보장
    /*
    inline void EnsureFacingInwards(Plane& P, const FVector& InsidePoint)
    {
        if (FVector::Dot(P.Normal, InsidePoint) - P.Distance < 0.0f)
        {
            P.Normal *= -1.0f;
            P.Distance *= -1.0f;
        }
    }
    */
}

Frustum CreateFrustumFromCamera(const UCameraComponent& Camera, float OverrideAspect)
{
    // 카메라 파라미터
    const float NearClip = Camera.GetNearClip();
    const float FarClip = Camera.GetFarClip();
    const float Aspect = OverrideAspect > 0.f ? OverrideAspect : Camera.GetAspectRatio();
    const float FovRad = DegreeToRadian(Camera.GetFOV());

    // 카메라 기준 좌표축(월드 공간)
    // Forward=+X, Right=+Y, Up=+Z
    const FVector4 Origin = MakePoint4(Camera.GetWorldLocation());
    const FVector4 Forward = MakeDir4(Camera.GetForward());
    const FVector4 Right = MakeDir4(Camera.GetRight());
    const FVector4 Up = MakeDir4(Camera.GetUp());

    // Far 평면에서의 절반 높이/너비
    const float HalfVSide = FarClip * tanf(FovRad * 0.5f); // 세로(Vertical) 반폭
    const float HalfHSide = HalfVSide * Aspect;            // 가로(Horizontal) 반폭
    const FVector4 FrontMultFar = FVector4(Forward.X * FarClip, Forward.Y * FarClip, Forward.Z * FarClip, 0.0f);        // Far까지의 전방 벡터

    Frustum Result;

    // ------------------------------------------------------------
    // Near / Far
    //  - 평면 법선이 "프러스텀 내부"를 향하도록 잡는다.
    //    Near: +Forward,  Far: -Forward
    // ------------------------------------------------------------
    Result.NearFace = MakePlane(Origin + Forward * NearClip, Forward);
    Result.FarFace = MakePlane(Origin + FrontMultFar, -Forward);


    // ------------------------------------------------------------
    // 측면 평면(Left/Right/Top/Bottom)
    //  - 외적은 RH 정의를 사용한다.
    //  - 피연산자 순서를 적절히 선택해 "내부"를 바라보는 법선을 만든다.
    // 
    //	- 현재 좌표축이 LH(Left-Handed) 기준이므로, 
    //  - Cross 연산 순서 또한 왼손으로 A 먼저, 그 후 B를 감았을 때 나오는 방향이 법선이 된다.
    // 
    //  - Right  : Cross( F*Far + R*HalfH,  U )  → (+X, -Y, 0)
    //  - Left   : Cross( U, F*Far - R*HalfH ) → (+X, +Y, 0)
    //  - Top    : Cross( R, F*Far + U*HalfV ) → (+X, 0, -Z)
    //  - Bottom : Cross( F*Far - U*HalfV,   R )  → (+X, 0, +Z)
    // ------------------------------------------------------------
    Result.RightFace = MakePlane(Origin, Cross3(FrontMultFar + Right * HalfHSide, Up));
    Result.LeftFace = MakePlane(Origin, Cross3(Up, FrontMultFar - Right * HalfHSide));
    Result.TopFace = MakePlane(Origin, Cross3(Right, FrontMultFar + Up * HalfVSide));
    Result.BottomFace = MakePlane(Origin, Cross3(FrontMultFar - Up * HalfVSide, Right));
    return Result;
}

// ------------------------------------------------------------
// AABB vs 프러스텀 판정
//  - 각 평면에 대해: 중심의 부호 + 박스의 "프로젝션 반경"으로 배제 테스트
//  - 규약: 안쪽 ≥ 0  ⇒  Distance + Radius >= 0 이면 그 평면을 통과(겹침)
//  - 하나라도 실패하면(음수) 절두체 밖 → 즉시 탈락
// ------------------------------------------------------------
bool Intersects(const Plane& P, const FVector4& Center, const FVector4& Extents) 
{
	// 평면과 박스사이의 거리 (양수면 평면의 법선 방향, 음수면 반대 방향)
    const float Distance = Dot3(P.Normal, Center) - P.Distance;
    // AABB를 평면 법선 방향으로 투영했을 때의 최대 반경
    const float Radius =
        abs(P.Normal.X) * Extents.X +
        abs(P.Normal.Y) * Extents.Y +
        abs(P.Normal.Z) * Extents.Z;
	//  최대 반경은 항상 양수이므로, Distance + Radius < 0 이면 절두체의 바깥
    return Distance + Radius >= 0.0f;
}

bool IsAABBVisible(const Frustum& Frustum, const FBound& Bound)
{
    // AABB 중심/반길이
    const FVector Center3 = (Bound.Min + Bound.Max) * 0.5f;
    const FVector Extents3 = (Bound.Max - Bound.Min) * 0.5f; // 항상 양수
    const FVector4 Center = MakePoint4(Center3);
    const FVector4 Extents = MakeDir4(Extents3); // 항상 양수
    // 6면 모두 통과해야 절두체의 안쪽이므로 "보이는 것"
    return Intersects(Frustum.LeftFace, Center, Extents) && 
           Intersects(Frustum.RightFace, Center, Extents)  &&
           Intersects(Frustum.TopFace, Center, Extents) &&
           Intersects(Frustum.BottomFace, Center, Extents) &&
           Intersects(Frustum.NearFace, Center, Extents) &&
           Intersects(Frustum.FarFace, Center, Extents);
}



// 추후에 절두체를 VP 행렬에서 바로 추출하는 방법도 필요하다면 아래를 참고.
// ---------- VP(=View*Proj)에서 평면 추출 ----------
// row-vector 규약(p' = p * M)에서 클립 경계는
// Left:   R3 + R0
// Right:  R3 - R0
// Bottom: R3 + R1
// Top:    R3 - R1
// Near:   R3 + R2
// Far:    R3 - R2
// 결합 결과 P=(a,b,c,d)에 대해:  a*x + b*y + c*z + d >= 0  (클립 내부)
// 우리의 평면식 dot(N,X) - D >= 0 과 맞추려면  N=(a,b,c), D=-d  을 사용.
// 정규화 스케일도 Distance에 나눠 반영해야 함.
/*
static inline Plane MakePlaneFromRowCombo(const FVector4& R3, const FVector4& Ri, int Sign) // +1 or -1
{
    // P = R3 + Sign * Ri
    const FVector4 P(R3.X + Sign * Ri.X,
        R3.Y + Sign * Ri.Y,
        R3.Z + Sign * Ri.Z,
        R3.W + Sign * Ri.W);

    // 법선/거리 추출 및 정규화
    Plane Out;
    const FVector4 N(P.X, P.Y, P.Z, 0.0f);
    const float    Len = Length3(N);

    if (Len > 0.0f)
    {
        Out.Normal = FVector4(N.X / Len, N.Y / Len, N.Z / Len, 0.0f);
        // dot(N,X) - D >= 0  와  a*x+b*y+c*z+d >= 0  을 맞추기 위해 D = -d/Len
        Out.Distance = -P.W / Len;
    }
    else
    {
        Out.Normal = FVector4(0, 1, 0, 0);
        Out.Distance = 0.0f;
    }
    return Out;
}

// 카메라에서 VP를 구할 수 있다면 직접 사용.
// 주의: row-vector 규약이면 반드시  VP = View * Proj  (이 순서)
static Frustum CreateFrustumFromVP(const FMatrix& View, const FMatrix& Proj)
{
    const FMatrix VP = View * Proj;

    // 행 접근 함수가 있다고 가정 (없다면 직접 구현)
    const FVector4 R0 = VP.GetRow(0);
    const FVector4 R1 = VP.GetRow(1);
    const FVector4 R2 = VP.GetRow(2);
    const FVector4 R3 = VP.GetRow(3);

    Frustum F;
    F.LeftFace = MakePlaneFromRowCombo(R3, R0, +1);
    F.RightFace = MakePlaneFromRowCombo(R3, R0, -1);
    F.BottomFace = MakePlaneFromRowCombo(R3, R1, +1);
    F.TopFace = MakePlaneFromRowCombo(R3, R1, -1);
    F.NearFace = MakePlaneFromRowCombo(R3, R2, +1);
    F.FarFace = MakePlaneFromRowCombo(R3, R2, -1);
    return F;
}

Frustum CreateFrustumFromCamera(const UCameraComponent& Camera, float)  // OverrideAspect
{
    // 카메라에서 뷰/프로젝션 행렬을 가져오는 API는 엔진에 맞춰 바꿔줘.
    // row-vector 규약: p' = p * View * Proj
    const FMatrix View = Camera.GetViewMatrix();         // 너의 엔진 API
    const FMatrix Proj = Camera.GetProjectionMatrix();   // 너의 엔진 API
    return CreateFrustumFromVP(View, Proj);
}

// ---------- AABB vs 평면 판정(기존 유지, std::abs 권장) ----------
bool Intersects(const Plane& P, const FVector4& Center, const FVector4& Extents)
{
    const float Distance = Dot3(P.Normal, Center) - P.Distance;

    // AABB의 법선 방향 투영 반경
    const float Radius =
        std::abs(P.Normal.X) * Extents.X +
        std::abs(P.Normal.Y) * Extents.Y +
        std::abs(P.Normal.Z) * Extents.Z;

    return Distance + Radius >= 0.0f;
}

bool IsAABBVisible(const Frustum& F, const FBound& B)
{
    const FVector  Center3 = (B.Min + B.Max) * 0.5f;
    const FVector  Extents3 = (B.Max - B.Min) * 0.5f; // 양수

    const FVector4 Center = FVector4(Center3.X, Center3.Y, Center3.Z, 1.0f); // 점
    const FVector4 Extents = FVector4(Extents3.X, Extents3.Y, Extents3.Z, 0.0f); // 방향

    return Intersects(F.LeftFace, Center, Extents) &&
        Intersects(F.RightFace, Center, Extents) &&
        Intersects(F.TopFace, Center, Extents) &&
        Intersects(F.BottomFace, Center, Extents) &&
        Intersects(F.NearFace, Center, Extents) &&
        Intersects(F.FarFace, Center, Extents);
}
*/