#include "pch.h"
#include "app.h"

#include <chrono>
#include <cstdio>
#include <iomanip>

namespace
{
    // 두 시점 사이의 경과 시간을 Milliseconds 단위로 계산하는 내부 헬퍼
    double CalculateElapsedMilliseconds
    (
        const std::chrono::steady_clock::time_point& startTime,
        const std::chrono::steady_clock::time_point& endTime
    )
    {
        return std::chrono::duration<double, std::milli>(endTime - startTime).count();
    }
}

bool App::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    // m_Window에 전달할 윈도우 너비
    constexpr int windowWidth = 1280;
    // m_Window에 전달할 윈도우 높이
    constexpr int windowHeight = 720;

    // 윈도우 초기화
    if (!m_window.Initialize(hInstance, nCmdShow, windowWidth, windowHeight, L"GPU-Driven VFX System"))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // 렌더러 초기화
    if (!m_renderer.Initialize(m_window.GetHwnd(), m_window.GetWidth(), m_window.GetHeight()))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // GPU Particle Update 구간의 Timestamp 측정에 사용할 Query 리소스 초기화
    if (!m_gpuUpdateTimestampQuery.Initialize(m_renderer.GetDevice()))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // GPU Particle Render 구간의 Timestamp 측정에 사용할 Query 리소스 초기화
    if (!m_gpuRenderTimestampQuery.Initialize(m_renderer.GetDevice()))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // 렌더링 화면의 세로 픽셀 크기
    const int renderHeight = m_renderer.GetHeight();

    // 랜더링 화면의 종횡비 (가로 픽셀 크기 / 세로 픽셀 크기)
    const float aspectRatio = (renderHeight > 0)
        ? static_cast<float>(m_renderer.GetWidth()) / static_cast<float>(renderHeight)
        : 1.0f;

    // 카메라가 바라보는 위치 설정
    m_camera.LookAt
    (
        DirectX::XMFLOAT3(0.0f, 0.0f, -2.0f),
        DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
        DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)
    );

    // 카메라 원근 투영을 위한 렌즈값 설정
    m_camera.SetLens
    (
        DirectX::XM_PIDIV4,     // 시야각 (45도로 설정)
        aspectRatio,            // 화면 종횡비
        0.1f,                   // 근평면
        100.0f                  // 원평면
    );

    // CPU Particle System 초기화
    m_cpuParticleSystem.Initialize();

    // GPU Particle System 초기화
    if (!m_gpuParticleSystem.Initialize(m_renderer.GetDevice()))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // CPU 파티클 시스템에서 Billboard 렌더링에 필요한 리소스 초기화
    if (!m_cpuBillboardRenderer.Initialize(m_renderer.GetDevice()))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // GPU 파티클 시스템에서 Billboard 렌더링에 필요한 리소스 초기화
    if (!m_gpuBillboardRenderer.Initialize(m_renderer.GetDevice()))
    {
        // 초기화하지 못한 경우 실패 처리
        return false;
    }

    // 프레임 시간 측정을 위한 타이머 초기화
    m_frameTimer.Initialize();

    // Particle 성능 측정 CSV 로그 파일 초기화
    InitializeParticlePerformanceCsvLog();

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

        m_currentParticlePerformanceStats = ParticlePerformanceStats{};

        // 현재 프레임 처리 시간 측정 시작
        const auto frameStartTime = std::chrono::steady_clock::now();

        // 애플리케이션 내부 데이터 및 상태 업데이트
        Update();
        // 애플리케이션 화면 렌더링
        Render();

        // 현재 프레임 처리 시간 측정 종료
        const auto frameEndTime = std::chrono::steady_clock::now();

        // 현재 프레임 전체 처리 시간 저장
        m_currentParticlePerformanceStats.frameMilliseconds =
            CalculateElapsedMilliseconds(frameStartTime, frameEndTime);

        // 현재 Particle 성능 측정 결과 출력
        PrintParticlePerformanceInfo
        (
            static_cast<float>(m_currentParticlePerformanceStats.frameMilliseconds / 1000.0)
        );
    }

    return 0;
}

