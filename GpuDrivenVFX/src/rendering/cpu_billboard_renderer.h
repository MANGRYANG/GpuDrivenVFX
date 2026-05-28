#pragma once

#include <DirectXMath.h>
#include <cstddef>
#include <vector>

#include "graphics/camera.h"
#include "rendering/billboard.h"
#include "rendering/shader.h"

// CPU에서 생성한 Billboard 목록을 인스턴스 버퍼 기반으로 렌더링하는 클래스
class CpuBillboardRenderer
{
public:
    // CpuBillboardRenderer 클래스 생성자
    CpuBillboardRenderer() = default;
    // CpuBillboardRenderer 클래스 소멸자
    ~CpuBillboardRenderer() = default;

    // CPU Billboard 렌더링에 필요한 GPU 리소스를 초기화하는 함수
    bool Initialize(ID3D11Device* device);

    // CPU에서 생성한 Billboard 목록을 인스턴스 버퍼 기반으로 렌더링하는 함수
    void Render
    (
        ID3D11DeviceContext* context,
        const Camera& camera,
        const std::vector<Billboard>& billboards
    );

private:
    // Billboard Quad 정점/인덱스 버퍼를 생성하는 함수
    bool CreateQuadBuffers(ID3D11Device* device);

    // CPU Billboard 인스턴스 입력 레이아웃을 생성하는 함수
    bool CreateInputLayout(ID3D11Device* device, ID3DBlob* vertexShaderBlob);

    // Billboard 렌더링에 사용할 카메라 변환 상수 버퍼를 생성하는 함수
    bool CreateCameraBuffer(ID3D11Device* device);

    // 현재 Billboard 개수를 담을 수 있는 인스턴스 버퍼를 확보하는 함수
    bool EnsureInstanceBuffer(ID3D11DeviceContext* context, std::size_t instanceCount);

    // 현재 프레임에서 사용할 카메라 변환 행렬을 갱신하는 함수
    void UpdateCameraBuffer(ID3D11DeviceContext* context, const Camera& camera);

    // 현재 프레임에서 렌더링할 Billboard 목록을 인스턴스 버퍼에 업로드하는 함수
    bool UpdateInstanceBuffer(ID3D11DeviceContext* context, const std::vector<Billboard>& billboards);

private:
    // CPU Billboard 렌더링에 사용할 셰이더
    Shader m_shader;

    // Billboard Quad 정점 데이터 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    // Billboard Quad 인덱스 데이터 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
    // CPU Billboard 인스턴스 데이터 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_instanceBuffer;
    // CPU Billboard 정점/인스턴스 입력 레이아웃
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    // Billboard 렌더링에 사용할 view/projection 상수 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_cameraBuffer;

    // Quad 정점 한 개가 차지하는 바이트 크기
    UINT m_vertexStride = 0;
    // Billboard 인스턴스 한 개가 차지하는 바이트 크기
    UINT m_instanceStride = 0;
    // Billboard Quad 인덱스 개수
    UINT m_indexCount = 0;
    // 현재 인스턴스 버퍼가 담을 수 있는 Billboard 개수
    std::size_t m_instanceCapacity = 0;
};