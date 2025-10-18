﻿#include "pch.h"
#include "Material.h"
#include "Shader.h"
#include "Texture.h"
#include "ResourceManager.h"

IMPLEMENT_CLASS(UMaterial)

UMaterial::UMaterial()
{
    // 기본 Material 생성 (기본 Phong 셰이더 사용)
    FString ShaderPath = "Shaders/Materials/UberLit.hlsl";
    TArray<FShaderMacro> DefaultMacros;
    DefaultMacros.push_back(FShaderMacro{ "LIGHTING_MODEL_PHONG", "1" });

    UShader* DefaultShader = UResourceManager::GetInstance().Load<UShader>(ShaderPath, DefaultMacros);
    if (DefaultShader)
    {
        SetShader(DefaultShader);
    }
}

void UMaterial::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
    // 기본 쉐이더 로드 (LayoutType에 따라)
    // dds 의 경우 
    if (InFilePath.find(".dds") != std::string::npos)
    {
        FString shaderName = UResourceManager::GetInstance().GetProperShader(InFilePath);

        Shader = UResourceManager::GetInstance().Load<UShader>(shaderName);
        UResourceManager::GetInstance().Load<UTexture>(InFilePath);
        MaterialInfo.DiffuseTextureFileName = InFilePath;
    } // hlsl 의 경우 
    else if (InFilePath.find(".hlsl") != std::string::npos)
    {
        Shader = UResourceManager::GetInstance().Load<UShader>(InFilePath);
    }
    else
    {
        throw std::runtime_error(".dds나 .hlsl만 입력해주세요. 현재 입력 파일명 : " + InFilePath);
    }
}

void UMaterial::SetShader( UShader* ShaderResource) {
    
	Shader = ShaderResource;
}

UShader* UMaterial::GetShader()
{
	return Shader;
}

void UMaterial::SetDiffuseTexture(const FString& TexturePath)
{
    MaterialInfo.DiffuseTextureFileName = TexturePath;
}

UTexture* UMaterial::GetDiffuseTexture()
{
    return UResourceManager::GetInstance().Load<UTexture>(MaterialInfo.DiffuseTextureFileName);
}
