﻿#include "pch.h"
#include "DecalComponent.h"
#include "OBB.h"
#include "StaticMeshComponent.h"

UDecalComponent::UDecalComponent()
{
	UResourceManager::GetInstance().Load<UMaterial>("DecalVS.hlsl", EVertexLayoutType::PositionColorTexturNormal);
	UResourceManager::GetInstance().Load<UMaterial>("DecalPS.hlsl", EVertexLayoutType::PositionColorTexturNormal);
}

void UDecalComponent::Serialize(bool bIsLoading, FDecalData& InOut)
{
}

void UDecalComponent::DuplicateSubObjects()
{
    
}


void UDecalComponent::RenderAffectedPrimitives(URenderer* Renderer, UPrimitiveComponent* Target, const FMatrix& View, const FMatrix& Proj)
{
	UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Target);
	if (!SMC)
	{
		return;
	}


	D3D11RHI* RHIDevice = Renderer->GetRHIDevice();

	// Constatn Buffer 업데이트
	RHIDevice->UpdateConstantBuffers(Target->GetWorldMatrix(), View, Proj);

	const FMatrix DecalMatrix = GetDecalProjectionMatrix();
	RHIDevice->UpdateDecalBuffer(DecalMatrix);

	// Shader 설정
	UShader* VS = UResourceManager::GetInstance().Load<UShader>("DecalVS.hlsl");
	UShader* PS = UResourceManager::GetInstance().Load<UShader>("DecalPS.hlsl");

	RHIDevice->GetDeviceContext()->VSSetShader(VS->GetVertexShader(), nullptr, 0);
	RHIDevice->GetDeviceContext()->PSSetShader(PS->GetPixelShader(), nullptr, 0);
	RHIDevice->GetDeviceContext()->IASetInputLayout(VS->GetInputLayout());

	// VertexBuffer, IndexBuffer 설정
	UStaticMesh* Mesh = SMC->GetStaticMesh();

	ID3D11Buffer* VertexBuffer = Mesh->GetVertexBuffer();
	ID3D11Buffer* IndexBuffer = Mesh->GetIndexBuffer();
	uint32 VertexCount = Mesh->GetVertexCount();
	uint32 IndexCount = Mesh->GetIndexCount();
	UINT Stride = sizeof(FVertexDynamic);
	UINT Offset = 0;

	RHIDevice->GetDeviceContext()->IASetVertexBuffers(
		0, 1, &VertexBuffer, &Stride, &Offset
	);
	RHIDevice->GetDeviceContext()->IASetIndexBuffer(
		IndexBuffer, DXGI_FORMAT_R32_UINT, 0
	);

	RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	RHIDevice->PSSetDefaultSampler(0);

	// DrawCall
	RHIDevice->GetDeviceContext()->DrawIndexed(IndexCount, 0, 0);
	
}

