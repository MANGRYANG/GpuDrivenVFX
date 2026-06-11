#include "pch.h"
#include "core/gpu_timestamp_query.h"

bool GpuTimestampQuery::Initialize(ID3D11Device* device)
{
    // 디바이스가 누락된 경우
    if (!device)
    {
        return false;
    }

    // 쿼리 프레임 생성
    for (QueryFrame& queryFrame : m_queryFrames)
    {
        if (!CreateQueryFrame(device, queryFrame))
        {
            return false;
        }
    }

    m_writeIndex = 0;
    m_recordingIndex = InvalidQueryIndex;
    m_recording = false;

    return true;
}

bool GpuTimestampQuery::CreateQueryFrame(ID3D11Device* device, QueryFrame& queryFrame)
{
    // 쿼리 설명자 구조체
    D3D11_QUERY_DESC queryDesc = {};
    queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    queryDesc.MiscFlags = 0;

    // disjointQuery는 D3D11_QUERY_TIMESTAMP_DISJOINT 타입으로 설정
    HRESULT hr = device->CreateQuery
    (
        &queryDesc,
        queryFrame.disjointQuery.GetAddressOf()
    );

    // 쿼리 생성에 실패한 경우 
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create Disjoint Query.", L"GpuTimestampQuery Error", MB_OK | MB_ICONERROR);

        return false;
    }

    queryDesc.Query = D3D11_QUERY_TIMESTAMP;

    // beginQuery는 D3D11_QUERY_TIMESTAMP 타입으로 설정
    hr = device->CreateQuery
    (
        &queryDesc,
        queryFrame.beginQuery.GetAddressOf()
    );

    // 쿼리 생성에 실패한 경우 
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create Begin Query.", L"GpuTimestampQuery Error", MB_OK | MB_ICONERROR);

        return false;
    }

    // endQuery는 D3D11_QUERY_TIMESTAMP 타입으로 설정
    hr = device->CreateQuery
    (
        &queryDesc,
        queryFrame.endQuery.GetAddressOf()
    );

    // 쿼리 생성에 실패한 경우 
    if (FAILED(hr))
    {
        // 메시지 박스 플로팅
        MessageBoxW(nullptr, L"Failed to create End Query.", L"GpuTimestampQuery Error", MB_OK | MB_ICONERROR);

        return false;
    }

    queryFrame.pending = false;

    return true;
}

void GpuTimestampQuery::Begin(ID3D11DeviceContext* context)
{
    // 디바이스 컨텍스트가 누락되었거나 이미 GPU Timestamp 측정 중인 경우
    if (!context || m_recording)
    {
        return;
    }

    // Timestamp 측정에 사용할 QueryFrame에 접근
    QueryFrame& queryFrame = m_queryFrames[m_writeIndex];

    // 아직 이전 Query 결과가 회수되지 않은 슬롯이면 측정하지 않음
    if (queryFrame.pending)
    {
        return;
    }

    context->Begin(queryFrame.disjointQuery.Get());
    context->End(queryFrame.beginQuery.Get());

    m_recordingIndex = m_writeIndex;
    m_recording = true;
}

void GpuTimestampQuery::End(ID3D11DeviceContext* context)
{
    // 디바이스 컨텍스트가 누락되었거나 GPU Timestamp 측정 중이 아닌 경우
    if (!context || !m_recording)
    {
        return;
    }

    // QueryFrame이 기록 중이 아닌 경우
    if (m_recordingIndex == InvalidQueryIndex)
    {
        m_recording = false;
        return;
    }

    // Timestamp 측정에 사용할 QueryFrame에 접근
    QueryFrame& queryFrame = m_queryFrames[m_recordingIndex];

    context->End(queryFrame.endQuery.Get());
    context->End(queryFrame.disjointQuery.Get());

    // QueryFrame의 측정 요청이 제출되었으며 회수 대기 중
    queryFrame.pending = true;

    // 다음 측정에 사용할 QueryFrame 슬롯 번호 갱신
    m_writeIndex = (m_writeIndex + 1) % QueryFrameCount;
    m_recordingIndex = InvalidQueryIndex;
    m_recording = false;
}

