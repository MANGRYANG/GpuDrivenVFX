#pragma once

#include <array>
#include <cstddef>
#include <limits>
#include <d3d11.h>
#include <wrl/client.h>

// GPU Timestamp 요청을 위한 클래스
class GpuTimestampQuery
{
public:
    GpuTimestampQuery() = default;
    ~GpuTimestampQuery() = default;

    // GPU Timestamp 쿼리 시스템 사용을 위한 리소스 초기화 함수
    bool Initialize(ID3D11Device* device);

    // 성능을 측정할 GPU 작업의 시작 지점을 설정하는 함수
    void Begin(ID3D11DeviceContext* context);
    // 성능을 측정할 GPU 작업의 종료 지점을 설정하는 함수
    void End(ID3D11DeviceContext* context);

    // GPU가 작성한 Timestamp 쿼리 결과를 CPU에서 수거하여 변환하는 함수
    bool Resolve(ID3D11DeviceContext* context, double& elapsedMilliseconds);

private:
    // GPU 작업의 실행 시간을 측정하기 위한 쿼리 구조체
    struct QueryFrame
    {
        // 측정된 Timestamp가 유효한지 확인하기 위한 쿼리
        Microsoft::WRL::ComPtr<ID3D11Query> disjointQuery;
        // 측정 시작 시점의 GPU timestamp 기록을 위한 쿼리
        Microsoft::WRL::ComPtr<ID3D11Query> beginQuery;
        // 측정 종료 시점의 GPU timestamp 기록을 위한 쿼리
        Microsoft::WRL::ComPtr<ID3D11Query> endQuery;
        // Begin, End로 측정 요청이 제출된 후 결과 회수 대기 중인지 나타내는 플래그
        bool pending = false;
    };

    // QueryFrame 구조체를 구성하는 쿼리들을 생성하는 함수
    bool CreateQueryFrame(ID3D11Device* device, QueryFrame& queryFrame);

    // Resolve에서 호출하는 쿼리 결과 수거를 위한 함수
    bool TryResolveQueryFrame(ID3D11DeviceContext* context, QueryFrame& queryFrame, double& elapsedMilliseconds);

private:
    // QueryFrame 배열에서 관리할 QueryFrame 개수
    static constexpr std::size_t QueryFrameCount = 4;
    // 유효하지 않은 QueryFrame 인덱스임을 나타내기 위한 값
    static constexpr std::size_t InvalidQueryIndex = std::numeric_limits<std::size_t>::max();

    // QueryFrame 관리를 위한 배열
    std::array<QueryFrame, QueryFrameCount> m_queryFrames;

    // 다음 Timestamp 측정에 사용할 QueryFrame 슬롯 번호
    std::size_t m_writeIndex = 0;
    // 현재 기록 중인 QueryFrame 슬롯 인덱스
    std::size_t m_recordingIndex = InvalidQueryIndex;

    // 현재 Timestamp 측정이 진행 중인지 나타내는 플래그
    bool m_recording = false;
};