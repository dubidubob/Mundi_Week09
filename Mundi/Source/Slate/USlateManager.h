﻿#pragma once
#include "Object.h"
#include "Windows/SWindow.h" // for FRect and SWindow types used by children
#include "Windows/SSplitterV.h"
#include "Windows/SSplitterH.h"
#include "Windows/SViewportWindow.h"

class SSceneIOWindow; // 새로 추가할 UI
class SDetailsWindow;
class UMenuBarWidget;
class UConsoleWindow; // Overlay console window

// 중앙 레이아웃/입력 라우팅/뷰포트 관리 매니저 (위젯 아님)
class USlateManager : public UObject
{
public:
    DECLARE_CLASS(USlateManager, UObject)

    // Singleton accessor
    static USlateManager& GetInstance();

    // 구성 저장/로드
    void SaveSplitterConfig();
    void LoadSplitterConfig();

    USlateManager();
    virtual ~USlateManager() override;

    USlateManager(const USlateManager&) = delete;
    USlateManager& operator=(const USlateManager&) = delete;

    void Initialize(ID3D11Device* Device, UWorld* World, const FRect& InRect);
    void SwitchLayout(EViewportLayoutMode NewMode);
    EViewportLayoutMode GetCurrentLayoutMode() const { return CurrentMode; }

    void SwitchPanel(SWindow* SwitchPanel);

    // 렌더/업데이트/입력 전달
    void Render();
    void Update(float deltaSecond);
    void ProcessInput();

    void OnMouseMove(FVector2D MousePos);
    void OnMouseDown(FVector2D MousePos, uint32 Button);
    void OnMouseUp(FVector2D MousePos, uint32 Button);

    void OnShutdown();
    void Shutdown();

    // 상태
    static SViewportWindow* ActiveViewport; // 현재 드래그 중인 뷰포트

    // 매니저 자체 위치/크기 (상위 윈도우 크기 기준)
    void SetRect(const FRect& InRect) { Rect = InRect; }
    const FRect& GetRect() const { return Rect; }

    void SetWorld(UWorld* InWorld)
    {
        World = InWorld;
    }

    void SetPIEWorld(UWorld* InWorld);

    // Console management
    void ToggleConsole();
    bool IsConsoleVisible() const { return bIsConsoleVisible; }

private:
    FRect Rect; // 이전엔 SWindow로부터 상속받던 영역 정보

    UWorld* World = nullptr;
    ID3D11Device* Device = nullptr;

    // 두 가지 레이아웃을 미리 생성해둠
    SSplitter* FourSplitLayout = nullptr;
    SSplitter* SingleLayout = nullptr;

    // 뷰포트
    SViewportWindow* Viewports[4];
    SViewportWindow* MainViewport;

    SSplitterV* LeftTop;
    SSplitterV* LeftBottom;

    // UI 패널들
    SWindow* ControlPanel = nullptr;
    SWindow* DetailPanel = nullptr;

    // 레이아웃 구조: TopPanel(좌우) -> Left: LeftPanel(뷰포트), Right: RightPanel(상하)
    SSplitterH* TopPanel = nullptr;       // 전체 화면 좌우 분할
    SSplitterH* LeftPanel = nullptr;      // 왼쪽 뷰포트 영역 (좌우 4분할)
    SSplitterV* RightPanel = nullptr;     // 오른쪽 UI 영역 (상하: Control + Details)

    // 현재 모드
    EViewportLayoutMode CurrentMode = EViewportLayoutMode::FourSplit;

    // 메뉴바 관련
    UMenuBarWidget* MenuBar;

    // Console overlay
    UConsoleWindow* ConsoleWindow = nullptr;
    bool bIsConsoleVisible = false;
    bool bIsConsoleAnimating = false;
    float ConsoleAnimationProgress = 0.0f; // 0.0 = hidden, 1.0 = fully visible
    const float ConsoleAnimationDuration = 0.25f; // seconds
    const float ConsoleHeightRatio = 0.3f; // 30% of screen height
};
