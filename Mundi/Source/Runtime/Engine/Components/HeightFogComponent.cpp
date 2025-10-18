﻿#include "pch.h"
#include "HeightFogComponent.h"

#include "Color.h"
#include "ResourceManager.h"
#include "BillboardComponent.h"

IMPLEMENT_CLASS(UHeightFogComponent)

BEGIN_PROPERTIES(UHeightFogComponent)
	MARK_AS_COMPONENT("하이트 포그 컴포넌트", "하이트 기반 포그 효과를 생성합니다.")
	ADD_PROPERTY_RANGE(float, FogDensity, "Fog", 0.0f, 10.0f, true, "안개 밀도입니다.")
	ADD_PROPERTY_RANGE(float, FogHeightFalloff, "Fog", 0.0f, 10.0f, true, "높이에 따른 안개 감쇠 정도입니다.")
	ADD_PROPERTY_RANGE(float, StartDistance, "Fog", 0.0f, 10000.0f, true, "안개가 시작되는 거리입니다.")
	ADD_PROPERTY_RANGE(float, FogCutoffDistance, "Fog", 0.0f, 100000.0f, true, "안개가 최대가 되는 거리입니다.")
	ADD_PROPERTY_RANGE(float, FogMaxOpacity, "Fog", 0.0f, 1.0f, true, "안개 최대 불투명도입니다.")
END_PROPERTIES()

UHeightFogComponent::UHeightFogComponent()
{
	// 사막 느낌
	FogInscatteringColor = new FLinearColor(0.93f, 0.79f, 0.69f, 1.0f);
	HeightFogShader = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/HeightFog_PS.hlsl");

	
}

UHeightFogComponent::~UHeightFogComponent()
{
	if (FogInscatteringColor != nullptr)
	{
		delete FogInscatteringColor;
		FogInscatteringColor = nullptr;
	}
}

void UHeightFogComponent::OnRegister()
{
	Super_t::OnRegister();
	UBillboardComponent* Icon = NewObject<UBillboardComponent>();
	Icon->SetOwner(this->GetOwner());
	Icon->SetupAttachment(this, EAttachmentRule::KeepRelative);
	//SetupAttachment는 오너엑터의 OwnedComponent에 컴포넌트를 추가해주지 않으며
	//이 기능을 추가한다고 해도 AddOwnedComponent 함수가 단순히 Add만 하는 게 아니라
	//Scene에 Register하고 루트 컴포넌트까지 지정하고 RemoveOwnedComponent도 비슷하게 작동하는데,
	//기즈모들도 그 대상에 포함되고 Owner 설정도 안 해주고 있어서 어떤 오류가 생길지 모르겠어서 
	//일단 가장 문제가 없을 만한 방식으로 아래처럼 처리함
	this->GetOwner()->AddOwnedComponent(Icon);
	Icon->SetEditability(false);
	Icon->SetTextureName("Data/UI/Icons/S_AtmosphericHeightFog.dds");

}

void UHeightFogComponent::RenderHeightFog(URenderer* Renderer)
{
}


void UHeightFogComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		// Load FogInscatteringColor
		if (InOutHandle.hasKey("FogInscatteringColor"))
		{
			FVector4 ColorVec;
			FJsonSerializer::ReadVector4(InOutHandle, "FogInscatteringColor", ColorVec);
			FogInscatteringColor = new FLinearColor(ColorVec);
		}

		// Load HeightFogShader
		if (InOutHandle.hasKey("HeightFogShader"))
		{
			FString shaderPath = InOutHandle["HeightFogShader"].ToString();
			if (!shaderPath.empty())
			{
				HeightFogShader = UResourceManager::GetInstance().Load<UShader>(shaderPath.c_str());
			}
		}

		// FogDensity, FogHeightFalloff, StartDistance, FogCutoffDistance, FogMaxOpacity도 로드
		FJsonSerializer::ReadFloat(InOutHandle, "FogDensity", FogDensity, 0.2f);
		FJsonSerializer::ReadFloat(InOutHandle, "FogHeightFalloff", FogHeightFalloff, 0.2f);
		FJsonSerializer::ReadFloat(InOutHandle, "StartDistance", StartDistance, 0.0f);
		FJsonSerializer::ReadFloat(InOutHandle, "FogCutoffDistance", FogCutoffDistance, 6000.0f);
		FJsonSerializer::ReadFloat(InOutHandle, "FogMaxOpacity", FogMaxOpacity, 1.0f);
	}
	else
	{
		// Save FogInscatteringColor
		if (FogInscatteringColor != nullptr)
		{
			InOutHandle["FogInscatteringColor"] = FJsonSerializer::Vector4ToJson(FogInscatteringColor->ToFVector4());
		}

		// Save HeightFogShader
		if (HeightFogShader != nullptr)
		{
			InOutHandle["HeightFogShader"] = HeightFogShader->GetFilePath();
		}
		else
		{
			InOutHandle["HeightFogShader"] = "";
		}

		// FogDensity, FogHeightFalloff, StartDistance, FogCutoffDistance, FogMaxOpacity도 저장
		InOutHandle["FogDensity"] = FogDensity;
		InOutHandle["FogHeightFalloff"] = FogHeightFalloff;
		InOutHandle["StartDistance"] = StartDistance;
		InOutHandle["FogCutoffDistance"] = FogCutoffDistance;
		InOutHandle["FogMaxOpacity"] = FogMaxOpacity;

	}

	// 리플렉션 기반 자동 직렬화
	AutoSerialize(bInIsLoading, InOutHandle, UHeightFogComponent::StaticClass());
}

void UHeightFogComponent::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
}
