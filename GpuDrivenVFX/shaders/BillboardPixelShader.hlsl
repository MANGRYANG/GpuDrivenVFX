// 래스터라이저에서 보간되어 전달된 Billboard 정점의 데이터 구조체
struct VSOutput
{
    // 각 픽셀에 대한 4차원 동차 좌표계 위치 (X, Y, Z, W)
    float4 Pos : SV_POSITION;
    // 각 픽셀 위치에 맞게 보간된 색상 데이터 (R, G, B, A)
    float4 Color : COLOR;
};

// 각 픽셀마다 독립적으로 실행되는 픽셀 셰이더의 Entry Point
float4 PS_Main(VSOutput input) : SV_TARGET
{
    // 색상 데이터를 변환하지 않고 반환
    return input.Color;
}