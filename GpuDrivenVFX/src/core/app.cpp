#include "pch.h"
#include "app.h"

#include <cstdio>

namespace
{
    // 변환 버퍼 데이터 구조체
    struct TransformBufferData
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };

    // 변환 버퍼는 상수 버퍼로 생성되므로, 16바이트 정렬 확인
    static_assert(sizeof(TransformBufferData) % 16 == 0, "Constant buffer size must be 16-byte aligned.(TransformBuffer)");
}

bool App::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    // m_Window에 전달할 윈도우 너비
    constexpr int windowWidth = 1280;
    // m_Window에 전달할 윈도우 높이
    constexpr int windowHeight = 720;

    // 윈도우 초기화
    if (!m_window.Initialize(hInstance, nCmdShow, windowWidth, windowHeight, L"GPU-Driven VFX System"))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // 렌더러 초기화
    if (!m_renderer.Initialize(m_window.GetHwnd(), m_window.GetWidth(), m_window.GetHeight()))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // 셰이더 컴파일
    if (!m_shader.Initialize(m_renderer.GetDevice(), L"shaders/VertexShader.hlsl", L"shaders/PixelShader.hlsl"))
    {
        // 컴파일하지 못한 경우 실패 처리
        return false;
    }

    // 렌더링 화면의 세로 픽셀 크기
    const int renderHeight = m_renderer.GetHeight();

    // 랜더링 화면의 종횡비 (가로 픽셀 크기 / 세로 픽셀 크기)
    const float aspectRatio = (renderHeight > 0)
        ? static_cast<float>(m_renderer.GetWidth()) / static_cast<float>(renderHeight)
        : 1.0f;

    // 카메라가 바라보는 위치 설정
    m_camera.LookAt
    (
        DirectX::XMFLOAT3(0.0f, 0.0f, -2.0f),
        DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
        DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)
    );

    // 카메라 원근 투영을 위한 렌즈값 설정
    m_camera.SetLens
    (
        DirectX::XM_PIDIV4,     // 시야각 (45도로 설정)
        aspectRatio,            // 화면 종횡비
        0.1f,                   // 근평면
        100.0f                  // 원평면
    );

    // 사각형을 구성할 정점 데이터를 담은 배열
    const std::vector<Vertex> quadVertices =
    {
        {
            DirectX::XMFLOAT3(-0.5f, 0.5f, 0.0f),
            DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f),
            DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(0.5f, 0.5f, 0.0f),
            DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f),
            DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f),
            DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f),
            DirectX::XMFLOAT4(0.0f, 0.3f, 1.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f),
            DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)
        }
    };

    // 정점 인덱스 데이터
    const std::vector<uint32_t> quadIndices =
    {
        0, 1, 2,
        1, 3, 2
    };

    // 메쉬 정점 리소스 초기화
    if (!m_quadMesh.Initialize(m_renderer.GetDevice(), m_shader.GetVertexShaderBlob(), quadVertices, quadIndices))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // Vertex Shader에 전달할 변환 버퍼 생성
    if (!CreateTransformBuffer())
    {
        // 변환 버퍼를 생성하지 못한 경우 실패 처리
        return false;
    }

    // CPU Particle System 초기화
    m_cpuParticleSystem.Initialize();

    // GPU Particle System 초기화
    if (!m_gpuParticleSystem.Initialize(m_renderer.GetDevice()))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // CPU 파티클 시스템에서 Billboard 렌더링에 필요한 리소스 초기화
    if (!m_cpuBillboardRenderer.Initialize(m_renderer.GetDevice()))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // GPU 파티클 시스템에서 Billboard 렌더링에 필요한 리소스 초기화
    if (!m_gpuBillboardRenderer.Initialize(m_renderer.GetDevice()))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // 프레임 시간 측정을 위한 타이머 초기화
    m_frameTimer.Initialize();

    return true;
}

int App::Run()
{
    // 애플리케이션 실행 루프
    while (m_running)
    {
        // OS 메시지 확인 및 처리
        m_window.ProcessMessages(m_running);

        // 종료 메시지를 수신한 경우 루프 탈출
        if (!m_running)
        {
            break;
        }

        // 애플리케이션 내부 데이터 및 상태 업데이트
        Update();
        // 애플리케이션 화면 렌더링
        Render();
    }

    return 0;
}

