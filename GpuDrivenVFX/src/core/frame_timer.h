#pragma once

#include <chrono>

class FrameTimer
{
public:
    // FrameTimer 클래스 생성자
    FrameTimer() = default;
    // FrameTimer 클래스 소멸자
    ~FrameTimer() = default;

    // 타이머 기준 시각을 초기화하는 함수
    void Initialize();

    // 현재 프레임의 deltaTime과 totalTime을 갱신하는 함수
    void Tick();

    // 이전 프레임과 현재 프레임 사이의 경과 시간을 초 단위로 반환하는 함수
    float GetDeltaTime() const;

    // 타이머 초기화 이후 누적 경과 시간을 초 단위로 반환하는 함수
    double GetTotalTime() const;

private:
    // 코드 실행 시간 측정 및 간격 계산용 단조 시계 타입 정의
    using Clock = std::chrono::steady_clock;
    // 단조 시계의 특정 시점을 나타내는 타입 정의
    using TimePoint = std::chrono::time_point<Clock>;

private:
    // 비정상적으로 큰 deltaTime을 제한하기 위한 최대 프레임 시간
    static constexpr float MaxDeltaTime = 0.1f;

    // 타이머가 시작된 시각
    TimePoint m_startTime = Clock::now();
    // 이전 프레임의 시각
    TimePoint m_previousTime = Clock::now();

    // 이전 프레임과 현재 프레임 사이의 경과 시간
    float m_deltaTime = 0.0f;
    // 타이머 초기화 이후 누적 경과 시간
    double m_totalTime = 0.0;
};