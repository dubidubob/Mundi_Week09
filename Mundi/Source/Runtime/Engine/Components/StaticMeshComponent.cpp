﻿#include "pch.h"
#include "StaticMeshComponent.h"
#include "StaticMesh.h"
#include "Shader.h"
#include "Texture.h"
#include "ResourceManager.h"
#include "ObjManager.h"
#include "World.h"
#include "WorldPartitionManager.h"
#include "JsonSerializer.h"
#include "CameraActor.h"
#include "CameraComponent.h"

IMPLEMENT_CLASS(UStaticMeshComponent)

BEGIN_PROPERTIES(UStaticMeshComponent)
	MARK_AS_COMPONENT("스태틱 메시 컴포넌트", "스태틱 메시를 렌더링하는 컴포넌트입니다.")
END_PROPERTIES()

UStaticMeshComponent::UStaticMeshComponent()
{
	/*SetMaterial("Shaders/Materials/UberLit.hlsl");*/

	SetStaticMesh("Data/cube-tex.obj");     // 임시 기본 static mesh 설정
	SetLightingModel(ELightingModel::Phong); // 기본 조명 모델 설정 (Phong)
}

UStaticMeshComponent::~UStaticMeshComponent()
{
	if (StaticMesh != nullptr)
	{
		StaticMesh->EraseUsingComponets(this);
	}
}

void UStaticMeshComponent::SetLightingModel(ELightingModel InModel)
{
    // Material이 이미 생성되어 있고 모델이 같다면 반환 (최적화)
    if (Material && LightingModel == InModel)
        return;

    LightingModel = InModel;

    // 조명 모델에 따라 적절한 셰이더로 전환
    FString ShaderPath = "Shaders/Materials/UberLit.hlsl";
    
    TArray<FShaderMacro> Macros;
    
    switch (LightingModel)
    {
        case ELightingModel::Gouraud:
            Macros.push_back(FShaderMacro{ "LIGHTING_MODEL_GOURAUD", "1" });
            break;
            
        case ELightingModel::Lambert:
            Macros.push_back(FShaderMacro{ "LIGHTING_MODEL_LAMBERT", "1" });
            break;
            
        case ELightingModel::Phong:
            Macros.push_back(FShaderMacro{ "LIGHTING_MODEL_PHONG", "1" });
            break;
            
        case ELightingModel::None:
        default:
            // 매크로 없이 기본 동작 (조명 없음)
            break;
    }

    // UResourceManager를 통해 매크로가 적용된 셰이더 로드
    UShader* NewShader = UResourceManager::GetInstance().Load<UShader>(ShaderPath, Macros);
    
    if (NewShader)
    {
        // Material이 없으면 생성
        if (!Material)
        {
            Material = UResourceManager::GetInstance().GetOrCreateMaterial(
                ShaderPath + "_Material", 
                EVertexLayoutType::PositionColorTexturNormal
            );
        }
        
        // 셰이더 교체
        Material->SetShader(NewShader);
        
        UE_LOG("Lighting model changed to %s", 
            LightingModel == ELightingModel::Gouraud ? "Gouraud" :
            LightingModel == ELightingModel::Lambert ? "Lambert" :
            LightingModel == ELightingModel::Phong ? "Phong" : "None");
    }
    else
    {
        UE_LOG("Failed to load shader for lighting model");
    }
}

void UStaticMeshComponent::Render(URenderer* Renderer, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix)
{
	UStaticMesh* Mesh = GetStaticMesh();
	if (Mesh && Mesh->GetStaticMeshAsset())
	{
		FMatrix WorldMatrix = GetWorldMatrix();
		FMatrix WorldInverseTranspose = WorldMatrix.InverseAffine().Transpose();
		Renderer->GetRHIDevice()->SetAndUpdateConstantBuffer(ModelBufferType(WorldMatrix, WorldInverseTranspose));
		Renderer->GetRHIDevice()->SetAndUpdateConstantBuffer(ViewProjBufferType(ViewMatrix, ProjectionMatrix));
		Renderer->GetRHIDevice()->SetAndUpdateConstantBuffer(ColorBufferType(FVector4(), this->InternalIndex));
		// b7: CameraBuffer - Renderer에서 카메라 위치 가져오기
		FVector CameraPos = FVector::Zero();
		if (ACameraActor* Camera = Renderer->GetCurrentCamera())
		{
			if (UCameraComponent* CamComp = Camera->GetCameraComponent())
			{
				CameraPos = CamComp->GetWorldLocation();
			}
		}
		Renderer->GetRHIDevice()->SetAndUpdateConstantBuffer(CameraBufferType(CameraPos, 0.0f));

		Renderer->GetRHIDevice()->PrepareShader(GetMaterial()->GetShader());
		Renderer->DrawIndexedPrimitiveComponent(GetStaticMesh(), D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST, MaterialSlots);
	}
}

