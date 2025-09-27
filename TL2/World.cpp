﻿#include "pch.h"
#include "SelectionManager.h"
#include "Picking.h"
#include "SceneLoader.h"
#include "CameraActor.h"
#include "StaticMeshActor.h"
#include "CameraComponent.h"
#include "ObjectFactory.h"
#include "TextRenderComponent.h"
#include "AABoundingBoxComponent.h"
#include "FViewport.h"
#include "SViewportWindow.h"
#include "SMultiViewportWindow.h"
#include "StaticMesh.h"
#include "ObjManager.h"
#include "SceneRotationUtils.h"
#include "WorldPartitionManager.h"
#include "PrimitiveComponent.h"
#include "Octree.h"
#include "BVHierachy.h"
#include "Frustum.h"
#include "Occlusion.h"

extern float CLIENTWIDTH;
extern float CLIENTHEIGHT;

UWorld::UWorld() : ResourceManager(UResourceManager::GetInstance())
, UIManager(UUIManager::GetInstance())
, InputManager(UInputManager::GetInstance())
, SelectionManager(USelectionManager::GetInstance())
{
}
UWorld& UWorld::GetInstance()
{
	static UWorld* Instance = nullptr;
	if (Instance == nullptr)
	{
		Instance = NewObject<UWorld>();
	}
	return *Instance;
}


UWorld::~UWorld()
{
	for (AActor* Actor : Actors)
	{
		ObjectFactory::DeleteObject(Actor);
	}
	Actors.clear();

	// 카메라 정리
	ObjectFactory::DeleteObject(MainCameraActor);
	MainCameraActor = nullptr;

	// Grid 정리 
	ObjectFactory::DeleteObject(GridActor);
	GridActor = nullptr;

	// ObjManager 정리
	FObjManager::Clear();
}

static void DebugRTTI_UObject(UObject* Obj, const char* Title)
{
	if (!Obj)
	{
		UE_LOG("[RTTI] Obj == null\r\n");
		return;
	}

	char buf[256];
	UE_LOG("========== RTTI CHECK ==========\r\n");
	if (Title)
	{
		std::snprintf(buf, sizeof(buf), "[RTTI] %s\r\n", Title);
		UE_LOG(buf);
	}

	std::snprintf(buf, sizeof(buf), "[RTTI] TypeName = %s\r\n", Obj->GetClass()->Name);
	UE_LOG(buf);

	std::snprintf(buf, sizeof(buf), "[RTTI] IsA<AActor>      = %d\r\n", (int)Obj->IsA<AActor>());
	UE_LOG(buf);
	std::snprintf(buf, sizeof(buf), "[RTTI] IsA<ACameraActor> = %d\r\n", (int)Obj->IsA<ACameraActor>());
	UE_LOG(buf);

	UE_LOG("[RTTI] Inheritance chain: ");
	for (const UClass* c = Obj->GetClass(); c; c = c->Super)
	{
		std::snprintf(buf, sizeof(buf), "%s%s", c->Name, c->Super ? " <- " : "\r\n");
		UE_LOG(buf);
	}
	//FString Name = Obj->GetName();
	std::snprintf(buf, sizeof(buf), "[RTTI] TypeName = %s\r\n", Obj->GetName().c_str());
	OutputDebugStringA(buf);
	OutputDebugStringA("================================\r\n");
}

void UWorld::Initialize()
{
	FObjManager::Preload();
	CreateNewScene();

	InitializeMainCamera();
	InitializeGrid();
	InitializeGizmo();

	// 액터 간 참조 설정
	SetupActorReferences();

}

void UWorld::InitializeMainCamera()
{
	MainCameraActor = NewObject<ACameraActor>();
	MainCameraActor->SetWorld(this);

	DebugRTTI_UObject(MainCameraActor, "MainCameraActor");
	UIManager.SetCamera(MainCameraActor);

	EngineActors.Add(MainCameraActor);
}

void UWorld::InitializeGrid()
{
	GridActor = NewObject<AGridActor>();
	GridActor->SetWorld(this);
	GridActor->Initialize();

	// Add GridActor to Actors array so it gets rendered in the main loop
	EngineActors.push_back(GridActor);
	//EngineActors.push_back(GridActor);
}

