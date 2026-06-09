#include "pch.h"
#include "cpu_billboard_renderer.h"

namespace
{
    // CPU Billboard 정점 정보 구조체 (공통 특성)
    struct CpuBillboardVertex
    {
        DirectX::XMFLOAT2 corner;
    };

    // CPU Billboard 인스턴스 정보 구조체 (개별 특성)
    struct CpuBillboardInstanceData
    {
        DirectX::XMFLOAT3 position;
        float size;
        DirectX::XMFLOAT4 color;
    };

    // 카메라의 뷰/투영 변환 행렬 구조체
    struct CameraBufferData
    {
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };

    // CPU Billboard 인스턴스 데이터는 입력 레이아웃과 메모리 배치가 일치해야 함
    static_assert(sizeof(CpuBillboardInstanceData) == 32, "CpuBillboardInstanceData size must be 32 bytes.");
    // 카메라 버퍼는 상수 버퍼로 생성되므로, 16바이트 정렬 확인
    static_assert(sizeof(CameraBufferData) % 16 == 0, "Constant buffer size must be 16-byte aligned.(CameraBuffer)");
}

bool CpuBillboardRenderer::Initialize(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // CPU Billboard 렌더링용 셰이더 컴파일 및 생성
    if (!m_shader.Initialize(device, L"shaders/CpuBillboardVertexShader.hlsl", L"shaders/BillboardPixelShader.hlsl"))
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

    // CPU Billboard 인스턴스 입력 레이아웃 생성
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

    // Billboard 투명도 표현에 사용할 Blend State 객체 생성
    if (!CreateAlphaBlendState(device))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    // Billboard의 깊이 버퍼 쓰기 설정을 끄기 위한 Depth Stencil State 객체 생성
    if (!CreateDepthStencilState(device))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    return true;
}

