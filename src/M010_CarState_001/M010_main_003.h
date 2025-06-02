#pragma once

// ====================================================================================================
// 프로젝트: 자동차 후방 로봇 눈
// 설명: ESP32 + MPU6050 활용, 차량 움직임 감지 및 LED Matrix 로봇 눈 표정 표현.
//       본 코드는 MPU6050 데이터 처리 및 자동차 상태 정의.
// 개발 환경: PlatformIO Arduino Core (ESP32)
// 사용 라이브러리: MPU6050_DMP6 (I2Cdevlib by Jeff Rowberg) - MotionApps612 사용
// ====================================================================================================

#include <Wire.h> // I2C 통신
#include <I2Cdev.h> // I2C 장치 통신
#include <MPU6050_6Axis_MotionApps612.h> // MPU6050 DMP 기능 - MotionApps612로 변경!

// MPU6050 객체
MPU6050 g_M010_mpu;

// ====================================================================================================
// 전역 상수 (G_M010_으로 시작)
// ====================================================================================================

// MPU6050 관련
const int G_M010_MPU_INTERRUPT_PIN = 4; // MPU6050 INT -> ESP32 GPIO 4
const float G_M010_ACCEL_ALPHA = 0.8;   // 가속도 필터링 상보 필터 계수
const float G_M010_GRAVITY_MPS2 = 9.80665; // 중력 가속도 (m/s^2)

// 자동차 상태 감지 임계값
const float G_M010_SPEED_THRESHOLD_KMH = 0.5;       // 속도 임계값 (km/h)
const float G_M010_ACCEL_STOP_THRESHOLD_MPS2 = 0.5; // 정차 가속도 변화 임계값 (m/s^2)
const float G_M010_ACCEL_DECEL_THRESHOLD_MPS2 = -3.0; // 급감속 Y축 가속도 임계값 (m/s^2)
const float G_M010_ACCEL_BUMP_THRESHOLD_MPS2 = 5.0; // 방지턱 Z축 가속도 변화 임계값 (m/s^2)
const unsigned long G_M010_BUMP_COOLDOWN_MS = 1000; // 방지턱 감지 후 쿨다운 (ms)

// 정차/주차 시간 기준 (초)
const unsigned long G_M010_STOP_GRACE_PERIOD_MS = 2000; // 2초 이상 움직임 없으면 정차
const unsigned long G_M010_SIGNAL_WAIT1_SECONDS = 60;   // 신호대기 1 기준
const unsigned long G_M010_SIGNAL_WAIT2_SECONDS = 120;  // 신호대기 2 기준
const unsigned long G_M010_STOP1_SECONDS = 300;         // 정차 1 기준 (5분)
const unsigned long G_M010_STOP2_SECONDS = 600;         // 정차 2 기준 (10분)
const unsigned long G_M010_PARK_SECONDS = 600;          // 주차 기준 (10분)

// 시리얼 출력 주기 (ms)
const unsigned long G_M010_SERIAL_PRINT_INTERVAL_MS = 5000;

// ====================================================================================================
// 자동차 움직임 상태 열거형
// ====================================================================================================
enum CarMovementState {
    G_M010_STATE_UNKNOWN,       // 초기/알 수 없음
    G_M010_STATE_STOPPED_INIT,  // 정차 초기
    G_M010_STATE_SIGNAL_WAIT1,  // 신호대기 1
    G_M010_STATE_SIGNAL_WAIT2,  // 신호대기 2
    G_M010_STATE_STOPPED1,      // 정차 1
    G_M010_STATE_STOPPED2,      // 정차 2
    G_M010_STATE_PARKED,        // 주차
    G_M010_STATE_FORWARD,       // 전진
    G_M010_STATE_REVERSE,       // 후진
    G_M010_STATE_DECELERATING,  // 급감속
    G_M010_STATE_SPEED_BUMP     // 과속 방지턱 통과
};

// ====================================================================================================
// 자동차 상태 정보 구조체
// ====================================================================================================
struct M010_CarStatus {
    CarMovementState v_currentMovementState; // 현재 움직임 상태

