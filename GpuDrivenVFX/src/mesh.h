#pragma once

class Mesh
{
public:
    // Mesh 클래스 생성자
    Mesh() = default;
    // Mesh 클래스 소멸자
    ~Mesh() = default;

    // DirectX 11 최소 렌더링 파이프라인 검증용 삼각형 정점 리소스 초기화 함수
    bool InitializeTriangle(ID3D11Device* device, ID3DBlob* vertexShaderBlob);

    // Mesh를 그리기 위한 그래픽 파이프라인 설정 및 Draw call 호출 함수
    void Draw(ID3D11DeviceContext* context);

private:
    // GPU 메모리에 생성되는 정점 데이터 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    // 정점 버퍼의 데이터 구조를 정점 셰이더에 매칭하는 입력 레이아웃 객체
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

    // 정점 한 개가 차지하는 바이트 크기
    UINT m_vertexStride = 0;
    // 버퍼에 저장된 총 정점의 개수
    UINT m_vertexCount = 0;
};