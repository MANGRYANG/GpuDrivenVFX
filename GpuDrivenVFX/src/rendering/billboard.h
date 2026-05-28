#pragma once

#include <DirectXMath.h>

// Billboard Quad 정보 구조체
struct Billboard
{
    // Billboard Quad의 월드 공간 위치
    DirectX::XMFLOAT3 position;
    // Billboard Quad의 크기
    float size;
    // Billboard Quad의 색상
    DirectX::XMFLOAT4 color;
};