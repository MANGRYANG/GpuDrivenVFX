#include "pch.h"
#include "renderer.h"

bool Renderer::Initialize(HWND hwnd, int width, int height)
{
    // 렌더 타겟의 가로 해상도 설정
    m_width = width;
    // 렌더 타겟의 세로 해상도 설정
    m_height = height;

    // 디바이스, 스왑 체인 생성에 실패한 경우 실패 처리
    if (!CreateDeviceAndSwapChain(hwnd, width, height))
    {
        return false;
    }

    // 렌더 타겟 뷰 생성에 실패한 경우 실패 처리
    if (!CreateRenderTargetView())
    {
        return false;
    }

    // 깊이-스텐실 뷰 생성에 실패한 경우 실패 처리
    if (!CreateDepthStencilView(width, height))
    {
        return false;
    }

    // 뷰포트 설정
    SetViewport(width, height);

    return true;
}

bool Renderer::CreateDeviceAndSwapChain(HWND hwnd, int width, int height)
{
    // 스왑 체인 설명자 구조체
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    // 백 버퍼 표시 모드 설정
    // 백 버퍼의 가로 픽셀 크기 설정
    swapChainDesc.BufferDesc.Width = width;
    // 백 버퍼의 세로 픽셀 크기 설정
    swapChainDesc.BufferDesc.Height = height;
    // 목표 주사율을 60Hz로 설정
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    // 화면 색상 데이터 포맷 지정 (RGBA 각각 8비트씩 총 32비트 사용)
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    // 스캔라인 출력 순서 설정 (기본값 사용)
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    // 화면 스케일링 방식 설정 (기본값 사용)
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    // 안티앨리어싱을 사용하지 않도록 설정
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    // 생성할 백 버퍼의 사용 용도 설정 (렌더 타겟으로 사용)
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    // 생성할 백 버퍼의 개수를 1개로 설정
    swapChainDesc.BufferCount = 1;
    // 스왑 체인의 결과물을 전달할 윈도우의 핸들
    swapChainDesc.OutputWindow = hwnd;
    // 창 모드로 설정
    swapChainDesc.Windowed = TRUE;
    // 버퍼 교체 시 이전 내용을 삭제하도록 설정
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    // 기타 특수 기능 사용하지 않음
    swapChainDesc.Flags = 0;

    // 디바이스 생성 시 사용할 특수 기능을 담는 변수
    UINT createDeviceFlags = 0;

// Debug 모드로 빌드하는 경우 debug layer 기능 플래그 추가
#if defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // 렌더러가 요구하는 그래픽 카드의 하드웨어 기능 수준 목록
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // 그래픽 카드 검사 후 최종적으로 선택될 기능 수준을 담는 변수
    D3D_FEATURE_LEVEL createdFeatureLevel = {};

    // 디바이스 및 스왑 체인 생성
    HRESULT hr = D3D11CreateDeviceAndSwapChain
    (
        nullptr,                        // 메인 그래픽 카드를 기본값으로 사용
        D3D_DRIVER_TYPE_HARDWARE,       // 드라이버 타입을 하드웨어 방식으로 설정
        nullptr,                        // 드라이버가 하드웨어 방식이라 사용하지 않음
        createDeviceFlags,              // 디버그 모드 관련 플래그 전달
        featureLevels,                  // 그래픽 카드에 요구할 기능 수준 목록 전달
        ARRAYSIZE(featureLevels),       // 기능 수준 목록의 원소 개수 전달
        D3D11_SDK_VERSION,              // DirectX 11 SDK 버전 전달
        &swapChainDesc,                 // 스왑 체인 구조체 주소 전달
        m_swapChain.GetAddressOf(),     // 할당된 스왑 체인 객체를 저장할 멤버 변수의 주소
        m_device.GetAddressOf(),        // 할당된 디바이스 객체를 저장할 멤버 변수의 주소
        &createdFeatureLevel,           // 최종 선정된 기능 수준을 담을 주소 전달
        m_context.GetAddressOf()        // 할당된 디바이스 컨텍스트 객체를 저장할 멤버 변수의 주소
    );

    // 디바이스 및 스왑 체인 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create D3D11 device and swap chain.", L"Renderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

bool Renderer::CreateRenderTargetView()
{
    // 스왑 체인의 백 버퍼를 임시로 가리키는 스마트 포인터
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;

    // backBuffer가 스왑 체인의 백 버퍼에 엑세스할 수 있도록 설정
    HRESULT hr = m_swapChain->GetBuffer
    (
        0,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(backBuffer.GetAddressOf())
    );

    // 스왑 체인에서 백 버퍼를 꺼내오지 못한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to get swap chain back buffer.", L"Renderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // backBuffer와 연결된 렌더 타겟 뷰 생성
    hr = m_device->CreateRenderTargetView
    (
        backBuffer.Get(),
        nullptr,
        m_renderTargetView.GetAddressOf()
    );

    // 렌더 타겟 뷰 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create render target view.", L"Renderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

bool Renderer::CreateDepthStencilView(int width, int height)
{
    // 2D 텍스처 설명자 구조체
    D3D11_TEXTURE2D_DESC depthDesc = {};
    // 텍스처의 가로 픽셀 크기 설정
    depthDesc.Width = width;
    // 텍스처의 세로 픽셀 크기 설정
    depthDesc.Height = height;
    // 밉맵 체인을 생성하지 않고 단일 2D 텍스처 데이터 레이아웃으로 설정
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    // 각 픽셀 데이터의 메모리 포맷 지정 (Depth 24비트, Stencil 8비트)
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // 안티앨리어싱을 사용하지 않도록 설정
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    // 리소스의 메모리 접근 패턴 설정 (기본값)
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    // 출력 병합 단계의 깊이 스텐실 대상으로 텍스처를 바인딩
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    // CPU가 텍스처에 직접 엑세스할 수 없도록 설정
    depthDesc.CPUAccessFlags = 0;
    // 기타 특수 기능 사용하지 않음
    depthDesc.MiscFlags = 0;

    // 2D 텍스처 생성
    HRESULT hr = m_device->CreateTexture2D
    (
        &depthDesc,
        nullptr,
        m_depthStencilBuffer.GetAddressOf()
    );

    // 텍스처 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create depth stencil buffer.", L"Renderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 깊이-스텐실 뷰 생성
    hr = m_device->CreateDepthStencilView
    (
        m_depthStencilBuffer.Get(),
        nullptr,
        m_depthStencilView.GetAddressOf()
    );

    // 깊이-스텐실 뷰 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create depth stencil view.", L"Renderer Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

void Renderer::SetViewport(int width, int height)
{
    // 뷰포트의 차원을 정의하는 구조체
    D3D11_VIEWPORT viewport = {};
    // 뷰포트 영역의 Top-Left X 좌표를 0으로 고정
    viewport.TopLeftX = 0.0f;
    // 뷰포트 영역의 Top-Left Y 좌표를 0으로 고정
    viewport.TopLeftY = 0.0f;
    // 뷰포트의 가로 픽셀 크기 설정
    viewport.Width = static_cast<float>(width);
    // 뷰포트의 세로 픽셀 크기 설정
    viewport.Height = static_cast<float>(height);
    // 뷰포트의 최소 깊이 값 설정
    viewport.MinDepth = 0.0f;
    // 뷰포트의 최대 깊이 값 설정
    viewport.MaxDepth = 1.0f;

    // 래스터라이저 단계에 뷰포트를 설정
    m_context->RSSetViewports(1, &viewport);
}

void Renderer::BeginFrame(float r, float g, float b, float a)
{
    // 화면을 채울 RGBA 색상 데이터 배열을 저장한 배열
    const float clearColor[4] = { r, g, b, a };

    // 출력 병합기 단계에 렌더 타겟 뷰와 깊이-스텐실 뷰를 바인딩
    m_context->OMSetRenderTargets
    (
        1,
        m_renderTargetView.GetAddressOf(),
        m_depthStencilView.Get()
    );

    // 렌더 타겟 뷰가 참조하고 있는 백 버퍼의 모든 픽셀을 지정된 색상으로 초기화
    m_context->ClearRenderTargetView
    (
        m_renderTargetView.Get(),
        clearColor
    );

    // 깊이 버퍼와 스텐실 버퍼 영역을 각각 최대 거리(1.0f) 및 기본값(0)으로 초기화
    m_context->ClearDepthStencilView
    (
        m_depthStencilView.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.0f,
        0
    );
}

void Renderer::EndFrame()
{
    // 백 버퍼와 프론트 버퍼를 교체 (수직 동기화 적용)
    m_swapChain->Present(1, 0);
}