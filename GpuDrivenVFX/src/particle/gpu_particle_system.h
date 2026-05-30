#pragma once

#include <DirectXMath.h>
#include <cstddef>
#include <vector>

// GPU 기반 Particle 시뮬레이션에서 사용할 Particle 데이터 구조체
struct GpuParticleData
{
    // Particle의 월드 공간 위치
    DirectX::XMFLOAT3 position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    // Particle을 Billboard로 렌더링할 때 사용할 크기
    float size = 0.0f;

    // Particle의 이동 속도
    DirectX::XMFLOAT3 velocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    // Particle의 총 생존 시간
    float lifetime = 0.0f;

    // Particle을 Billboard로 렌더링할 때 사용할 색상
    DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

    // Particle이 생성된 뒤 경과한 시간
    float age = 0.0f;
    // 현재 Particle이 활성 상태인지 여부
    std::uint32_t active = 0;
    // GPU 구조체 정렬을 맞추기 위한 패딩
    DirectX::XMFLOAT2 padding = DirectX::XMFLOAT2(0.0f, 0.0f);
};

// GPU Particle 데이터는 Structured Buffer stride와 일치해야 함
static_assert(sizeof(GpuParticleData) == 64, "GpuParticleData size must be 64 bytes.");

class GpuParticleSystem
{
public:
    // GpuParticleSystem 클래스 생성자
    GpuParticleSystem() = default;
    // GpuParticleSystem 클래스 소멸자
    ~GpuParticleSystem() = default;

    // GPU Particle System에 필요한 GPU 리소스를 초기화하는 함수
    bool Initialize(ID3D11Device* device);

    // GPU Particle 데이터를 읽기 위한 SRV를 반환하는 함수
    ID3D11ShaderResourceView* GetParticleSrv() const;

    // GPU Particle 데이터를 갱신하기 위한 UAV를 반환하는 함수
    ID3D11UnorderedAccessView* GetParticleUav() const;

    // GPU Particle 최대 개수를 반환하는 함수
    std::size_t GetMaxParticleCount() const;

private:
    // GPU Particle Structured Buffer를 생성하는 함수
    bool CreateParticleBuffer(ID3D11Device* device);

private:
    // 현재 테스트 단계에서 사용할 GPU Particle 최대 개수
    static constexpr std::size_t MaxParticleCount = 64;

    // GPU Particle 데이터를 저장할 Structured Buffer
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_particleBuffer;
    // Vertex Shader 또는 Compute Shader에서 Particle 데이터를 읽기 위한 SRV
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_particleSrv;
    // Compute Shader에서 Particle 데이터를 쓰기 위한 UAV
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_particleUav;
};