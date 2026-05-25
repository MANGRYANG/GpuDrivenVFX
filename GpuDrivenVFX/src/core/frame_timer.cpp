#include "pch.h"
#include "core/frame_timer.h"

void FrameTimer::Initialize()
{
    // 현재 시각을 기준 시각으로 설정
    const TimePoint currentTime = Clock::now();

    // 시작 시각 초기화
    m_startTime = currentTime;
    // 이전 프레임 시각 초기화
    m_previousTime = currentTime;

    // 첫 프레임 이전에는 경과 시간이 없으므로 0으로 초기화
    m_deltaTime = 0.0f;
    // 누적 시간 초기화
    m_totalTime = 0.0;
}

void FrameTimer::Tick()
{
    // 현재 프레임 시각 측정
    const TimePoint currentTime = Clock::now();

    // 이전 프레임과 현재 프레임 사이의 시간 차 계산
    const std::chrono::duration<float> deltaDuration = currentTime - m_previousTime;
    // 시작 시각부터 현재 프레임까지의 누적 시간 계산
    const std::chrono::duration<double> totalDuration = currentTime - m_startTime;

    // 다음 Tick을 위해 이전 프레임 시각 갱신
    m_previousTime = currentTime;

    // 초 단위 deltaTime 계산
    float deltaTime = deltaDuration.count();

    // deltaTime이 음수인 경우 방지
    if (deltaTime < 0.0f)
    {
        deltaTime = 0.0f;
    }

    // 디버깅 중 멈춤이나 긴 프레임 지연으로 인한 과도한 이동 방지
    if (deltaTime > MaxDeltaTime)
    {
        deltaTime = MaxDeltaTime;
    }

    // 계산된 deltaTime 저장
    m_deltaTime = deltaTime;
    // 누적 시간 저장
    m_totalTime = totalDuration.count();
}

float FrameTimer::GetDeltaTime() const
{
    return m_deltaTime;
}

double FrameTimer::GetTotalTime() const
{
    return m_totalTime;
}