bool App::CreateTransformBuffer()
{
    // 변환 버퍼 설명자 구조체 
    D3D11_BUFFER_DESC bufferDesc = {};
    // 버퍼를 구성할 데이터 배열의 메모리 크기 설정
    bufferDesc.ByteWidth = sizeof(TransformBufferData);
    // 버퍼의 사용 방식을 생성 후 읽기 및 쓰기 가능으로 설정
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    // 버퍼의 사용 용도를 파이프라인의 상수 버퍼로 지정
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    // CPU가 버퍼에 직접 엑세스할 수 없도록 설정
    bufferDesc.CPUAccessFlags = 0;
    // 기타 특수 기능 사용하지 않음
    bufferDesc.MiscFlags = 0;
    // Structured Buffer가 아니므로 기본값으로 설정
    bufferDesc.StructureByteStride = 0;

    // 변환 버퍼 생성
    HRESULT hr = m_renderer.GetDevice()->CreateBuffer
    (
        &bufferDesc,
        nullptr,
        m_transformBuffer.GetAddressOf()
    );

    // 버퍼 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create transform constant buffer.", L"App Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

DirectX::XMMATRIX App::BuildQuadWorldMatrix() const
{
    // 스케일 행렬 생성
    const DirectX::XMMATRIX scale = DirectX::XMMatrixScaling
    (
        m_quadScale,
        m_quadScale,
        m_quadScale
    );

    // 회전 행렬 생성
    const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw
    (
        m_quadRotation.x,
        m_quadRotation.y,
        m_quadRotation.z
    );

    // 이동 행렬 생성
    const DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation
    (
        m_quadPosition.x,
        m_quadPosition.y,
        m_quadPosition.z
    );

    // 변환 행렬 계산 (SRT)
    return scale * rotation * translation;
}

void App::UpdateTransformBuffer(ID3D11DeviceContext* context)
{
    // 디바이스 컨텍스트나 변환 버퍼가 누락된 경우 실패 처리
    if (!context || !m_transformBuffer.Get())
    {
        return;
    }

    // 월드 변환 행렬
    const DirectX::XMMATRIX world = BuildQuadWorldMatrix();

    // 뷰 변환 행렬
    const DirectX::XMMATRIX view = m_camera.GetViewMatrix();

    // 투영 변환 행렬
    const DirectX::XMMATRIX projection = m_camera.GetProjectionMatrix();

    // 변환 버퍼 데이터 설정
    TransformBufferData transformData = {};
    // XMMATRIX 형식의 월드 변환 행렬을 전치한 뒤 XMFLOAT4X4 형식으로 전환하여 변환 버퍼 데이터 구조체에 전달
    DirectX::XMStoreFloat4x4(&transformData.world, DirectX::XMMatrixTranspose(world));
    // XMMATRIX 형식의 뷰 변환 행렬을 전치한 뒤 XMFLOAT4X4 형식으로 전환하여 변환 버퍼 데이터 구조체에 전달
    DirectX::XMStoreFloat4x4(&transformData.view, DirectX::XMMatrixTranspose(view));
    // XMMATRIX 형식의 투영 변환 행렬을 전치한 뒤 XMFLOAT4X4 형식으로 전환하여 변환 버퍼 데이터 구조체에 전달
    DirectX::XMStoreFloat4x4(&transformData.projection, DirectX::XMMatrixTranspose(projection));

    // CPU 메모리의 변환 버퍼 데이터를 GPU의 변환 버퍼에 업데이트
    context->UpdateSubresource
    (
        m_transformBuffer.Get(),    // 업데이트 대상 리소스 (변환 버퍼)
        0,                          // 하위 리소스 인덱스 (변환 버퍼는 단일 리소스이므로 0)
        nullptr,                    // 업데이트할 영역 지정 (버퍼 전체 영역)
        &transformData,             // 메모리의 원본 데이터 (변환 버퍼 데이터)
        0,                          // 텍스처 데이터가 아니므로 의미 없음
        0                           // 텍스처 데이터가 아니므로 의미 없음
    );

    // 파이프라인 입력 슬롯에 전달할 상수 버퍼 포인터 배열
    ID3D11Buffer* constantBuffers[] =
    {
        // 0번 슬롯에 할당할 상수 버퍼 (변환 버퍼)
        m_transformBuffer.Get()
    };

    // 정점 셰이더 단계의 상수 버퍼 설정
    context->VSSetConstantBuffers
    (
        0,                  // 버퍼를 바인딩할 입력 슬롯 번호
        1,                  // 설정할 버퍼의 개수
        constantBuffers     // 상수 버퍼 포인터들이 담긴 배열
    );
}

void App::Update()
{
    // 현재 프레임의 deltaTime 갱신
    m_frameTimer.Tick();

    // 현재 프레임에서 사용할 deltaTime 값 조회
    const float deltaTime = m_frameTimer.GetDeltaTime();

    // 실제 deltaTime 값으로 CPU Particle System 업데이트
    m_cpuParticleSystem.Update(deltaTime);

    // GPU Compute Shader를 사용해 GPU Particle System 업데이트
    m_gpuParticleSystem.Update(m_renderer.GetContext(), deltaTime);

    // 현재 프레임의 Particle 상태 디버그 메시지 출력
    PrintParticleDebugInfo(deltaTime);
    
}

void App::Render()
{
    // 프레임 드로우 준비 및 배경 색 초기화 (#0D141F)
    m_renderer.BeginFrame(0.05f, 0.08f, 0.12f, 1.0f);

    // CPU Particle Billboard Quads 렌더링
    m_cpuBillboardRenderer.Render
    (
        m_renderer.GetContext(),
        m_camera,
        m_cpuParticleSystem.GetBillboards()
    );

    // GPU Particle Buffer를 Billboard Quads로 렌더링
    m_gpuBillboardRenderer.Render
    (
        m_renderer.GetContext(),
        m_camera,
        m_gpuParticleSystem.GetParticleSrv(),
        m_gpuParticleSystem.GetMaxParticleCount()
    );

    // 최종 화면 출력
    m_renderer.EndFrame();
}

void App::PrintParticleDebugInfo(float deltaTime)
{
    // Particle 상태 디버그 출력 누적 시간 갱신
    m_particleDebugPrintAccumulator += deltaTime;

    // 1초마다 Particle 상태를 Output 창에 출력
    if (m_particleDebugPrintAccumulator >= 1.0f)
    {
        // 다음 출력 주기를 위해 누적 시간 감소
        m_particleDebugPrintAccumulator -= 1.0f;

        // Particle 상태 출력 문자열 구성
        wchar_t debugText[128] = {};
        swprintf_s
        (
            debugText,
            128,
            L"[Particle] render=%zu / max=%zu, dropped=%zu\n",
            m_cpuParticleSystem.GetRenderParticleCount(),
            m_cpuParticleSystem.GetMaxParticleCount(),
            m_cpuParticleSystem.GetDroppedSpawnCount()
        );

        // Output 창에 Particle 상태 출력
        OutputDebugStringW(debugText);
    }
}
