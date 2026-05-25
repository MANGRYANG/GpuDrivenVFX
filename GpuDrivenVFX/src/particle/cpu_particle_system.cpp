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

    // 초기 테스트 Particle 생성
    SpawnParticle
    (
        DirectX::XMFLOAT3(-0.5f, 0.5f, 1.0f),
        DirectX::XMFLOAT3(0.25f, 0.0f, -0.25f),
        0.25f,
        1.0f,
        DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)
    );

    SpawnParticle
    (
        DirectX::XMFLOAT3(0.5f, 0.5f, 0.0f),
        DirectX::XMFLOAT3(0.0f, -0.25f, 0.25f),
        0.25f,
        3.0f,
        DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
    );

    SpawnParticle
    (
        DirectX::XMFLOAT3(0.5f, -0.5f, 1.0f),
        DirectX::XMFLOAT3(-0.25f, 0.0f, -0.25f),
        0.25f,
        5.0f,
        DirectX::XMFLOAT4(0.0f, 0.0f, 0.1f, 1.0f)
    );

    SpawnParticle
    (
        DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f),
        DirectX::XMFLOAT3(0.0f, 0.25f, 0.25f),
        0.25f,
        7.0f,
        DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
    );

    // 초기 Particle 데이터를 렌더링용 Billboard 목록으로 변환
    RebuildBillboards();
}

void CpuParticleSystem::Update(float deltaTime)
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