void App::ProcessParticleSimulationModeInput()
{
    // 숫자 1 키의 현재 입력 상태 확인
    const bool isCpuModeKeyDown = (GetAsyncKeyState('1') & 0x8000) != 0;
    // 숫자 2 키의 현재 입력 상태 확인
    const bool isGpuModeKeyDown = (GetAsyncKeyState('2') & 0x8000) != 0;

    // 숫자 1 키가 이번 프레임에 새로 눌린 경우 CPU Particle 모드로 전환
    if (isCpuModeKeyDown && !m_wasCpuModeKeyDown)
    {
        m_particleSimulationMode = ParticleSimulationMode::CPU;

        OutputDebugStringW(L"[Particle] mode = CPU\n");
    }

    // 숫자 2 키가 이번 프레임에 새로 눌린 경우 GPU Particle 모드로 전환
    if (isGpuModeKeyDown && !m_wasGpuModeKeyDown)
    {
        m_particleSimulationMode = ParticleSimulationMode::GPU;

        OutputDebugStringW(L"[Particle] mode = GPU\n");
    }

    // 다음 프레임 입력 비교를 위해 현재 키 상태 저장
    m_wasCpuModeKeyDown = isCpuModeKeyDown;
    m_wasGpuModeKeyDown = isGpuModeKeyDown;
}

void App::Update()
{
    // 현재 프레임의 deltaTime 갱신
    m_frameTimer.Tick();

    // 현재 프레임에서 사용할 deltaTime 값 조회
    const float deltaTime = m_frameTimer.GetDeltaTime();

    // Particle 시뮬레이션 모드 전환 입력 처리
    ProcessParticleSimulationModeInput();

    // Particle Update 구간 시간 측정 시작
    const auto updateStartTime = std::chrono::steady_clock::now();

    // 현재 선택된 시뮬레이션 모드에 따라 하나의 경로만 업데이트
    switch (m_particleSimulationMode)
    {
    case ParticleSimulationMode::CPU:
        // 실제 deltaTime 값으로 CPU Particle System 업데이트
        m_cpuParticleSystem.Update(deltaTime);
        break;

    case ParticleSimulationMode::GPU:
        ID3D11DeviceContext* context = m_renderer.GetContext();

        // GPU Timestamp Query에서 회수한 Particle Update 구간의 실행 시간(ms)을 담을 변수
        double resolvedGpuUpdateMilliseconds = 0.0;

        // Particle Update 구간의 실행 시간 회수에 성공한 경우
        if (m_gpuUpdateTimestampQuery.Resolve(context, resolvedGpuUpdateMilliseconds))
        {
            m_currentParticlePerformanceStats.gpuUpdateMilliseconds = resolvedGpuUpdateMilliseconds;
            m_currentParticlePerformanceStats.hasGpuUpdateMilliseconds = true;
        }

        // GPU Particle Update 구간의 시작 Timestamp 기록
        m_gpuUpdateTimestampQuery.Begin(context);

        // GPU Compute Shader를 사용해 GPU Particle System 업데이트
        m_gpuParticleSystem.Update(context, deltaTime);

        // GPU Particle Update 구간의 종료 Timestamp 기록
        m_gpuUpdateTimestampQuery.End(context);

        break;
    }

    // Particle Update 구간 시간 측정 종료
    const auto updateEndTime = std::chrono::steady_clock::now();

    // 현재 프레임의 Particle Update 구간 시간 저장
    m_currentParticlePerformanceStats.updateMilliseconds =
        CalculateElapsedMilliseconds(updateStartTime, updateEndTime);
}

void App::Render()
{
    m_renderer.BeginFrame(0.05f, 0.08f, 0.12f, 1.0f);

    // Particle Render 구간 시간 측정 시작
    const auto renderStartTime = std::chrono::steady_clock::now();

    // 현재 선택된 Particle 시뮬레이션 모드에 따라 하나의 렌더링 경로만 실행
    switch (m_particleSimulationMode)
    {
    case ParticleSimulationMode::CPU:
        m_cpuBillboardRenderer.Render
        (
            m_renderer.GetContext(),
            m_camera,
            m_cpuParticleSystem.GetBillboards()
        );
        break;

    case ParticleSimulationMode::GPU:
        ID3D11DeviceContext* context = m_renderer.GetContext();

        // GPU Timestamp Query에서 회수한 Particle Render 구간의 실행 시간(ms)을 담을 변수
        double resolvedGpuRenderMilliseconds = 0.0;

        // Particle Render 구간의 실행 시간 회수에 성공한 경우
        if (m_gpuRenderTimestampQuery.Resolve(context, resolvedGpuRenderMilliseconds))
        {
            m_currentParticlePerformanceStats.gpuRenderMilliseconds = resolvedGpuRenderMilliseconds;
            m_currentParticlePerformanceStats.hasGpuRenderMilliseconds = true;
        }

        // GPU Particle Render 구간의 시작 Timestamp 기록
        m_gpuRenderTimestampQuery.Begin(context);

        m_gpuBillboardRenderer.Render
        (
            context,
            m_camera,
            m_gpuParticleSystem.GetParticleSrv(),
            m_gpuParticleSystem.GetAliveIndexSrv(),
            m_gpuParticleSystem.GetAliveCountSrv()
        );

        // GPU Particle Render 구간의 종료 Timestamp 기록
        m_gpuRenderTimestampQuery.End(context);

        break;
    }

    // Particle Render 구간 시간 측정 종료
    const auto renderEndTime = std::chrono::steady_clock::now();

    // 현재 프레임의 Particle Render 구간 시간 저장
    m_currentParticlePerformanceStats.renderMilliseconds =
        CalculateElapsedMilliseconds(renderStartTime, renderEndTime);

    m_renderer.EndFrame();
}

