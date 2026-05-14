# GpuDrivenVFX

<p align="center">
  <img width="600" height="340" alt="particle" src="https://github.com/user-attachments/assets/bc1ba92e-3222-41c3-950f-0b7a57374270" />
</p>

---

## 🧭 프로젝트 개요

본 프로젝트는 C++과 DirectX 11을 사용해 대량의 입자를 실시간으로 시뮬레이션하고 렌더링하는 VFX 시스템 프로젝트입니다.

주요 목표는 다음과 같습니다.

- DirectX 11 렌더링 파이프라인을 직접 구성할 수 있다.
- CPU Particle System과 GPU Particle System의 구조 차이를 이해한다.
- HLSL Compute Shader로 입자 시뮬레이션을 GPU에서 처리할 수 있다.
- 성능 측정 데이터를 기반으로 병목을 설명할 수 있다.
- 게임 클라이언트 VFX 시스템의 최적화 관점을 설명할 수 있다.

---

## 🪄 주요 기능

GpuDrivenVFX를 통해 구현하고 경험한 핵심 기능은 다음과 같습니다.

- DirectX 11 기반 3D 입자 렌더링 시스템 구현
- CPU 기반 Particle Simulation과 GPU Compute Shader 기반 Particle Simulation 구현
- 입자 수 증가에 따른 성능 측정 및 분석 시스템 구현

---

## 🔧 사용 기술

- C++ : 입자 시스템, 렌더링 구조, 성능 측정 로직 구현
- DirectX 11 : 3D 입자 렌더링 및 GPU 리소스 관리
- HLSL : Vertex / Pixel / Compute Shader 작성

---

## 📄 문서

[![GpuDrivenVFX Docs](https://img.shields.io/badge/GpuDrivenVFX-Documentation-purple?style=for-the-badge&logo=obsidian)](https://github.com/MANGRYANG/GpuDrivenVFX-docs)

---

## ⚖️ 라이선스

본 프로젝트는 MIT 라이선스를 따릅니다. 다만 개인적인 용도로 개발되어 외부 기여를 받지 않고 있습니다.  
프로젝트를 참조하거나 사용하는 데는 문제가 없으며, 필요한 경우 포크하여 개인 프로젝트로 사용하셔도 무방합니다.
