﻿#pragma once
#include "SWindow.h"
#include "Enums.h"

class FViewport;
class FViewportClient;
class UTexture;

class SViewportWindow : public SWindow
{
public:
    SViewportWindow();
    virtual ~SViewportWindow();

    bool Initialize(float StartX, float StartY, float Width, float Height, UWorld* World, ID3D11Device* Device, EViewportType InViewportType);

    virtual void OnRender() override;
    virtual void OnUpdate(float DeltaSeconds) override;

    virtual void OnMouseMove(FVector2D MousePos) override;
    virtual void OnMouseDown(FVector2D MousePos, uint32 Button) override;
    virtual void OnMouseUp(FVector2D MousePos, uint32 Button) override;

    void SetActive(bool bInActive) { bIsActive = bInActive; }
    bool IsActive() const { return bIsActive; }

    FViewport* GetViewport() const { return Viewport; }
    FViewportClient* GetViewportClient() const { return ViewportClient; }
    void SetVClientWorld(UWorld* InWorld);
private:
    void RenderToolbar();
    void RenderGizmoModeButtons();
    void RenderGizmoSpaceButton();
    void LoadToolbarIcons(ID3D11Device* Device);

private:
    FViewport* Viewport = nullptr;
    FViewportClient* ViewportClient = nullptr;

    EViewportType ViewportType;
    FName ViewportName;

    bool bIsActive;
    bool bIsMouseDown;

    // ViewMode 관련 상태 저장
    int CurrentLitSubMode = 0; // 0=default(Phong) 1=Gouraud, 2=Lambert, 3=Phong [기본값: default(Phong)]
    int CurrentBufferVisSubMode = 1; // 0=SceneDepth, 1=WorldNormal (기본값: WorldNormal)

    // 툴바 아이콘 텍스처
    UTexture* IconSelect = nullptr;
    UTexture* IconMove = nullptr;
    UTexture* IconRotate = nullptr;
    UTexture* IconScale = nullptr;
    UTexture* IconWorldSpace = nullptr;
    UTexture* IconLocalSpace = nullptr;
};