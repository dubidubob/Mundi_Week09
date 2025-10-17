﻿#pragma once
// b0 in VS    
#include "Color.h"
#include "LightInfo.h"

struct ModelBufferType
{
    FMatrix Model;
};

struct DecalBufferType
{
    FMatrix DecalMatrix;
    float Opacity;
};

struct PostProcessBufferType // b0
{
    float Near;
    float Far;
    int IsOrthographic; // 0 = Perspective, 1 = Orthographic
    float Padding; // 16바이트 정렬을 위한 패딩
};

static_assert(sizeof(PostProcessBufferType) % 16 == 0, "PostProcessBufferType size must be multiple of 16!");

struct InvViewProjBufferType // b1
{
    FMatrix InvView;
    FMatrix InvProj;
};

static_assert(sizeof(InvViewProjBufferType) % 16 == 0, "InvViewProjBufferType size must be multiple of 16!");

struct FogBufferType // b2
{
    float FogDensity;
    float FogHeightFalloff;
    float StartDistance;
    float FogCutoffDistance;

    FVector4 FogInscatteringColor; // 16 bytes alignment 위해 중간에 넣음

    float FogMaxOpacity;
    float FogHeight; // fog base height
    float Padding[2]; // 16바이트 정렬을 위한 패딩
};
static_assert(sizeof(FogBufferType) % 16 == 0, "FogBufferType size must be multiple of 16!");


struct FXAABufferType // b2
{
    FVector2D ScreenSize; // 화면 해상도 (e.g., float2(1920.0f, 1080.0f))
    FVector2D InvScreenSize; // 1.0f / ScreenSize (픽셀 하나의 크기)

    float EdgeThresholdMin; // 엣지 감지 최소 휘도 차이 (0.0833f 권장)
    float EdgeThresholdMax; // 엣지 감지 최대 휘도 차이 (0.166f 권장)
    float QualitySubPix; // 서브픽셀 품질 (낮을수록 부드러움, 0.75 권장)
    int32_t QualityIterations; // 엣지 탐색 반복 횟수 (12 권장)
};
static_assert(sizeof(FXAABufferType) % 16 == 0, "FXAABufferType size must be multiple of 16!");


// b0 in PS
struct FMaterialInPs
{
    FVector DiffuseColor; // Kd
    float OpticalDensity; // Ni

    FVector AmbientColor; // Ka
    float Transparency; // Tr Or d

    FVector SpecularColor; // Ks
    float SpecularExponent; // Ns

    FVector EmissiveColor; // Ke
    uint32 IlluminationModel; // illum. Default illumination model to Phong for non-Pbr materials

    FVector TransmissionFilter; // Tf
    float dummy; // 4 bytes padding
    FMaterialInPs() = default;
    FMaterialInPs(const FObjMaterialInfo& MaterialInfo)
        :DiffuseColor(MaterialInfo.DiffuseColor),
        OpticalDensity(MaterialInfo.OpticalDensity),
        AmbientColor(MaterialInfo.AmbientColor),
        Transparency(MaterialInfo.Transparency),
        SpecularColor(MaterialInfo.SpecularColor),
        SpecularExponent(MaterialInfo.SpecularExponent),
        EmissiveColor(MaterialInfo.EmissiveColor),
        IlluminationModel(MaterialInfo.IlluminationModel),
        TransmissionFilter(MaterialInfo.TransmissionFilter),
        dummy(0)
    { 

    }
};


struct FPixelConstBufferType
{
    FMaterialInPs Material;
    bool bHasMaterial; // 1 bytes
    bool Dummy[3]; // 3 bytes padding
    bool bHasTexture; // 1 bytes
    bool Dummy2[11]; // 11 bytes padding
};


static_assert(sizeof(FPixelConstBufferType) % 16 == 0, "PixelConstData size mismatch!");

// b1
struct ViewProjBufferType
{
    FMatrix View;
    FMatrix Proj;
};


// b2
struct HighLightBufferType
{
    uint32 Picked;
    FVector Color;
    uint32 X;
    uint32 Y;
    uint32 Z;
    uint32 Gizmo;
};


struct ColorBufferType
{
    FVector4 Color;
    uint32 UUID;
    FVector Padding;
};


struct BillboardBufferType
{
    FVector pos;
    FMatrix View;
    FMatrix Proj;
    FMatrix InverseViewMat;
    /*FVector cameraRight;
    FVector cameraUp;*/
};

struct FireBallBufferType
{
    FVector Center;
    float Radius;
    float Intensity;
    float Falloff;
    float Padding[2];
    FLinearColor Color;
};

struct FLightBufferType
{
    FAmbientLightInfo AmbientLight;
    FDirectionalLightInfo DirectionalLight;
    FPointLightInfo PointLights[NUM_POINT_LIGHT_MAX];
    FSpotLightInfo SpotLights[NUM_SPOT_LIGHT_MAX];

    uint32 PointLightCount;
    uint32 SpotLightCount;
    FVector2D Padding;
};

#define CONSTANT_BUFFER_INFO(TYPE, SLOT, VS, PS) \
constexpr uint32 TYPE##Slot = SLOT;\
constexpr bool TYPE##IsVS = VS;\
constexpr bool TYPE##IsPS = PS;

//매크로를 인자로 받고 그 매크로 함수에 버퍼 전달
#define CONSTANT_BUFFER_LIST(MACRO) \
MACRO(ModelBufferType)              \
MACRO(DecalBufferType)              \
MACRO(PostProcessBufferType)        \
MACRO(InvViewProjBufferType)        \
MACRO(FogBufferType)                \
MACRO(FXAABufferType)               \
MACRO(FPixelConstBufferType)        \
MACRO(ViewProjBufferType)           \
MACRO(HighLightBufferType)          \
MACRO(ColorBufferType)              \
MACRO(BillboardBufferType)          \
MACRO(FireBallBufferType)           \
MACRO(FLightBufferType)

//VS, PS 세팅은 함수 파라미터로 결정하게 하는게 훨씬 나을듯 나중에 수정 필요
//그리고 UV Scroll 상수버퍼도 처리해줘야함
CONSTANT_BUFFER_INFO(ModelBufferType, 0, true, false)
CONSTANT_BUFFER_INFO(DecalBufferType, 6, true, true)
CONSTANT_BUFFER_INFO(PostProcessBufferType, 0, false, true)
CONSTANT_BUFFER_INFO(InvViewProjBufferType, 1, false, true)
CONSTANT_BUFFER_INFO(FogBufferType, 2, false, true)
CONSTANT_BUFFER_INFO(FXAABufferType, 2, false, true)
CONSTANT_BUFFER_INFO(FPixelConstBufferType, 4, false, true)
CONSTANT_BUFFER_INFO(ViewProjBufferType, 1, true, false)
CONSTANT_BUFFER_INFO(HighLightBufferType, 2, true, false)
CONSTANT_BUFFER_INFO(ColorBufferType, 3, false, true)
CONSTANT_BUFFER_INFO(BillboardBufferType, 0, true, false)
CONSTANT_BUFFER_INFO(FireBallBufferType, 7, false, true)
CONSTANT_BUFFER_INFO(FLightBufferType, 8, true, true)

