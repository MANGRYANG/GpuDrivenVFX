#include "pch.h"
#include "cpu_particle_system.h"

void CpuParticleSystem::Initialize()
{
    // 기존 Particle 데이터 초기화
    m_particles.clear();
    // 기존 Billboard 데이터 초기화
    m_billboards.clear();

    // 초기 테스트 Particle 목록 생성
    m_particles =
    {
        {
            DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f),
            DirectX::XMFLOAT3(0.02f, 0.0f, 0.3f),
            0.2f,
            3.0f,
            0.0f,
            DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f),
            true
        },
        {
            DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f),
            DirectX::XMFLOAT3(0.05f, 0.0f, -0.1f),
            0.25f,
            5.0f,
            0.0f,
            DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f),
            true
        },
        {
            DirectX::XMFLOAT3(0.0f, 0.6f, 0.0f),
            DirectX::XMFLOAT3(0.0f, -0.06f, 0.0f),
            0.15f,
            7.0f,
            0.0f,
            DirectX::XMFLOAT4(1.0f, 0.4f, 0.1f, 1.0f),
            true
        }
    };

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