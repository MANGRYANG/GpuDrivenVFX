#pragma once

#include <DirectXMath.h>
#include <vector>

#include "rendering/shader.h"

#include "graphics/camera.h"

// Billboard Quad 정보 구조체
struct Billboard
{
    // Billboard Quad의 월드 공간 위치
    DirectX::XMFLOAT3 position;
    // Billboard Quad의 크기
    float size;
    // Billboard Quad의 색상
    DirectX::XMFLOAT4 color;
};

class BillboardRenderer
{
public:
    // BillboardRenderer 클래스 생성자
    BillboardRenderer() = default;
    // BillboardRenderer 클래스 소멸자
    ~BillboardRenderer() = default;

    // Billboard 렌더링에 필요한 GPU 리소스를 초기화하는 함수
    bool Initialize(ID3D11Device* device);

    // 단일 Billboard Quad를 렌더링하는 함수
    void Render(ID3D11DeviceContext* context, const Camera& camera, const std::vector<Billboard>& billboards);

private:
    // Billboard 정점/인덱스 버퍼와 입력 레이아웃을 생성하는 함수
    bool CreateBuffers(ID3D11Device* device, ID3DBlob* vertexShaderBlob);

    // Billboard 중심점에 적용할 world/view/projection 변환 행렬을 담는 버퍼를 생성하는 함수
    bool CreateTransformBuffer(ID3D11Device* device);

    // Billboard를 카메라 기준으로 펼치기 위한 정보를 담는 버퍼를 생성하는 함수
    bool CreateBillboardInfoBuffer(ID3D11Device* device);

    // Billboard 중심점에 적용할 월드 변환 행렬을 생성하는 함수
    DirectX::XMMATRIX BuildBillboardWorldMatrix(const DirectX::XMFLOAT3& position) const;

    // 현재 프레임에서 사용할 Billboard 변환 행렬을 갱신하는 함수
    void UpdateTransformBuffer(ID3D11DeviceContext* context, const Camera& camera, const DirectX::XMFLOAT3& position);

    // 현재 카메라 기준으로 Billboard를 펼치기 위한 정보를 갱신하는 함수
    void UpdateBillboardInfoBuffer(ID3D11DeviceContext* context, const Camera& camera, const Billboard& billboard);

private:
    // Billboard 렌더링에 사용할 셰이더
    Shader m_shader;

    // Billboard Quad 정점 데이터 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    // Billboard Quad 인덱스 데이터 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
    // Billboard 정점 구조를 Billboard 정점 셰이더에 매칭하는 입력 레이아웃
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

    // Billboard 중심점에 적용할 world/view/projection 변환 행렬을 담는 상수 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_transformBuffer;
    // Billboard를 카메라 기준으로 펼치기 위한 정보를 담는 상수 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_billboardInfoBuffer;

    // 정점 한 개가 차지하는 바이트 크기
    UINT m_vertexStride = 0;
    // 그려야 할 인덱스의 총 개수
    UINT m_indexCount = 0;
};