    float v_currentSpeed_kmh;      // 현재 속도 (km/h, 가속도 적분 추정)
    float v_accelerationX_ms2;     // X축 가속도 (m/s^2)
    float v_accelerationY_ms2;     // Y축 가속도 (m/s^2, 전진 기준: 양수-가속, 음수-감속)
    float v_accelerationZ_ms2;     // Z축 가속도 (m/s^2, 노면 충격 감지)

    float v_currentYawAngle_deg;   // Yaw 각도 (도)
    float v_yawRate_degps;         // Yaw 각속도 (도/초, 양수-우회전, 음수-좌회전)

    float v_pitchAngle_deg;        // Pitch 각도 (도, 양수-오르막, 음수-내리막)

    bool v_speedBumpDetected;      // 과속 방지턱 감지 여부
    bool v_isEmergencyBraking;     // 급감속 감지 여부

    unsigned long v_lastMovementTime_ms; // 마지막 움직임 감지 시간 (ms)
    unsigned long v_stopStartTime_ms;    // 정차 시작 시간 (ms)
    unsigned long v_currentStopTime_ms;  // 현재 정차 지속 시간 (ms)
};

// ====================================================================================================
// 전역 변수 (g_M010_으로 시작)
// ====================================================================================================
M010_CarStatus g_M010_carStatus; // 자동차 상태 구조체 인스턴스

// MPU6050 DMP 관련
bool g_M010_dmpReady = false;    // DMP 초기화 완료 여부
uint8_t g_M010_mpuIntStatus;     // MPU6050 인터럽트 상태
uint8_t g_M010_devStatus;       // 장치 상태 (0=성공, >0=오류)
uint16_t g_M010_packetSize;     // DMP 패킷 크기
uint16_t g_M010_fifoCount;      // FIFO 바이트 수
uint8_t g_M010_fifoBuffer[64];  // FIFO 버퍼

// 쿼터니언 및 오일러 각
Quaternion g_M010_q;            // 쿼터니언
VectorFloat g_M010_gravity;     // 중력 벡터 
float g_M010_ypr[3];            // Yaw, Pitch, Roll (radian)
float g_M010_yawRateDegPs;      // Yaw 각속도 (deg/s)

// 가속도 데이터 (raw 및 필터링)
// int16_t g_M010_ax, g_M010_ay, g_M010_az; // Raw 가속도 (더 이상 개별 변수로 필요 없음)
float g_M010_filteredAx, g_M010_filteredAy, g_M010_filteredAz; // 필터링 가속도 (m/s^2)

// 시간 관련
unsigned long g_M010_lastSampleTime_ms = 0;      // 마지막 MPU6050 샘플링 시간
unsigned long g_M010_lastSerialPrintTime_ms = 0; // 마지막 시리얼 출력 시간
unsigned long g_M010_lastBumpDetectionTime_ms = 0; // 마지막 방지턱 감지 시간

volatile bool g_M010_mpuInterrupt = false; // MPU6050 인터럽트 발생 여부
void M010_dmpDataReady() {
    g_M010_mpuInterrupt = true;
}

// ====================================================================================================
// 함수 선언 (프로토타입) - 함수가 정의되기 전에 미리 선언하여 컴파일러에게 알립니다.
// ====================================================================================================
void M010_defineCarState(unsigned long p_currentTime_ms); // 이 줄을 추가

// ====================================================================================================
// 함수 정의 (M010_으로 시작)
// ====================================================================================================

/**
 * @brief MPU6050 및 DMP 초기화.
 */
