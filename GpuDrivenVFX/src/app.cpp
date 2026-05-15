#include "pch.h"
#include "app.h"

bool App::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    // m_Window에 전달할 윈도우 너비
    constexpr int windowWidth = 1280;
    // m_Window에 전달할 윈도우 높이
    constexpr int windowHeight = 720;

    // 윈도우 초기화에 실패한 경우 실패 처리
    if (!m_window.Initialize(hInstance, nCmdShow, windowWidth, windowHeight, L"GPU-Driven VFX System"))
    {
        return false;
    }

    // 렌더러 초기화에 실패한 경우 실패 처리
    if (!m_renderer.Initialize(m_window.GetHwnd(), m_window.GetWidth(), m_window.GetHeight()))
    {
        return false;
    }

    // 셰이더 컴파일에 실패한 경우 실패 처리
    if (!m_shader.Initialize(m_renderer.GetDevice(), L"shaders/VertexShader.hlsl", L"shaders/PixelShader.hlsl"))
    {
        return false;
    }

    return true;
}

int App::Run()
{
    // 애플리케이션 실행 루프
    while (m_running)
    {
        // OS 메시지 확인 및 처리
        m_window.ProcessMessages(m_running);

        // 종료 메시지를 수신한 경우 루프 탈출
        if (!m_running)
        {
            break;
        }

        // 애플리케이션 내부 데이터 및 상태 업데이트
        Update();
        // 애플리케이션 화면 렌더링
        Render();
    }

    return 0;
}

void App::Update()
{
    // 이후 작성
}

void App::Render()
{
    // 프레임 드로우 준비 및 배경 색 초기화 (#0D141F)
    m_renderer.BeginFrame(0.05f, 0.08f, 0.12f, 1.0f);

    // 셰이더 바인딩
    m_shader.Bind(m_renderer.GetContext());

    // 최종 화면 출력
    m_renderer.EndFrame();
}