// b0: ModelBuffer (VS) - Matches ModelBufferType exactly (128 bytes)
cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix; // 64 bytes
    row_major float4x4 WorldInverseTranspose; // 64 bytes - For correct normal transformation
};

// b1: ViewProjBuffer (VS) - Matches ViewProjBufferType
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
};

cbuffer ColorId : register(b3)
{
    float4 Color;
    uint UUID;
}
struct VS_INPUT
{
    float3 localPos : POSITION;   // quad local offset (-0.5~0.5)
    float2 uv       : TEXCOORD0;  // per-vertex UV
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
    uint UUID : SV_Target1;
};

Texture2D BillboardTex : register(t0);
SamplerState LinearSamp : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT o;
    
    // 1. 뷰별 데이터(b1)에서 InverseViewMatrix 읽기
    float3 posAligned = mul(float4(input.localPos, 0.0f), InverseViewMatrix).xyz;
    
    // 2. 객체별 데이터(b0)의 WorldMatrix에서 월드 위치 추출
    // (row_major 행렬의 4번째 행이 위치(translation) 정보)
    float3 worldCenterPos = WorldMatrix[3].xyz;
    float3 worldPos = worldCenterPos + posAligned;

    // 3. 뷰별 데이터(b1)에서 View/Projection 행렬 읽기
    o.pos = mul(float4(worldPos, 1.0f), mul(ViewMatrix, ProjectionMatrix));
    o.uv = input.uv;
    return o;
}

PS_OUTPUT mainPS(PS_INPUT i) 
{
    PS_OUTPUT Output;
    
    float4 c = BillboardTex.Sample(LinearSamp, i.uv);
    if (c.a < 0.1f)
        discard;
    c = c * Color;
    Output.Color = c;
    Output.UUID = UUID;
    return Output;
}
