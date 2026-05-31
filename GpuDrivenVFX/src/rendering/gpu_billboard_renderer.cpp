#include "pch.h"
#include "gpu_billboard_renderer.h"

namespace
{
    // GPU Billboard 정점 정보 구조체
    struct GpuBillboardVertex
    {
        DirectX::XMFLOAT2 corner;
    };

    // 카메라의 뷰/투영 변환 행렬 구조체
    struct CameraBufferData
    {
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };

    // 카메라 버퍼는 상수 버퍼로 생성되므로, 16바이트 정렬 확인
    static_assert(sizeof(CameraBufferData) % 16 == 0, "Constant buffer size must be 16-byte aligned.(CameraBuffer)");
}

bool GpuBillboardRenderer::Initialize(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device.", L"GpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // GPU Billboard 렌더링용 셰이더 컴파일 및 생성
    if (!m_shader.Initialize(device, L"shaders/GpuBillboardVertexShader.hlsl", L"shaders/BillboardPixelShader.hlsl"))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // Billboard Quad 정점/인덱스 버퍼 생성
    if (!CreateQuadBuffers(device))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    // GPU Billboard 입력 레이아웃 생성
    if (!CreateInputLayout(device, m_shader.GetVertexShaderBlob()))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    // Billboard 렌더링에 사용할 카메라 변환 상수 버퍼 생성
    if (!CreateCameraBuffer(device))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    return true;
}

bool GpuBillboardRenderer::CreateQuadBuffers(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device.", L"GpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // Billboard Quad를 구성할 정점 데이터
    const std::vector<GpuBillboardVertex> vertices =
    {
        { DirectX::XMFLOAT2(-0.5f, 0.5f) },
        { DirectX::XMFLOAT2(0.5f, 0.5f) },
        { DirectX::XMFLOAT2(-0.5f, -0.5f) },
        { DirectX::XMFLOAT2(0.5f, -0.5f) }
    };

    // Billboard Quad를 구성할 인덱스 데이터
    const std::vector<uint32_t> indices =
    {
        0, 1, 2,
        1, 3, 2
    };

    // 정점 버퍼 설명자 구조체
    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(GpuBillboardVertex) * vertices.size());
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    // 생성될 버퍼에 채워 넣을 초기화 데이터
    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.pSysMem = vertices.data();

    // Billboard 정점 버퍼 생성
    HRESULT hr = device->CreateBuffer
    (
        &vertexBufferDesc,
        &initialData,
        m_vertexBuffer.GetAddressOf()
    );

    // 버퍼 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create GPU billboard vertex buffer.", L"GpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 인덱스 버퍼 설명자 구조체
    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    initialData = {};
    initialData.pSysMem = indices.data();

    // Billboard 인덱스 버퍼 생성
    hr = device->CreateBuffer
    (
        &indexBufferDesc,
        &initialData,
        m_indexBuffer.GetAddressOf()
    );

    // 버퍼 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create GPU billboard index buffer.", L"GpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 정점 바이트 크기 멤버 변수 초기화
    m_vertexStride = sizeof(GpuBillboardVertex);
    // 인덱스 개수 멤버 변수 초기화
    m_indexCount = static_cast<UINT>(indices.size());

    return true;
}

