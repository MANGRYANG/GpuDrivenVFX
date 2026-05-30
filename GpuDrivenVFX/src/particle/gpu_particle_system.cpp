#include "pch.h"
#include "gpu_particle_system.h"

bool GpuParticleSystem::Initialize(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // GPU Particle Structured Buffer 생성
    if (!CreateParticleBuffer(device))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    return true;
}

ID3D11ShaderResourceView* GpuParticleSystem::GetParticleSrv() const
{
    return m_particleSrv.Get();
}

ID3D11UnorderedAccessView* GpuParticleSystem::GetParticleUav() const
{
    return m_particleUav.Get();
}

std::size_t GpuParticleSystem::GetMaxParticleCount() const
{
    return MaxParticleCount;
}

bool GpuParticleSystem::CreateParticleBuffer(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        return false;
    }

    // GPU Particle 목록 설정
    std::vector<GpuParticleData> initialParticles;
    initialParticles.resize(MaxParticleCount);

    // GPU Particle Structured Buffer 설명자 구조체
    D3D11_BUFFER_DESC bufferDesc = {};
    // 전체 Particle 데이터의 메모리 크기 설정
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(GpuParticleData) * MaxParticleCount);
    // GPU에서 읽고 쓰는 기본 버퍼로 설정
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    // Compute Shader 쓰기와 Shader 읽기 모두 가능하도록 설정
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    // CPU 직접 접근은 사용하지 않음
    bufferDesc.CPUAccessFlags = 0;
    // Structured Buffer로 생성
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    // Structured Buffer 한 원소의 바이트 크기
    bufferDesc.StructureByteStride = sizeof(GpuParticleData);

    // 버퍼 초기 데이터 설정
    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.pSysMem = initialParticles.data();

    // GPU Particle Structured Buffer 생성
    HRESULT hr = device->CreateBuffer
    (
        &bufferDesc,
        &initialData,
        m_particleBuffer.GetAddressOf()
    );

    // 버퍼 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create GPU particle buffer.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // Particle Buffer를 읽기 위한 SRV 설명자 구조체
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    // Structured Buffer는 Format을 UNKNOWN으로 설정
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    // Buffer SRV로 설정
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    // 첫 번째 원소부터 읽기
    srvDesc.Buffer.FirstElement = 0;
    // 전체 Particle 원소 개수 설정
    srvDesc.Buffer.NumElements = static_cast<UINT>(MaxParticleCount);

    // Particle Buffer SRV 생성
    hr = device->CreateShaderResourceView
    (
        m_particleBuffer.Get(),
        &srvDesc,
        m_particleSrv.GetAddressOf()
    );

    // SRV 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create GPU particle SRV.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // Particle Buffer를 쓰기 위한 UAV 설명자 구조체
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    // Structured Buffer는 Format을 UNKNOWN으로 설정
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    // Buffer UAV로 설정
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    // 첫 번째 원소부터 쓰기
    uavDesc.Buffer.FirstElement = 0;
    // 전체 Particle 원소 개수 설정
    uavDesc.Buffer.NumElements = static_cast<UINT>(MaxParticleCount);
    // Append/Counter Buffer는 아직 사용하지 않음
    uavDesc.Buffer.Flags = 0;

    // Particle Buffer UAV 생성
    hr = device->CreateUnorderedAccessView
    (
        m_particleBuffer.Get(),
        &uavDesc,
        m_particleUav.GetAddressOf()
    );

    // UAV 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create GPU particle UAV.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}