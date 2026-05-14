#include "pch.h"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR lpCmdLine,
    _In_ int nCmdShow
)
{
    MessageBoxW(nullptr, L"GpuDrivenVFX project", L"GpuDrivenVFX", MB_OK);

    return 0;
}