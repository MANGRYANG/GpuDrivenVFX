#pragma once

#include <cstddef>

#include "graphics/camera.h"

#include "rendering/shader.h"

// GPU Particle Buffer를 Billboard 형태로 렌더링하는 클래스
class GpuBillboardRenderer
{
public:
    // GpuBillboardRenderer 클래스 생성자
    GpuBillboardRenderer() = default;
    // GpuBillboardRenderer 클래스 소멸자
    ~GpuBillboardRenderer() = default;

    // GPU Billboard 렌더링에 필요한 GPU 리소스를 초기화하는 함수
    bool Initialize(ID3D11Device* device);

    // GPU Particle Buffer를 읽어 Billboard Quad를 렌더링하는 함수
    void Render
    (
        ID3D11DeviceContext* context,
        const Camera& camera,
        ID3D11ShaderResourceView* particleSrv,
        ID3D11ShaderResourceView* aliveIndexSrv,
        ID3D11ShaderResourceView* aliveCountSrv
    );

private:
    // Billboard Quad 정점/인덱스 버퍼를 생성하는 함수
    bool CreateQuadBuffers(ID3D11Device* device);

    // GPU Billboard 입력 레이아웃을 생성하는 함수
    bool CreateInputLayout(ID3D11Device* device, ID3DBlob* vertexShaderBlob);

    // Billboard 렌더링에 사용할 카메라 변환 상수 버퍼를 생성하는 함수
    bool CreateCameraBuffer(ID3D11Device* device);

    // DrawIndexedInstancedIndirect에 사용할 argument buffer를 생성하는 함수
    bool CreateIndirectArgsBuffer(ID3D11Device* device);

    // Indirect draw argument buffer를 갱신할 Compute Shader를 생성하는 함수
    bool CreateIndirectArgsComputeShader(ID3D11Device* device);

    // 현재 프레임에서 사용할 카메라 변환 행렬을 갱신하는 함수
    void UpdateCameraBuffer(ID3D11DeviceContext* context, const Camera& camera);

    // 현재 active Particle 개수를 기반으로 indirect draw argument buffer를 갱신하는 함수
    void UpdateIndirectArgsBuffer(ID3D11DeviceContext* context, ID3D11ShaderResourceView* aliveCountSrv);

private:
    // GPU Billboard 렌더링에 사용할 셰이더
    Shader m_shader;

    // Billboard Quad 정점 데이터 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    // Billboard Quad 인덱스 데이터 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
    // GPU Billboard 정점 입력 레이아웃
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    // Billboard 렌더링에 사용할 view/projection 상수 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_cameraBuffer;

    // DrawIndexedInstancedIndirect 호출에 사용할 argument buffer
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indirectArgsBuffer;

    // Compute Shader에서 indirect argument buffer를 쓰기 위한 UAV
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_indirectArgsUav;

    // indirect argument buffer를 갱신하는 Compute Shader
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_indirectArgsComputeShader;

    // Quad 정점 한 개가 차지하는 바이트 크기
    UINT m_vertexStride = 0;
    // Billboard Quad 인덱스 개수
    UINT m_indexCount = 0;
};