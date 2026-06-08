#pragma once

#include <DirectXMath.h>
#include <cstddef>

namespace ParticleConfig
{
    // CPU, GPU Particle System에서 사용할 최대 Particle 슬롯 개수
    constexpr std::size_t ParticleCapacity = 64;

    // 공통 Emitter 위치
    inline const DirectX::XMFLOAT3 EmitterPosition = DirectX::XMFLOAT3(0.0f, -0.5f, 0.0f);

    // 공통 Emitter 초기 속도
    inline const DirectX::XMFLOAT3 EmitterVelocity = DirectX::XMFLOAT3(0.0f, 0.25f, 0.0f);

    // 공통 Particle 크기
    constexpr float ParticleSize = 0.01f;

    // 공통 Particle 생존 시간
    constexpr float ParticleLifetime = 3.0f;

    // 공통 초당 생성 개수
    constexpr float SpawnRate = 10.0f;
}