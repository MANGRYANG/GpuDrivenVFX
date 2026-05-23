#include "pch.h"
#include "billboard_renderer.h"

namespace
{
    // Billboard 정점 정보 구조체
    struct BillboardVertex
    {
        DirectX::XMFLOAT3 center;
        DirectX::XMFLOAT2 corner;
    };

    // Billboard 변환 버퍼 데이터 구조체
    struct TransformBufferData
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };

    // Billboard 생성 정보 버퍼 데이터 구조체 (16바이트 패킹 고려 배치)
    struct BillboardInfoBufferData
    {
        DirectX::XMFLOAT3 cameraRight;
        float billboardSize;

        DirectX::XMFLOAT3 cameraUp;
        float padding;

        DirectX::XMFLOAT4 billboardColor;
    };

    // 변환 버퍼는 상수 버퍼로 생성되므로, 16바이트 정렬 확인
    static_assert(sizeof(TransformBufferData) % 16 == 0, "Constant buffer size must be 16-byte aligned.(TransformBuffer)");
    // Billboard 정보 버퍼는 상수 버퍼로 생성되므로, 16바이트 정렬 확인
    static_assert(sizeof(BillboardInfoBufferData) % 16 == 0, "Constant buffer size must be 16-byte aligned.(BillboardInfoBuffer)");
}

bool BillboardRenderer::Initialize(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device.", L"BillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // Billboard 렌더링용 셰이더 컴파일 및 생성
    if (!m_shader.Initialize(device, L"shaders/BillboardVertexShader.hlsl", L"shaders/BillboardPixelShader.hlsl"))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // Billboard Quad 렌더링에 필요한 버퍼와 입력 레이아웃 생성
    if (!CreateBuffers(device, m_shader.GetVertexShaderBlob()))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // Billboard 변환 정보를 담을 상수 버퍼 생성
    if (!CreateTransformBuffer(device))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    // Billboard 생성 정보를 담을 상수 버퍼 생성
    if (!CreateBillboardInfoBuffer(device))
    {
        // 생성하지 못한 경우 실패 처리
        return false;
    }

    return true;
}

