﻿#pragma once
#include "ResourceBase.h"
#include "Enums.h"

class UShader;
class UTexture;

// 텍스처 슬롯을 명확하게 구분하기 위한 Enum (선택 사항이지만 권장)
enum class EMaterialTextureSlot : uint8
{
	Diffuse = 0,
	Normal,
	//Specular,
	//Emissive,
	// ... 기타 슬롯 ...
	Max // 배열 크기 지정용
};

class UMaterialInterface : public UResourceBase
{
	DECLARE_CLASS(UMaterialInterface, UResourceBase)
public:
	virtual UShader* GetShader() = 0;
	virtual UTexture* GetTexture(EMaterialTextureSlot Slot) const = 0;
	virtual bool HasTexture(EMaterialTextureSlot Slot) const = 0;
	virtual const FMaterialInfo& GetMaterialInfo() const = 0;
};

class UMaterial : public UMaterialInterface
{
	DECLARE_CLASS(UMaterial, UMaterialInterface)
public:
	UMaterial();
	void Load(const FString& InFilePath, ID3D11Device* InDevice);

protected:
	~UMaterial() override = default;

public:
	void SetShader(UShader* InShaderResource);
	void SetShaderByName(const FString& InShaderName);
	UShader* GetShader() override;
	UTexture* GetTexture(EMaterialTextureSlot Slot) const override;
	bool HasTexture(EMaterialTextureSlot Slot) const override;
	const FMaterialInfo& GetMaterialInfo() const override;

	// MaterialInfo의 텍스처 경로들을 기반으로 ResolvedTextures 배열을 채웁니다.
	void ResolveTextures();

	void SetMaterialInfo(const FMaterialInfo& InMaterialInfo)
	{
		MaterialInfo = InMaterialInfo;
		ResolveTextures();
	}

	void SetTexture(EMaterialTextureSlot Slot, const FString& TexturePath);
	void SetMaterialName(FString& InMaterialName) { MaterialInfo.MaterialName = InMaterialName; }


protected:
	// 이 머티리얼이 사용할 셰이더 프로그램 (예: UberLit.hlsl)
	UShader* Shader = nullptr;
	FMaterialInfo MaterialInfo;
	// MaterialInfo 이름 기반으로 찾은 (Textures[0] = Diffuse, Textures[1] = Normal)
	TArray<UTexture*> ResolvedTextures;
};

class UMaterialInstanceDynamic : public UMaterialInterface
{
	DECLARE_CLASS(UMaterialInstanceDynamic, UMaterialInterface)
public:
	UMaterialInstanceDynamic();

	// 부모 머티리얼을 기반으로 인스턴스를 생성하는 정적 함수
	static UMaterialInstanceDynamic* Create(UMaterialInterface* InParentMaterial);

	void CopyParametersFrom(const UMaterialInstanceDynamic* Other);

	// UMaterialInterface의 순수 가상 함수들을 구현합니다.
	UShader* GetShader() override;
	UTexture* GetTexture(EMaterialTextureSlot Slot) const override;
	bool HasTexture(EMaterialTextureSlot Slot) const override;
	const FMaterialInfo& GetMaterialInfo() const override;
	UMaterialInterface* GetParentMaterial() const { return ParentMaterial; }

	// 텍스처 파라미터 값을 런타임에 변경하는 함수
	void SetTextureParameterValue(EMaterialTextureSlot Slot, UTexture* Value);
	// 벡터 파라미터 값을 런타임에 변경하는 함수
	void SetVectorParameterValue(const FString& ParameterName, const FLinearColor& Value);
	// 스칼라 파라미터 값을 런타임에 변경하는 함수
	void SetScalarParameterValue(const FString& ParameterName, float Value);

protected:
	// 생성자에서 부모 머티리얼의 포인터를 저장합니다.
	UMaterialInstanceDynamic(UMaterialInterface* InParentMaterial);

private:
	UMaterialInterface* ParentMaterial;

	// 이 인스턴스에서 덮어쓴 값들만 저장합니다.
	TMap<EMaterialTextureSlot, UTexture*> OverriddenTextures;
	TMap<FString, float> OverriddenScalarParameters;
	TMap<FString, FLinearColor> OverriddenVectorParameters;
	
	// GetMaterialInfo()가 호출될 때마다 부모의 정보를 복사하고
	// 아래 값들로 덮어쓴 뒤 반환하기 위한 캐시 데이터입니다.
	mutable FMaterialInfo CachedMaterialInfo;
	mutable bool bIsCachedMaterialInfoDirty = true;
};