void UWorld::InitializeGizmo()
{
	// === 기즈모 엑터 초기화 ===
	GizmoActor = NewObject<AGizmoActor>();
	GizmoActor->SetWorld(this);
	GizmoActor->SetActorTransform(FTransform(FVector{ 0, 0, 0 }, FQuat::MakeFromEuler(FVector{ 0, -90, 0 }),
		FVector{ 1, 1, 1 }));
	// 기즈모에 카메라 참조 설정
	if (MainCameraActor)
	{
		GizmoActor->SetCameraActor(MainCameraActor);
	}

	UIManager.SetGizmoActor(GizmoActor);
}
void UWorld::SetRenderer(URenderer* InRenderer)
{
	Renderer = InRenderer;
}

void UWorld::Render()
{
	Renderer->BeginFrame();
	UIManager.Render();

	if (MultiViewport)
	{
		MultiViewport->OnRender();
	}

	//프레임 종료 
	UIManager.EndFrame();
	Renderer->EndFrame();
}

void UWorld::RenderSingleViewport()
{
	FMatrix ViewMatrix = MainCameraActor->GetViewMatrix();
	FMatrix ProjectionMatrix = MainCameraActor->GetProjectionMatrix();
	FMatrix ModelMatrix;
	FVector rgb(1.0f, 1.0f, 1.0f);

	if (!Renderer) return;
	// === Begin Frame ===
	Renderer->BeginFrame();

	// === Begin Line Batch for all actors ===
	Renderer->BeginLineBatch();

    // === Draw Actors with Show Flag checks ===
    Renderer->SetViewModeType(ViewModeIndex);

	// 일반 액터들 렌더링 (Primitives Show Flag 체크)
	if (IsShowFlagEnabled(EEngineShowFlags::SF_Primitives))
	{
		for (AActor* Actor : Actors)
		{
			if (!Actor) continue;
			if (Actor->GetActorHiddenInGame()) continue;

			// StaticMesh Show Flag 체크
			if (Cast<AStaticMeshActor>(Actor) && !IsShowFlagEnabled(EEngineShowFlags::SF_StaticMeshes))
				continue;

			bool bIsSelected = SelectionManager.IsActorSelected(Actor);
			if (bIsSelected) {
				Renderer->OMSetDepthStencilState(EComparisonFunc::Always);
			}
			Renderer->UpdateHighLightConstantBuffer(bIsSelected, rgb, 0, 0, 0, 0);

			for (USceneComponent* Component : Actor->GetComponents())
			{
				if (!Component) continue;

				if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
				{
					if (!ActorComp->IsActive()) continue;
				}

				// Text Render Component Show Flag 체크
				if (Cast<UTextRenderComponent>(Component) && !IsShowFlagEnabled(EEngineShowFlags::SF_BillboardText))
					continue;

				// Bounding Box Show Flag 체크  
				if (Cast<UAABoundingBoxComponent>(Component) && !IsShowFlagEnabled(EEngineShowFlags::SF_BoundingBoxes))
					continue;

				if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
				{
					Renderer->SetViewModeType(ViewModeIndex);
					Primitive->Render(Renderer, ViewMatrix, ProjectionMatrix);
					Renderer->OMSetDepthStencilState(EComparisonFunc::LessEqual);
				}
			}
			// 블랜드 스테이드 종료
			Renderer->OMSetBlendState(false);
		}
	}

	// Engine Actors (그리드 등) 렌더링
	for (AActor* EngineActor : EngineActors)
	{
		if (!EngineActor) continue;
		if (EngineActor->GetActorHiddenInGame()) continue;

		// Grid Show Flag 체크
		if (Cast<AGridActor>(EngineActor) && !IsShowFlagEnabled(EEngineShowFlags::SF_Grid))
			continue;

		for (USceneComponent* Component : EngineActor->GetComponents())
		{
			if (!Component) continue;

			if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
			{
				if (!ActorComp->IsActive()) continue;
			}
			if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
			{
				Renderer->SetViewModeType(ViewModeIndex);
				Primitive->Render(Renderer, ViewMatrix, ProjectionMatrix);
				Renderer->OMSetDepthStencilState(EComparisonFunc::LessEqual);
			}
		}
		// 블랜드 스테이드 종료
		Renderer->OMSetBlendState(false);
	}
    // Octree debug draw (draw as overlay: depth test always, no write)
    if (IsShowFlagEnabled(EEngineShowFlags::SF_OctreeDebug))
    {
        if (FOctree* Octree = UWorldPartitionManager::GetInstance()->GetSceneOctree())
        {
            Renderer->OMSetDepthStencilState(EComparisonFunc::Always);
            Octree->DebugDraw(Renderer);
            Renderer->OMSetDepthStencilState(EComparisonFunc::LessEqual);
        }
    }

    Renderer->EndLineBatch(FMatrix::Identity(), ViewMatrix, ProjectionMatrix);



	Renderer->UpdateHighLightConstantBuffer(false, rgb, 0, 0, 0, 0);
	UIManager.Render();
	// === End Frame ===
	Renderer->EndFrame();
}