void M010_setupMPU6050() {
    Wire.begin(); // I2C 시작
    Wire.setClock(400000); // I2C 속도 400kHz

    Serial.println(F("MPU6050 초기화 중..."));
    Serial.print(F("MPU6050 연결 테스트: "));
    Serial.println(g_M010_mpu.testConnection() ? F("성공") : F("실패"));

    Serial.println(F("DMP 로딩 중..."));
    g_M010_devStatus = g_M010_mpu.dmpInitialize();

    // MPU6050 DMP 초기화 시, setDMPEnabled(true) 전에 setXGyroOffset, setYGyroOffset 등을
    // 적절히 호출하여 오프셋을 설정하는 것이 좋습니다.
    // 여기서는 예제 코드를 기반으로 하지만, 실제 사용에서는 캘리브레이션 루틴이 필요할 수 있습니다.
    // 예: g_M010_mpu.setXGyroOffset(220); g_M010_mpu.setYGyroOffset(76); 등

    if (g_M010_devStatus == 0) {
        Serial.println(F("DMP 활성화 중..."));
        g_M010_mpu.setDMPEnabled(true);

        Serial.print(F("MPU6050 인터럽트 핀 (GPIO "));
        Serial.print(G_M010_MPU_INTERRUPT_PIN);
        Serial.println(F(") 설정 중..."));
        pinMode(G_M010_MPU_INTERRUPT_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(G_M010_MPU_INTERRUPT_PIN), M010_dmpDataReady, RISING);
        g_M010_mpuIntStatus = g_M010_mpu.getIntStatus();

        // MotionApps612에서는 dmpGetFIFOPacketSize()가 MPU6050.h가 아닌
        // MPU6050_6Axis_MotionApps612.h에 직접 정의되어 있지 않을 수 있습니다.
        // 대신 DMP_FIFO_RATE를 설정할 때 내부적으로 패킷 크기가 결정됩니다.
        // 하지만 편의상 이전 버전과 동일하게 사용하거나, 
        // dmpGetCurrentFIFOPacket이 알아서 처리하므로 크게 중요하지 않을 수도 있습니다.
        // 일반적으로 42바이트입니다.
        g_M010_packetSize = 42; // MPU6050_6Axis_MotionApps612의 기본 DMP 패킷 크기
        // g_M010_packetSize = g_M010_mpu.dmpGetFIFOPacketSize(); // 이 함수는 612에서 사라졌을 수 있습니다.

        g_M010_dmpReady = true;
        Serial.println(F("DMP 초기화 완료!"));
    } else {
        Serial.print(F("DMP 초기화 실패 (오류 코드: "));
        Serial.print(g_M010_devStatus);
        Serial.println(F(")"));
        while (true); // 오류 시 무한 대기
    }
}

/**
 * @brief MPU6050 데이터 읽기 및 자동차 상태 업데이트.
 */
