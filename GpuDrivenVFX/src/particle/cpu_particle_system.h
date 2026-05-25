#pragma once

#include <DirectXMath.h>
#include <cstddef>
#include <vector>

#include "rendering/billboard_renderer.h"

// CPU 기반 연산으로 시뮬레이션할 Particle 데이터 구조체
struct Particle
{
    // Particle의 월드 공간 위치
    DirectX::XMFLOAT3 position = DirectX::XMFLOAT3(0, 0, 0);
    // Particle의 이동 속도
    DirectX::XMFLOAT3 velocity = DirectX::XMFLOAT3(0, 0, 0);
    // Particle을 Billboard로 렌더링할 때 사용할 크기
    float size = 0.0;
    // Particle의 총 생존 시간
    float lifetime = 0.0;
    // Particle이 생성된 뒤 경과한 시간
    float age = 0.0;
    // Particle을 Billboard로 렌더링할 때 사용할 색상
    DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(0, 0, 0, 0);
    // 현재 Particle이 활성 상태인지 여부
    bool active = false;
};

// Particle을 일정 비율로 생성하기 위한 Emitter 상태 구조체
struct ParticleEmitter
{
    // Particle이 생성될 월드 공간 위치
    DirectX::XMFLOAT3 position = DirectX::XMFLOAT3(0, 0, 0);
    // 생성될 Particle의 초기 이동 속도
    DirectX::XMFLOAT3 velocity = DirectX::XMFLOAT3(0, 0, 0);
    // 생성될 Particle의 크기
    float particleSize = 0.0f;
    // 생성될 Particle의 생존 시간
    float particleLifetime = 0.0f;
    // 초당 생성할 Particle 개수
    float spawnRate = 0.0f;
    // 프레임마다 누적되는 생성 요청 수
    float spawnAccumulator = 0.0f;
    // 생성될 Particle의 색상
    DirectX::XMFLOAT4 particleColor = DirectX::XMFLOAT4(0, 0, 0, 0);
};

class CpuParticleSystem
{
public:
    // CpuParticleSystem 클래스 생성자
    CpuParticleSystem() = default;
    // CpuParticleSystem 클래스 소멸자
    ~CpuParticleSystem() = default;

    // CPU Particle System의 초기 Particle 데이터를 생성하고 렌더링용 Billboard 목록을 구성하는 함수
    void Initialize();

    // CPU Particle의 수명 및 위치를 갱신하고 렌더링용 Billboard 목록을 재구성하는 함수
    void Update(float deltaTime);

    // 렌더링에 사용할 Billboard 목록을 반환하는 함수
    const std::vector<Billboard>& GetBillboards() const;

    // 현재 렌더링되고 있는 Particle의 개수를 반환하는 함수 
    std::size_t GetRenderParticleCount() const;
    // 생성에 실패하여 누락된 Particle 생성 요청 개수를 반환하는 함수
    std::size_t GetDroppedSpawnCount() const;
    // 렌더링 가능한 최대 Particle 개수를 반환하는 함수
    std::size_t GetMaxParticleCount() const;

private:
    // 비활성 Particle 슬롯에 새로운 Particle 데이터를 생성하는 함수
    bool SpawnParticle
    (
        const DirectX::XMFLOAT3& position,
        const DirectX::XMFLOAT3& velocity,
        float size,
        float lifetime,
        const DirectX::XMFLOAT4& color
    );

    // 활성 상태의 Particle 수명 및 위치를 갱신하는 함수
    void UpdateParticles(float deltaTime);

    // Emitter의 누적 생성 요청을 처리하는 함수
    void EmitParticles(float deltaTime);

    // 활성 Particle 목록을 렌더링용 Billboard 목록으로 변환하는 함수
    void RebuildBillboards();

private:
    // 현재 테스트 단계에서 사용할 최대 Particle 개수
    static constexpr std::size_t MaxParticleCount = 64;

    // CPU Particle 생성 상태를 관리하는 Emitter
    ParticleEmitter m_emitter;

    // CPU에서 관리하는 Particle 데이터 목록
    std::vector<Particle> m_particles;
    // BillboardRenderer에 전달할 렌더링용 Billboard 목록
    std::vector<Billboard> m_billboards;

    // 생성에 실패하여 누락된 Particle 생성 요청 개수
    std::size_t m_droppedSpawnCount = 0;
};