bool BillboardRenderer::CreateBuffers(ID3D11Device* device, ID3DBlob* vertexShaderBlob)
{
    // 디바이스 또는 정점 셰이더 블롭이 누락된 경우
    if (!device || !vertexShaderBlob)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device or vertex shader blob.", L"BillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // Billboard Quad를 구성할 정점 데이터
    const std::vector<BillboardVertex> vertices =
    {
        {
            DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
            DirectX::XMFLOAT2(-0.5f, 0.5f),
        },
        {
            DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
            DirectX::XMFLOAT2(0.5f, 0.5f),
        },
        {
            DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
            DirectX::XMFLOAT2(-0.5f, -0.5f),
        },
        {
            DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
            DirectX::XMFLOAT2(0.5f, -0.5f),
        }
    };

    // Billboard Quad를 구성할 인덱스 데이터
    const std::vector<uint32_t> indices =
    {
        0, 1, 2,
        1, 3, 2
    };

    // 정점 버퍼 설명자 구조체
    D3D11_BUFFER_DESC vertexBufferDesc = {};
    // 버퍼를 구성할 정점 데이터 배열의 메모리 크기 설정
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(BillboardVertex) * vertices.size());
    // 버퍼의 사용 방식을 생성 후 변경 불가능한 읽기 전용으로 설정
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    // 버퍼의 사용 용도를 파이프라인의 정점 버퍼로 지정
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    // CPU가 버퍼에 직접 엑세스할 수 없도록 설정
    vertexBufferDesc.CPUAccessFlags = 0;
    // 기타 특수 기능 사용하지 않음
    vertexBufferDesc.MiscFlags = 0;
    // Structured Buffer가 아니므로 기본값으로 설정
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
        MessageBoxW(nullptr, L"Failed to create billboard vertex buffer.", L"BillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 인덱스 버퍼 설명자 구조체
    D3D11_BUFFER_DESC indexBufferDesc = {};
    // 버퍼를 구성할 인덱스 데이터 배열의 메모리 크기 설정
    indexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
    // 버퍼의 사용 방식을 생성 후 변경 불가능한 읽기 전용으로 설정
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    // 버퍼의 사용 용도를 파이프라인의 인덱스 버퍼로 지정
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    // CPU가 버퍼에 직접 엑세스할 수 없도록 설정
    indexBufferDesc.CPUAccessFlags = 0;
    // 기타 특수 기능 사용하지 않음
    indexBufferDesc.MiscFlags = 0;
    // Structured Buffer가 아니므로 기본값으로 설정
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
        MessageBoxW(nullptr, L"Failed to create billboard index buffer.", L"BillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // Billboard 정점 입력 레이아웃 설명자 구조체 배열
    const D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
    {
        {
            "POSITION",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            0,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        },
        {
            "TEXCOORD",
            0,
            DXGI_FORMAT_R32G32_FLOAT,
            0,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        }
    };

    // Billboard 입력 레이아웃 객체 생성
    hr = device->CreateInputLayout
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
        MessageBoxW(nullptr, L"Failed to create billboard input layout.", L"BillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 정점 바이트 크기 멤버 변수 초기화
    m_vertexStride = sizeof(BillboardVertex);
    // 인덱스 개수 멤버 변수 초기화
    m_indexCount = static_cast<UINT>(indices.size());

    return true;
}

bool BillboardRenderer::CreateTransformBuffer(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device.", L"BillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 변환 버퍼 설명자 구조체
    D3D11_BUFFER_DESC bufferDesc = {};
    // 버퍼를 구성할 데이터 구조체의 메모리 크기 설정
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
    HRESULT hr = device->CreateBuffer
    (
        &bufferDesc,
        nullptr,
        m_transformBuffer.GetAddressOf()
    );

    // 버퍼 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create billboard transform buffer.", L"BillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

bool BillboardRenderer::CreateBillboardInfoBuffer(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device.", L"BillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // Billboard 정보 버퍼 설명자 구조체
    D3D11_BUFFER_DESC bufferDesc = {};
    // 버퍼를 구성할 데이터 구조체의 메모리 크기 설정
    bufferDesc.ByteWidth = sizeof(BillboardInfoBufferData);
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

    // Billboard 정보 버퍼 생성
    HRESULT hr = device->CreateBuffer
    (
        &bufferDesc,
        nullptr,
        m_billboardInfoBuffer.GetAddressOf()
    );

    // 버퍼 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create billboard info buffer.", L"BillboardRenderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

DirectX::XMMATRIX BillboardRenderer::BuildBillboardWorldMatrix(const DirectX::XMFLOAT3& position) const
{
    // Billboard 중심 위치를 월드 공간으로 옮기는 이동 행렬 생성
    return DirectX::XMMatrixTranslation
    (
        position.x,
        position.y,
        position.z
    );
}

void BillboardRenderer::UpdateTransformBuffer(ID3D11DeviceContext* context, const Camera& camera, const DirectX::XMFLOAT3& position)
{
    // 디바이스 컨텍스트나 변환 버퍼가 누락된 경우 실패 처리
    if (!context || !m_transformBuffer.Get())
    {
        return;
    }

    // Billboard 중심 위치에 적용할 월드 변환 행렬
    const DirectX::XMMATRIX world = BuildBillboardWorldMatrix(position);

    // 뷰 변환 행렬
    const DirectX::XMMATRIX view = camera.GetViewMatrix();

    // 투영 변환 행렬
    const DirectX::XMMATRIX projection = camera.GetProjectionMatrix();

    // 변환 버퍼 데이터 설정
    TransformBufferData transformData = {};
    DirectX::XMStoreFloat4x4(&transformData.world, DirectX::XMMatrixTranspose(world));
    DirectX::XMStoreFloat4x4(&transformData.view, DirectX::XMMatrixTranspose(view));
    DirectX::XMStoreFloat4x4(&transformData.projection, DirectX::XMMatrixTranspose(projection));

    // CPU 메모리의 변환 버퍼 데이터를 GPU 버퍼에 업데이트
    context->UpdateSubresource
    (
        m_transformBuffer.Get(),
        0,
        nullptr,
        &transformData,
        0,
        0
    );
}

void BillboardRenderer::UpdateBillboardInfoBuffer(ID3D11DeviceContext* context, const Camera& camera, const Billboard& billboard)
{
    // 디바이스 컨텍스트나 Billboard 정보 버퍼가 누락된 경우 실패 처리
    if (!context || !m_billboardInfoBuffer.Get())
    {
        return;
    }

    DirectX::XMFLOAT3 cameraRight = {};
    DirectX::XMFLOAT3 cameraUp = {};
    DirectX::XMFLOAT3 cameraForward = {};

    // Camera에서 현재 카메라의 정규화된 기저 벡터를 가져옴
    camera.GetCameraAxes(cameraRight, cameraUp, cameraForward);

    // Billboard 정보 버퍼 데이터 설정
    BillboardInfoBufferData billboardInfoData = {};
    billboardInfoData.cameraRight = cameraRight;
    billboardInfoData.billboardSize = billboard.size;
    billboardInfoData.cameraUp = cameraUp;
    billboardInfoData.padding = 0.0f;
    billboardInfoData.billboardColor = billboard.color;

    // CPU 메모리의 Billboard 정보 버퍼 데이터를 GPU 버퍼에 업데이트
    context->UpdateSubresource
    (
        m_billboardInfoBuffer.Get(),
        0,
        nullptr,
        &billboardInfoData,
        0,
        0
    );
}

void BillboardRenderer::Render(ID3D11DeviceContext* context, const Camera& camera, const std::vector<Billboard>& billboards)
{
    // 디바이스 컨텍스트가 누락된 경우 실패 처리
    if (!context)
    {
        return;
    }

    // 렌더링할 Billboard가 없는 경우 처리하지 않음
    if (billboards.empty())
    {
        return;
    }

    // Billboard 렌더링용 셰이더 바인딩
    m_shader.Bind(context);

    // 파이프라인 입력 슬롯에 전달할 상수 버퍼 포인터 배열
    ID3D11Buffer* constantBuffers[] =
    {
        m_transformBuffer.Get(),
        m_billboardInfoBuffer.Get()
    };

    // 정점 셰이더 단계의 상수 버퍼 설정
    context->VSSetConstantBuffers
    (
        0,
        2,
        constantBuffers
    );

    // 시작 오프셋
    UINT offset = 0;

    // 파이프라인 입력 슬롯에 전달할 정점 버퍼 포인터 배열
    ID3D11Buffer* vertexBuffers[] =
    {
        m_vertexBuffer.Get()
    };

    // 입력 조립기 단계의 입력 레이아웃 설정
    context->IASetInputLayout(m_inputLayout.Get());

    // 입력 조립기 단계의 정점 버퍼 설정
    context->IASetVertexBuffers
    (
        0,
        1,
        vertexBuffers,
        &m_vertexStride,
        &offset
    );

    // 입력 조립기 단계의 인덱스 버퍼 설정
    context->IASetIndexBuffer
    (
        m_indexBuffer.Get(),
        DXGI_FORMAT_R32_UINT,
        0
    );

    // 입력 조립기 단계의 데이터 형식 설정
    context->IASetPrimitiveTopology
    (
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );

    // Billboard 데이터 목록을 순회하며 각각의 위치, 크기, 색상으로 렌더링
    for (const Billboard& billboard : billboards)
    {
        // 현재 Billboard의 위치를 기준으로 변환 버퍼 갱신
        UpdateTransformBuffer(context, camera, billboard.position);

        // 현재 Billboard의 크기, 색상, 카메라 기준 축 정보 갱신
        UpdateBillboardInfoBuffer(context, camera, billboard);

        // 현재 Billboard Quad 렌더링
        context->DrawIndexed
        (
            m_indexCount,
            0,
            0
        );
    }
}