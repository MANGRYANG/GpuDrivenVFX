#pragma once

#include <DirectXMath.h>

class Camera
{
public:
    // Camera 클래스 생성자
    Camera() = default;
    // Camera 클래스 소멸자
    ~Camera() = default;

    // 카메라가 바라보는 위치를 설정하는 함수
    void LookAt(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

    // 원근 투영에 사용할 렌즈 값을 설정하는 함수
    void SetLens(float fovY, float aspectRatio, float nearZ, float farZ);

    // 뷰 변환 행렬 Getter
    DirectX::XMMATRIX GetViewMatrix() const;
    // 투영 변환 행렬 Getter
    DirectX::XMMATRIX GetProjectionMatrix() const;

    // 뷰 좌표계 기저 벡터를 계산하는 함수
    void GetCameraAxes(DirectX::XMFLOAT3& right, DirectX::XMFLOAT3& up, DirectX::XMFLOAT3& forward) const;

private:
    // 카메라의 월드 공간 상 위치
    DirectX::XMFLOAT3 m_position = DirectX::XMFLOAT3(0.0f, 0.0f, -2.0f);
    // 카메라가 주시하는 타겟의 지점
    DirectX::XMFLOAT3 m_target = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    // 카메라의 월드 상방 참조 벡터
    DirectX::XMFLOAT3 m_up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

    // 카메라 시야각 (45도로 초기화)
    float m_fovY = DirectX::XM_PIDIV4;
    // 화면 종횡비 (16:9으로 초기화)
    float m_aspectRatio = 16.0f / 9.0f;
    // 카메라 근평면 (0.1로 초기화)
    float m_nearZ = 0.1f;
    // 카메라 원평면 (100으로 초기화)
    float m_farZ = 100.0f;
};