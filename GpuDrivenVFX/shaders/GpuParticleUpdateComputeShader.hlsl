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

// GPU Particle 업데이트 상수 버퍼
cbuffer ParticleUpdateBuffer : register(b0)
{
    float3 emitterPosition;
    float particleSize;

    float3 emitterVelocity;
    float particleLifetime;

    float4 emitterColor;

    float deltaTime;
    uint particleCount;
    uint spawnStartIndex;
    uint spawnCount;
};

// Compute Shader에서 읽고 쓸 GPU Particle Buffer
RWStructuredBuffer<GpuParticleData> particles : register(u0);

// 하나의 thread group에서 64개의 Particle 슬롯을 처리
[numthreads(64, 1, 1)]
void CS_Main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    // 현재 thread가 담당할 Particle 인덱스
    uint particleIndex = dispatchThreadId.x;

    // Particle 범위를 벗어난 thread는 처리하지 않음
    if (particleIndex >= particleCount)
    {
        return;
    }

    // 현재 Particle 데이터 읽기
    GpuParticleData particle = particles[particleIndex];

    // 활성 Particle의 수명 및 위치 갱신
    if (particle.active != 0)
    {
        // Particle 생존 시간 누적
        particle.age += deltaTime;

        // 수명이 끝난 Particle은 비활성화
        if (particle.age >= particle.lifetime)
        {
            particle.active = 0;
        }
        else
        {
            // Particle 위치를 속도와 deltaTime 기반으로 갱신
            particle.position += particle.velocity * deltaTime;
        }

        // 갱신된 Particle 데이터를 GPU Buffer에 다시 기록
        particles[particleIndex] = particle;
    }
    
    // 이번 프레임에 Spawn할 Particle 슬롯인지 확인
    bool shouldSpawn = false;

    // Ring Buffer 방식으로 spawnStartIndex부터 spawnCount개 슬롯에 새 Particle 생성
    for (uint spawnOffset = 0; spawnOffset < spawnCount; ++spawnOffset)
    {
        uint targetIndex = (spawnStartIndex + spawnOffset) % particleCount;

        if (particleIndex == targetIndex)
        {
            shouldSpawn = true;
            break;
        }
    }

    if (!shouldSpawn)
    {
        return;
    }

    // 슬롯 인덱스를 사용해 수평 퍼짐 부여
    float spread = (float) (particleIndex % 5);

    GpuParticleData spawnedParticle;
    spawnedParticle.position = emitterPosition + float3(spread * 0.03f, 0.0f, 0.0f);
    spawnedParticle.size = particleSize;
    spawnedParticle.velocity = emitterVelocity + float3(spread * 0.03f, 0.0f, 0.0f);
    spawnedParticle.lifetime = particleLifetime;
    spawnedParticle.color = emitterColor;
    spawnedParticle.age = 0.0f;
    spawnedParticle.active = 1;
    spawnedParticle.padding = float2(0.0f, 0.0f);

    // 새 Particle 데이터를 GPU Buffer에 기록
    particles[particleIndex] = spawnedParticle;
}