#include "pch.h"
#include "camera.h"

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
    const DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&m_up);

    return DirectX::XMMatrixLookAtLH(position, target, up);
}

DirectX::XMMATRIX Camera::GetProjectionMatrix() const
{
    return DirectX::XMMatrixPerspectiveFovLH(m_fovY, m_aspectRatio, m_nearZ, m_farZ);
}