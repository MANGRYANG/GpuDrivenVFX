#include "pch.h"
#include "camera.h"

namespace
{
    constexpr float kEpsilon = 0.000001f;

    // 크기가 0에 가까운 벡터를 정규화하지 않기 위한 내부 헬퍼
    DirectX::XMVECTOR NormalizeOrDefault(DirectX::XMVECTOR vector, DirectX::XMVECTOR defaultVector)
    {
        // 벡터의 크기가 0에 가까운 경우 기본 벡터 반환
        if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vector)) <= kEpsilon)
        {
            return defaultVector;
        }

        // 벡터의 크기가 충분히 큰 경우 정규화 수행 후 반환
        return DirectX::XMVector3Normalize(vector);
    }
}

void Camera::LookAt(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up)
{
    m_position = position;
    m_target = target;
    m_up = up;
}

void Camera::SetLens(float fovY, float aspectRatio, float nearZ, float farZ)
{
    m_fovY = fovY;
    m_aspectRatio = aspectRatio > 0.0f ? aspectRatio : 1.0f;
    m_nearZ = nearZ;
    m_farZ = farZ;
}

DirectX::XMMATRIX Camera::GetViewMatrix() const
{
    const DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&m_position);
    const DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&m_target);
    const DirectX::XMVECTOR up = NormalizeOrDefault
    (
        DirectX::XMLoadFloat3(&m_up),
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    );

    return DirectX::XMMatrixLookAtLH(position, target, up);
}

DirectX::XMMATRIX Camera::GetProjectionMatrix() const
{
    return DirectX::XMMatrixPerspectiveFovLH(m_fovY, m_aspectRatio, m_nearZ, m_farZ);
}

void Camera::GetCameraAxes(DirectX::XMFLOAT3& right, DirectX::XMFLOAT3& up, DirectX::XMFLOAT3& forward) const
{
    const DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&m_position);
    const DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&m_target);

    // 현재 카메라의 전방 기저 단위벡터 계산
    const DirectX::XMVECTOR forwardVector = NormalizeOrDefault
    (
        DirectX::XMVectorSubtract(target, position),
        DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)
    );

    // 카메라의 상방 참조 벡터 계산
    const DirectX::XMVECTOR upHint = NormalizeOrDefault
    (
        DirectX::XMLoadFloat3(&m_up),
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    );

    // 현재 카메라의 우향 기저 단위벡터 계산
    const DirectX::XMVECTOR rightVector = NormalizeOrDefault
    (
        DirectX::XMVector3Cross(upHint, forwardVector),
        DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f)
    );

    // 현재 카메라의 상방 기저 단위벡터 계산
    const DirectX::XMVECTOR upVector = NormalizeOrDefault
    (
        DirectX::XMVector3Cross(forwardVector, rightVector),
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    );

    DirectX::XMStoreFloat3(&right, rightVector);
    DirectX::XMStoreFloat3(&up, upVector);
    DirectX::XMStoreFloat3(&forward, forwardVector);
}