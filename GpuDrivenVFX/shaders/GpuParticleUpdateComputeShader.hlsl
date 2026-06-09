// GPU Particle 데이터 구조체
struct GpuParticleData
{
    float3 position;
    float size;

    float3 velocity;
    float lifetime;

    float4 color;

    float age;
    uint active;
    float orbitRadius;
    float orbitAngle;

    float angularVelocity;
    float radialVelocity;
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

    uint spawnSequenceStart;
    uint spiralArmCount;
    float orbitStartRadius;
    float orbitAngularVelocity;

    float orbitRadialVelocity;
    float spiralDepthScale;
    float spiralDepthFrequency;
    float fadeOutStartRatio;

    float4 spiralRight;
    
    float4 spiralUp;
    
    float4 spiralForward;
};

// Compute Shader에서 읽고 쓸 GPU Particle Buffer
RWStructuredBuffer<GpuParticleData> particles : register(u0);

// active Particle의 원본 Particle 인덱스를 기록할 Buffer
RWStructuredBuffer<uint> aliveIndices : register(u1);

// 현재 프레임의 active Particle 개수를 기록할 Buffer
RWStructuredBuffer<uint> aliveCount : register(u2);

// 2 PI 근사
static const float TwoPi = 6.28318530718f;

// Spiral Arm을 균등 분배하기 위한 계산기
float CalculateSpiralArmBaseAngle(uint armIndex)
{
    return (float) armIndex * (TwoPi / (float) spiralArmCount);
}

// 중심 방출형 파티클 평면에 대한 회전을 적용하기 위한 함수
float3 RotateSpiralPosition(float3 localPosition)
{
    return (localPosition.x * spiralRight.xyz) + (localPosition.y * spiralUp.xyz) + (localPosition.z * spiralForward.xyz);
}

// 중심 방출형 Spiral Particle의 로컬 위치 계산
float3 CalculateSpiralLocalPosition(float orbitAngle, float orbitRadius, float age)
{
    float localX = cos(orbitAngle) * orbitRadius;
    float localY = sin(orbitAngle) * orbitRadius;

    float zOffset = sin(orbitAngle * spiralDepthFrequency + age * spiralDepthFrequency) * (orbitRadius * spiralDepthScale);

    return RotateSpiralPosition(float3(localX, localY, zOffset));
}

// fade-out 효과를 위한 알파 값 계산 함수
float CalculateFadeOutAlpha(float normalizedAge)
{
    // 생존 시간이 기준 이하인 경우 투명도 적용하지 않음
    if (normalizedAge <= fadeOutStartRatio)
    {
        return 1.0f;
    }

    // 투명도가 적용되는 구간 계산
    float fadeDuration = 1.0f - fadeOutStartRatio;
    // 투명 진행도 계산
    float fadeProgress = (normalizedAge - fadeOutStartRatio) / fadeDuration;

    // 최종 알파 값 계산 (0-1 구간으로 클램핑)
    return saturate(1.0f - fadeProgress);
}

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
            // 수명 비율 계산
            float normalizedAge = particle.age / particle.lifetime;
            
            // 수명 비율에 따라 바깥쪽 Particle의 회전 속도를 점진적으로 감속
            float speedFactor = 1.0f - normalizedAge;
            
            // 중심 회전형 Particle의 각도와 반지름 갱신
            particle.orbitAngle += (particle.angularVelocity * speedFactor) * deltaTime;
            particle.orbitRadius += particle.radialVelocity * deltaTime;
            
            // 중심점을 기준으로 회전 좌표를 계산하여 Particle 위치 갱신
            float3 rotatedPosition = CalculateSpiralLocalPosition(particle.orbitAngle, particle.orbitRadius, particle.age);

            particle.position = emitterPosition + rotatedPosition;
            
            // 파티클에 적용할 알파 값 계산
            float fadeAlpha = CalculateFadeOutAlpha(normalizedAge);
            // 계산된 알파 값 적용
            particle.color.a = emitterColor.a * fadeAlpha;
        }

        // 갱신된 Particle 데이터를 GPU Buffer에 다시 기록
        particles[particleIndex] = particle;
    }
    
    bool shouldSpawn = false;
    uint spawnOffset = 0;

    // Ring Buffer 방식으로 spawnStartIndex부터 spawnCount개 슬롯에 새 Particle 생성
    uint spawnEndIndex = spawnStartIndex + spawnCount;

    if (spawnCount > 0)
    {
        // 배열 끝을 넘지 않는 경우
        if (spawnEndIndex <= particleCount)
        {
            shouldSpawn = (particleIndex >= spawnStartIndex) && (particleIndex < spawnEndIndex);

            if (shouldSpawn)
            {
                spawnOffset = particleIndex - spawnStartIndex;
            }
        }

        // 배열 끝을 넘어서 맨 앞으로 돌아가는 경우
        else
        {
            uint wrappedEndIndex = spawnEndIndex % particleCount;

            if (particleIndex >= spawnStartIndex)
            {
                shouldSpawn = true;
                spawnOffset = particleIndex - spawnStartIndex;
            }
            else if (particleIndex < wrappedEndIndex)
            {
                shouldSpawn = true;
                spawnOffset = (particleCount - spawnStartIndex) + particleIndex;
            }
        }
    }

    // Spawn 대상 슬롯이면 새 Particle 데이터로 덮어쓰기
    if (shouldSpawn)
    {
        uint spawnSequence = spawnSequenceStart + spawnOffset;
        uint armIndex = spawnSequence % spiralArmCount;

        float orbitAngle = CalculateSpiralArmBaseAngle(armIndex);

        particle.orbitRadius = orbitStartRadius;
        particle.orbitAngle = orbitAngle;
        particle.angularVelocity = orbitAngularVelocity;
        particle.radialVelocity = orbitRadialVelocity;

        particle.position = emitterPosition +
            float3
            (
                cos(particle.orbitAngle) * particle.orbitRadius,
                sin(particle.orbitAngle) * particle.orbitRadius,
                0.0f
            );

        particle.size = particleSize;
        particle.velocity = emitterVelocity;
        particle.lifetime = particleLifetime;
        particle.color = emitterColor;
        particle.age = 0.0f;
        particle.active = 1;
        particle.padding = float2(0.0f, 0.0f);
    }

    // 새 Particle 데이터를 GPU Buffer에 기록
    particles[particleIndex] = particle;
    
    // 최종 상태가 active이면 alive index list에 현재 Particle 인덱스 기록
    if (particle.active != 0)
    {
        uint writeIndex = 0;

        // active Particle 개수를 Atomic하게 1 증가시키고, 기록할 위치 획득
        InterlockedAdd(aliveCount[0], 1, writeIndex);

        // Particle 개수 범위 안에서만 기록
        if (writeIndex < particleCount)
        {
            aliveIndices[writeIndex] = particleIndex;
        }
    }
}