void M010_updateCarStatus() {
    if (!g_M010_dmpReady) return; // DMP 미준비 시 종료

    // MPU 인터럽트 발생 여부 또는 FIFO에 충분한 데이터가 있는지 확인 (MotionApps612 방식)
    if (!g_M010_mpuInterrupt && g_M010_fifoCount < g_M010_packetSize) {
        return; // 인터럽트가 없거나 데이터가 부족하면 함수 종료
    }
    
    // MPU 인터럽트 플래그 리셋 (인터럽트 루틴에서 설정)
    g_M010_mpuInterrupt = false;

    // DMP 패킷을 읽어옴. 이 함수는 FIFO 리셋 등의 내부 처리를 포함합니다.
    // dmpGetCurrentFIFOPacket은 FIFO에서 하나의 패킷을 읽어와 fifoBuffer에 저장합니다.
    if (g_M010_mpu.dmpGetCurrentFIFOPacket(g_M010_fifoBuffer)) { 
        unsigned long v_currentTime_ms = millis();
        float v_deltaTime_s = (v_currentTime_ms - g_M010_lastSampleTime_ms) / 1000.0f;
        g_M010_lastSampleTime_ms = v_currentTime_ms;

        // 쿼터니언, Yaw/Pitch/Roll 계산
        g_M010_mpu.dmpGetQuaternion(&g_M010_q, g_M010_fifoBuffer);
        g_M010_mpu.dmpGetGravity(&g_M010_gravity, &g_M010_q); 
        g_M010_mpu.dmpGetYawPitchRoll(g_M010_ypr, &g_M010_q, &g_M010_gravity); // g_M010_gravity는 VectorFloat* 타입

        g_M010_carStatus.v_currentYawAngle_deg = g_M010_ypr[0] * 180 / M_PI;
        g_M010_carStatus.v_pitchAngle_deg = g_M010_ypr[1] * 180 / M_PI;

        // 선형 가속도 (중력분 제거) 및 필터링
        VectorInt16 aa; // Raw 가속도 (int16)
        VectorInt16 linAccel; // 선형 가속도 결과를 VectorInt16으로 받음

        g_M010_mpu.dmpGetAccel(&aa, g_M010_fifoBuffer); // Raw 가속도 가져오기
        g_M010_mpu.dmpGetLinearAccel(&linAccel, &aa, &g_M010_gravity); // 수정된 함수 호출

        // MPU6050_6Axis_MotionApps612는 일반적으로 linAccel을 'g' 단위로 반환합니다.
        // 따라서 g_M010_GRAVITY_MPS2를 곱하여 m/s^2 단위로 변환합니다.
        float v_currentAx_ms2 = (float)linAccel.x * G_M010_GRAVITY_MPS2;
        float v_currentAy_ms2 = (float)linAccel.y * G_M010_GRAVITY_MPS2;
        float v_currentAz_ms2 = (float)linAccel.z * G_M010_GRAVITY_MPS2;

        g_M010_filteredAx = G_M010_ACCEL_ALPHA * g_M010_filteredAx + (1 - G_M010_ACCEL_ALPHA) * v_currentAx_ms2;
        g_M010_filteredAy = G_M010_ACCEL_ALPHA * g_M010_filteredAy + (1 - G_M010_ACCEL_ALPHA) * v_currentAy_ms2;
        g_M010_filteredAz = G_M010_ACCEL_ALPHA * g_M010_filteredAz + (1 - G_M010_ACCEL_ALPHA) * v_currentAz_ms2;

        g_M010_carStatus.v_accelerationX_ms2 = g_M010_filteredAx;
        g_M010_carStatus.v_accelerationY_ms2 = g_M010_filteredAy;
        g_M010_carStatus.v_accelerationZ_ms2 = g_M010_filteredAz;

        // Yaw 각속도 (Z축 자이로)
        VectorInt16 gyr; // 자이로 데이터 (VectorInt16)
        g_M010_mpu.dmpGetGyro(&gyr, g_M010_fifoBuffer); // VectorInt16*와 const uint8_t* packet=0 오버로드 사용

        g_M010_yawRateDegPs = (float)gyr.z / 131.0f; // 131 LSB/deg/s @ +/-250 deg/s
        g_M010_carStatus.v_yawRate_degps = g_M010_yawRateDegPs;

        // 속도 추정 (Y축 가속도 적분, 오차 누적 유의)
        float v_speedChange_mps = g_M010_carStatus.v_accelerationY_ms2 * v_deltaTime_s;
        g_M010_carStatus.v_currentSpeed_kmh += (v_speedChange_mps * 3.6); // m/s -> km/h

        // 정지 시 속도 드리프트 보정
        if (fabs(g_M010_carStatus.v_accelerationY_ms2) < G_M010_ACCEL_STOP_THRESHOLD_MPS2 &&
            fabs(g_M010_carStatus.v_yawRate_degps) < 1.0) {
            g_M010_carStatus.v_currentSpeed_kmh = 0.0;
        }

        // 자동차 상태 정의
        M010_defineCarState(v_currentTime_ms);
    }
}

/**
 * @brief MPU6050 데이터 기반 자동차 상태 정의.
 * @param p_currentTime_ms 현재 시간 (millis())
 */
