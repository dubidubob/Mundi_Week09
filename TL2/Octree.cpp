﻿#include "pch.h"
#include "Octree.h"

FOctree::FOctree(const FBound& InBounds, int InDepth, int InMaxDepth, int InMaxObjects)
	: Bounds(InBounds), Depth(InDepth), MaxDepth(InMaxDepth), MaxObjects(InMaxObjects)
{
	// 안전하게 nullpter 
	for (int i = 0; i < 8; i++)
	{
		Children[i] = nullptr;
	}
}

FOctree::~FOctree()
{
	for (int i = 0; i < 8; i++)
	{
		if (Children[i])
		{
			delete Children[i];
			Children[i] = nullptr;
		}
	}
}

// 런타임 중에 옥트리 초기화 해야할 때 !
void FOctree::Clear()  
{
	// 액터 배열 정리
	Actors.Empty();

	// 자식들도 재귀적으로 정리
	for (int i = 0; i < 8; i++)
	{
		if (Children[i])
		{
			delete Children[i];
			Children[i] = nullptr;
		}
	}
}

// 벌크 삽입 최적화 - 대량 액터 처리용
void FOctree::BulkInsert(const TArray<std::pair<AActor*, FBound>>& ActorsAndBounds)
{
    if (ActorsAndBounds.empty()) return;
    
    // 모든 액터를 대상 영역에 바로 추가
    Actors.reserve(Actors.size() + ActorsAndBounds.size());
    for (const auto& ActorBoundPair : ActorsAndBounds)
    {
        Actors.push_back(ActorBoundPair.first);
        ActorLastBounds[ActorBoundPair.first] = ActorBoundPair.second;
    }
    
    // 임계값 초과 시 분할 시작
    if (Actors.size() > MaxObjects && Depth < MaxDepth)
    {
        if (!Children[0])
        {
            Split();
        }
        
        // 대량 데이터를 옥탄트별로 그룹화
        TArray<std::pair<AActor*, FBound>> OctantGroups[8];
        
        auto It = Actors.begin();
        while (It != Actors.end())
        {
            AActor* ActorPtr = *It;
            auto CachedIt = ActorLastBounds.find(ActorPtr);
            FBound Box = (CachedIt != ActorLastBounds.end()) ? CachedIt->second : ActorPtr->GetBounds();
            
            int OptimalOctant = GetOctantIndex(Box);
            if (CanFitInOctant(Box, OptimalOctant))
            {
                OctantGroups[OptimalOctant].push_back({ActorPtr, Box});
                It = Actors.erase(It);
            }
            else
            {
                // 다른 옥탄트 처리
                bool bMoved = false;
                for (int i = 0; i < 8; i++)
                {
                    if (i != OptimalOctant && Children[i]->Contains(Box))
                    {
                        OctantGroups[i].push_back({ActorPtr, Box});
                        It = Actors.erase(It);
                        bMoved = true;
                        break;
                    }
                }
                if (!bMoved) ++It;
            }
        }
        
        // 옥탄트별로 재귀 벌크 삽입
        for (int i = 0; i < 8; i++)
        {
            if (!OctantGroups[i].empty())
            {
                Children[i]->BulkInsert(OctantGroups[i]);
            }
        }
    }
}