void UStaticMeshComponent::SetStaticMesh(const FString& PathFileName)
{
	if (StaticMesh != nullptr)
	{
		StaticMesh->EraseUsingComponets(this);
	}

	StaticMesh = FObjManager::LoadObjStaticMesh(PathFileName);
	if (StaticMesh && StaticMesh->GetStaticMeshAsset())
	{
		StaticMesh->AddUsingComponents(this);

		const TArray<FGroupInfo>& GroupInfos = StaticMesh->GetMeshGroupInfo();
		if (MaterialSlots.size() < GroupInfos.size())
		{
			MaterialSlots.resize(GroupInfos.size());
		}

		// MaterailSlots.size()가 GroupInfos.size() 보다 클 수 있기 때문에, GroupInfos.size()로 설정
		for (int i = 0; i < GroupInfos.size(); ++i)
		{
			if (MaterialSlots[i].bChangedByUser == false)
			{
				MaterialSlots[i].MaterialName = GroupInfos[i].InitialMaterialName;
			}
		}
		MarkWorldPartitionDirty();
	}
}

void UStaticMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FString ObjPath;
		FJsonSerializer::ReadString(InOutHandle, "ObjStaticMeshAsset", ObjPath);
		if (!ObjPath.empty())
		{
			SetStaticMesh(ObjPath);
		}
	}
	else
	{
		if (UStaticMesh* Mesh = GetStaticMesh())
		{
			InOutHandle["ObjStaticMeshAsset"] = Mesh->GetAssetPathFileName();
		}
		else
		{
			InOutHandle["ObjStaticMeshAsset"] = "";
		}
	}

	// 리플렉션 기반 자동 직렬화 (추가 프로퍼티용)
	AutoSerialize(bInIsLoading, InOutHandle, UStaticMeshComponent::StaticClass());
}

void UStaticMeshComponent::SetMaterialByUser(const uint32 InMaterialSlotIndex, const FString& InMaterialName)
{
	assert((0 <= InMaterialSlotIndex && InMaterialSlotIndex < MaterialSlots.size()) && "out of range InMaterialSlotIndex");

	if (0 <= InMaterialSlotIndex && InMaterialSlotIndex < MaterialSlots.size())
	{
		MaterialSlots[InMaterialSlotIndex].MaterialName = InMaterialName;
		MaterialSlots[InMaterialSlotIndex].bChangedByUser = true;

		bChangedMaterialByUser = true;
	}
	else
	{
		UE_LOG("out of range InMaterialSlotIndex: %d", InMaterialSlotIndex);
	}

	assert(MaterialSlots[InMaterialSlotIndex].bChangedByUser == true);
}

FAABB UStaticMeshComponent::GetWorldAABB() const
{
	const FTransform WorldTransform = GetWorldTransform();
	const FMatrix WorldMatrix = GetWorldMatrix();

	if (!StaticMesh)
	{
		const FVector Origin = WorldTransform.TransformPosition(FVector());
		return FAABB(Origin, Origin);
	}

	const FAABB LocalBound = StaticMesh->GetLocalBound();
	const FVector LocalMin = LocalBound.Min;
	const FVector LocalMax = LocalBound.Max;

	const FVector LocalCorners[8] = {
		FVector(LocalMin.X, LocalMin.Y, LocalMin.Z),
		FVector(LocalMax.X, LocalMin.Y, LocalMin.Z),
		FVector(LocalMin.X, LocalMax.Y, LocalMin.Z),
		FVector(LocalMax.X, LocalMax.Y, LocalMin.Z),
		FVector(LocalMin.X, LocalMin.Y, LocalMax.Z),
		FVector(LocalMax.X, LocalMin.Y, LocalMax.Z),
		FVector(LocalMin.X, LocalMax.Y, LocalMax.Z),
		FVector(LocalMax.X, LocalMax.Y, LocalMax.Z)
	};

	FVector4 WorldMin4 = FVector4(LocalCorners[0].X, LocalCorners[0].Y, LocalCorners[0].Z, 1.0f) * WorldMatrix;
	FVector4 WorldMax4 = WorldMin4;

	for (int32 CornerIndex = 1; CornerIndex < 8; ++CornerIndex)
	{
		const FVector4 WorldPos = FVector4(LocalCorners[CornerIndex].X
			, LocalCorners[CornerIndex].Y
			, LocalCorners[CornerIndex].Z
			, 1.0f)
			* WorldMatrix;
		WorldMin4 = WorldMin4.ComponentMin(WorldPos);
		WorldMax4 = WorldMax4.ComponentMax(WorldPos);
	}

	FVector WorldMin = FVector(WorldMin4.X, WorldMin4.Y, WorldMin4.Z);
	FVector WorldMax = FVector(WorldMax4.X, WorldMax4.Y, WorldMax4.Z);
	return FAABB(WorldMin, WorldMax);
}

void UStaticMeshComponent::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
}

void UStaticMeshComponent::OnTransformUpdatedChildImpl()
{
	MarkWorldPartitionDirty();
}

void UStaticMeshComponent::MarkWorldPartitionDirty()
{
	if (UWorld* World = GetWorld())
	{
		if (UWorldPartitionManager* Partition = World->GetPartitionManager())
		{
			Partition->MarkDirty(this);
		}
	}
}
