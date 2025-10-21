#pragma once
#include "pch.h"

// Per-world render settings (view mode + show flags)
class URenderSettings {
public:
    URenderSettings() = default;
    ~URenderSettings() = default;

    // View mode
    void SetViewModeIndex(EViewModeIndex In) { ViewModeIndex = In; }
    EViewModeIndex GetViewModeIndex() const { return ViewModeIndex; }

    // Show flags
    EEngineShowFlags GetShowFlags() const { return ShowFlags; }
    void SetShowFlags(EEngineShowFlags In) { ShowFlags = In; }
    void EnableShowFlag(EEngineShowFlags Flag) { ShowFlags |= Flag; }
    void DisableShowFlag(EEngineShowFlags Flag) { ShowFlags &= ~Flag; }
    void ToggleShowFlag(EEngineShowFlags Flag) { ShowFlags = HasShowFlag(ShowFlags, Flag) ? (ShowFlags & ~Flag) : (ShowFlags | Flag); }
    bool IsShowFlagEnabled(EEngineShowFlags Flag) const { return HasShowFlag(ShowFlags, Flag); }

    // FXAA parameters
    void SetFXAAEdgeThresholdMin(float Value) { FXAAEdgeThresholdMin = Value; }
    float GetFXAAEdgeThresholdMin() const { return FXAAEdgeThresholdMin; }

    void SetFXAAEdgeThresholdMax(float Value) { FXAAEdgeThresholdMax = Value; }
    float GetFXAAEdgeThresholdMax() const { return FXAAEdgeThresholdMax; }

    void SetFXAAQualitySubPix(float Value) { FXAAQualitySubPix = Value; }
    float GetFXAAQualitySubPix() const { return FXAAQualitySubPix; }

    void SetFXAAQualityIterations(int32 Value) { FXAAQualityIterations = Value; }
    int32 GetFXAAQualityIterations() const { return FXAAQualityIterations; }

    // Tile-based light culling
    void SetTileSize(uint32 Value) { TileSize = Value; }
    uint32 GetTileSize() const { return TileSize; }

private:
    EEngineShowFlags ShowFlags = EEngineShowFlags::SF_DefaultEnabled;
    EViewModeIndex ViewModeIndex = EViewModeIndex::VMI_Lit_Phong;

    // FXAA parameters
    float FXAAEdgeThresholdMin = 0.0833f;   // 엣지 감지 최소 휘도 차이 (권장: 0.0833)
    float FXAAEdgeThresholdMax = 0.166f;    // 엣지 감지 최대 휘도 차이 (권장: 0.166)
    float FXAAQualitySubPix = 0.75f;        // 서브픽셀 품질 (낮을수록 부드러움, 권장: 0.75)
    int32 FXAAQualityIterations = 12;       // 엣지 탐색 반복 횟수 (권장: 12)

    // Tile-based light culling
    uint32 TileSize = 16;                   // 타일 크기 (픽셀, 기본값: 16)
};
