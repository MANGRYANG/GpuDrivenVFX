// 변환 상수 버퍼를 0번 슬롯에 바인딩
cbuffer TransformBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
};

// Vertex Buffer으로부터 입력받는 정점의 데이터 구조체
struct VSInput
{
    // 3차원 로컬 공간에서의 정점 위치 (X, Y, Z)
    float3 Pos : POSITION;
    // 정점에 대한 노멀 벡터 (X, Y, Z)
    float3 Normal : NORMAL;
    // 정점 고유의 색상 데이터 (R, G, B, A)
    float4 Color : COLOR;
};

// Vertex Shader 연산을 마친 후 래스터라이저로 넘길 데이터 구조체
struct VSOutput
{
    // 4차원 동차 좌표계 위치 (X, Y, Z, W)
    float4 Pos : SV_POSITION;
    // Vertex Shader 연산을 마친 노멀 벡터 (X, Y, Z)
    float3 Normal : NORMAL;
    // Vertex Shader 연산을 마친 색상 데이터 (R, G, B, A)
    float4 Color : COLOR;
};

// 각 정점마다 독립적으로 실행되는 정점 셰이더의 Entry Point
VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    
    // 변환 행렬 계산을 위한 동차 좌표계 전환
    float4 position = float4(input.Pos, 1.0f);

    // 로컬 좌표계에서 월드 좌표계로의 변환 수행
    position = mul(position, world);
    // 월드 좌표계에서 뷰 좌표계로의 변환 수행
    position = mul(position, view);
    // 뷰 좌표계에서 투영 좌표계로의 변환 수행
    position = mul(position, projection);
    
    // 정점 위치 데이터 갱신
    output.Pos = position;
    // 정점 노말 벡터 데이터는 변환하지 않음
    output.Normal = input.Normal;
    // 정점 색상 데이터는 변환하지 않음
    output.Color = input.Color;
    
    return output;
}