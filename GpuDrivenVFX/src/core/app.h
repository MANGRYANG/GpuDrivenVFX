#pragma once

#include <DirectXMath.h>
#include <vector>

#include "core/frame_timer.h"

#include "platform/window.h"

#include "rendering/renderer.h"
#include "rendering/shader.h"
#include "rendering/cpu_billboard_renderer.h"

#include "graphics/mesh.h"
#include "graphics/camera.h"

#include "particle/cpu_particle_system.h"

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
    // 정점 셰이더에 전달할 변환 버퍼를 생성하는 함수
    bool CreateTransformBuffer();
    // Quad의 위치, 회전, 스케일 정보를 기반으로 월드 변환 행렬을 생성하는 함수
    DirectX::XMMATRIX BuildQuadWorldMatrix() const;
    // 현재 프레임에서 사용할 변환 행렬을 GPU에 전달하는 함수
    void UpdateTransformBuffer(ID3D11DeviceContext* context);

    // 애플리케이션 내부 데이터 및 상태 업데이트 함수
    void Update();
    // 애플리케이션 화면 렌더링 함수 
    void Render();

private:
    // 애플리케이션이 관리하는 윈도우
    Window m_window;
    // 애플리케이션이 관리하는 타이머
    FrameTimer m_frameTimer;
    // 애플리케이션이 관리하는 렌더러
    Renderer m_renderer;
    // 애플리케이션이 관리하는 셰이더
    Shader m_shader;
    // 애플리케이션이 관리하는 카메라
    Camera m_camera;
    // 인덱스 버퍼 기반 사각형 렌더링을 검증하기 위한 임시 Quad 메쉬
    Mesh m_quadMesh;
    // CPU 기반 Particle을 렌더링하기 위한 Billboard 렌더러
    CpuBillboardRenderer m_cpuBillboardRenderer;
    // CPU 기반 Particle 데이터를 관리하는 시스템
    CpuParticleSystem m_cpuParticleSystem;

    // 월드 좌표계에서의 Quad 위치 정보
    DirectX::XMFLOAT3 m_quadPosition = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    // 월드 좌표계에서의 Quad 회전 정보
    DirectX::XMFLOAT3 m_quadRotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    // 월드 좌표계에서의 Quad 크기 배율
    float m_quadScale = 1.0f;

    // 정점 셰이더에 전달할 변환 버퍼
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_transformBuffer;

    // 애플리케이션 종료 조건 제어용 변수
    bool m_running = true;

private:
    // Particle 상태 디버그 메시지 출력 주기를 제어하기 위한 누적 시간
    float m_particleDebugPrintAccumulator = 0.0f;

    // 현재 프레임의 Particle 상태 디버그 메시지를 출력하기 위한 함수
    void PrintParticleDebugInfo(float deltaTime);
};