void UWorld::RenderViewports(ACameraActor* Camera, FViewport* Viewport)
{

	// 뷰포트의 실제 크기로 aspect ratio 계산
	float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(Viewport->GetSizeY());
	if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f; // 0으로 나누기 방지

	FMatrix ViewMatrix = Camera->GetViewMatrix();
	FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);

	Frustum ViewFrustum;
	UCameraComponent* CamComp = nullptr;
	if (CamComp = Camera->GetCameraComponent())
	{
		ViewFrustum = CreateFrustumFromCamera(*CamComp, ViewportAspectRatio);
	}
	if (!Renderer) return;

	FVector rgb(1.0f, 1.0f, 1.0f);

	// === Begin Line Batch for all actors ===
	Renderer->BeginLineBatch();

	// === Draw Actors with Show Flag checks ===
	Renderer->SetViewModeType(ViewModeIndex);


	// ============ Culling Logic Dispatch ========= //
	for (AActor* Actor : Actors)
	{
		Actor->SetCulled(true);
	}

	UWorldPartitionManager::GetInstance()->FrustumQuery(ViewFrustum);

	// ---------------------- CPU HZB Occlusion ----------------------
	if (bUseCPUOcclusion)
	{
		// 1) 그리드 사이즈 보정(해상도 변화 대응)
		UpdateOcclusionGridSizeForViewport(Viewport);

		// 2) 오클루더/오클루디 수집
		TArray<FCandidateDrawable> Occluders, Occludees;
		BuildCpuOcclusionSets(ViewFrustum, ViewMatrix, ProjectionMatrix, Occluders, Occludees);

		// 3) 오클루더로 저해상도 깊이 빌드 + HZB
		OcclusionCPU.BuildOccluderDepth(Occluders, Viewport->GetSizeX(), Viewport->GetSizeY());
		OcclusionCPU.BuildHZB();

		// 4) 가시성 판정 → VisibleFlags[UUID] = 0/1
		//     VisibleFlags 크기 보장
		uint32_t maxUUID = 0;
		for (auto& C : Occludees) maxUUID = std::max(maxUUID, C.ActorIndex);
		if (VisibleFlags.size() <= size_t(maxUUID))
			VisibleFlags.assign(size_t(maxUUID + 1), 1); // 기본 보임

		OcclusionCPU.TestOcclusion(Occludees, Viewport->GetSizeX(), Viewport->GetSizeY(), VisibleFlags);
	}
	// ----------------------------------------------------------------

	// 일반 액터들 렌더링
	if (IsShowFlagEnabled(EEngineShowFlags::SF_Primitives))
	{
		for (AActor* Actor : Actors)
		{
			if (!Actor) continue;
			if (Actor->GetActorHiddenInGame()) continue;
			if (Actor->GetCulled()) continue;

			// ★★★ CPU 오클루전 컬링: UUID로 보임 여부 확인
			if (bUseCPUOcclusion)
			{
				uint32_t id = Actor->UUID;
				if (id < VisibleFlags.size() && VisibleFlags[id] == 0)
				{
					continue; // 가려짐 → 스킵
				}
			}

			if (Cast<AStaticMeshActor>(Actor) && !IsShowFlagEnabled(EEngineShowFlags::SF_StaticMeshes))
			{
				continue;
			}


			//if (CamComp)
			//{
			//	if (AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor))
			//	{
			//		if (UAABoundingBoxComponent* Box = Cast<UAABoundingBoxComponent>(MeshActor->CollisionComponent))
			//		{
			//			const FBound Bound = Box->GetWorldBound();
			//			if (!IsAABBVisible(ViewFrustum, Bound))
			//			{
			//				continue;
			//			}
			//		}
			//	}
			//}
			bool bIsSelected = SelectionManager.IsActorSelected(Actor);
			/*if (bIsSelected)
				Renderer->OMSetDepthStencilState(EComparisonFunc::Always);*/ // 이렇게 하면, 같은 메시에 속한 정점끼리도 뒤에 있는게 앞에 그려지는 경우가 발생해, 이상하게 렌더링 됨.

			Renderer->UpdateHighLightConstantBuffer(bIsSelected, rgb, 0, 0, 0, 0);

			for (USceneComponent* Component : Actor->GetComponents())
			{
				if (!Component)
				{
					continue;
				}
				if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
				{
					if (!ActorComp->IsActive())
					{
						continue;
					}
				}


				if (Cast<UTextRenderComponent>(Component) && !IsShowFlagEnabled(EEngineShowFlags::SF_BillboardText))
				{
					continue;
				}

				if (Cast<UAABoundingBoxComponent>(Component) && !IsShowFlagEnabled(EEngineShowFlags::SF_BoundingBoxes))
				{
					continue;
				}
				if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
				{
					Renderer->SetViewModeType(ViewModeIndex);
					Primitive->Render(Renderer, ViewMatrix, ProjectionMatrix);
					Renderer->OMSetDepthStencilState(EComparisonFunc::LessEqual);
				}
			}
			Renderer->OMSetBlendState(false);
		}
	}

	// 엔진 액터들 (그리드 등)
	for (AActor* EngineActor : EngineActors)
	{
		if (!EngineActor)
		{
			continue;
		}
		if (EngineActor->GetActorHiddenInGame())
		{
			continue;
		}

		if (Cast<AGridActor>(EngineActor) && !IsShowFlagEnabled(EEngineShowFlags::SF_Grid))
		{
			continue;
		}

		for (USceneComponent* Component : EngineActor->GetComponents())
		{
			if (!Component)
			{
				continue;
			}
			if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
			{
				if (!ActorComp->IsActive())
				{
					continue;
				}
			}

			if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
			{
				Renderer->SetViewModeType(ViewModeIndex);
				Primitive->Render(Renderer, ViewMatrix, ProjectionMatrix);
				Renderer->OMSetDepthStencilState(EComparisonFunc::LessEqual);
			}
		}
		Renderer->OMSetBlendState(false);
	}

	// Debug draw (exclusive: BVH first, else Octree)
	if (IsShowFlagEnabled(EEngineShowFlags::SF_BVHDebug))
	{
		if (auto* PM = UWorldPartitionManager::GetInstance())
		{
			if (FBVHierachy* BVH = PM->GetBVH())
			{
				BVH->DebugDraw(Renderer);
			}
		}
	}
	else if (IsShowFlagEnabled(EEngineShowFlags::SF_OctreeDebug))
	{
		if (auto* PM = UWorldPartitionManager::GetInstance())
		{
			if (FOctree* Octree = PM->GetSceneOctree())
			{
				Octree->DebugDraw(Renderer);
			}
		}
	}

	Renderer->EndLineBatch(FMatrix::Identity(), ViewMatrix, ProjectionMatrix);
	Renderer->UpdateHighLightConstantBuffer(false, rgb, 0, 0, 0, 0);

}



