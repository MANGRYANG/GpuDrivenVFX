#include "pch.h"
#include "cpu_particle_system.h"

#include <cmath>

namespace
{
    // 2 PI 근사
    constexpr float TwoPi = 6.28318530718f;

    // Spiral Arm을 균등 분배하기 위한 계산기
    float CalculateSpiralArmBaseAngle(std::uint32_t armIndex)
    {
        return static_cast<float>(armIndex) * (TwoPi / static_cast<float>(ParticleConfig::SpiralArmCount));
    }

    // 투명도 적용을 위한 계산기
    float CalculateFadeOutAlpha(float normalizedAge)
    {
        // 생존 시간이 기준 이하인 경우 투명도 적용하지 않음
        if (normalizedAge <= ParticleConfig::ParticleFadeOutStartRatio)
        {
            return 1.0f;
        }

        // 투명도가 적용되는 구간 계산
        const float fadeDuration = 1.0f - ParticleConfig::ParticleFadeOutStartRatio;
        // 투명 진행도 계산
        const float fadeProgress = (normalizedAge - ParticleConfig::ParticleFadeOutStartRatio) / fadeDuration;

        // 최종 알파 값 계산
        const float alpha = 1.0f - fadeProgress;

        // 0-1 사이의 값으로 클램핑
        if (alpha < 0.0f)
        {
            return 0.0f;
        }
        else if (alpha > 1.0f)
        {
            return 1.0f;
        }

        // Alpha 값의 제곱을 반환
        return alpha * alpha;
    }
}

void CpuParticleSystem::Initialize()
{
    // 기존 Particle 데이터 초기화
    m_particles.clear();
    // 기존 Billboard 데이터 초기화
    m_billboards.clear();
    // 생성 실패 카운터 초기화
    m_droppedSpawnCount = 0;

    // 테스트 단계에서 사용할 Particle 슬롯 공간 확보
    m_particles.resize(ParticleConfig::ParticleCapacity);

    // Emitter 상태 초기화
    m_emitter = ParticleEmitter
    {
        ParticleConfig::EmitterPosition,            // 초기 월드 공간 위치
        ParticleConfig::EmitterVelocity,            // 초기 이동 속도
        ParticleConfig::ParticleSize,               // 크기
        ParticleConfig::ParticleLifetime,           // 생존 시간
        ParticleConfig::SpawnRate,                  // 초당 생성해야 하는 Particle 개수
        0.0f,                                       // 프레임마다 누적되는 생성 요청 수
        0,                                          // 다음에 Particle을 생성할 순환 슬롯 인덱스
        0,                                          // Spiral Arm 배치를 계산하기 위한 Particle 생성 순서
        0.0f,                                       // Spiral Particle 방출 평면의 현재 회전 각도
        DirectX::XMFLOAT4(1.0f, 0.65f, 0.1f, 1.0f)  // Particle 색상
    };

    // 초기 Particle 데이터를 렌더링용 Billboard 목록으로 변환
    RebuildBillboards();
}

void CpuParticleSystem::Update(float deltaTime)
{
    // Spiral Particle 방출 평면의 회전 각도 갱신
    UpdateEmitterPlaneRotation(deltaTime);

    // 활성 Particle의 수명 및 위치 갱신
    UpdateParticles(deltaTime);

    // Emitter의 spawnRate를 기준으로 새 Particle 생성 
    EmitParticles(deltaTime);

    // 현재 Particle 상태를 렌더링용 Billboard 목록으로 변환
    RebuildBillboards();
}

const std::vector<Billboard>& CpuParticleSystem::GetBillboards() const
{
    return m_billboards;
}

std::size_t CpuParticleSystem::GetRenderParticleCount() const
{
    return m_billboards.size();
}

std::size_t CpuParticleSystem::GetDroppedSpawnCount() const
{
    return m_droppedSpawnCount;
}

std::size_t CpuParticleSystem::GetMaxParticleCount() const
{
    return ParticleConfig::ParticleCapacity;
}

bool CpuParticleSystem::SpawnParticle
(
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT3& velocity,
    float size,
    float lifetime,
    const DirectX::XMFLOAT4& color
)
{
    // 사용할 수 있는 Particle 슬롯이 없는 경우
    if (m_particles.empty())
    {
        ++m_droppedSpawnCount;
        return false;
    }

    // 이번 프레임에 생성할 Particle 슬롯 선택
    const std::size_t particleIndex = m_emitter.spawnIndex;
    Particle& particle = m_particles[particleIndex];

    // 다음 생성 슬롯을 순환 방식으로 갱신
    m_emitter.spawnIndex = (m_emitter.spawnIndex + 1) % m_particles.size();

    // 중심 방출형 Particle 데이터 설정
    particle.orbitRadius = ParticleConfig::OrbitStartRadius;
    particle.angularVelocity = ParticleConfig::OrbitAngularVelocity;
    particle.radialVelocity = ParticleConfig::OrbitRadialVelocity;

    // 생성 순서를 기반으로 Spiral Arm 선택
    const std::uint32_t spawnSequence = (m_emitter.spawnSequence)++;
    const std::uint32_t armIndex = spawnSequence % ParticleConfig::SpiralArmCount;

    // 각 Arm의 시작 각도 계산
    const float orbitAngle = CalculateSpiralArmBaseAngle(armIndex);
    particle.orbitAngle = orbitAngle;

    const DirectX::XMMATRIX rotationMatrix =
        DirectX::XMMatrixRotationRollPitchYaw
        (
            ParticleConfig::SpiralPitch,
            ParticleConfig::SpiralYaw,
            m_emitter.planeRotationAngle
        );

    const DirectX::XMVECTOR localPos = DirectX::XMVectorSet
    (
        std::cos(particle.orbitAngle) * particle.orbitRadius,
        std::sin(particle.orbitAngle) * particle.orbitRadius,
        0.0f,
        0.0f
    );

    const DirectX::XMVECTOR rotatedPos = DirectX::XMVector3TransformNormal(localPos, rotationMatrix);

    // Particle 초기 위치 설정
    particle.position = DirectX::XMFLOAT3
    (
        position.x + DirectX::XMVectorGetX(rotatedPos),
        position.y + DirectX::XMVectorGetY(rotatedPos),
        position.z + DirectX::XMVectorGetZ(rotatedPos)
    );

    // Particle 속도 설정
    particle.velocity = velocity;
    // Particle 렌더링 크기 설정
    particle.size = size;
    // Particle 생존 시간 설정
    particle.lifetime = lifetime;
    // Particle 경과 시간 초기화
    particle.age = 0.0f;
    // Particle 색상 설정
    particle.color = color;
    // Particle 활성화
    particle.active = true;

    return true;
}