void UDecalComponent::RenderDebugVolume(URenderer* Renderer, const FMatrix& View, const FMatrix& Proj) const
{
	// 로컬 단위 큐브의 정점과 선 정보 정의 (위와 동일)
	//const FVector4 LocalVertices[8] = {
	//	FVector4(-0.5f, -0.5f, -0.5f, 1.0f), FVector4(0.5f, -0.5f, -0.5f, 1.0f),
	//	FVector4(0.5f, 0.5f, -0.5f, 1.0f), FVector4(-0.5f, 0.5f, -0.5f, 1.0f),
	//	FVector4(-0.5f, -0.5f, 0.5f, 1.0f), FVector4(0.5f, -0.5f, 0.5f, 1.0f),
	//	FVector4(0.5f, 0.5f, 0.5f, 1.0f), FVector4(-0.5f, 0.5f, 0.5f, 1.0f)
	//};

	//const int Edges[12][2] = {
	//	{0, 1}, {1, 2}, {2, 3}, {3, 0}, // 하단
	//	{4, 5}, {5, 6}, {6, 7}, {7, 4}, // 상단
	//	{0, 4}, {1, 5}, {2, 6}, {3, 7}  // 기둥
	//};

	//// 컴포넌트의 월드 변환 행렬
	//const FMatrix WorldMatrix = GetWorldMatrix();

	// 라인 색상
	const FVector4 BoxColor(1.0f, 1.0f, 0.0f, 1.0f); // 노란색

	// AddLines 함수에 전달할 데이터 배열들을 준비
	TArray<FVector> StartPoints;
	TArray<FVector> EndPoints;
	TArray<FVector4> Colors;

	//// 12개의 선 데이터를 배열에 채워 넣습니다.
	//for (int i = 0; i < 12; ++i)
	//{
	//	// 월드 좌표로 변환
	//	const FVector4 WorldStart = (LocalVertices[Edges[i][0]]) * WorldMatrix;
	//	const FVector4 WorldEnd = (LocalVertices[Edges[i][1]]) * WorldMatrix;

	//	StartPoints.Add(FVector(WorldStart.X, WorldStart.Y, WorldStart.Z));
	//	EndPoints.Add(FVector(WorldEnd.X, WorldEnd.Y, WorldEnd.Z));
	//	Colors.Add(BoxColor);
	//}

	
	TArray<FVector> Coners = GetOBB().GetCorners();

	const int Edges2[12][2] = {
		{6, 4}, {7, 5}, {6, 7}, {4, 5}, // 앞면
		{4, 0}, {5, 1}, {6, 2}, {7, 3}, // 옆면
		{0, 2}, {1, 3}, {0, 1}, {2, 3}  // 뒷면
	};

	// 12개의 선 데이터를 배열에 채워 넣습니다.
	for (int i = 0; i < 12; ++i)
	{
		// 월드 좌표로 변환
		const FVector WorldStart = Coners[Edges2[i][0]];
		const FVector WorldEnd = Coners[Edges2[i][1]];

		StartPoints.Add(WorldStart);
		EndPoints.Add(WorldEnd);
		Colors.Add(BoxColor);
	}

	// 모든 데이터를 준비한 뒤, 단 한 번의 호출로 렌더러에 전달합니다.
	Renderer->AddLines(StartPoints, EndPoints, Colors);
}

void UDecalComponent::SetDecalTexture(UTexture* InTexture)
{
}

void UDecalComponent::SetDecalTexture(const FString& TexturePath)
{
}

FAABB UDecalComponent::GetWorldAABB() const
{
    return FAABB();
}

FOBB UDecalComponent::GetOBB() const
{
    const FVector Center = GetWorldLocation();
    const FVector HalfExtent = GetWorldScale() / 2.0f;

    const FQuat Quat = GetWorldRotation();

    FVector Axes[3];
    Axes[0] = Quat.GetForwardVector();
    Axes[1] = Quat.GetRightVector();
    Axes[2] = Quat.GetUpVector();

    FOBB Obb(Center, HalfExtent, Axes);

    return Obb;
}

FMatrix UDecalComponent::GetDecalProjectionMatrix() const
{
    const FOBB Obb = GetOBB();

	const FMatrix DecalWorld = FMatrix::FromTRS(GetWorldLocation(), GetWorldRotation(), {1.0f, 1.0f, 1.0f});
	const FMatrix DecalView = DecalWorld.InverseAffine();

	const FVector Scale = GetWorldScale();
	const FMatrix DecalProj = FMatrix::OrthoLH_XForward(Scale.Y * 2.0f, Scale.Z, -Obb.HalfExtent.X / 2.0f, Obb.HalfExtent.X / 2.0f);
	//const FMatrix DecalProj = FMatrix::OrthoLH(Scale.Y, Scale.Z, -Obb.HalfExtent.X, Obb.HalfExtent.X);

	FMatrix DecalViewProj = DecalView * DecalProj;

    return DecalViewProj;
}