void UWorld::Tick(float DeltaSeconds)
{
	// Update spatial indices first so any previous-frame changes are reflected
	if (UWorldPartitionManager::GetInstance())
	{
		UWorldPartitionManager::GetInstance()->Update(DeltaSeconds, /*budget*/256);
	}

	//순서 바꾸면 안댐
	for (AActor* Actor : Actors)
	{
		if (Actor) Actor->Tick(DeltaSeconds);
	}
	for (AActor* EngineActor : EngineActors)
	{
		if (EngineActor) EngineActor->Tick(DeltaSeconds);
	}
	GizmoActor->Tick(DeltaSeconds);

	//ProcessActorSelection();
	ProcessViewportInput();
	//Input Manager가 카메라 후에 업데이트 되어야함


	// 뷰포트 업데이트 - UIManager의 뷰포트 전환 상태에 따라

	if (MultiViewport)
	{
		MultiViewport->OnUpdate(DeltaSeconds);
	}

	InputManager.Update();
	UIManager.Update(DeltaSeconds);
}

float UWorld::GetTimeSeconds() const
{
	return 0.0f;
}

FString UWorld::GenerateUniqueActorName(const FString& ActorType)
{
	// Get current count for this type
	int32& CurrentCount = ObjectTypeCounts[ActorType];
	FString UniqueName = ActorType + "_" + std::to_string(CurrentCount);
	CurrentCount++;
	return UniqueName;
}

