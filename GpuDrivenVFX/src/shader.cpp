#include "pch.h"
#include "shader.h"

bool Shader::Initialize
(
    ID3D11Device* device,
    const std::wstring& vertexShaderPath,
    const std::wstring& pixelShaderPath
)
{
    // 정점 셰이더 컴파일
    if (!CompileShaderFromFile(vertexShaderPath, "VS_Main", "vs_5_0", m_vertexShaderBlob ))
    {
        // 컴파일하지 못한 경우 실패 처리
        return false;
    }

    // 정점 셰이더 객체 생성
    HRESULT hr = device->CreateVertexShader
    (
        // 컴파일된 정점 셰이더에 대한 포인터
        m_vertexShaderBlob->GetBufferPointer(),
        // 컴파일된 정점 셰이더의 크기
        m_vertexShaderBlob->GetBufferSize(),
        // 클래스 연결 인터페이스 포인터 (사용하지 않음)
        nullptr,
        // 컴파일된 정점 셰이더를 가리킬 포인터의 주소
        m_vertexShader.GetAddressOf()
    );

    // 정점 셰이더 객체 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create vertex shader.", L"Shader Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // 픽셀 셰이더 컴파일
    if (!CompileShaderFromFile(pixelShaderPath, "PS_Main", "ps_5_0", m_pixelShaderBlob))
    {
        // 컴파일하지 못한 경우 실패 처리
        return false;
    }

    // 픽셀 셰이더 객체 생성
    hr = device->CreatePixelShader
    (
        // 컴파일된 픽셀 셰이더에 대한 포인터
        m_pixelShaderBlob->GetBufferPointer(),
        // 컴파일된 픽셀 셰이더의 크기
        m_pixelShaderBlob->GetBufferSize(),
        // 클래스 연결 인터페이스 포인터 (사용하지 않음)
        nullptr,
        // 컴파일된 픽셀 셰이더를 가리킬 포인터의 주소
        m_pixelShader.GetAddressOf()
    );

    // 픽셀 셰이더 객체 생성에 실패한 경우
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create pixel shader.", L"Shader Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

void Shader::Bind(ID3D11DeviceContext* context)
{
    // 정점 셰이더 객체를 정점 셰이더 단계에 바인딩
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

    // 픽셀 셰이더 객체를 픽셀 셰이더 단계에 바인딩
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
}

bool Shader::CompileShaderFromFile
(
    const std::wstring& filePath,
    const char* entryPoint,
    const char* target,
    Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob
)
{
    // 컴파일 시 사용할 특수 기능을 담는 변수
    UINT compileFlags = 0;

// Debug 모드로 빌드하는 경우 디버그 정보를 출력 코드에 삽입하고 최적화를 건너뛰도록 설정
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG;
    compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // 에러 보관용 블롭
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    // HLSL 셰이더 파일에 대한 컴파일 작업 수행
    HRESULT hr = D3DCompileFromFile
    (
        filePath.c_str(),                   // 파일 경로
        nullptr,                            // 매크로 정의 배열 (사용하지 않음)
        D3D_COMPILE_STANDARD_FILE_INCLUDE,  // Include 파일을 처리하는 데 사용되는 처리기 설정
        entryPoint,                         // 셰이더의 Entry Point
        target,                             // 타겟 셰이더 모델
        compileFlags,                       // 셰이더 컴파일 옵션
        0,                                  // 효과 컴파일 옵션 (사용하지 않음)
        shaderBlob.GetAddressOf(),          // 컴파일된 셰이더 블롭 객체를 가리키는 포인터
        errorBlob.GetAddressOf()            // 컴파일 에러 메시지 블롭 객체를 가리키는 포인터
    );

    // 셰이더 파일에 대한 컴파일에 실패한 경우
    if (FAILED(hr))
    {
        // 컴파일 에러 메시지가 존재하는 경우
        if (errorBlob)
        {
            // 디버깅 메시지 출력
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
        }

        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to compile shader. Check the Output window.", L"Shader Compile Error", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}