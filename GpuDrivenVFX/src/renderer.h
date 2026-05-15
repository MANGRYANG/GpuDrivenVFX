#pragma once

class Renderer
{
public:
    // Renderer 클래스 생성자
    Renderer() = default;
    // Renderer 클래스 소멸자
    ~Renderer() = default;

    // 렌더러 초기화 함수
    bool Initialize(HWND hwnd, int width, int height);

    // 프레임을 그리기 전 화면을 배경색으로 채우는 함수
    void BeginFrame(float r, float g, float b, float a);

    // 버퍼를 스왑하여 화면에 최종 결과물을 출력하는 함수
    void EndFrame();

private:
    // 디바이스 및 스왑 체인을 생성하는 함수
    bool CreateDeviceAndSwapChain(HWND hwnd, int width, int height);

    // 렌더 타겟 뷰를 생성하는 함수
    bool CreateRenderTargetView();
    // 깊이-스텐실 뷰를 생성하는 함수
    bool CreateDepthStencilView(int width, int height);

    // 뷰포트를 설정하는 함수 
    void SetViewport(int width, int height);

private:
    // GPU 리소스를 생성하는 객체
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    // 리소스를 조작하고 GPU에 Draw 명령을 내리는 객체
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    // 버퍼 스왑을 통해 화면 송출을 요청하는 객체
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;

    // 렌더 타겟을 가리키는 뷰
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    // 깊이-스텐실 데이터가 저장되는 2D 텍스처 버퍼
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
    // 깊이-스텐실 버퍼에 접근하기 위한 뷰
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;

    // 렌더링 타겟의 가로 해상도
    int m_width = 0;
    // 렌더링 타겟의 세로 해상도
    int m_height = 0;
};