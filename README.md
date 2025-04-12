

# 👟 SafeStep: 스마트 신발 마모도 측정 시스템

> **신발 밑창 마모도 자동 측정과 사고 예방을 위한 AI + IoT 융합 시스템**


---

## 📌 프로젝트 개요

SafeStep은 **신발 밑창 마모도를 자동으로 인식**하고, 마모도가 심각한 경우 **미끄럼 방지 처치**까지 수행하는 **AI 기반 IoT 시스템**입니다.  
낙상 사고를 예방하고, 사용자에게 직관적인 마모 정보를 제공하기 위한 **엣지 AI + 센서 제어 + 웹 연동** 융합 솔루션입니다.

---

## 🔧 사용 기술

| 분류           | 내용 |
|----------------|------|
| **AI 모델링**   | YOLOv8 (Ultralytics), PyTorch, OpenCV |
| **디바이스 제어** | Jetson Nano, Raspberry Pi, Arduino MEGA |
| **언어 및 환경** | Python, C++, PHP, HTML, JavaScript, MariaDB |
| **통신**        | Wi-Fi, NFS, Socket 통신 |
| **센서/모듈**   | Load Cell, IR Sensor, Servo Motor, Webcam, LCD |
| **서버/프론트** | Raspberry Pi Web Server, 실시간 UI 시각화 |
| **운영체제**    | JetPack, Raspberry Pi OS, Ubuntu 24.04, Windows 11 |

---

## ⚙️ 시스템 구성

[신발 무게 감지] → [Jetson Nano 이미지 촬영] → [YOLOv8 마모도 분석] → [Raspberry Pi 서버 전송 + DB 저장] → [Arduino 스프레이 분사 제어] → [LCD 실시간 결과 출력 + 웹 UI 제공]

## 🎯 주요 기능

- 🧠 **YOLOv8 마모도 판단**: 4단계 마모도 (정상, 30%, 50%, 70%) 분류
- 📸 **Jetson Nano 이미지 분석**: 실시간 촬영 + OpenCV 전처리
- 🔒 **보안 인증 소켓 통신**: Jetson ↔ Raspberry Pi ↔ Arduino 안전 제어
- 🧪 **센서 기반 분사 제어**:
  - 마모도 50% → 스프레이 박스 개방 + 사용자 경고
  - 마모도 70% → 자동 스프레이 직접 분사
- 📊 **웹 UI**: 마모도 결과 실시간 시각화, 사용자에게 맞춤형 행동 권장

---

## 🧱 기술 구현 상세

### 🔹 YOLOv8 기반 AI 모델
- 슬리퍼 마모 데이터를 2,400장 직접 촬영 및 라벨링
- PyTorch 기반 학습, RTX 4060 GPU, mAP 0.95 달성
- Jetson Nano에 실시간 추론 모델 배포

### 🔹 하드웨어 연동
- Load Cell로 무게 감지 → Raspberry Pi에 Flag 전송
- 카메라 촬영 → YOLOv8 추론 → JSON 전송
- Arduino에서 Servo, IR 센서, Buzzer 제어

### 🔹 데이터베이스 & 웹
- PHP + MariaDB 기반 RESTful 데이터 처리
- 마모 상태 기록 및 이미지 경로 저장
- HTML + JS 기반 실시간 시각화 웹 대시보드 구현

---

## 문제점과 해결

| 문제 | 해결 방안 |
|------|-----------|
| 공개된 신발 마모 데이터셋 부재 | 직접 촬영 및 마모도 단계별 데이터 구축 (2400장) |
| 불안정한 객체 감지 정확도 | YOLOv8 소형 모델 선택 + 밝기/모자이크 데이터 증강 |
| 사용자 입력 없이 자동 측정 필요 | 무게 센서 + 거리 센서 연동 통한 자동 작동 흐름 구성 |
| 하드웨어 간 복잡한 통신 | Raspberry Pi를 중심으로 안정적인 소켓 기반 구조 설계 |



---

## 📈 기대 효과

- 낙상 사고 예방 (노인, 어린이, 산업 근로자 대상)
- 겨울철 빙판길, 미끄럼 위험 지역에서 효율적 활용
- 안전 취약 계층 맞춤형 신발 관리 시스템 구현 가능
- 신발 상태 점검의 자동화 및 일상화 실현

---



## 📎 참고 자료

- [Ultralytics YOLOv8](https://github.com/ultralytics/ultralytics)  
- [OpenCV 공식 문서](https://github.com/opencv/opencv)

---
