#include "pch.h"
#include "gpu_particle_system.h"

#include "rendering/shader.h"

namespace
{
    // GPU Particle 업데이트 상수 버퍼 데이터 구조체
    struct ParticleUpdateBufferData
    {
        DirectX::XMFLOAT3 emitterPosition;
        float particleSize;

        DirectX::XMFLOAT3 emitterVelocity;
        float particleLifetime;

        DirectX::XMFLOAT4 emitterColor;

        float deltaTime;
        std::uint32_t particleCount;
        std::uint32_t spawnStartIndex;
        std::uint32_t spawnCount;
    };

    // GPU Particle 업데이트 상수 버퍼는 16바이트 정렬을 만족해야 함
    static_assert(sizeof(ParticleUpdateBufferData) % 16 == 0, "Constant buffer size must be 16-byte aligned.");
}

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

    // GPU active Particle 인덱스 목록 버퍼 생성
    if (!CreateAliveIndexBuffer(device))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    // GPU active Particle 개수 버퍼 생성
    if (!CreateAliveCountBuffer(device))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    // GPU Particle 업데이트용 Compute Shader 생성
    if (!CreateComputeShader(device))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    // GPU Particle 업데이트 Constant Buffer 생성
    if (!CreateParticleUpdateBuffer(device))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    return true;
}

void GpuParticleSystem::Update(ID3D11DeviceContext* context, float deltaTime)
{
    // 디바이스 컨텍스트가 누락된 경우
    if (!context)
    {
        return;
    }

    // 업데이트에 필요한 리소스가 누락된 경우
    if (!m_computeShader.Get() ||
        !m_particleUav.Get() ||
        !m_aliveIndexUav.Get() ||
        !m_aliveCountUav.Get() ||
        !m_particleUpdateBuffer.Get()
    )
    {
        return;
    }

    // 이번 프레임에 생성할 GPU Particle 개수 계산
    const std::uint32_t spawnCount = ConsumeSpawnCount(deltaTime);

    // 이번 프레임의 GPU Particle 생성 시작 슬롯 인덱스 기록
    const std::uint32_t spawnStartIndex = m_emitter.spawnIndex;

    // 다음 프레임의 GPU Particle 생성 시작 슬롯 인덱스 갱신
    if (spawnCount > 0)
    {
        m_emitter.spawnIndex = static_cast<std::uint32_t>((m_emitter.spawnIndex + spawnCount) % ParticleConfig::ParticleCapacity);
    }

    // GPU Particle 업데이트 상수 버퍼 갱신
    UpdateParticleUpdateBuffer(context, deltaTime, spawnStartIndex, spawnCount);

    // 이번 프레임 active Particle 개수를 0으로 초기화
    const UINT clearValues[4] = { 0, 0, 0, 0 };

    context->ClearUnorderedAccessViewUint
    (
        m_aliveCountUav.Get(),
        clearValues
    );

    // Compute Shader 바인딩
    context->CSSetShader(m_computeShader.Get(), nullptr, 0);

    // Particle UAV를 Compute Shader에 바인딩
    ID3D11UnorderedAccessView* unorderedAccessViews[] =
    {
        m_particleUav.Get(),
        m_aliveIndexUav.Get(),
        m_aliveCountUav.Get()
    };

    context->CSSetUnorderedAccessViews
    (
        0,
        3,
        unorderedAccessViews,
        nullptr
    );

    // 64개 thread를 하나의 group으로 사용
    static constexpr UINT ThreadGroupSize = 64;

    // 전체 Particle 개수를 처리할 thread group 수 계산
    const UINT threadGroupCount = static_cast<UINT>((ParticleConfig::ParticleCapacity + ThreadGroupSize - 1) / ThreadGroupSize);

    // GPU Compute Shader Dispatch
    context->Dispatch(threadGroupCount, 1, 1);

    // 이후 렌더링 단계에서 동일 버퍼를 SRV로 사용할 수 있도록 UAV 바인딩 해제
    ID3D11UnorderedAccessView* nullUnorderedAccessViews[] =
    {
        nullptr,
        nullptr,
        nullptr
    };

    context->CSSetUnorderedAccessViews
    (
        0,
        3,
        nullUnorderedAccessViews,
        nullptr
    );

    // Compute Shader 상수 버퍼 바인딩 해제
    ID3D11Buffer* nullConstantBuffers[] = { nullptr };

    context->CSSetConstantBuffers
    (
        0,
        1,
        nullConstantBuffers
    );

    // Compute Shader 바인딩 해제
    context->CSSetShader(nullptr, nullptr, 0);
}

ID3D11ShaderResourceView* GpuParticleSystem::GetParticleSrv() const
{
    return m_particleSrv.Get();
}