bool GpuBillboardRenderer::CreateInputLayout(ID3D11Device* device, ID3DBlob* vertexShaderBlob)
{
    // 디바이스 또는 정점 셰이더 블롭이 누락된 경우
    if (!device || !vertexShaderBlob)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device or vertex shader blob.", L"GpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // GPU Billboard 정점 입력 레이아웃 설명자 구조체 배열
    const D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
    {
        {
            "TEXCOORD",
            0,
            DXGI_FORMAT_R32G32_FLOAT,
            0,
            0,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        }
    };

    // GPU Billboard 입력 레이아웃 객체 생성
    HRESULT hr = device->CreateInputLayout
    (
        inputLayoutDesc,
        ARRAYSIZE(inputLayoutDesc),
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        m_inputLayout.GetAddressOf()
    );

    // 입력 레이아웃 객체 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create GPU billboard input layout.", L"GpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

bool GpuBillboardRenderer::CreateCameraBuffer(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device.", L"GpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 카메라 버퍼 설명자 구조체
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(CameraBufferData);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    // 카메라 버퍼 생성
    HRESULT hr = device->CreateBuffer
    (
        &bufferDesc,
        nullptr,
        m_cameraBuffer.GetAddressOf()
    );

    // 버퍼 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create GPU billboard camera buffer.", L"GpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

void GpuBillboardRenderer::UpdateCameraBuffer(ID3D11DeviceContext* context, const Camera& camera)
{
    // 디바이스 컨텍스트나 카메라 버퍼가 누락된 경우
    if (!context || !m_cameraBuffer.Get())
    {
        return;
    }

    const DirectX::XMMATRIX view = camera.GetViewMatrix();
    const DirectX::XMMATRIX projection = camera.GetProjectionMatrix();

    // 카메라 버퍼 데이터 설정
    CameraBufferData cameraData = {};
    // HLSL에서 사용할 수 있도록 뷰 행렬을 전치하여 저장
    DirectX::XMStoreFloat4x4(&cameraData.view, DirectX::XMMatrixTranspose(view));
    // HLSL에서 사용할 수 있도록 투영 행렬을 전치하여 저장
    DirectX::XMStoreFloat4x4(&cameraData.projection, DirectX::XMMatrixTranspose(projection));

    // CPU 메모리의 카메라 버퍼 데이터를 GPU의 카메라 버퍼에 업데이트
    context->UpdateSubresource
    (
        m_cameraBuffer.Get(),
        0,
        nullptr,
        &cameraData,
        0,
        0
    );

    // 파이프라인 입력 슬롯에 전달할 상수 버퍼 포인터 배열
    ID3D11Buffer* constantBuffers[] =
    {
        m_cameraBuffer.Get()
    };

    // 정점 셰이더 단계의 상수 버퍼 설정
    context->VSSetConstantBuffers
    (
        0,
        1,
        constantBuffers
    );
}

void GpuBillboardRenderer::Render
(
    ID3D11DeviceContext* context,
    const Camera& camera,
    ID3D11ShaderResourceView* particleSrv,
    std::size_t particleCount
)
{
    // 디바이스 컨텍스트가 누락된 경우
    if (!context)
    {
        return;
    }

    // 렌더링에 필요한 Particle SRV가 누락된 경우
    if (!particleSrv)
    {
        return;
    }

    // 렌더링할 Particle 슬롯이 없는 경우
    if (particleCount == 0)
    {
        return;
    }

    // GPU Billboard 셰이더 바인딩
    m_shader.Bind(context);

    // 현재 프레임에서 사용할 카메라 변환 행렬을 정점 셰이더에 전달
    UpdateCameraBuffer(context, camera);

    // GPU Particle Buffer SRV를 정점 셰이더에 바인딩
    ID3D11ShaderResourceView* shaderResourceViews[] =
    {
        particleSrv
    };

    context->VSSetShaderResources
    (
        0,
        1,
        shaderResourceViews
    );

    // GPU Billboard 입력 레이아웃 설정
    context->IASetInputLayout(m_inputLayout.Get());

    // 정점 버퍼를 입력 조립 단계에 바인딩
    ID3D11Buffer* vertexBuffers[] =
    {
        m_vertexBuffer.Get()
    };

    const UINT strides[] =
    {
        m_vertexStride
    };

    const UINT offsets[] =
    {
        0
    };

    context->IASetVertexBuffers
    (
        0,
        1,
        vertexBuffers,
        strides,
        offsets
    );

    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // GPU Particle Buffer의 각 Particle 슬롯을 하나의 Billboard 인스턴스로 렌더링
    context->DrawIndexedInstanced
    (
        m_indexCount,
        static_cast<UINT>(particleCount),
        0,
        0,
        0
    );

    // 다음 Compute Shader 업데이트에서 동일 버퍼를 UAV로 사용할 수 있도록 SRV 바인딩 해제
    ID3D11ShaderResourceView* nullShaderResourceViews[] = { nullptr };

    context->VSSetShaderResources
    (
        0,
        1,
        nullShaderResourceViews
    );
}