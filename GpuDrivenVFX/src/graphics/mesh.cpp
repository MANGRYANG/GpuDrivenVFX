#include "pch.h"
#include "mesh.h"

bool Mesh::Initialize
(
    ID3D11Device* device,
    ID3DBlob* vertexShaderBlob,
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices
)
{
    // 디바이스 또는 정점 셰이더 블롭이 누락된 경우
    if (!device || !vertexShaderBlob)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Invalid device or vertex shader blob.", L"Mesh Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 정점 구조체 배열 또는 인덱스 배열이 비어 있는 경우
    if (vertices.empty() || indices.empty())
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Mesh vertices or indices are empty.", L"Mesh Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // 버퍼 설명자 구조체
    D3D11_BUFFER_DESC vertexBufferDesc = {};
    // 버퍼를 구성할 데이터 배열의 메모리 크기 설정
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
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
    // 정점 데이터 배열을 초기화 데이터로 설정
    initialData.pSysMem = vertices.data();

    // 정점 버퍼 생성
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
        MessageBoxW(nullptr, L"Failed to create vertex buffer.", L"Mesh Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 인덱스 버퍼 설명자 구조체
    D3D11_BUFFER_DESC indexBufferDesc = {};
    // 버퍼를 구성할 데이터 배열의 메모리 크기 설정
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
    // 인덱스 데이터 배열을 초기화 데이터로 설정
    initialData.pSysMem = indices.data();

    // 인덱스 버퍼 생성
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
        MessageBoxW(nullptr, L"Failed to create index buffer.", L"Mesh Error", MB_OK | MB_ICONERROR);

        return false;
    }
    

    // 입력 레이아웃 설명자 구조체 배열
    const D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
    {
        {
            "POSITION",                     // 의미 체계명
            0,                              // 의미 체계 인덱스 (동일 체계명 구분용)
            DXGI_FORMAT_R32G32B32_FLOAT,    // 데이터 포맷
            0,                              // 파이프라인 입력 슬롯 번호
            0,                              // 정점 시작점으로부터의 바이트 오프셋 (첫 원소이므로 0)
            D3D11_INPUT_PER_VERTEX_DATA,    // 입력 데이터 클래스 식별 (정점별 데이터)
            0                               // 정점별 데이터인 경우 0으로 설정
        },
        {
            "NORMAL",                       // 의미 체계명
            0,                              // 의미 체계 인덱스
            DXGI_FORMAT_R32G32B32_FLOAT,    // 데이터 포맷
            0,                              // 파이프라인 입력 슬롯 번호
            D3D11_APPEND_ALIGNED_ELEMENT,   // 이전 원소 크기에 맞춰 바이트 오프셋 자동 정렬 배치
            D3D11_INPUT_PER_VERTEX_DATA,    // 입력 데이터 클래스 식별 (정점별 데이터)
            0                               // 정점별 데이터인 경우 0으로 설정
        },
        {
            "COLOR",                        // 의미 체계명
            0,                              // 의미 체계 인덱스
            DXGI_FORMAT_R32G32B32A32_FLOAT, // 데이터 포맷
            0,                              // 파이프라인 입력 슬롯 번호
            D3D11_APPEND_ALIGNED_ELEMENT,   // 이전 원소 크기에 맞춰 바이트 오프셋 자동 정렬 배치
            D3D11_INPUT_PER_VERTEX_DATA,    // 입력 데이터 클래스 식별 (정점별 데이터)
            0                               // 정점별 데이터인 경우 0으로 설정
        }
    };

    // 입력 레이아웃 객체 생성
    hr = device->CreateInputLayout
    (
        inputLayoutDesc,                        // 입력 레이아웃 설명자 구조체 배열
        ARRAYSIZE(inputLayoutDesc),             // 입력 레이아웃 설명자 구조체 배열의 원소 개수
        vertexShaderBlob->GetBufferPointer(),   // 정점 셰이더 바이트코드의 주소
        vertexShaderBlob->GetBufferSize(),      // 정점 셰이더 바이트코드의 크기
        m_inputLayout.GetAddressOf()            // 생성된 입력 레이아웃 객체를 가리킬 포인터 주소
    );

    // 입력 레이아웃 객체 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create input layout.", L"Mesh Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 정점 바이트 크기 멤버 변수 초기화
    m_vertexStride = sizeof(Vertex);
    // 인덱스 개수 멤버 변수 초기화
    m_indexCount = static_cast<UINT>(indices.size());

    return true;
}

void Mesh::Draw(ID3D11DeviceContext* context)
{
    // 디바이스 컨텍스트가 누락된 경우 실패 처리
    if (!context)
    {
        return;
    }

    // 시작 오프셋 (0으로 설정)
    UINT offset = 0;

    // 파이프라인 입력 슬롯에 전달할 정점 버퍼 포인터 배열
    ID3D11Buffer* vertexBuffers[] =
    {
        // 0번 슬롯에 할당할 정점 버퍼
        m_vertexBuffer.Get()
    };

    // 입력 조립기 단계의 입력 레이아웃 설정
    context->IASetInputLayout(m_inputLayout.Get());

    // 입력 조립기 단계의 정점 버퍼 설정
    context->IASetVertexBuffers
    (
        0,                  // 버퍼를 바인딩할 입력 슬롯 번호
        1,                  // 설정할 버퍼의 개수
        vertexBuffers,      // 정점 버퍼 포인터들이 담긴 배열
        &m_vertexStride,    // 정점 한 개가 차지하는 바이트 크기
        &offset             // 버퍼 시작점으로부터의 바이트 오프셋
    );

    // 입력 조립기 단계의 인덱스 버퍼 설정
    context->IASetIndexBuffer
    (
        m_indexBuffer.Get(),  // 인덱스 버퍼
        DXGI_FORMAT_R32_UINT, // 인덱스 버퍼의 데이터 형식
        0                     // 버퍼 시작점으로부터의 바이트 오프셋
    );

    // 입력 조립기 단계의 데이터 형식 설정 
    context->IASetPrimitiveTopology
    (
        // 정점 데이터를 삼각형 리스트로 해석
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );

    // Draw call 호출
    context->DrawIndexed
    (
        m_indexCount,   // 그려야 할 인덱스의 총 개수
        0,              // 버퍼에서 처음으로 읽을 인덱스
        0               // 정점 버퍼에 더할 기준 정점 오프셋
    );
}