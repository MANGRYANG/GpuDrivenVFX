#pragma once

#include <DirectXMath.h>
#include <cstddef>
#include <cstdint>
#include <d3d11.h>
#include <wrl/client.h>

#include "particle/particle_config.h"

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
    // 중심 회전형 Particle의 현재 반지름
    float orbitRadius = 0.0f;
    // 중심 회전형 Particle의 현재 각도
    float orbitAngle = 0.0f;

    // 중심 회전형 Particle의 각속도
    float angularVelocity = 0.0f;
    // 중심에서 바깥쪽으로 퍼지는 속도
    float radialVelocity = 0.0f;
    // GPU 구조체 정렬을 맞추기 위한 패딩
    DirectX::XMFLOAT2 padding = DirectX::XMFLOAT2(0.0f, 0.0f);
};

// GPU Particle 데이터는 Structured Buffer stride와 일치해야 함
static_assert(sizeof(GpuParticleData) == 80, "GpuParticleData size must be 80 bytes.");

class GpuParticleSystem
{
public:
    // GpuParticleSystem 클래스 생성자
    GpuParticleSystem() = default;
    // GpuParticleSystem 클래스 소멸자
    ~GpuParticleSystem() = default;

    // GPU Particle System에 필요한 GPU 리소스를 초기화하는 함수
    bool Initialize(ID3D11Device* device);

    // GPU Compute Shader를 사용해 Particle 데이터를 갱신하는 함수
    void Update(ID3D11DeviceContext* context, float deltaTime);

    // GPU Particle 데이터를 읽기 위한 SRV를 반환하는 함수
    ID3D11ShaderResourceView* GetParticleSrv() const;

    // GPU active Particle 인덱스 목록을 읽기 위한 SRV를 반환하는 함수
    ID3D11ShaderResourceView* GetAliveIndexSrv() const;

    // GPU active Particle 개수를 읽기 위한 SRV를 반환하는 함수
    ID3D11ShaderResourceView* GetAliveCountSrv() const;

    // GPU Particle 데이터를 갱신하기 위한 UAV를 반환하는 함수
    ID3D11UnorderedAccessView* GetParticleUav() const;

    // GPU Particle 최대 개수를 반환하는 함수
    std::size_t GetMaxParticleCount() const;

private:
    // GPU Particle Structured Buffer를 생성하는 함수
    bool CreateParticleBuffer(ID3D11Device* device);

    // GPU Particle 업데이트용 Compute Shader를 생성하는 함수
    bool CreateComputeShader(ID3D11Device* device);

    // GPU Particle 업데이트에 사용할 상수 버퍼를 생성하는 함수
    bool CreateParticleUpdateBuffer(ID3D11Device* device);

    // Active 상태의 GPU Particle 인덱스 목록 버퍼를 생성하는 함수
    bool CreateAliveIndexBuffer(ID3D11Device* device);

    // Active 상태의 GPU Particle 개수 버퍼를 생성하는 함수
    bool CreateAliveCountBuffer(ID3D11Device* device);

    // 중심 방출형 파티클 평면에 대한 회전 행렬을 계산하는 함수
    void CalculateSpiralBasis(float planeRotationAngle);

    // Spiral Particle 방출 평면의 회전 각도를 갱신하는 함수
    void UpdateEmitterPlaneRotation(float deltaTime);

    // GPU Particle 업데이트 상수 버퍼를 갱신하는 함수
    void UpdateParticleUpdateBuffer
    (
        ID3D11DeviceContext* context,
        float deltaTime,
        std::uint32_t spawnStartIndex,
        std::uint32_t spawnCount,
        std::uint32_t spawnSequenceStart
    );

    // 이번 프레임에 생성할 GPU Particle 개수를 계산하는 함수
    std::uint32_t ConsumeSpawnCount(float deltaTime);

private:
    // GPU Particle을 지속적으로 생성하기 위한 Emitter 설정 구조체
    struct ParticleEmitter
    {
        // GPU Particle이 생성될 월드 공간 위치
        DirectX::XMFLOAT3 position = ParticleConfig::EmitterPosition;
        // GPU Particle의 초기 이동 속도
        DirectX::XMFLOAT3 velocity = ParticleConfig::EmitterVelocity;
        // GPU Particle을 Billboard로 렌더링할 때 사용할 크기
        float particleSize = ParticleConfig::ParticleSize;
        // GPU Particle의 총 생존 시간
        float particleLifetime = ParticleConfig::ParticleLifetime;
        // 초당 생성해야 하는 GPU Particle 개수
        float spawnRate = ParticleConfig::SpawnRate;
        // 프레임마다 누적되는 생성 요청 수
        float spawnAccumulator = 0.0f;
        // 다음 프레임에 GPU Particle을 생성할 순환 슬롯 인덱스
        std::uint32_t spawnIndex = 0;
        // Spiral Arm 배치를 계산하기 위한 GPU Particle 생성 순서
        std::uint32_t spawnSequence = 0;
        // Spiral Particle 방출 평면의 현재 회전 각도
        float planeRotationAngle = 0.0f;
        // GPU Particle을 Billboard로 렌더링할 때 사용할 색상
        DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(0.5f, 0.5f, 1.0f, 1.0f);
    };

    // GPU Particle 데이터를 저장할 Structured Buffer
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_particleBuffer;
    // GPU Particle 업데이트에 사용할 Constant Buffer
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_particleUpdateBuffer;
    
    // active Particle의 원본 Particle 인덱스를 저장할 Structured Buffer
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_aliveIndexBuffer;
    // active Particle 개수를 저장할 Structured Buffer
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_aliveCountBuffer;

    // Vertex Shader 또는 Compute Shader에서 Particle 데이터를 읽기 위한 SRV
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_particleSrv;
    // Compute Shader에서 Particle 데이터를 쓰기 위한 UAV
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_particleUav;

    // Vertex Shader에서 active Particle 인덱스 목록을 읽기 위한 SRV
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_aliveIndexSrv;
    // Vertex Shader에서 active Particle 개수를 읽기 위한 SRV
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_aliveCountSrv;

    // Compute Shader에서 active Particle 인덱스 목록을 쓰기 위한 UAV
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_aliveIndexUav;
    // Compute Shader에서 active Particle 개수를 갱신하기 위한 UAV
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_aliveCountUav;

    // GPU Particle 업데이트를 수행할 Compute Shader
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_computeShader;

    // GPU Particle Emitter
    ParticleEmitter m_emitter;

    // 중심 방출형 파티클 평면의 로컬 X축을 월드 방향으로 변환한 기저
    DirectX::XMFLOAT4 m_spiralRight;
    // 중심 방출형 파티클 평면의 로컬 Y축을 월드 방향으로 변환한 기저
    DirectX::XMFLOAT4 m_spiralUp;
    // 중심 방출형 파티클 평면의 로컬 Z축을 월드 방향으로 변환한 기저
    DirectX::XMFLOAT4 m_spiralForward;
};