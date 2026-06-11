#pragma once

#include <DirectXMath.h>
#include <fstream>
#include <vector>

#include "core/frame_timer.h"
#include "core/gpu_timestamp_query.h"

#include "platform/window.h"

#include "rendering/renderer.h"
#include "rendering/cpu_billboard_renderer.h"
#include "rendering/gpu_billboard_renderer.h"

#include "graphics/camera.h"

#include "particle/cpu_particle_system.h"
#include "particle/gpu_particle_system.h"

// 시뮬레이션 실행 모드
enum class ParticleSimulationMode
{
    CPU,
    GPU
};

// Particle 성능 측정 결과 구조체
struct ParticlePerformanceStats
{
    // 현재 모드의 Particle Update 처리에 걸리는 시간
    double updateMilliseconds = 0.0;
    // 현재 모드의 Particle Render 처리에 걸리는 시간
    double renderMilliseconds = 0.0;
    // 현재 프레임에 대한 전체 처리 시간
    double frameMilliseconds = 0.0;

    // GPU Timestamp Query에서 회수한 Particle Update 구간의 실행 시간(ms)
    double gpuUpdateMilliseconds = 0.0;
    // GPU Timestamp Query에서 회수한 Particle Render 구간의 실행 시간(ms)
    double gpuRenderMilliseconds = 0.0;

    // 이번 프레임에 유효한 GPU Update 측정값을 회수하였는지 나타내는 플래그
    bool hasGpuUpdateMilliseconds = false;
    // 이번 프레임에 유효한 GPU Render 측정값을 회수하였는지 나타내는 플래그
    bool hasGpuRenderMilliseconds = false;
};

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
    // 시뮬레이션 모드 전환 입력을 처리하는 함수
    void ProcessParticleSimulationModeInput();

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
    // 애플리케이션이 관리하는 카메라
    Camera m_camera;

    // CPU 기반 Particle 데이터를 관리하는 시스템
    CpuParticleSystem m_cpuParticleSystem;
    // GPU 기반 Particle 데이터를 관리하는 시스템
    GpuParticleSystem m_gpuParticleSystem;

    // CPU 기반 Particle을 렌더링하기 위한 Billboard 렌더러
    CpuBillboardRenderer m_cpuBillboardRenderer;
    // GPU 기반 Particle을 렌더링하기 위한 Billboard 렌더러
    GpuBillboardRenderer m_gpuBillboardRenderer;

    // 애플리케이션 종료 조건 제어용 변수
    bool m_running = true;

private:
    // 현재 적용 중인 시뮬레이션 모드
    ParticleSimulationMode m_particleSimulationMode = ParticleSimulationMode::CPU;

    // 이전 프레임에서 CPU 모드 전환 키가 눌려 있었는지 여부
    bool m_wasCpuModeKeyDown = false;

    // 이전 프레임에서 GPU 모드 전환 키가 눌려 있었는지 여부
    bool m_wasGpuModeKeyDown = false;

private:
    // Particle 성능 측정 메시지 출력 주기를 제어하기 위한 누적 시간
    float m_particlePerformancePrintAccumulator = 0.0f;

    // 현재 프레임의 Particle 성능 측정 결과
    ParticlePerformanceStats m_currentParticlePerformanceStats;

    // 일정 시간 동안 누적한 Particle 성능 측정 결과
    ParticlePerformanceStats m_accumulatedParticlePerformanceStats;

    // GPU Particle Update 구간의 실제 GPU 실행 시간을 측정하기 위한 Timestamp Query
    GpuTimestampQuery m_gpuUpdateTimestampQuery;
    // GPU Particle Render 구간의 실제 GPU 실행 시간을 측정하기 위한 Timestamp Query
    GpuTimestampQuery m_gpuRenderTimestampQuery;

    // 현재 누적 구간에서 회수한 GPU Particle Update Timestamp 측정 샘플 수
    std::size_t m_gpuUpdateTimestampSampleCount = 0;
    // 현재 누적 구간에서 회수한 GPU Particle Render Timestamp 측정 샘플 수
    std::size_t m_gpuRenderTimestampSampleCount = 0;

    // 성능 측정 평균을 계산하기 위한 누적 샘플 개수
    std::size_t m_particlePerformanceSampleCount = 0;

    // Particle 성능 측정 결과를 기록할 CSV 파일 스트림
    std::ofstream m_particlePerformanceCsvFile;

    // 현재 Particle 성능 측정 결과를 Output 창에 출력하는 함수
    void PrintParticlePerformanceInfo(float elapsedSeconds);

    // Particle 성능 측정 결과를 기록할 CSV 파일을 초기화하는 함수
    void InitializeParticlePerformanceCsvLog();

    // 현재 평균 성능 측정 결과를 CSV 파일에 기록하는 함수
    void WriteParticlePerformanceCsvRow
    (
        double averageUpdateMilliseconds,
        double averageRenderMilliseconds,
        double averageFrameMilliseconds,
        double averageGpuUpdateMilliseconds,
        double averageGpuRenderMilliseconds,
        double fps
    );
};