void CpuParticleSystem::UpdateParticles(float deltaTime)
{
    // 중심 방출형 Particle의 회전 반경 설정
    const float pitch = ParticleConfig::SpiralPitch;
    const float yaw = ParticleConfig::SpiralYaw;

    // 기본 기울기와 시간 기반 평면 회전을 조합한 회전 행렬 생성
    DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, m_emitter.planeRotationAngle);

    // 활성 Particle의 수명 및 위치 갱신
    for (Particle& particle : m_particles)
    {
        if (!particle.active)
        {
            continue;
        }

        // 활성 Particle의 생존 시간 누적
        particle.age += deltaTime;

        // 수명이 끝난 Particle은 비활성화
        if (particle.age >= particle.lifetime)
        {
            particle.active = false;
            continue;
        }

        // 수명이 끝나갈수록 Particle을 투명하게 처리
        const float normalizedAge = particle.age / particle.lifetime;
        const float fadeAlpha = CalculateFadeOutAlpha(normalizedAge);
        particle.color.w = m_emitter.particleColor.w * fadeAlpha;

        // 중심 방출형 Particle의 각도와 반지름 갱신
        float speedFactor = 1.0f - (particle.age / particle.lifetime);
        particle.orbitAngle += (particle.angularVelocity * speedFactor) * deltaTime;
        particle.orbitRadius += particle.radialVelocity * deltaTime;

        // 중심 방출형 Particle의 평면 상 XY 좌표 계산
        float localX = std::cos(particle.orbitAngle) * particle.orbitRadius;
        float localY = std::sin(particle.orbitAngle) * particle.orbitRadius;

        // 중심 방출형 Particle의 입체감 부여를 위한 zOffset 적용
        const float zOffset = 
            std::sin(particle.orbitAngle * ParticleConfig::SpiralDepthFrequency + particle.age * ParticleConfig::SpiralDepthFrequency)
                * (particle.orbitRadius * ParticleConfig::SpiralDepthScale);

        // 중심 방출형 Particle의 로컬 위치 계산
        DirectX::XMVECTOR localPos = DirectX::XMVectorSet(localX, localY, zOffset, 0.0f);

        // 회전 행렬곱 수행
        DirectX::XMVECTOR rotatedPos = DirectX::XMVector3TransformNormal(localPos, rotationMatrix);

        // 최종 Particle 위치 계산
        particle.position.x = m_emitter.position.x + DirectX::XMVectorGetX(rotatedPos);
        particle.position.y = m_emitter.position.y + DirectX::XMVectorGetY(rotatedPos);
        particle.position.z = m_emitter.position.z + DirectX::XMVectorGetZ(rotatedPos);
    }
}

void CpuParticleSystem::UpdateEmitterPlaneRotation(float deltaTime)
{
    // Spiral Particle 방출 평면을 시간에 따라 회전
    m_emitter.planeRotationAngle += ParticleConfig::SpiralPlaneRotationSpeed * deltaTime;

    // 각도가 너무 커지는 것을 방지
    if (m_emitter.planeRotationAngle > DirectX::XM_2PI)
    {
        m_emitter.planeRotationAngle -= DirectX::XM_2PI;
    }
}

void CpuParticleSystem::EmitParticles(float deltaTime)
{
    // 생성해야 할 Particle 누적 개수 계산
    m_emitter.spawnAccumulator += m_emitter.spawnRate * deltaTime;

    // 누적된 생성 요청이 1개 이상이면 Particle 생성 시도
    while (m_emitter.spawnAccumulator >= 1.0f)
    {
        // Emitter 설정을 기반으로 새 Particle 생성 요청
        const bool spawned = SpawnParticle
        (
            m_emitter.position,
            m_emitter.velocity,
            m_emitter.particleSize,
            m_emitter.particleLifetime,
            m_emitter.particleColor
        );

        // 생성 요청 하나를 처리했으므로 누적 값 감소
        m_emitter.spawnAccumulator -= 1.0f;

        // 비활성 슬롯이 없어 생성하지 못한 경우 현재 프레임의 추가 생성 시도 중단
        if (!spawned)
        {
            break;
        }
    }
}

void CpuParticleSystem::RebuildBillboards()
{
    // 이전 프레임에서 생성한 Billboard 목록 초기화
    m_billboards.clear();
    // Particle 개수만큼 Billboard 저장 공간 예약
    m_billboards.reserve(m_particles.size());

    // 활성 Particle만 Billboard로 변환
    for (const Particle& particle : m_particles)
    {
        if (!particle.active)
        {
            continue;
        }

        m_billboards.push_back
        (
            Billboard
            {
                particle.position,
                particle.size,
                particle.color
            }
        );
    }
}