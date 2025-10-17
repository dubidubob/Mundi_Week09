﻿#pragma once

// 프로퍼티 타입 열거형
enum class EPropertyType : uint8
{
	Unknown,
	Bool,
	Int32,
	Float,
	FVector,
	FLinearColor,
	FString,
	FName,
	ObjectPtr,      // UObject* 및 파생 타입
	Enum,
	Struct
};

// 프로퍼티 메타데이터
struct FProperty
{
	const char* Name = nullptr;              // 프로퍼티 이름
	EPropertyType Type = EPropertyType::Unknown;  // 프로퍼티 타입
	size_t Offset = 0;                       // 클래스 인스턴스 내 오프셋
	const char* Category = nullptr;          // UI 카테고리
	float MinValue = 0.0f;                   // 범위 최소값
	float MaxValue = 0.0f;                   // 범위 최대값
	bool bIsEditAnywhere = false;            // UI에 노출 여부
	const char* Tooltip = nullptr;           // 툴팁 설명

	// 객체 인스턴스에서 프로퍼티 값의 포인터를 가져옴
	template<typename T>
	T* GetValuePtr(void* ObjectInstance) const
	{
		return reinterpret_cast<T*>(reinterpret_cast<char*>(ObjectInstance) + Offset);
	}

	// 객체 인스턴스에서 프로퍼티 값의 const 포인터를 가져옴
	template<typename T>
	const T* GetValuePtr(const void* ObjectInstance) const
	{
		return reinterpret_cast<const T*>(reinterpret_cast<const char*>(ObjectInstance) + Offset);
	}
};
