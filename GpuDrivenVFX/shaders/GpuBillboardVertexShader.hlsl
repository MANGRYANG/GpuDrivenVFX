// GPU Particle 데이터 구조체
struct GpuParticleData
{
    // Particle의 월드 공간 위치
    float3 position;
    // Particle을 Billboard로 렌더링할 때 사용할 크기
    float size;

    // Particle의 이동 속도
    float3 velocity;
    // Particle의 총 생존 시간
    float lifetime;

    // Particle을 Billboard로 렌더링할 때 사용할 색상
    float4 color;

    // Particle이 생성된 뒤 경과한 시간
    float age;
    // 현재 Particle이 활성 상태인지 여부
    uint active;
    // GPU 구조체 정렬을 맞추기 위한 패딩
    float2 padding;
};

// GPU Billboard 렌더링에 사용할 카메라 변환 버퍼를 0번 슬롯에 바인딩
cbuffer CameraBuffer : register(b0)
{
    matrix view;
    matrix projection;
};

// GPU Particle Buffer를 정점 셰이더에서 읽기 위한 Structured Buffer
StructuredBuffer<GpuParticleData> particles : register(t0);

// active Particle의 원본 Particle 인덱스 목록
StructuredBuffer<uint> aliveIndices : register(t1);

// GPU Billboard Vertex Buffer로부터 입력받는 데이터 구조체
struct VSInput
{
    // Billboard 중심점을 기준으로 Quad의 각 정점이 펼쳐질 방향 (X, Y)
    float2 Corner : TEXCOORD0;
};

// Vertex Shader 연산 후 래스터라이저로 넘길 데이터 구조체
struct VSOutput
{
    // 4차원 동차 좌표계 위치 (X, Y, Z, W)
    float4 Pos : SV_POSITION;
    // Pixel Shader로 전달할 색상 데이터 (R, G, B, A)
    float4 Color : COLOR;
};

// 각 GPU Billboard 정점마다 실행되는 정점 셰이더의 Entry Point
VSOutput VS_Main(VSInput input, uint instanceId : SV_InstanceID)
{
    VSOutput output;
    
    // 현재 인스턴스에 대응하는 active Particle의 원본 Particle 인덱스 조회
    uint particleIndex = aliveIndices[instanceId];

    // active Particle 인덱스를 통해 GPU Particle 데이터 조회
    GpuParticleData particle = particles[particleIndex];

    // Billboard 중심점을 월드 공간에서 뷰 공간으로 변환
    float4 centerView = mul(float4(particle.position, 1.0f), view);

    // 뷰 공간의 X/Y 평면에서 Quad를 확장해 카메라를 향하는 Billboard를 구성
    float4 positionView = centerView;
    positionView.xy += input.Corner.xy * particle.size;

    // 뷰 공간 위치를 투영 좌표계로 변환
    output.Pos = mul(positionView, projection);

    // Particle 색상을 Pixel Shader로 전달
    output.Color = particle.color;

    return output;
}