void FOctree::Insert(AActor* InActor, const FBound& ActorBounds)
{
    // 마지막 바운드 캐시 갱신
    ActorLastBounds[InActor] = ActorBounds;

    // 현재 노드가 자식이 있는 경우 → 최적화된 옥탄트 계산
    if (Children[0])
    {
        int OptimalOctant = GetOctantIndex(ActorBounds);
        if (CanFitInOctant(ActorBounds, OptimalOctant))
        {
            Children[OptimalOctant]->Insert(InActor, ActorBounds);
            return;
        }
        // 최적 옥탄트에 맞지 않으면 다른 옥탄트 확인 (드문 경우)
        for (int i = 0; i < 8; i++)
        {
            if (i != OptimalOctant && Children[i]->Contains(ActorBounds))
            {
                Children[i]->Insert(InActor, ActorBounds);
                return;
            }
        }
    }

    // 자식에 못 들어가면 현재 노드에 저장
    // 자식에 넣지 못하는 객체를 가질 수 도 있다. 
    Actors.push_back(InActor);

    // 분할 조건 체크
    // 현재 노드에 들어있는 객체 수가 Max초과 , 최대 깊이 보다 얕은 레벨이면 스플릿 
    if (Actors.size() > MaxObjects && Depth < MaxDepth)
    {
        if (!Children[0])
        {
            Split();
        }

        // 재분배 - 최적화된 옥탄트 계산 사용
        auto It = Actors.begin();
        while (It != Actors.end())
        {
            AActor* ActorPtr = *It;
            // 캐시된 바운드 사용 (이미 ActorLastBounds에 저장됨)
            auto CachedIt = ActorLastBounds.find(ActorPtr);
            FBound Box = (CachedIt != ActorLastBounds.end()) ? CachedIt->second : ActorPtr->GetBounds();

            bool bMoved = false;
            // 최적 옥탄트부터 시도
            int OptimalOctant = GetOctantIndex(Box);
            if (CanFitInOctant(Box, OptimalOctant))
            {
                Children[OptimalOctant]->Insert(ActorPtr, Box);
                It = Actors.erase(It);
                bMoved = true;
            }
            else
            {
                // 최적 옥탄트에 맞지 않으면 다른 옥탄트 확인
                for (int i = 0; i < 8; i++)
                {
                    if (i != OptimalOctant && Children[i]->Contains(Box))
                    {
                        Children[i]->Insert(ActorPtr, Box);
                        It = Actors.erase(It);
                        bMoved = true;
                        break;
                    }
                }
            }

            if (!bMoved) ++It;
        }
    }
}
bool FOctree::Contains(const FBound& Box) const
{
    return Bounds.Contains(Box);
}

bool FOctree::Remove(AActor* InActor, const FBound& ActorBounds)
{
    // 현재 노드에 있는 경우
    auto It = std::find(Actors.begin(), Actors.end(), InActor);
    if (It != Actors.end())
    {
        Actors.erase(It);
        // 캐시 제거
        ActorLastBounds.erase(InActor);
        return true;
    }

    // 자식 노드에 위임 - 최적화된 옥탄트 계산
    if (Children[0])
    {
        int OptimalOctant = GetOctantIndex(ActorBounds);
        if (CanFitInOctant(ActorBounds, OptimalOctant))
        {
            return Children[OptimalOctant]->Remove(InActor, ActorBounds);
        }
        // 최적 옥탄트에 없으면 다른 옥탄트 확인
        for (int i = 0; i < 8; i++)
        {
            if (i != OptimalOctant && Children[i]->Contains(ActorBounds))
            {
                return Children[i]->Remove(InActor, ActorBounds);
            }
        }
    }
    return false;
}

void FOctree::Update(AActor* InActor, const FBound& OldBounds, const FBound& NewBounds)
{
    Remove(InActor, OldBounds);
    Insert(InActor, NewBounds);
}

void FOctree::Remove(AActor* InActor)
{
    auto it = ActorLastBounds.find(InActor);
    if (it != ActorLastBounds.end())
    {
        Remove(InActor, it->second);
        // Remove에서 캐시도 정리됨
    }
}

void FOctree::Update(AActor* InActor)
{
    auto it = ActorLastBounds.find(InActor);
    if (it != ActorLastBounds.end())
    {
        Update(InActor, it->second, InActor->GetBounds());
    }
    else
    {
        Insert(InActor, InActor->GetBounds());
    }
}


// 최적화된 옥탄트 계산 - Contains() 대신 사용
int FOctree::GetOctantIndex(const FBound& ActorBounds) const
{
    FVector Center = Bounds.GetCenter();
    FVector ActorCenter = ActorBounds.GetCenter();
    
    int OctantIndex = 0;
    if (ActorCenter.X >= Center.X) OctantIndex |= 1;
    if (ActorCenter.Y >= Center.Y) OctantIndex |= 2; 
    if (ActorCenter.Z >= Center.Z) OctantIndex |= 4;
    
    return OctantIndex;
}

bool FOctree::CanFitInOctant(const FBound& ActorBounds, int OctantIndex) const
{
    if (!Children[0]) return false; // 자식이 없으면 확인 불가
    return Children[OctantIndex]->Contains(ActorBounds);
}

void FOctree::Split()
{
    FVector Center = Bounds.GetCenter();
    FVector Extent = Bounds.GetExtent() * 0.5f;

    for (int i = 0; i < 8; i++)
    {
        FBound ChildBounds = Bounds.CreateOctant(i);

        Children[i] = new FOctree(ChildBounds, Depth + 1, MaxDepth, MaxObjects);
    }
}

int FOctree::TotalNodeCount() const
{
    // 자신 노드 
    int Count = 1; 
    if (Children[0])
    {
        for (int i = 0; i < 8; ++i)
        {
            if (Children[i]) Count += Children[i]->TotalNodeCount();
        }
    }
    return Count;
}

