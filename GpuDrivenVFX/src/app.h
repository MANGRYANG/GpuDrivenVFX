#pragma once

#include "window.h"
#include "renderer.h"
#include "shader.h"

class App
{
public:
    // App 클래스 생성자
    App() = default;
    // App 클래스 소멸자
    ~App() = default;

    // 애플리케이션 실행 시 최초 1회 실행되는 초기화 함수
    bool Initialize(HINSTANCE hInstance, int nCmdShow);

    // 애플리케이션 실행 로직을 반복하는 함수
    int Run();

private:
    // 애플리케이션 내부 데이터 및 상태 업데이트 함수
    void Update();
    // 애플리케이션 화면 렌더링 함수 
    void Render();

private:
    // 애플리케이션이 관리하는 윈도우
    Window m_window;
    // 애플리케이션이 관리하는 렌더러
    Renderer m_renderer;
    // 애플리케이션이 관리하는 셰이더
    Shader m_shader;

    // 애플리케이션 종료 조건 제어용 변수
    bool m_running = true;
};