//
// 액터 제거
//
bool UWorld::DestroyActor(AActor* Actor)
{
	if (!Actor)
	{
		return false; // nullptr 들어옴 → 실패
	}

	// SelectionManager에서 선택 해제 (메모리 해제 전에 하자)
	USelectionManager::GetInstance().DeselectActor(Actor);

	// UIManager에서 픽된 액터 정리
	if (UIManager.GetPickedActor() == Actor)
	{
		UIManager.ResetPickedActor();
	}

	// 배열에서 제거 시도
	auto it = std::find(Actors.begin(), Actors.end(), Actor);
	if (it != Actors.end())
	{
		// 만약 StaticMeshActor라면, 전용 배열에서도 제거
		if (AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor))
		{
			auto mesh_it = std::find(StaticMeshActors.begin(), StaticMeshActors.end(), MeshActor);
			if (mesh_it != StaticMeshActors.end())
			{
				StaticMeshActors.erase(mesh_it);
			}
		}

		// 옥트리에서 제거
		OnActorDestroyed(Actor);

		Actors.erase(it);

		// 메모리 해제
		ObjectFactory::DeleteObject(Actor);

		// 삭제된 액터 정리
		USelectionManager::GetInstance().CleanupInvalidActors();

		return true; // 성공적으로 삭제
	}

	return false; // 월드에 없는 액터
}

void UWorld::OnActorSpawned(AActor* Actor)
{
	if (UWorldPartitionManager::GetInstance() && Actor)
	{
		UWorldPartitionManager::GetInstance()->Register(Actor);
	}
}

void UWorld::OnActorDestroyed(AActor* Actor)
{
	if (UWorldPartitionManager::GetInstance() && Actor)
	{
		UWorldPartitionManager::GetInstance()->Unregister(Actor);
	}
}

inline FString ToObjFileName(const FString& TypeName)
{
	return "Data/" + TypeName + ".obj";
}

inline FString RemoveObjExtension(const FString& FileName)
{
	const FString Extension = ".obj";

	// 마지막 경로 구분자 위치 탐색 (POSIX/Windows 모두 지원)
	const uint64 Sep = FileName.find_last_of("/\\");
	const uint64 Start = (Sep == FString::npos) ? 0 : Sep + 1;

	// 확장자 제거 위치 결정
	uint64 End = FileName.size();
	if (End >= Extension.size() &&
		FileName.compare(End - Extension.size(), Extension.size(), Extension) == 0)
	{
		End -= Extension.size();
	}

	// 베이스 이름(확장자 없는 파일명) 반환
	if (Start <= End)
		return FileName.substr(Start, End - Start);

	// 비정상 입력 시 원본 반환 (안전장치)
	return FileName;
}

void UWorld::CreateNewScene()
{
	// Safety: clear interactions that may hold stale pointers
	SelectionManager.ClearSelection();
	UIManager.ResetPickedActor();

	for (AActor* Actor : Actors)
	{
		ObjectFactory::DeleteObject(Actor);
	}
	Actors.Empty();
	StaticMeshActors.Empty();

	// 이름 카운터 초기화: 씬을 새로 시작할 때 각 BaseName 별 suffix를 0부터 다시 시작
	ObjectTypeCounts.clear();

	if (UWorldPartitionManager::GetInstance())
	{
		UWorldPartitionManager::GetInstance()->Clear();
	}
}