bool GpuTimestampQuery::Resolve(ID3D11DeviceContext* context, double& elapsedMilliseconds)
{
    // 디바이스 컨텍스트가 누락된 경우
    if (!context)
    {
        return false;
    }

    bool resolved = false;
    // 마지막으로 회수된 측정값
    double latestElapsedMilliseconds = 0.0;

    for (QueryFrame& queryFrame : m_queryFrames)
    {
        double queryElapsedMilliseconds = 0.0;

        // 쿼리 결과 수거에 성공한 경우
        if (TryResolveQueryFrame(context, queryFrame, queryElapsedMilliseconds))
        {
            latestElapsedMilliseconds = queryElapsedMilliseconds;
            resolved = true;
        }
    }

    if (resolved)
    {
        elapsedMilliseconds = latestElapsedMilliseconds;
    }

    return resolved;
}

bool GpuTimestampQuery::TryResolveQueryFrame
(
    ID3D11DeviceContext* context,
    QueryFrame& queryFrame,
    double& elapsedMilliseconds
)
{
    // 쿼리 결과 회수 대기 중이 아닌 경우 무시
    if (!queryFrame.pending)
    {
        return false;
    }

    // disjointQuery 결과를 받아오기 위한 구조체
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData = {};

    HRESULT hr = context->GetData
    (
        queryFrame.disjointQuery.Get(),
        &disjointData,
        sizeof(disjointData),
        // 결과가 준비되지 않은 경우 Flush하지 않도록 설정
        D3D11_ASYNC_GETDATA_DONOTFLUSH
    );

    // disjointQuery 결과가 준비되지 않은 경우
    if (hr == S_FALSE)
    {
        return false;
    }

    // disjointQuery 결과 가져오기에 실패한 경우
    if (FAILED(hr))
    {
        queryFrame.pending = false;
        return false;
    }
    
    // beginQuery 결과를 담기 위한 64비트 정수
    UINT64 beginTimestamp = 0;
    // endQuery 결과를 담기 위한 64비트 정수
    UINT64 endTimestamp = 0;

    hr = context->GetData
    (
        queryFrame.beginQuery.Get(),
        &beginTimestamp,
        sizeof(beginTimestamp),
        // 결과가 준비되지 않은 경우 Flush하지 않도록 설정
        D3D11_ASYNC_GETDATA_DONOTFLUSH
    );

    // beginQuery 결과가 준비되지 않은 경우
    if (hr == S_FALSE)
    {
        return false;
    }

    // beginQuery 결과 가져오기에 실패한 경우
    if (FAILED(hr))
    {
        queryFrame.pending = false;
        return false;
    }

    hr = context->GetData
    (
        queryFrame.endQuery.Get(),
        &endTimestamp,
        sizeof(endTimestamp),
        // 결과가 준비되지 않은 경우 Flush하지 않도록 설정
        D3D11_ASYNC_GETDATA_DONOTFLUSH
    );

    // endQuery 결과가 준비되지 않은 경우
    if (hr == S_FALSE)
    {
        return false;
    }

    // endQuery 결과 가져오기에 실패한 경우
    if (FAILED(hr))
    {
        queryFrame.pending = false;
        return false;
    }

    // 결과 회수 완료
    queryFrame.pending = false;

    // Timestamp 측정 결과를 시간 값으로 변환해도 되는지 검증
    if (disjointData.Disjoint || disjointData.Frequency == 0 || endTimestamp < beginTimestamp)
    {
        return false;
    }

    // Timestamp 델타 값 계산
    const UINT64 timestampDelta = endTimestamp - beginTimestamp;

    // Milliseconds 단위로 변환
    elapsedMilliseconds = static_cast<double>(timestampDelta) * 1000.0 / static_cast<double>(disjointData.Frequency);

    return true;
}