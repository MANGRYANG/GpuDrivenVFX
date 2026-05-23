#include "pch.h"
#include "core/app.h"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR lpCmdLine,
    _In_ int nCmdShow
)
{
    App app;

    if (!app.Initialize(hInstance, nCmdShow))
    {
        MessageBoxW(nullptr, L"Failed to initialize application.", L"Error", MB_OK | MB_ICONERROR);

        return -1;
    }

    return app.Run();
}