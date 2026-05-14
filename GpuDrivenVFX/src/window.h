#pragma once

class Window
{
public:
    // Window 클래스 생성자
    Window() = default;
    // Window 클래스 소멸자
    ~Window();

    // 윈도우 생성 및 초기 설정 함수
    bool Initialize(HINSTANCE hInstance, int nCmdShow, int width, int height, const wchar_t* title);

    // OS 메시지 확인 및 처리 함수
    void ProcessMessages(bool& running);

    // Getter
    HWND GetHwnd() const { return m_hwnd; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    // OS로부터 메시지를 직접 받는 고정 주소 콜백 윈도우 프로시저
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 인스턴스 내부에서 메시지를 처리하는 윈도우 프로시저
    LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    // 인스턴스 핸들
    HINSTANCE m_hInstance = nullptr;
    // 윈도우 핸들
    HWND m_hwnd = nullptr;

    // 윈도우 너비
    int m_width = 0;
    // 윈도우 높이
    int m_height = 0;

    // 윈도우 타이틀
    std::wstring m_title;
};