bool CpuBillboardRenderer::CreateQuadBuffers(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // Billboard Quad를 구성할 정점 데이터
    const std::vector<CpuBillboardVertex> vertices =
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
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(CpuBillboardVertex) * vertices.size());
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    // 생성될 버퍼에 채워 넣을 초기화 데이터
    D3D11_SUBRESOURCE_DATA initialData = {};
    // Billboard 정점 데이터 배열을 초기화 데이터로 설정
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
        MessageBoxW(nullptr, L"Failed to create CPU billboard vertex buffer.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

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
    // Billboard 인덱스 데이터 배열을 초기화 데이터로 설정
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
        MessageBoxW(nullptr, L"Failed to create CPU billboard index buffer.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 정점 바이트 크기 멤버 변수 초기화
    m_vertexStride = sizeof(CpuBillboardVertex);
    // 인스턴스 바이트 크기 멤버 변수 초기화
    m_instanceStride = sizeof(CpuBillboardInstanceData);
    // 인덱스 개수 멤버 변수 초기화
    m_indexCount = static_cast<UINT>(indices.size());

    return true;
}

bool CpuBillboardRenderer::CreateInputLayout(ID3D11Device* device, ID3DBlob* vertexShaderBlob)
{
    // 디바이스 또는 정점 셰이더 블롭이 누락된 경우
    if (!device || !vertexShaderBlob)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device or vertex shader blob.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // CPU Billboard 정점/인스턴스 입력 레이아웃 설명자 구조체 배열
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
        },
        {
            "POSITION",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            1,
            0,
            D3D11_INPUT_PER_INSTANCE_DATA,
            1
        },
        {
            "TEXCOORD",
            1,
            DXGI_FORMAT_R32_FLOAT,
            1,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_INSTANCE_DATA,
            1
        },
        {
            "COLOR",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_INSTANCE_DATA,
            1
        }
    };

    // CPU Billboard 입력 레이아웃 객체 생성
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
        MessageBoxW(nullptr, L"Failed to create CPU billboard input layout.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

bool CpuBillboardRenderer::CreateCameraBuffer(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

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
        MessageBoxW(nullptr, L"Failed to create CPU billboard camera buffer.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

bool CpuBillboardRenderer::CreateAlphaBlendState(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // Alpha blending 설정
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;

    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    // Blend State 객체 생성
    HRESULT hr = device->CreateBlendState
    (
        &blendDesc,
        m_alphaBlendState.GetAddressOf()
    );

    // Blend State 객체 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create CPU billboard alpha blend state.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

bool CpuBillboardRenderer::CreateDepthStencilState(ID3D11Device* device)
{
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D11_COMPARISON_LESS;

    // Depth Stencil State 객체 생성
    HRESULT hr = device->CreateDepthStencilState
    (
        &desc,
        m_noDepthWriteState.GetAddressOf()
    );

    // Depth Stencil State 객체 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create CPU billboard depth stencil state.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

bool CpuBillboardRenderer::EnsureInstanceBuffer(ID3D11DeviceContext* context, std::size_t instanceCount)
{
    // 디바이스 컨텍스트가 누락된 경우
    if (!context)
    {
        return false;
    }

    // 요청한 인스턴스 개수를 이미 담을 수 있는 경우
    if (m_instanceBuffer.Get() && m_instanceCapacity >= instanceCount)
    {
        return true;
    }

    // 디바이스 컨텍스트에서 디바이스 조회
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    context->GetDevice(device.GetAddressOf());

    // 디바이스를 조회하지 못한 경우
    if (!device)
    {
        return false;
    }

    // 인스턴스 버퍼 설명자 구조체
    D3D11_BUFFER_DESC bufferDesc = {};
    // 버퍼를 구성할 인스턴스 데이터 배열의 메모리 크기 설정
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(CpuBillboardInstanceData) * instanceCount);
    // 매 프레임 CPU에서 새 인스턴스 데이터를 업로드하기 위해 동적 버퍼로 설정
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    // 버퍼의 사용 용도를 파이프라인의 정점 버퍼로 지정
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    // CPU가 쓰기 위해 버퍼에 접근할 수 있도록 설정
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    // 기타 특수 기능 사용하지 않음
    bufferDesc.MiscFlags = 0;
    // Structured Buffer가 아니므로 기본값으로 설정
    bufferDesc.StructureByteStride = 0;

    // CPU Billboard 인스턴스 버퍼 생성
    HRESULT hr = device->CreateBuffer
    (
        &bufferDesc,
        nullptr,
        m_instanceBuffer.ReleaseAndGetAddressOf()
    );

    // 버퍼 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create CPU billboard instance buffer.", L"CpuBillboardRenderer Error", MB_OK | MB_ICONERROR);

        m_instanceCapacity = 0;
        return false;
    }

    // 현재 인스턴스 버퍼 용량 갱신
    m_instanceCapacity = instanceCount;

    return true;
}

void CpuBillboardRenderer::UpdateCameraBuffer(ID3D11DeviceContext* context, const Camera& camera)
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

bool CpuBillboardRenderer::UpdateInstanceBuffer(ID3D11DeviceContext* context, const std::vector<Billboard>& billboards)
{
    // 디바이스 컨텍스트가 누락된 경우
    if (!context)
    {
        return false;
    }

    // 인스턴스가 없는 경우 실패 처리
    if (billboards.empty())
    {
        return false;
    }

    // 현재 Billboard 개수를 담을 수 있는 인스턴스 버퍼 확보
    if (!EnsureInstanceBuffer(context, billboards.size()))
    {
        return false;
    }

    // GPU 인스턴스 버퍼에 데이터를 쓰기 위해 매핑
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    HRESULT hr = context->Map
    (
        m_instanceBuffer.Get(),
        0,
        D3D11_MAP_WRITE_DISCARD,
        0,
        &mappedResource
    );

    // 매핑에 실패한 경우
    if (FAILED(hr))
    {
        return false;
    }

    CpuBillboardInstanceData* instanceData = static_cast<CpuBillboardInstanceData*>(mappedResource.pData);

    // CPU Billboard 목록을 GPU 인스턴스 데이터로 변환
    for (std::size_t i = 0; i < billboards.size(); ++i)
    {
        instanceData[i].position = billboards[i].position;
        instanceData[i].size = billboards[i].size;
        instanceData[i].color = billboards[i].color;
    }

    // GPU 인스턴스 버퍼 매핑 해제
    context->Unmap(m_instanceBuffer.Get(), 0);

    return true;
}

void CpuBillboardRenderer::Render
(
    ID3D11DeviceContext* context,
    const Camera& camera,
    const std::vector<Billboard>& billboards
)
{
    // 디바이스 컨텍스트가 누락된 경우
    if (!context)
    {
        return;
    }

    // 렌더링할 Billboard가 없는 경우
    if (billboards.empty())
    {
        return;
    }

    // 현재 프레임에서 사용할 인스턴스 데이터를 GPU 버퍼에 업로드
    if (!UpdateInstanceBuffer(context, billboards))
    {
        return;
    }

    // CPU Billboard 셰이더 바인딩
    m_shader.Bind(context);

    // 현재 프레임에서 사용할 카메라 변환 행렬을 정점 셰이더에 전달
    UpdateCameraBuffer(context, camera);

    // CPU Billboard 입력 레이아웃 설정
    context->IASetInputLayout(m_inputLayout.Get());

    // 정점 버퍼와 인스턴스 버퍼를 함께 입력 조립 단계에 바인딩
    ID3D11Buffer* vertexBuffers[] =
    {
        m_vertexBuffer.Get(),
        m_instanceBuffer.Get()
    };

    const UINT strides[] =
    {
        m_vertexStride,
        m_instanceStride
    };

    const UINT offsets[] =
    {
        0,
        0
    };

    context->IASetVertexBuffers
    (
        0,
        2,
        vertexBuffers,
        strides,
        offsets
    );

    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Billboard alpha 값을 기준으로 배경과 섞이도록 Alpha Blend State 설정
    const float blendFactor[4] =
    {
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };

    // 출력 병합기 단계에 Blend State 바인딩
    context->OMSetBlendState
    (
        m_alphaBlendState.Get(),
        blendFactor,
        0xffffffff
    );

    // 출력 병합기 단계에 Depth Stencil State 바인딩
    context->OMSetDepthStencilState
    (
        m_noDepthWriteState.Get(),
        0
    );

    // 모든 CPU Billboard 인스턴스를 한 번의 Draw Call을 통해 렌더링
    context->DrawIndexedInstanced
    (
        m_indexCount,
        static_cast<UINT>(billboards.size()),
        0,
        0,
        0
    );

    // 이후 렌더링 상태에 영향을 주지 않도록 기본 Depth Stencil State로 복원
    context->OMSetDepthStencilState
    (
        nullptr,
        0
    );

    // 이후 렌더링 상태에 영향을 주지 않도록 기본 Blend State로 복원
    context->OMSetBlendState
    (
        nullptr,
        blendFactor,
        0xffffffff
    );
}