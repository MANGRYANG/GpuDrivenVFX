#include "pch.h"
#include "window.h"

Window::~Window()
{
    // 윈도우 핸들이 존재하는 경우
    if (m_hwnd)
    {
        // 윈도우를 메모리에서 제거
        DestroyWindow(m_hwnd);
        // 댕글링 포인터 방지
        m_hwnd = nullptr;
    }
}

bool Window::Initialize(HINSTANCE hInstance, int nCmdShow, int width, int height, const wchar_t* title)
{
    // 멤버 변수 설정
    m_hInstance = hInstance;
    m_width = width;
    m_height = height;
    m_title = title;

    // 윈도우 클래스명 설정
    const wchar_t* CLASS_NAME = L"Gpu-Driven-VFX-Window-Class";

    // 윈도우 클래스 구조체 설정
    WNDCLASSEXW wc = {};
    // 구조체 크기 명시
    wc.cbSize = sizeof(WNDCLASSEXW);
    // 클래스 스타일 설정
    wc.style = CS_HREDRAW | CS_VREDRAW;
    // 윈도우 프로시저 함수 주소 설정
    wc.lpfnWndProc = Window::StaticWindowProc;
    // 인스턴스 핸들 설정
    wc.hInstance = m_hInstance;
    // 윈도우 클래스 커서 핸들 설정
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // 윈도우 클래스 배경 브러시 설정
    wc.hbrBackground = nullptr;
    // 윈도우 클래스명 등록
    wc.lpszClassName = CLASS_NAME;

    // 윈도우 클래스 등록에 실패한 경우
    if (!RegisterClassExW(&wc))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to register window class.", L"Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 윈도우 크기 확보를 위한 RECT 구조체 설정
    RECT windowRect = {};
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = m_width;
    windowRect.bottom = m_height;

    // 테두리를 포함한 윈도우 크기 계산
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // 최종 윈도우 너비 
    const int windowWidth = windowRect.right - windowRect.left;
    // 최종 윈도우 높이
    const int windowHeight = windowRect.bottom - windowRect.top;

    // 윈도우 생성
    m_hwnd = CreateWindowExW(
        0,                      // 확장 윈도우 스타일 (기본값)
        CLASS_NAME,             // 윈도우 클래스명
        m_title.c_str(),        // 윈도우 타이틀
        WS_OVERLAPPEDWINDOW,    // 윈도우 스타일
        CW_USEDEFAULT,          // 화면 X 좌표
        CW_USEDEFAULT,          // 화면 Y 좌표
        windowWidth,            // 윈도우 너비
        windowHeight,           // 윈도우 높이
        nullptr,                // 부모 윈도우
        nullptr,                // 메뉴 바 핸들
        m_hInstance,            // 인스턴스 핸들
        this                    // 추가 파라미터
    );

    // 윈도우 생성에 실패한 경우
    if (!m_hwnd)
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create window.", L"Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 윈도우 표시 상태 설정
    ShowWindow(m_hwnd, nCmdShow);
    // 메시지 큐를 우회하여 윈도우 프로시저애 WM_PAINT 메시지 즉시 전달
    UpdateWindow(m_hwnd);

    return true;
}

void Window::ProcessMessages(bool& running)
{
    MSG msg = {};

    // 메시지 큐에 메시지가 존재하는 경우
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            running = false;
            return;
        }

        // 가상 키 메시지를 문자 메시지로 변환
        TranslateMessage(&msg);
        // 메시지를 윈도우 프로시저로 전달 (WndProc)
        DispatchMessageW(&msg);
    }
}

LRESULT CALLBACK Window::StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* window = nullptr;

    // 윈도우가 처음 만들어지는 시점인 경우
    if (uMsg == WM_NCCREATE)
    {
        // 윈도우 생성 정보 구조체 가져오기
        CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        // CreateWindowEx의 lpParam으로 전달한 값 추출 (this 포인터)
        window = reinterpret_cast<Window*>(createStruct->lpCreateParams);

        // 추출한 객체 주소를 윈도우 내부 데이터 영역(GWLP_USERDATA)에 저장
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

        // CreateWindowExW으로 생성되었던 윈도우 핸들 보관
        window->m_hwnd = hwnd;
    }

    // 윈도우 생성 이후 시점인 경우
    else
    {
        // 윈도우 내부 데이터 영역(GWLP_USERDATA)에서 윈도우 객체 주소 추출
        window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    // 윈도우와 연결된 객체가 존재하지 않는 경우
    if (!window)
    {
        // 기본 윈도우 프로시저 호출
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    // WindowProc 함수 호출
    return window->WindowProc(uMsg, wParam, lParam);
}

LRESULT Window::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:              // 사용자가 닫기 버튼을 눌렀을 때
        DestroyWindow(m_hwnd);  // 윈도우 제거 함수 호출
        return 0;

    case WM_DESTROY:            // 윈도우 제거 함수가 호출되었을 때
        PostQuitMessage(0);     // 애플리케이션 종료 함수 호출
        return 0;

    default:
        // 기본 윈도우 프로시저로 전달
        return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
    }
}