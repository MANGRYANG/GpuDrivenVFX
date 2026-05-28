// CPU Billboard 렌더링에 사용할 카메라 변환 버퍼를 0번 슬롯에 바인딩
cbuffer CameraBuffer : register(b0)
{
    matrix view;
    matrix projection;
};

// CPU Billboard Vertex Buffer와 Instance Buffer로부터 입력받는 데이터 구조체
struct VSInput
{
    // Billboard 중심점을 기준으로 Quad의 각 정점이 펼쳐질 방향 (X, Y)
    float2 Corner : TEXCOORD0;

    // Billboard 인스턴스의 월드 공간 중심 위치
    float3 InstancePosition : POSITION;
    // Billboard 인스턴스의 크기
    float InstanceSize : TEXCOORD1;
    // Billboard 인스턴스의 색상
    float4 InstanceColor : COLOR;
};

// Vertex Shader 연산 후 래스터라이저로 넘길 데이터 구조체
struct VSOutput
{
    // 4차원 동차 좌표계 위치 (X, Y, Z, W)
    float4 Pos : SV_POSITION;
    // Pixel Shader로 전달할 색상 데이터 (R, G, B, A)
    float4 Color : COLOR;
};

// 각 CPU Billboard 정점마다 실행되는 정점 셰이더의 Entry Point
VSOutput VS_Main(VSInput input)
{
    VSOutput output;

    // Billboard 중심점을 월드 공간에서 뷰 공간으로 변환
    float4 centerView = mul(float4(input.InstancePosition, 1.0f), view);

    // 뷰 공간의 XY 평면에서 Quad를 확장해 카메라를 향하는 Billboard를 구성
    float4 positionView = centerView;
    positionView.xy += input.Corner.xy * input.InstanceSize;

    // 뷰 공간 위치를 투영 좌표계로 변환
    output.Pos = mul(positionView, projection);

    // 인스턴스 색상을 Pixel Shader로 전달
    output.Color = input.InstanceColor;

    return output;
}