void M010_defineCarState(unsigned long p_currentTime_ms) {
    float v_speed = g_M010_carStatus.v_currentSpeed_kmh;
    float v_accelY = g_M010_carStatus.v_accelerationY_ms2;
    float v_accelZ = g_M010_carStatus.v_accelerationZ_ms2;

    // 과속 방지턱 감지 (Z축 가속도 급변)
    if (fabs(v_accelZ) > G_M010_ACCEL_BUMP_THRESHOLD_MPS2 &&
        (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) > G_M010_BUMP_COOLDOWN_MS) {
        g_M010_carStatus.v_speedBumpDetected = true;
        g_M010_lastBumpDetectionTime_ms = p_currentTime_ms;
        g_M010_carStatus.v_currentMovementState = G_M010_STATE_SPEED_BUMP;
        return; // 최우선 감지
    } else if (g_M010_carStatus.v_speedBumpDetected &&
               (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) > G_M010_BUMP_COOLDOWN_MS) {
        g_M010_carStatus.v_speedBumpDetected = false; // 쿨다운 후 리셋
    }

    // 급감속 감지 (Y축 가속도 급감)
    if (v_accelY < G_M010_ACCEL_DECEL_THRESHOLD_MPS2) {
        g_M010_carStatus.v_isEmergencyBraking = true;
        g_M010_carStatus.v_currentMovementState = G_M010_STATE_DECELERATING;
        return; // 최우선 감지
    } else {
        g_M010_carStatus.v_isEmergencyBraking = false;
    }

    // 전진, 후진, 정차, 주차 판단
    if (fabs(v_speed) < G_M010_SPEED_THRESHOLD_KMH) { // 정차/주차 상태
        if (g_M010_carStatus.v_currentMovementState != G_M010_STATE_STOPPED_INIT &&
            g_M010_carStatus.v_currentMovementState != G_M010_STATE_SIGNAL_WAIT1 &&
            g_M010_carStatus.v_currentMovementState != G_M010_STATE_SIGNAL_WAIT2 &&
            g_M010_carStatus.v_currentMovementState != G_M010_STATE_STOPPED1 &&
            g_M010_carStatus.v_currentMovementState != G_M010_STATE_STOPPED2 &&
            g_M010_carStatus.v_currentMovementState != G_M010_STATE_PARKED) {
            g_M010_carStatus.v_stopStartTime_ms = p_currentTime_ms; // 정차 시작 시간 기록
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_STOPPED_INIT;
        }

        g_M010_carStatus.v_currentStopTime_ms = p_currentTime_ms - g_M010_carStatus.v_stopStartTime_ms;
        unsigned long v_stopSeconds = g_M010_carStatus.v_currentStopTime_ms / 1000;

        if (v_stopSeconds >= G_M010_PARK_SECONDS) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_PARKED;
        } else if (v_stopSeconds >= G_M010_STOP2_SECONDS) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_STOPPED2;
        } else if (v_stopSeconds >= G_M010_STOP1_SECONDS) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_STOPPED1;
        } else if (v_stopSeconds >= G_M010_SIGNAL_WAIT2_SECONDS) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_SIGNAL_WAIT2;
        } else if (v_stopSeconds >= G_M010_SIGNAL_WAIT1_SECONDS) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_SIGNAL_WAIT1;
        } else {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_STOPPED_INIT;
        }
        g_M010_carStatus.v_lastMovementTime_ms = 0; // 정지 중
    } else { // 움직이는 상태
        g_M010_carStatus.v_lastMovementTime_ms = p_currentTime_ms; // 마지막 움직임 시간 업데이트
        g_M010_carStatus.v_stopStartTime_ms = 0; // 정차 시작 시간 리셋

        if (v_speed > G_M010_SPEED_THRESHOLD_KMH) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_FORWARD;
        } else if (v_speed < -G_M010_SPEED_THRESHOLD_KMH) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_REVERSE;
        } else {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_UNKNOWN;
        }
    }
}

/**
 * @brief 자동차 상태 정보 시리얼 출력.
 */