// 액터 인터페이스 관리 메소드들
void UWorld::SetupActorReferences()
{
	if (GizmoActor && MainCameraActor)
	{
		GizmoActor->SetCameraActor(MainCameraActor);
	}

}
//마우스 피킹관련 메소드
void UWorld::ProcessActorSelection()
{
	if (InputManager.IsMouseButtonPressed(LeftButton))
	{
		const FVector2D MousePosition = UInputManager::GetInstance().GetMousePosition();
		{
			if (MultiViewport)
			{
				MultiViewport->OnMouseDown(MousePosition,0);
			}
		}
	}
	if (InputManager.IsMouseButtonPressed(RightButton))
	{
		const FVector2D MousePosition = UInputManager::GetInstance().GetMousePosition();
		{
			if (MultiViewport)
			{
				MultiViewport->OnMouseDown(MousePosition, 0);
			}
		}
	}
	if (InputManager.IsMouseButtonPressed(RightButton) )
	{
		const FVector2D MousePosition = UInputManager::GetInstance().GetMousePosition();
		{
			if (MultiViewport)
			{
				MultiViewport->OnMouseDown(MousePosition,1);
			}
		}
	}
	if (InputManager.IsMouseButtonReleased(RightButton))
	{
		const FVector2D MousePosition = UInputManager::GetInstance().GetMousePosition();
		{
			if (MultiViewport)
			{
				MultiViewport->OnMouseUp(MousePosition,1);
			}
		}
	}

}
void UWorld::ProcessViewportInput()
{
	const FVector2D MousePosition = UInputManager::GetInstance().GetMousePosition();

	if (InputManager.IsMouseButtonPressed(LeftButton))
	{
		const FVector2D MousePosition = UInputManager::GetInstance().GetMousePosition();
		{
			if (MultiViewport)
			{
				MultiViewport->OnMouseDown(MousePosition, 0);
			}
		}
	}
	if (InputManager.IsMouseButtonPressed(RightButton))
	{
		const FVector2D MousePosition = UInputManager::GetInstance().GetMousePosition();
		{
			if (MultiViewport)
			{
				MultiViewport->OnMouseDown(MousePosition, 1);
			}
		}
	}
	if (InputManager.IsMouseButtonReleased(LeftButton))
	{
		const FVector2D MousePosition = UInputManager::GetInstance().GetMousePosition();
		{
			if (MultiViewport)
			{
				MultiViewport->OnMouseUp(MousePosition, 0);
			}
		}
	}
	if (InputManager.IsMouseButtonReleased(RightButton))
	{
		const FVector2D MousePosition = UInputManager::GetInstance().GetMousePosition();
		{
			if (MultiViewport)
			{
				MultiViewport->OnMouseUp(MousePosition, 1);
			}
		}
	}
	MultiViewport->OnMouseMove(MousePosition);
}

