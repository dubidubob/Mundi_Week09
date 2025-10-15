﻿#pragma once
#include "Object.h"

#include "SceneComponent.h"

class FLinearColor;

class UHeightFogComponent : public USceneComponent
{
public:
    DECLARE_CLASS(UHeightFogComponent, USceneComponent)
    
    UHeightFogComponent();
    ~UHeightFogComponent() override;
    
    // Component Lifecycle
    /*void InitializeComponent() override;
    void TickComponent(float DeltaTime) override;*/
    
    // Fog Parameters Getters
    float GetFogDensity() const { return FogDensity; }
    float GetFogHeightFalloff() const { return FogHeightFalloff; }
    float GetStartDistance() const { return StartDistance; }
    float GetFogCutoffDistance() const { return FogCutoffDistance; }
    float GetFogMaxOpacity() const { return FogMaxOpacity; }
    FLinearColor* GetFogInscatteringColor() const { return FogInscatteringColor; }
    
    // Fog Parameters Setters
    void SetFogDensity(float InDensity) { FogDensity = InDensity; }
    void SetFogHeightFalloff(float InFalloff) { FogHeightFalloff = InFalloff; }
    void SetStartDistance(float InDistance) { StartDistance = InDistance; }
    void SetFogCutoffDistance(float InDistance) { FogCutoffDistance = InDistance; }
    void SetFogMaxOpacity(float InOpacity) { FogMaxOpacity = InOpacity; }
    void SetFogInscatteringColor(FLinearColor* InColor) { FogInscatteringColor = InColor; }
    
    // Rendering
    void RenderHeightFog(URenderer* Renderer);

	// Serialize
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// ───── 복사 관련 ────────────────────────────
	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(UHeightFogComponent)
    
private:
    float FogDensity = 0.02f;
    float FogHeightFalloff = 0.2f;
    float StartDistance = 0.0f;
    float FogCutoffDistance = 1000.0f;
    float FogMaxOpacity = 1.0f;

    FLinearColor* FogInscatteringColor = nullptr;

    // Full Screen Quad Resources
    class UStaticMesh* FullScreenQuadMesh = nullptr;
    class UShader* HeightFogShader = nullptr;
};
