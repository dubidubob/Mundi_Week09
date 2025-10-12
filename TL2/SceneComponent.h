﻿#pragma once
#include "Vector.h"
#include "ActorComponent.h"



// 부착 시 로컬을 유지할지, 월드를 유지할지
enum class EAttachmentRule
{
    KeepRelative,
    KeepWorld
};

class URenderer;
class USceneComponent : public UActorComponent
{
public:
    DECLARE_CLASS(USceneComponent, UActorComponent)
    USceneComponent();

protected:
    ~USceneComponent() override;


public:
    // ──────────────────────────────
    // Relative Transform API
    // ──────────────────────────────
    void SetRelativeLocation(const FVector& NewLocation);
    FVector GetRelativeLocation() const;

    void SetRelativeRotation(const FQuat& NewRotation);
    FQuat GetRelativeRotation() const;

    void SetRelativeScale(const FVector& NewScale);
    FVector GetRelativeScale() const;

    void AddRelativeLocation(const FVector& DeltaLocation);
    void AddRelativeRotation(const FQuat& DeltaRotation);
    void AddRelativeScale3D(const FVector& DeltaScale);

    // ──────────────────────────────
    // World Transform API
    // ──────────────────────────────
    FTransform GetWorldTransform() const;
    void SetWorldTransform(const FTransform& W);

    void SetWorldLocation(const FVector& L);
    FVector GetWorldLocation() const;

    void SetWorldRotation(const FQuat& R);
    FQuat GetWorldRotation() const;

    void SetWorldScale(const FVector& S);
    FVector GetWorldScale() const;

    void AddWorldOffset(const FVector& Delta);
    void AddWorldRotation(const FQuat& DeltaRot);
    void SetWorldLocationAndRotation(const FVector& L, const FQuat& R);

    void AddLocalOffset(const FVector& Delta);
    void AddLocalRotation(const FQuat& DeltaRot);
    void SetLocalLocationAndRotation(const FVector& L, const FQuat& R);

    FMatrix GetWorldMatrix() const; // ToMatrixWithScale

    // ──────────────────────────────
    // Attach/Detach
    // ──────────────────────────────
    void SetupAttachment(USceneComponent* InParent, EAttachmentRule Rule = EAttachmentRule::KeepWorld);
    void DetachFromParent(bool bKeepWorld = true);

    // ──────────────────────────────
    // Hierarchy Access
    // ──────────────────────────────
    USceneComponent* GetAttachParent() const { return AttachParent; }
    const TArray<USceneComponent*>& GetAttachChildren() const { return AttachChildren; }
    UWorld* GetWorld();

    // ───── 복사 관련 ────────────────────────────
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(USceneComponent)

    // DuplicateSubObjects에서 쓰기 위함
    void SetParent(USceneComponent* InParent)
    {
        AttachParent = InParent;
    }

    // Serialize
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    /**
     * @brief 로컬 트랜스폼 변경시 생기는 영향을 처리하기 위한 메소드
     * @note 이 메소드는 모든 SceneComponent 공통 로직을 처리.
     * Derived class별 특수 로직은 OnTransformUpdatedChildImpl()를 이용.
     */
    void OnTransformUpdated();

    // SceneId
    uint32 GetSceneId() const { return SceneId; }
    void SetSceneId(uint32 InId) { SceneId = InId; }
    uint32 GetParentId() const { return ParentId; }
    void SetParentId(uint32 InParentId) { ParentId = InParentId; }

    
    static TMap<uint32, USceneComponent*>& GetSceneIdMap()
    {
        return SceneIdMap;
    }
protected:
    /** @brief OnTransformUpdated() 내부에서 클래스 별 특수 로직을 처리하기 위한 가상함수 */
    virtual void OnTransformUpdatedChildImpl();
    
    /**
     * @brief Transform 갱신시 자식 컴포넌트의 OnTransformUpdated를 강제 호출.
     * @note 부모 컴포넌트의 트랜스폼 변화로 인해 월드 관점에서 생길 영향을 처리하기 위한 메소드로,
     * 자식 컴포넌트들의 로컬 트랜스폼을 직접 변경시키지 않습니다. 
     */
    void PropagateTransformUpdate();

    FVector RelativeLocation{ 0,0,0 };
    FQuat   RelativeRotation;
    FVector RelativeScale{ 1,1,1 };

    
    // Hierarchy
    USceneComponent* AttachParent = nullptr;
    TArray<USceneComponent*> AttachChildren;

    // 로컬(부모 기준) 트랜스폼
    FTransform RelativeTransform;

    void UpdateRelativeTransform();
    
    uint32 SceneId; // Scene파일에서 불러온 Id. 컴포넌트끼리 자식부모관계 연결하기 위해 저장. Scene에 저장할 때는 UUID를 저장
    uint32 ParentId;
    static TMap<uint32, USceneComponent*> SceneIdMap; // 부모를 찾기 위한 Map
};
