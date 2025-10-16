﻿#pragma once
#include "ResourceBase.h"
#include "Enums.h"
#include "MeshBVH.h"
#include <d3d11.h>

class FMeshBVH;
class UStaticMesh : public UResourceBase
{
public:
    DECLARE_CLASS(UStaticMesh, UResourceBase)

    UStaticMesh() = default;
    virtual ~UStaticMesh() override;

    void Load(const FString& InFilePath, ID3D11Device* InDevice, EVertexLayoutType InVertexType = EVertexLayoutType::PositionColorTexturNormal);
    void Load(FMeshData* InData, ID3D11Device* InDevice, EVertexLayoutType InVertexType = EVertexLayoutType::PositionColorTexturNormal);

    ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }
    ID3D11Buffer* GetIndexBuffer() const { return IndexBuffer; }
    uint32 GetVertexCount() const { return VertexCount; }
    uint32 GetIndexCount() const { return IndexCount; }
    EVertexLayoutType GetVertexType() const { return VertexType; }
    void SetIndexCount(uint32 Cnt) { IndexCount = Cnt; }

	const FString& GetAssetPathFileName() const { return StaticMeshAsset ? StaticMeshAsset->PathFileName : FilePath; }
    void SetStaticMeshAsset(FStaticMesh* InStaticMesh) { StaticMeshAsset = InStaticMesh; }
	FStaticMesh* GetStaticMeshAsset() const { return StaticMeshAsset; }

    const TArray<FGroupInfo>& GetMeshGroupInfo() const { return StaticMeshAsset->GroupInfos; }
    bool HasMaterial() const { return StaticMeshAsset->bHasMaterial; }

    uint64 GetMeshGroupCount() const { return StaticMeshAsset->GroupInfos.size(); }
    
    FAABB GetLocalBound() const {return LocalBound; }
    
    // BVH GETTER 
    // const FMeshBVH* GetBVH() const { return MeshBVH; }

    bool EraseUsingComponets(UStaticMeshComponent* InStaticMeshComponent);
    bool AddUsingComponents(UStaticMeshComponent* InStaticMeshComponent);

    /*const TArray<UStaticMeshComponent*>& GetUsingComponents() const
    {
        return UsingComponents;
    }*/

    TArray<UStaticMeshComponent*>& GetUsingComponents()
    {
        return UsingComponents;
    }

private:
    void CreateVertexBuffer(FMeshData* InMeshData, ID3D11Device* InDevice, EVertexLayoutType InVertexType);
	void CreateVertexBuffer(FStaticMesh* InStaticMesh, ID3D11Device* InDevice, EVertexLayoutType InVertexType);
    void CreateIndexBuffer(FMeshData* InMeshData, ID3D11Device* InDevice);
	void CreateIndexBuffer(FStaticMesh* InStaticMesh, ID3D11Device* InDevice);
    void CreateLocalBound(const FMeshData* InMeshData);
    void CreateLocalBound(const FStaticMesh* InStaticMesh);
    void ReleaseResources();

    // GPU 리소스
    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
    uint32 VertexCount = 0;     // 정점 개수
    uint32 IndexCount = 0;     // 버텍스 점의 개수 
    EVertexLayoutType VertexType = EVertexLayoutType::PositionColorTexturNormal;  // 버텍스 타입

	// CPU 리소스
    FStaticMesh* StaticMeshAsset = nullptr;

    // 메시 단위 BVH (ResourceManager에서 캐싱, 소유)
    // 초기화되지 않는 멤버변수 (참조도 ResourceManager에서만 이루어짐) 
    // FMeshBVH* MeshBVH = nullptr;

    // 로컬 AABB. (스태틱메시 액터 전체 경계 계산에 사용. StaticMeshAsset 로드할 때마다 갱신)
    FAABB LocalBound;
    
    TArray<UStaticMeshComponent*> UsingComponents; // 유저에 의해 Material이 안 바뀐 이 Mesh를 사용 중인 Component들(render state sorting 위함)
};