void UWorld::LoadScene(const FString& SceneName)
{
	namespace fs = std::filesystem;
	fs::path path = fs::path("Scene") / SceneName;
	if (path.extension().string() != ".Scene")
	{
		path.replace_extension(".Scene");
	}

	const FString FilePath = path.make_preferred().string();

	// [1] 로드 시작 전 현재 카운터 백업
	const uint32 PreLoadNext = UObject::PeekNextUUID();

	// [2] 파일 NextUUID는 현재보다 클 때만 반영(절대 하향 설정 금지)
	uint32 LoadedNextUUID = 0;
	if (FSceneLoader::TryReadNextUUID(FilePath, LoadedNextUUID))
	{
		if (LoadedNextUUID > UObject::PeekNextUUID())
		{
			UObject::SetNextUUID(LoadedNextUUID);
		}
	}

	// [3] 기존 씬 비우기
	CreateNewScene();

    // [4] 로드
    FPerspectiveCameraData CamData{};
    const TArray<FPrimitiveData>& Primitives = FSceneLoader::Load(FilePath, &CamData);

    // 마우스 델타 초기화
    const FVector2D CurrentMousePos = UInputManager::GetInstance().GetMousePosition();
    UInputManager::GetInstance().SetLastMousePosition(CurrentMousePos);

    // 카메라 적용
    if (MainCameraActor && MainCameraActor->GetCameraComponent())
    {
        UCameraComponent* Cam = MainCameraActor->GetCameraComponent();

        // 위치/회전(월드 트랜스폼)
        MainCameraActor->SetActorLocation(CamData.Location);
        MainCameraActor->SetActorRotation(FQuat::MakeFromEuler(CamData.Rotation));

        // 입력 경로와 동일한 방식으로 각도/회전 적용
        // 매핑: Pitch = CamData.Rotation.Y, Yaw = CamData.Rotation.Z
        MainCameraActor->SetAnglesImmediate(CamData.Rotation.Y, CamData.Rotation.Z);

		// UIManager의 카메라 회전 상태도 동기화
		UIManager.UpdateMouseRotation(CamData.Rotation.Y, CamData.Rotation.Z);

        // 프로젝션 파라미터
        Cam->SetFOV(CamData.FOV);
        Cam->SetClipPlanes(CamData.NearClip, CamData.FarClip);

		// UI 위젯에 현재 카메라 상태로 재동기화 요청
		UIManager.SyncCameraControlFromCamera();
    }

	// 1) 현재 월드에서 이미 사용 중인 UUID 수집(엔진 액터 + 기즈모)
	std::unordered_set<uint32> UsedUUIDs;
	auto AddUUID = [&](AActor* A) { if (A) UsedUUIDs.insert(A->UUID); };
	for (AActor* Eng : EngineActors) 
	{
		AddUUID(Eng);
	}
	AddUUID(GizmoActor); // Gizmo는 EngineActors에 안 들어갈 수 있으므로 명시 추가

	uint32 MaxAssignedUUID = 0;
	// 벌크 삽입을 위해 액터들을 먼저 모두 생성
	TArray<AActor*> SpawnedActors;
	SpawnedActors.reserve(Primitives.size());

	for (const FPrimitiveData& Primitive : Primitives)
	{
		// 스폰 시 필요한 초기 트랜스포은 그대로 넘김
		AStaticMeshActor* StaticMeshActor = SpawnActor<AStaticMeshActor>(
			FTransform(Primitive.Location,
				SceneRotUtil::QuatFromEulerZYX_Deg(Primitive.Rotation),
				Primitive.Scale));

		PushBackToStaticMeshActors(StaticMeshActor);

		// 스폰 시점에 자동 발급된 고유 UUID (충돌 시 폴백으로 사용)
		uint32 Assigned = StaticMeshActor->UUID;

		// 우선 스폰된 UUID를 사용 중으로 등록
		UsedUUIDs.insert(Assigned);

		// 2) 파일의 UUID를 우선 적용하되, 충돌이면 스폰된 UUID 유지
		if (Primitive.UUID != 0)
		{
			if (UsedUUIDs.find(Primitive.UUID) == UsedUUIDs.end())
			{
				// 스폰된 ID 등록 해제 후 교체
				UsedUUIDs.erase(Assigned);
				StaticMeshActor->UUID = Primitive.UUID;
				Assigned = Primitive.UUID;
				UsedUUIDs.insert(Assigned);
			}
			else
			{
				// 충돌: 파일 UUID 사용 불가 → 경고 로그 및 스폰된 고유 UUID 유지
				UE_LOG("LoadScene: UUID collision detected (%u). Keeping generated %u for actor.",
					Primitive.UUID, Assigned);
			}
		}

		MaxAssignedUUID = std::max(MaxAssignedUUID, Assigned);

		if (UStaticMeshComponent* SMC = StaticMeshActor->GetStaticMeshComponent())
		{
			FPrimitiveData Temp = Primitive;
			SMC->Serialize(true, Temp);

			FString LoadedAssetPath;
			if (UStaticMesh* Mesh = SMC->GetStaticMesh())
			{
				LoadedAssetPath = Mesh->GetAssetPathFileName();
			}

			if (LoadedAssetPath == "Data/Sphere.obj")
			{
				StaticMeshActor->SetCollisionComponent(EPrimitiveType::Sphere);
			}
			else
			{
				StaticMeshActor->SetCollisionComponent();
			}

			FString BaseName = "StaticMesh";
			if (!LoadedAssetPath.empty())
			{
				BaseName = RemoveObjExtension(LoadedAssetPath);
			}
			StaticMeshActor->SetName(GenerateUniqueActorName(BaseName));
		}
		
		//UWorldPartitionManager::GetInstance()->BulkRegister();
		// 벌크 삽입을 위해 목록에 추가
		SpawnedActors.push_back(StaticMeshActor);
	}
	
	// 모든 액터를 한 번에 벌크 등록 하여 성능 최적화
	if (UWorldPartitionManager::GetInstance() && !SpawnedActors.empty())
	{
		UE_LOG("LoadScene: Using bulk registration for %zu actors\r\n", SpawnedActors.size());
		UWorldPartitionManager::GetInstance()->BulkRegister(SpawnedActors);
	}

	// 3) 최종 보정: 전역 카운터는 절대 하향 금지 + 현재 사용된 최대값 이후로 설정
	const uint32 DuringLoadNext = UObject::PeekNextUUID();
	const uint32 SafeNext = std::max({ DuringLoadNext, MaxAssignedUUID + 1, PreLoadNext });
	UObject::SetNextUUID(SafeNext);
}