void M010_printCarStatus() {
    Serial.println(F("\n--- 자동차 현재 상태 ---"));
    Serial.print(F("  상태: "));
    switch (g_M010_carStatus.v_currentMovementState) {
        case G_M010_STATE_UNKNOWN: Serial.println(F("알 수 없음")); break;
        case G_M010_STATE_STOPPED_INIT: Serial.println(F("정차 중 (초기)")); break;
        case G_M010_STATE_SIGNAL_WAIT1: Serial.print(F("신호대기 1 (60초 미만), 시간: ")); Serial.print(g_M010_carStatus.v_currentStopTime_ms / 1000); Serial.println(F("s")); break;
        case G_M010_STATE_SIGNAL_WAIT2: Serial.print(F("신호대기 2 (120초 미만), 시간: ")); Serial.print(g_M010_carStatus.v_currentStopTime_ms / 1000); Serial.println(F("s")); break;
        case G_M010_STATE_STOPPED1: Serial.print(F("정차 1 (5분 미만), 시간: ")); Serial.print(g_M010_carStatus.v_currentStopTime_ms / 1000); Serial.println(F("s")); break;
        case G_M010_STATE_STOPPED2: Serial.print(F("정차 2 (10분 미만), 시간: ")); Serial.print(g_M010_carStatus.v_currentStopTime_ms / 1000); Serial.println(F("s")); break;
        case G_M010_STATE_PARKED: Serial.print(F("주차 중 (10분 이상), 시간: ")); Serial.print(g_M010_carStatus.v_currentStopTime_ms / 1000); Serial.println(F("s")); break;
        case G_M010_STATE_FORWARD: Serial.println(F("전진 중")); break;
        case G_M010_STATE_REVERSE: Serial.println(F("후진 중")); break;
        case G_M010_STATE_DECELERATING: Serial.println(F("급감속 중")); break;
        case G_M010_STATE_SPEED_BUMP: Serial.println(F("과속 방지턱 통과")); break;
    }
    Serial.print(F("  추정 속도: ")); Serial.print(g_M010_carStatus.v_currentSpeed_kmh, 2); Serial.println(F(" km/h"));
    Serial.print(F("  가속도(X,Y,Z): "));
    Serial.print(g_M010_carStatus.v_accelerationX_ms2, 2); Serial.print(F(" m/s^2, "));
    Serial.print(g_M010_carStatus.v_accelerationY_ms2, 2); Serial.print(F(" m/s^2, "));
    Serial.print(g_M010_carStatus.v_accelerationZ_ms2, 2); Serial.println(F(" m/s^2"));
    Serial.print(F("  Yaw 각도: ")); Serial.print(g_M010_carStatus.v_currentYawAngle_deg, 2); Serial.println(F(" 도"));
    Serial.print(F("  Pitch 각도: ")); Serial.print(g_M010_carStatus.v_pitchAngle_deg, 2); Serial.println(F(" 도"));
    Serial.print(F("  Yaw 각속도: ")); Serial.print(g_M010_carStatus.v_yawRate_degps, 2); Serial.println(F(" 도/초"));
    Serial.print(F("  급감속: ")); Serial.println(g_M010_carStatus.v_isEmergencyBraking ? F("감지됨") : F("아님"));
    Serial.print(F("  과속 방지턱: ")); Serial.println(g_M010_carStatus.v_speedBumpDetected ? F("감지됨") : F("아님"));
    Serial.println(F("-----------------------"));
}

/**
 * @brief 초기 설정 (ESP32 시작 시 1회 실행).
 */
void M010_MPU_init() {
    
    M010_setupMPU6050(); // MPU6050 초기화

    // 자동차 상태 구조체 초기화
    g_M010_carStatus.v_currentMovementState = G_M010_STATE_UNKNOWN;
    g_M010_carStatus.v_currentSpeed_kmh = 0.0;
    g_M010_carStatus.v_accelerationX_ms2 = 0.0;
    g_M010_carStatus.v_accelerationY_ms2 = 0.0;
    g_M010_carStatus.v_accelerationZ_ms2 = 0.0;
    g_M010_carStatus.v_currentYawAngle_deg = 0.0;
    g_M010_carStatus.v_yawRate_degps = 0.0;
    g_M010_carStatus.v_pitchAngle_deg = 0.0;
    g_M010_carStatus.v_speedBumpDetected = false;
    g_M010_carStatus.v_isEmergencyBraking = false;
    g_M010_carStatus.v_lastMovementTime_ms = millis();
    g_M010_carStatus.v_stopStartTime_ms = 0;
    g_M010_carStatus.v_currentStopTime_ms = 0;

    g_M010_lastSampleTime_ms = millis();
    g_M010_lastSerialPrintTime_ms = millis();

    Serial.println(F("Setup 완료!"));
}

/**
 * @brief 메인 루프 (ESP32 반복 실행).
 */
void M010_MPU_run() {
    M010_updateCarStatus(); // MPU6050 데이터 및 상태 업데이트

    // 5초 간격 시리얼 출력
    if (millis() - g_M010_lastSerialPrintTime_ms >= G_M010_SERIAL_PRINT_INTERVAL_MS) {
        M010_printCarStatus();
        g_M010_lastSerialPrintTime_ms = millis();
    }

    // LED Matrix 업데이트 등 추가 작업 (예: M010_updateLEDMatrix(g_M010_carStatus.v_currentMovementState);)
}
