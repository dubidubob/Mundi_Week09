﻿#pragma once
#include "Object.h"
#include "Enums.h"
#include "RenderSettings.h"

// Forward Declarations
class UResourceManager;
class UUIManager;
class UInputManager;
class USelectionManager;
class AActor;
class URenderer;
class ACameraActor;
class AGizmoActor;
class AGridActor;
class FViewport;
class USlateManager;
class URenderManager;
struct FTransform;
struct FPrimitiveData;
class SViewportWindow;
class UWorldPartitionManager;
class AStaticMeshActor;
class BVHierachy;
class UStaticMesh;
class FOcclusionCullingManagerCPU;
class Frustum;
struct FCandidateDrawable;

class UWorld final : public UObject
{
public:
    DECLARE_CLASS(UWorld, UObject)
    UWorld();
    ~UWorld() override;
    static UWorld& GetInstance();

public:
    /** 초기화 */
    void Initialize();
    void InitializeGrid();
    void InitializeGizmo();

    template<class T>
    T* SpawnActor();

    template<class T>
    T* SpawnActor(const FTransform& Transform);

    bool DestroyActor(AActor* Actor);

    // Partial hooks
    void OnActorSpawned(AActor* Actor);
    void OnActorDestroyed(AActor* Actor);

    void CreateNewScene();
    void LoadScene(const FString& SceneName);
    void SaveScene(const FString& SceneName);
    ACameraActor* GetCameraActor() { return MainCameraActor; }
    void SetCameraActor(ACameraActor* InCamera);


    /** Generate unique name for actor based on type */
    FString GenerateUniqueActorName(const FString& ActorType);

    /** === 타임 / 틱 === */
    virtual void Tick(float DeltaSeconds);

    /** === 필요한 엑터 게터 === */
    const TArray<AActor*>& GetActors() { return Actors; }
    const TArray<AActor*>& GetEditorActors() { return EditorActors; }
    AGizmoActor* GetGizmoActor();
    AGridActor* GetGridActor() { return GridActor; }
    UWorldPartitionManager* GetPartitionManager() { return Partition.get(); }

    // Per-world render settings
    URenderSettings& GetRenderSettings() { return RenderSettings; }
    const URenderSettings& GetRenderSettings() const { return RenderSettings; }

    void SetStaticMeshs();
    const TArray<UStaticMesh*>& GetStaticMeshs() { return StaticMeshs; }
    
    /** === 레벨 / 월드 구성 === */
    // TArray<ULevel*> Levels;
private:
    /** === 액터 관리 === */
    TArray<AActor*> EditorActors;
    ACameraActor* MainCameraActor = nullptr;
    AGridActor* GridActor = nullptr;
    AGizmoActor* GizmoActor = nullptr;

    /** === 액터 관리 === */
    TArray<AActor*> Actors;

    /** A dedicated array for static mesh actors to optimize culling. */
    TArray<UStaticMesh*> StaticMeshs;

    // Object naming system
    TMap<FString, int32> ObjectTypeCounts;

    // Per-world render settings
    URenderSettings RenderSettings;

    //partition
    std::unique_ptr<UWorldPartitionManager> Partition = nullptr;
};

template<class T>
inline T* UWorld::SpawnActor()
{
    return SpawnActor<T>(FTransform());
}

template<class T>
inline T* UWorld::SpawnActor(const FTransform& Transform)
{
    static_assert(std::is_base_of<AActor, T>::value, "T must be derived from AActor");

    // 새 액터 생성
    T* NewActor = NewObject<T>();

    // 초기 트랜스폼 적용
    NewActor->SetActorTransform(Transform);

    //  월드 등록
    NewActor->SetWorld(this);

    // 월드에 등록
    Actors.Add(NewActor);

    return NewActor;
}