ID3D11ShaderResourceView* GpuParticleSystem::GetAliveIndexSrv() const
{
    return m_aliveIndexSrv.Get();
}

ID3D11ShaderResourceView* GpuParticleSystem::GetAliveCountSrv() const
{
    return m_aliveCountSrv.Get();
}

ID3D11UnorderedAccessView* GpuParticleSystem::GetParticleUav() const
{
    return m_particleUav.Get();
}

std::size_t GpuParticleSystem::GetMaxParticleCount() const
{
    return ParticleConfig::ParticleCapacity;
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
    initialParticles.resize(ParticleConfig::ParticleCapacity);

    // GPU Particle Structured Buffer 설명자 구조체
    D3D11_BUFFER_DESC bufferDesc = {};
    // 전체 Particle 데이터의 메모리 크기 설정
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(GpuParticleData) * ParticleConfig::ParticleCapacity);
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
    srvDesc.Buffer.NumElements = static_cast<UINT>(ParticleConfig::ParticleCapacity);

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
    uavDesc.Buffer.NumElements = static_cast<UINT>(ParticleConfig::ParticleCapacity);
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

bool GpuParticleSystem::CreateComputeShader(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        return false;
    }

    // Compute Shader 컴파일 결과 Blob
    Microsoft::WRL::ComPtr<ID3DBlob> computeShaderBlob;

    // GPU Particle 업데이트 Compute Shader 컴파일
    if (!Shader::CompileShaderFromFile
    (
        L"shaders/GpuParticleUpdateComputeShader.hlsl",
        "CS_Main",
        "cs_5_0",
        computeShaderBlob
    ))
    {
        // 컴파일하지 못한 경우 실패 처리
        return false;
    }

    // Compute Shader 객체 생성
    HRESULT hr = device->CreateComputeShader
    (
        computeShaderBlob->GetBufferPointer(),
        computeShaderBlob->GetBufferSize(),
        nullptr,
        m_computeShader.GetAddressOf()
    );

    // Compute Shader 생성에 실패한 경우
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"Failed to create compute shader for GPU particle update.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

bool GpuParticleSystem::CreateParticleUpdateBuffer(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        return false;
    }

    // GPU Particle 업데이트 상수 버퍼 설명자 구조체
    D3D11_BUFFER_DESC bufferDesc = {};
    // 버퍼를 구성할 데이터 구조체의 메모리 크기 설정
    bufferDesc.ByteWidth = sizeof(ParticleUpdateBufferData);
    // CPU에서 매 프레임 UpdateSubresource로 갱신할 기본 버퍼로 설정
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    // 상수 버퍼로 바인딩
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    // CPU 직접 접근은 사용하지 않음
    bufferDesc.CPUAccessFlags = 0;
    // 기타 특수 기능 사용하지 않음
    bufferDesc.MiscFlags = 0;
    // Structured Buffer가 아니므로 기본값으로 설정
    bufferDesc.StructureByteStride = 0;

    // GPU Particle 업데이트 상수 버퍼 생성
    HRESULT hr = device->CreateBuffer
    (
        &bufferDesc,
        nullptr,
        m_particleUpdateBuffer.GetAddressOf()
    );

    // 상수 버퍼 생성에 실패한 경우
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"Failed to create GPU particle update buffer.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

bool GpuParticleSystem::CreateAliveIndexBuffer(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        return false;
    }

    // active Particle 인덱스 목록 초기 데이터
    std::vector<std::uint32_t> initialAliveIndices;
    initialAliveIndices.resize(ParticleConfig::ParticleCapacity);

    // active Particle 인덱스 목록 Structured Buffer 설명자 구조체
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(std::uint32_t) * ParticleConfig::ParticleCapacity);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(std::uint32_t);

    // 버퍼 초기 데이터 설정
    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.pSysMem = initialAliveIndices.data();

    // active Particle 인덱스 목록 Buffer 생성
    HRESULT hr = device->CreateBuffer
    (
        &bufferDesc,
        &initialData,
        m_aliveIndexBuffer.GetAddressOf()
    );

    // 버퍼 생성에 실패한 경우
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"Failed to create GPU alive index buffer.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // active Particle 인덱스 목록 SRV 설명자 구조체
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = static_cast<UINT>(ParticleConfig::ParticleCapacity);

    // active Particle 인덱스 목록 SRV 생성
    hr = device->CreateShaderResourceView
    (
        m_aliveIndexBuffer.Get(),
        &srvDesc,
        m_aliveIndexSrv.GetAddressOf()
    );

    // SRV 생성에 실패한 경우
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"Failed to create GPU alive index SRV.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // active Particle 인덱스 목록 UAV 설명자 구조체
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = static_cast<UINT>(ParticleConfig::ParticleCapacity);
    uavDesc.Buffer.Flags = 0;

    // active Particle 인덱스 목록 UAV 생성
    hr = device->CreateUnorderedAccessView
    (
        m_aliveIndexBuffer.Get(),
        &uavDesc,
        m_aliveIndexUav.GetAddressOf()
    );

    // UAV 생성에 실패한 경우
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"Failed to create GPU alive index UAV.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

