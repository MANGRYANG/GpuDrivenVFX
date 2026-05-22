// Billboard 변환 버퍼를 0번 슬롯에 바인딩
cbuffer TransformBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
};

// Billboard 생성 정보 버퍼를 1번 슬롯에 바인딩
cbuffer BillboardInfoBuffer : register(b1)
{
    float3 cameraRight;
    float billboardSize;

    float3 cameraUp;
    float padding;
    
    float4 billboardColor;
};

// Billboard Vertex Buffer으로부터 입력받는 정점의 데이터 구조체
struct VSInput
{
    // Billboard의 로컬 공간 중심점 위치 (X, Y, Z)
    float3 Center : POSITION;
    // Billboard의 중심점을 기준으로 Quad의 각 정점이 펼쳐질 방향 (X, Y)
    float2 Corner : TEXCOORD;
};

// Vertex Shader 연산 후 래스터라이저로 넘길 데이터 구조체
struct VSOutput
{
    // 4차원 동차 좌표계 위치 (X, Y, Z, W)
    float4 Pos : SV_POSITION;
    // Pixel Shader로 전달할 색상 데이터 (R, G, B, A)
    float4 Color : COLOR;
};

// 각 Billboard 정점마다 실행되는 정점 셰이더의 Entry Point
VSOutput VS_Main(VSInput input)
{
    VSOutput output;

    // Billboard 중심점 위치를 로컬 공간에서 월드 공간으로 변환
    float4 centerWorld = mul(float4(input.Center, 1.0f), world);

    // 카메라의 Right/Up 기저를 통해 Billboard Quad 정점의 월드 좌표 계산
    float3 worldPosition =
        centerWorld.xyz
        + cameraRight * input.Corner.x * billboardSize
        + cameraUp * input.Corner.y * billboardSize;

    // 변환 행렬 계산을 위한 동차 좌표계 전환
    float4 position = float4(worldPosition, 1.0f);

    // 월드 좌표계에서 뷰 좌표계로의 변환 수행
    position = mul(position, view);
    // 뷰 좌표계에서 투영 좌표계로의 변환 수행
    position = mul(position, projection);

    // 정점 위치 데이터 갱신
    output.Pos = position;
    // Billboard 색상을 Pixel Shader로 전달
    output.Color = billboardColor;

    return output;
}