void App::PrintParticlePerformanceInfo(float elapsedSeconds)
{
    // Particle 성능 측정 출력 누적 시간 갱신
    m_particlePerformancePrintAccumulator += elapsedSeconds;

    // 현재 프레임의 CPU 타이머 기반 Particle Update 구간 시간 누적
    m_accumulatedParticlePerformanceStats.updateMilliseconds +=
        m_currentParticlePerformanceStats.updateMilliseconds;

    // 현재 프레임의 CPU 타이머 기반 Particle Render 구간 시간 누적
    m_accumulatedParticlePerformanceStats.renderMilliseconds +=
        m_currentParticlePerformanceStats.renderMilliseconds;

    // 이번 프레임에 유효한 GPU Update Timestamp 결과를 회수한 경우에만 누적
    if (m_currentParticlePerformanceStats.hasGpuUpdateMilliseconds)
    {
        m_accumulatedParticlePerformanceStats.gpuUpdateMilliseconds +=
            m_currentParticlePerformanceStats.gpuUpdateMilliseconds;
        
        // GPU Particle Update Timestamp 측정 샘플 수 증가
        ++m_gpuUpdateTimestampSampleCount;
    }

    // 이번 프레임에 유효한 GPU Render Timestamp 결과를 회수한 경우에만 누적
    if (m_currentParticlePerformanceStats.hasGpuRenderMilliseconds)
    {
        m_accumulatedParticlePerformanceStats.gpuRenderMilliseconds +=
            m_currentParticlePerformanceStats.gpuRenderMilliseconds;

        // GPU Particle Render Timestamp 측정 샘플 수 증가
        ++m_gpuRenderTimestampSampleCount;
    }

    // 현재 프레임 전체 처리 시간 누적
    m_accumulatedParticlePerformanceStats.frameMilliseconds +=
        m_currentParticlePerformanceStats.frameMilliseconds;

    // CPU 타이머 기반 성능 측정 샘플 수 증가
    ++m_particlePerformanceSampleCount;

    // 1초마다 Particle 성능 측정 결과를 Output 창에 출력
    if (m_particlePerformancePrintAccumulator < 1.0f)
    {
        return;
    }

    // 샘플이 없는 경우 출력하지 않음
    if (m_particlePerformanceSampleCount == 0 && m_gpuUpdateTimestampSampleCount == 0)
    {
        return;
    }

    // 평균 계산에 사용할 샘플 개수
    const double sampleCount = static_cast<double>(m_particlePerformanceSampleCount);
    const double gpuSampleCount = static_cast<double>(m_gpuUpdateTimestampSampleCount);

    // 1초간 누적된 샘플을 바탕으로 평균 Update 소요 시간(ms) 계산
    const double averageUpdateMilliseconds =
        sampleCount > 0
        ? m_accumulatedParticlePerformanceStats.updateMilliseconds / sampleCount
        : 0.0;

    // 1초간 누적된 샘플을 바탕으로 평균 Render 소요 시간(ms) 계산
    const double averageRenderMilliseconds =
        sampleCount > 0
        ? m_accumulatedParticlePerformanceStats.renderMilliseconds / sampleCount
        : 0.0;

    // 1초간 누적된 샘플을 바탕으로 평균 총 프레임 처리 시간(ms) 계산
    const double averageFrameMilliseconds =
        sampleCount > 0
        ? m_accumulatedParticlePerformanceStats.frameMilliseconds / sampleCount
        : 0.0;

    // 회수된 GPU Update Timestamp 샘플을 기준으로 평균 GPU Update 실행 시간(ms) 계산
    const double averageGpuUpdateMilliseconds =
        gpuSampleCount > 0
        ? m_accumulatedParticlePerformanceStats.gpuUpdateMilliseconds / gpuSampleCount
        : 0.0;

    // 회수된 GPU Render Timestamp 샘플을 기준으로 평균 GPU Render 실행 시간(ms) 계산
    const double averageGpuRenderMilliseconds =
        gpuSampleCount > 0
        ? m_accumulatedParticlePerformanceStats.gpuRenderMilliseconds / gpuSampleCount
        : 0.0;

    // 평균 프레임 시간을 바탕으로 초당 프레임 수(FPS) 계산
    const double fps = averageFrameMilliseconds > 0.0
        ? 1000.0 / averageFrameMilliseconds
        : 0.0;

    // Particle 성능 측정 출력 문자열 구성
    wchar_t debugText[256] = {};

    switch (m_particleSimulationMode)
    {
    case ParticleSimulationMode::CPU:
        swprintf_s
        (
            debugText,
            256,
            L"[Perf][CPU] particles=%zu, cpu_update=%.3fms, cpu_render=%.3fms, frame=%.3fms, fps=%.1f\n",
            m_cpuParticleSystem.GetMaxParticleCount(),
            averageUpdateMilliseconds,
            averageRenderMilliseconds,
            averageFrameMilliseconds,
            fps
        );
        break;

    case ParticleSimulationMode::GPU:
        swprintf_s
        (
            debugText,
            256,
            L"[Perf][GPU] particles=%zu, gpu_update=%.3fms, gpu_render=%.3fms, frame=%.3fms, fps=%.1f\n",
            m_gpuParticleSystem.GetMaxParticleCount(),
            averageGpuUpdateMilliseconds,
            averageGpuRenderMilliseconds,
            averageFrameMilliseconds,
            fps
        );
        break;
    }

    // Output 창에 Particle 성능 측정 결과 출력
    OutputDebugStringW(debugText);

    // 계산된 평균 성능 측정 결과를 CSV 파일에 기록
    WriteParticlePerformanceCsvRow
    (
        averageUpdateMilliseconds,
        averageRenderMilliseconds,
        averageGpuUpdateMilliseconds,
        averageGpuRenderMilliseconds,
        averageFrameMilliseconds,
        fps
    );

    // 다음 출력 주기를 위해 누적 시간 감소
    m_particlePerformancePrintAccumulator -= 1.0f;

    // 누적 성능 측정값 초기화
    m_accumulatedParticlePerformanceStats = ParticlePerformanceStats{};

    // 누적 샘플 개수 초기화
    m_particlePerformanceSampleCount = 0;

    m_gpuUpdateTimestampSampleCount = 0;
    m_gpuRenderTimestampSampleCount = 0;
}

