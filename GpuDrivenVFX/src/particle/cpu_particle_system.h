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

    // 활성 Particle 목록을 렌더링용 Billboard 목록으로 변환하는 함수
    void RebuildBillboards();

private:
    // 현재 테스트 단계에서 사용할 최대 Particle 개수
    static constexpr std::size_t MaxParticleCount = 4;

    // CPU에서 관리하는 Particle 데이터 목록
    std::vector<Particle> m_particles;
    // BillboardRenderer에 전달할 렌더링용 Billboard 목록
    std::vector<Billboard> m_billboards;
};