int FOctree::TotalActorCount() const
{
    int Count = Actors.Num();
    if (Children[0])
    {
        for (int i = 0; i < 8; ++i)
        {
            if (Children[i]) Count += Children[i]->TotalActorCount();
        }
    }
    return Count;
}

int FOctree::MaxOccupiedDepth() const
{
    int MaxD = Depth;
    if (Children[0])
    {
        for (int i = 0; i < 8; ++i)
        {
            if (Children[i])
            {
                int ChildDepth = Children[i]->MaxOccupiedDepth();
                MaxD = (ChildDepth > MaxD) ? ChildDepth : MaxD;
            }
        }
    }
    return MaxD;
}
void FOctree::DebugDump() const
{
    UE_LOG("===== OCTREE DUMP BEGIN =====\r\n");
    // iterative DFS to access private members safely inside class method
    struct StackItem { const FOctree* Node; int D; };
    TArray<StackItem> stack;
    stack.push_back({ this, Depth });
    while (!stack.empty())
    {
        StackItem it = stack.back();
        stack.pop_back();

        const FOctree* N = it.Node;
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "[Octree] depth=%d, actors=%zu, bounds=[(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f)]\r\n",
            it.D,
            N->Actors.size(),
            N->Bounds.Min.X, N->Bounds.Min.Y, N->Bounds.Min.Z,
            N->Bounds.Max.X, N->Bounds.Max.Y, N->Bounds.Max.Z);
        UE_LOG(buf);

        if (N->Children[0])
        {
            for (int i = 7; i >= 0; --i)
            {
                if (N->Children[i])
                {
                    stack.push_back({ N->Children[i], it.D + 1 });
                }
            }
        }
    }
    UE_LOG("===== OCTREE DUMP END =====\r\n");
}

static void CreateLineDataFromAABB(
    const FVector& Min, const FVector& Max,
    OUT TArray<FVector>& Start,
    OUT TArray<FVector>& End,
    OUT TArray<FVector4>& Color,
    const FVector4& LineColor)
{
    const FVector v0(Min.X, Min.Y, Min.Z);
    const FVector v1(Max.X, Min.Y, Min.Z);
    const FVector v2(Max.X, Max.Y, Min.Z);
    const FVector v3(Min.X, Max.Y, Min.Z);
    const FVector v4(Min.X, Min.Y, Max.Z);
    const FVector v5(Max.X, Min.Y, Max.Z);
    const FVector v6(Max.X, Max.Y, Max.Z);
    const FVector v7(Min.X, Max.Y, Max.Z);

    Start.Add(v0); End.Add(v1); Color.Add(LineColor);
    Start.Add(v1); End.Add(v2); Color.Add(LineColor);
    Start.Add(v2); End.Add(v3); Color.Add(LineColor);
    Start.Add(v3); End.Add(v0); Color.Add(LineColor);

    Start.Add(v4); End.Add(v5); Color.Add(LineColor);
    Start.Add(v5); End.Add(v6); Color.Add(LineColor);
    Start.Add(v6); End.Add(v7); Color.Add(LineColor);
    Start.Add(v7); End.Add(v4); Color.Add(LineColor);

    Start.Add(v0); End.Add(v4); Color.Add(LineColor);
    Start.Add(v1); End.Add(v5); Color.Add(LineColor);
    Start.Add(v2); End.Add(v6); Color.Add(LineColor);
    Start.Add(v3); End.Add(v7); Color.Add(LineColor);
}

void FOctree::DebugDraw(URenderer* Renderer) const
{
    if (!Renderer) return;

    struct Item { const FOctree* Node; int D; };
    TArray<Item> st; st.push_back({ this, Depth });
    while (!st.empty())
    {
        Item it = st.back(); st.pop_back();
        const FOctree* N = it.Node;

        const int di = it.D % 3;
        FVector4 color(0.0f, 1.0f, 0.0f, 1.0f);
        if (di == 1) color = FVector4(0.2f, 0.8f, 1.0f, 1.0f);
        else if (di == 2) color = FVector4(1.0f, 0.6f, 0.1f, 1.0f);

        TArray<FVector> S, E; TArray<FVector4> C;
        CreateLineDataFromAABB(N->Bounds.Min, N->Bounds.Max, S, E, C, color);
        Renderer->AddLines(S, E, C);

        if (N->Children[0])
        {
            for (int i = 7; i >= 0; --i)
            {
                if (N->Children[i]) st.push_back({ N->Children[i], it.D + 1 });
            }
        }
    }
}