void App::InitializeParticlePerformanceCsvLog()
{
    // 기존 CSV 로그 파일을 덮어쓰기 방식으로 생성
    m_particlePerformanceCsvFile.open
    (
        "particle_performance.csv",
        std::ios::out | std::ios::trunc
    );

    // CSV 파일을 열지 못한 경우 성능 로그 파일 출력만 비활성화
    if (!m_particlePerformanceCsvFile.is_open())
    {
        OutputDebugStringW(L"[Perf] Failed to open particle_performance.csv\n");
        return;
    }

    // CSV Header 작성
    m_particlePerformanceCsvFile
        << "mode,"
        << "particle_capacity,"
        << "update_ms,"
        << "render_ms,"
        << "frame_ms,"
        << "fps\n";

    // Header가 바로 파일에 기록되도록 flush
    m_particlePerformanceCsvFile.flush();
}
void App::WriteParticlePerformanceCsvRow
(
    double averageUpdateMilliseconds,
    double averageRenderMilliseconds,
    double averageGpuUpdateMilliseconds,
    double averageGpuRenderMilliseconds,
    double averageFrameMilliseconds,
    double fps
)
{
    // CSV 파일이 열려 있지 않은 경우 기록하지 않음
    if (!m_particlePerformanceCsvFile.is_open())
    {
        return;
    }

    // 소수점 자릿수 고정
    m_particlePerformanceCsvFile << std::fixed << std::setprecision(6);

    switch (m_particleSimulationMode)
    {
    case ParticleSimulationMode::CPU:
        m_particlePerformanceCsvFile
            << "CPU,"
            << m_cpuParticleSystem.GetMaxParticleCount() << ","
            << averageUpdateMilliseconds << ","
            << averageRenderMilliseconds << ","
            << averageFrameMilliseconds << ","
            << fps
            << "\n";
        break;

    case ParticleSimulationMode::GPU:
        m_particlePerformanceCsvFile
            << "GPU,"
            << m_gpuParticleSystem.GetMaxParticleCount() << ","
            << averageGpuUpdateMilliseconds << ","
            << averageGpuRenderMilliseconds << ","
            << averageFrameMilliseconds << ","
            << fps
            << "\n";
        break;
    }

    // 측정 중 프로그램이 종료되어도 기록 손실을 줄이기 위해 매 row마다 flush
    m_particlePerformanceCsvFile.flush();
}