bool GpuParticleSystem::CreateAliveCountBuffer(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        return false;
    }

    // active Particle 개수 초기값
    const std::uint32_t initialAliveCount = 0;

    // active Particle 개수 Structured Buffer 설명자 구조체
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(std::uint32_t);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(std::uint32_t);

    // 버퍼 초기 데이터 설정
    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.pSysMem = &initialAliveCount;

    // active Particle 개수 Buffer 생성
    HRESULT hr = device->CreateBuffer
    (
        &bufferDesc,
        &initialData,
        m_aliveCountBuffer.GetAddressOf()
    );

    // 버퍼 생성에 실패한 경우
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"Failed to create GPU alive count buffer.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // active Particle 개수 SRV 설명자 구조체
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = 1;

    // active Particle 개수 SRV 생성
    hr = device->CreateShaderResourceView
    (
        m_aliveCountBuffer.Get(),
        &srvDesc,
        m_aliveCountSrv.GetAddressOf()
    );

    // SRV 생성에 실패한 경우
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"Failed to create GPU alive count SRV.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // active Particle 개수 UAV 설명자 구조체
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = 1;
    uavDesc.Buffer.Flags = 0;

    // active Particle 개수 UAV 생성
    hr = device->CreateUnorderedAccessView
    (
        m_aliveCountBuffer.Get(),
        &uavDesc,
        m_aliveCountUav.GetAddressOf()
    );

    // UAV 생성에 실패한 경우
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"Failed to create GPU alive count UAV.", L"GpuParticleSystem Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

void GpuParticleSystem::UpdateParticleUpdateBuffer
(
    ID3D11DeviceContext* context,
    float deltaTime,
    std::uint32_t spawnStartIndex,
    std::uint32_t spawnCount
)
{
    // 디바이스 컨텍스트나 상수 버퍼가 누락된 경우
    if (!context || !m_particleUpdateBuffer.Get())
    {
        return;
    }

    // GPU Particle 업데이트 상수 버퍼 데이터 구성
    ParticleUpdateBufferData bufferData = {};
    bufferData.emitterPosition = m_emitter.position;
    bufferData.particleSize = m_emitter.particleSize;
    bufferData.emitterVelocity = m_emitter.velocity;
    bufferData.particleLifetime = m_emitter.particleLifetime;
    bufferData.emitterColor = m_emitter.color;
    bufferData.deltaTime = deltaTime;
    bufferData.particleCount = static_cast<std::uint32_t>(ParticleConfig::ParticleCapacity);
    bufferData.spawnStartIndex = spawnStartIndex;
    bufferData.spawnCount = spawnCount;

    // CPU 메모리의 상수 버퍼 데이터를 GPU 상수 버퍼에 업데이트
    context->UpdateSubresource
    (
        m_particleUpdateBuffer.Get(),
        0,
        nullptr,
        &bufferData,
        0,
        0
    );

    // Compute Shader 단계에 상수 버퍼 바인딩
    ID3D11Buffer* constantBuffers[] =
    {
        m_particleUpdateBuffer.Get()
    };

    context->CSSetConstantBuffers
    (
        0,
        1,
        constantBuffers
    );
}

std::uint32_t GpuParticleSystem::ConsumeSpawnCount(float deltaTime)
{
    // 생성 요청 수 누적
    m_emitter.spawnAccumulator += m_emitter.spawnRate * deltaTime;

    // 이번 프레임에 생성할 수 있는 Particle 개수 계산
    const std::uint32_t spawnCount = static_cast<std::uint32_t>(m_emitter.spawnAccumulator);

    // 생성할 Particle이 없는 경우
    if (spawnCount == 0)
    {
        return 0;
    }

    // 처리한 생성 요청 수만큼 누적값 감소
    m_emitter.spawnAccumulator -= static_cast<float>(spawnCount);

    // 한 프레임에서 최대 Particle 수를 초과해 생성하지 않도록 제한
    if (spawnCount > ParticleConfig::ParticleCapacity)
    {
        return static_cast<std::uint32_t>(ParticleConfig::ParticleCapacity);
    }

    return spawnCount;
}