#pragma once

class Shader
{
public:
    // Shader 클래스 생성자
    Shader() = default;
    // Shader 클래스 소멸자
    ~Shader() = default;

    // 정점 셰이더 및 픽셀 셰이더 컴파일 및 생성 함수
    bool Initialize
    (
        ID3D11Device* device,                   // 디바이스 포인터
        const std::wstring& vertexShaderPath,   // 컴파일할 정점 셰이더의 파일 경로
        const std::wstring& pixelShaderPath     // 컴파일할 픽셀 셰이더의 파일 경로
    );

    // 디바이스 컨텍스트의 파이프라인에 셰이더를 바인딩하는 함수
    void Bind(ID3D11DeviceContext* context);

    // 컴파일된 정점 셰이더의 바이너리 데이터가 담긴 블롭의 주소를 반환하는 함수
    ID3DBlob* GetVertexShaderBlob() const
    {
        return m_vertexShaderBlob.Get();
    }

    // 셰이더 파일 경로를 받아 HLSL 셰이더 코드를 바이너리로 컴파일하는 함수
    static bool CompileShaderFromFile
    (
        const std::wstring& filePath,
        const char* entryPoint,
        const char* target,
        Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob
    );

private:
    // 컴파일 완료 후 생성된 정점 셰이더 객체
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    // 컴파일 완료 후 생성된 픽셀 셰이더 객체
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;

    // 컴파일된 정점 셰이더의 원시 바이너리 데이터를 보관하는 블롭 객체
    Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShaderBlob;
    // 컴파일된 픽셀 셰이더의 원시 바이너리 데이터를 보관하는 블롭 객체
    Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShaderBlob;
};