void UWorld::SaveScene(const FString& SceneName)
{
	TArray<FPrimitiveData> Primitives;

    for (AActor* Actor : Actors)
    {
        if (AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor))
        {
            FPrimitiveData Data;
            Data.UUID = Actor->UUID;
            Data.Type = "StaticMeshComp";
            if (UStaticMeshComponent* SMC = MeshActor->GetStaticMeshComponent())
            {
                SMC->Serialize(false, Data); // 여기서 RotUtil 적용됨(상위 Serialize)
            }
            Primitives.push_back(Data);
        }
        else
        {
            FPrimitiveData Data;
            Data.UUID = Actor->UUID;
            Data.Type = "Actor";

            if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
            {
                Prim->Serialize(false, Data); // 여기서 RotUtil 적용됨
            }
            else
            {
                // 루트가 Primitive가 아닐 때도 동일 규칙으로 저장
                Data.Location = Actor->GetActorLocation();
                Data.Rotation = SceneRotUtil::EulerZYX_Deg_FromQuat(Actor->GetActorRotation());
                Data.Scale = Actor->GetActorScale();
            }

            Data.ObjStaticMeshAsset.clear();
            Primitives.push_back(Data);
        }
    }

    // 카메라 데이터 채우기
    const FPerspectiveCameraData* CamPtr = nullptr;
    FPerspectiveCameraData CamData;
    if (MainCameraActor && MainCameraActor->GetCameraComponent())
    {
        UCameraComponent* Cam = MainCameraActor->GetCameraComponent();

        CamData.Location = MainCameraActor->GetActorLocation();

        // 내부 누적 각도로 저장: Pitch=Y, Yaw=Z, Roll=0
        CamData.Rotation.X = 0.0f;
        CamData.Rotation.Y = MainCameraActor->GetCameraPitch();
        CamData.Rotation.Z = MainCameraActor->GetCameraYaw();

        CamData.FOV = Cam->GetFOV();
        CamData.NearClip = Cam->GetNearClip();
        CamData.FarClip = Cam->GetFarClip();
        CamPtr = &CamData;
    }

    // Scene 디렉터리에 저장
    FSceneLoader::Save(Primitives, CamPtr, SceneName);
}

AGizmoActor* UWorld::GetGizmoActor()
{
	return GizmoActor;
}

// === World.cpp 패치: 그리드 리사이즈 ===
void UWorld::UpdateOcclusionGridSizeForViewport(FViewport* Viewport)
{
	if (!Viewport) return;
	int vw = (1 > Viewport->GetSizeX()) ? 1 : Viewport->GetSizeX();
	int vh = (1 > Viewport->GetSizeY()) ? 1 : Viewport->GetSizeY();
	int gw = std::max(1, vw / std::max(1, OcclGridDiv));
	int gh = std::max(1, vh / std::max(1, OcclGridDiv));
	// 매 프레임 호출해도 싸다. 내부에서 동일크기면 skip
	OcclusionCPU.Initialize(gw, gh);
}

// === World.cpp 패치: 후보 수집 ===
void UWorld::BuildCpuOcclusionSets(
	const Frustum& ViewFrustum,
	const FMatrix& View, const FMatrix& Proj,
	TArray<FCandidateDrawable>& OutOccluders,
	TArray<FCandidateDrawable>& OutOccludees)
{
	OutOccluders.clear();
	OutOccludees.clear();

	const FMatrix VP = View * Proj;

	// 간단 정책:
	//  - 화면 투영 면적이 큰 상위 물체를 오클루더로(보수적으로는 '모두'를 오클루더로 써도 됨)
	//  - 본문은 모두 Occludee에 넣음
	//  - 둘 다 동일 세트로 써도 동작 (속도-정확도는 씬에 맞춰 조절)
	for (AActor* Actor : Actors)
	{
		if (!Actor) continue;
		if (Actor->GetActorHiddenInGame()) continue;
		if (Actor->GetCulled()) continue; // 프러스텀 제외

		AStaticMeshActor* SMA = Cast<AStaticMeshActor>(Actor);
		if (!SMA) continue;

		// 충돌/바운딩 박스 컴포넌트
		UAABoundingBoxComponent* Box = Cast<UAABoundingBoxComponent>(SMA->CollisionComponent);
		if (!Box) continue;

		FCandidateDrawable C{};
		C.ActorIndex = Actor->UUID;          // UUID로 직접 인덱싱
		C.Bound = Box->GetWorldBound(); // Min/Max 보유
		C.WorldViewProj = VP;

		// 간단히 모두를 양쪽에 다 넣자(먼저 전체 동작 확인 → 이후 Top-K 정책으로 최적화)
		OutOccluders.push_back(C);
		OutOccludees.push_back(C);
	}
}