#pragma once

#include <DirectXMath.h>
#include <vector>

#include "rendering/billboard_renderer.h"

// CPU 기반 연산으로 시뮬레이션할 Particle 데이터 구조체
struct Particle
{
    // Particle의 월드 공간 위치
    DirectX::XMFLOAT3 position;
    // Particle의 이동 속도
    DirectX::XMFLOAT3 velocity;
    // Particle을 Billboard로 렌더링할 때 사용할 크기
    float size;
    // Particle의 총 생존 시간
    float lifetime;
    // Particle이 생성된 뒤 경과한 시간
    float age;
    // Particle을 Billboard로 렌더링할 때 사용할 색상
    DirectX::XMFLOAT4 color;
    // 현재 Particle이 활성 상태인지 여부
    bool active;
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
    // 활성 Particle 목록을 렌더링용 Billboard 목록으로 변환하는 함수
    void RebuildBillboards();

private:
    // CPU에서 관리하는 Particle 데이터 목록
    std::vector<Particle> m_particles;
    // BillboardRenderer에 전달할 렌더링용 Billboard 목록
    std::vector<Billboard> m_billboards;
};