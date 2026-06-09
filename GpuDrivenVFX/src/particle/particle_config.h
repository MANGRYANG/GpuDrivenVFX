#pragma once

#include <DirectXMath.h>
#include <cstddef>
#include <cstdint>

namespace ParticleConfig
{
    // CPU, GPU Particle System에서 사용할 최대 Particle 슬롯 개수
    constexpr std::size_t ParticleCapacity = 640000;

    // 공통 Emitter 위치
    inline const DirectX::XMFLOAT3 EmitterPosition = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

    // 공통 Emitter 초기 속도
    inline const DirectX::XMFLOAT3 EmitterVelocity = DirectX::XMFLOAT3(0.0f, 0.25f, 0.0f);

    // 공통 Particle 크기
    constexpr float ParticleSize = 0.005f;

    // 공통 Particle 생존 시간
    constexpr float ParticleLifetime = 20.0f;

    // 공통 초당 생성 개수
    constexpr float SpawnRate = 10000.0f;

    // 중심 방출형 Particle의 Spiral Arm 개수
    constexpr std::uint32_t SpiralArmCount = 8;

    // 중심 방출형 Particle이 생성될 시작 반지름
    constexpr float OrbitStartRadius = 0.01f;

    // 중심 방출형 Particle의 시작 각속도
    constexpr float OrbitAngularVelocity = 4.8f;

    // 중심 방출형 Particle의 확장 속도
    constexpr float OrbitRadialVelocity = 0.06f;

    // Spiral Particle 평면에 입체감을 주기 위한 pitch 회전값
    constexpr float SpiralPitch = 1.0f;

    // Spiral Particle 평면에 입체감을 주기 위한 yaw 회전값
    constexpr float SpiralYaw = 1.0f;

    // Spiral Particle 평면이 회전하는 속도
    constexpr float SpiralPlaneRotationSpeed = 0.2f;

    // Spiral Particle의 Z축 흔들림 강도
    constexpr float SpiralDepthScale = 0.1f;

    // Spiral Particle의 Z축 흔들림 주파수
    constexpr float SpiralDepthFrequency = 2.0f;

    // Particle 수명 중 fade-out이 시작되는 비율
    constexpr float ParticleFadeOutStartRatio = 0.0f;
}