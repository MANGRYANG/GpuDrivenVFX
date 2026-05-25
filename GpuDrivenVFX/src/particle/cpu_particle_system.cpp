#include "pch.h"
#include "cpu_particle_system.h"

void CpuParticleSystem::Initialize()
{
    // 기존 Particle 데이터 초기화
    m_particles.clear();
    // 기존 Billboard 데이터 초기화
    m_billboards.clear();

    // 테스트 단계에서 사용할 Particle 슬롯 공간 확보
    m_particles.resize(MaxParticleCount);

    // Emitter 상태 초기화
    m_emitter = ParticleEmitter
    {
        DirectX::XMFLOAT3(0.0f, -0.5f, 0.0f),       // 초기 월드 공간 위치
        DirectX::XMFLOAT3(0.0f, 0.25f, 0.0f),       // 초기 이동 속도
        0.01f,                                      // 크기
        4.0f,                                       // 생존 시간
        8.0f,                                       // 초당 생성해야 하는 Particle 개수
        0.0f,                                       // 프레임마다 누적되는 생성 요청 수
        DirectX::XMFLOAT4(1.0f, 0.65f, 0.1f, 1.0f)  // Particle 색상
    };

    // 초기 Particle 데이터를 렌더링용 Billboard 목록으로 변환
    RebuildBillboards();
}

void CpuParticleSystem::Update(float deltaTime)
{
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

bool CpuParticleSystem::SpawnParticle
(
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT3& velocity,
    float size,
    float lifetime,
    const DirectX::XMFLOAT4& color
)
{
    // 비활성 Particle 슬롯을 검색
    for (Particle& particle : m_particles)
    {
        // 활성 Particle인 경우 무시
        if (particle.active)
        {
            continue;
        }

        // Particle 위치 설정
        particle.position = position;
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

    // 사용 가능한 비활성 Particle 슬롯이 없는 경우
    return false;
}

void CpuParticleSystem::UpdateParticles(float deltaTime)
{
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

        // 활성 Particle의 위치를 속도와 deltaTime 기반으로 갱신
        particle.position.x += particle.velocity.x * deltaTime;
        particle.position.y += particle.velocity.y * deltaTime;
        particle.position.z += particle.velocity.z * deltaTime;
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