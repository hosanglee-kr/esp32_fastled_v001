#pragma once
// M010_main2_006.h
// ====================================================================================================
// 프로젝트: 자동차 후방 로봇 눈
// 설명: ESP32 + MPU6050 활용, 차량 움직임 감지 및 LED Matrix 로봇 눈 표정 표현.
//       본 코드는 MPU6000 데이터 처리 및 자동차 상태 정의.
// 개발 환경: PlatformIO Arduino Core (ESP32)
// 사용 라이브러리: MPU6050_DMP6 (I2Cdevlib by Jeff Rowberg) - MotionApps612 사용
// ====================================================================================================

// --- 시리얼 출력 제어 매크로 ---
// 디버그 출력을 활성화하려면 DEBUG_P1의 주석을 해제하세요.
#define DEBUG_P1 // 이 라인을 주석 처리하면 모든 dbgP1_ 매크로가 비활성화됩니다.
//#define DEBUG_P2 // 필요에 따라 추가적인 디버그 레벨/모듈을 정의할 수 있습니다.

#include "A01_debug_001.h"


// ====================================================================================================

#include <Wire.h> // I2C 통신
#include <I2Cdev.h> // I2C 장치 통신
#include <MPU6050_6Axis_MotionApps612.h> // MPU6050 DMP 기능

// MPU6050 객체
MPU6050 g_M010_Mpu;

// ====================================================================================================
// 전역 상수 (G_M010_으로 시작)
// ====================================================================================================

// MPU6050 관련
const int G_M010_MPU_INTERRUPT_PIN = 4; // MPU6050 INT -> ESP32 GPIO 4

const float G_M010_ACCEL_ALPHA = 0.8;   // 가속도 필터링 상보 필터 계수
const float G_M010_GRAVITY_MPS2 = 9.80665; // 중력 가속도 (m/s^2)

// 자동차 상태 감지 임계값 (상태 머신 전이 조건)
const float G_M010_SPEED_FORWARD_THRESHOLD_KMH = 0.8; // 정지 -> 전진 전환을 위한 최소 속도 (히스테리시스 상단)
const float G_M010_SPEED_REVERSE_THRESHOLD_KMH = 0.8; // 정지 -> 후진 전환을 위한 최소 속도 (히스테리시스 상단)
const float G_M010_SPEED_STOP_THRESHOLD_KMH = 0.2;    // 전진/후진 -> 정지 전환을 위한 최대 속도 (히스테리시스 하단)

const float G_M010_ACCEL_STOP_THRESHOLD_MPS2 = 0.2; // 정차 가속도 변화 임계값 (m/s^2)
const float G_M010_GYRO_STOP_THRESHOLD_DPS = 0.5;   // 정차 자이로 변화 임계값 (deg/s)

const unsigned long G_M010_STOP_STABLE_DURATION_MS = 200;  // 정지 안정화 지속 시간 (ms)
const unsigned long G_M010_MOVE_STABLE_DURATION_MS = 150;  // 움직임 감지 안정화 지속 시간 (ms)
const unsigned long G_M010_NORMAL_MOVE_DURATION_MS = 100; // 특수 상태(감속/방지턱) 해제 후 일반 움직임으로 복귀 시간 (ms)

// 급감속/방지턱 관련 임계값
const float G_M010_ACCEL_DECEL_THRESHOLD_MPS2 = -3.0; // 급감속 Y축 가속도 임계값 (m/s^2)
const float G_M010_ACCEL_BUMP_THRESHOLD_MPS2 = 5.0; // 방지턱 Z축 가속도 변화 임계값 (m/s^2)
const float G_M010_BUMP_MIN_SPEED_KMH = 5.0; // 방지턱 감지 최소 속도 (km/h)

const unsigned long G_M010_BUMP_COOLDOWN_MS = 1000; // 방지턱 감지 후 쿨다운 (ms) - 재감지 방지
const unsigned long G_M010_DECEL_HOLD_DURATION_MS = 10000; // 급감속 상태 유지 시간 (10초)
const unsigned long G_M010_BUMP_HOLD_DURATION_MS = 10000;  // 과속방지턱 감지 상태 유지 시간 (10초)

// 정차/주차 시간 기준 (초)
const unsigned long G_M010_STOP_GRACE_PERIOD_MS = 2000; // 2초 이상 움직임 없으면 정차 (현재 사용되지 않음, 참고용)
const unsigned long G_M010_SIGNAL_WAIT1_SECONDS = 60;   // 신호대기 1 기준
const unsigned long G_M010_SIGNAL_WAIT2_SECONDS = 120;  // 신호대기 2 기준
const unsigned long G_M010_STOP1_SECONDS = 300;         // 정차 1 기준 (5분)
const unsigned long G_M010_STOP2_SECONDS = 600;         // 정차 2 기준 (10분)
const unsigned long G_M010_PARK_SECONDS = 600;          // 주차 기준 (10분)

// 시리얼 출력 주기 (ms)
const unsigned long G_M010_SERIAL_PRINT_INTERVAL_MS = 5000;

// ====================================================================================================
// 자동차 움직임 상태 열거형 (State Machine State)
// ====================================================================================================
typedef enum {
    E_M010_STATE_UNKNOWN,       // 초기/알 수 없는 상태
    E_M010_STATE_STOPPED_INIT,  // 기본 정차 상태 (다른 정차 세부 상태의 진입점)
    E_M010_STATE_SIGNAL_WAIT1,  // 신호대기 1 (정차 시간 60초 미만)
    E_M010_STATE_SIGNAL_WAIT2,  // 신호대기 2 (정차 시간 120초 미만)
    E_M010_STATE_STOPPED1,      // 정차 1 (정차 시간 5분 미만)
    E_M010_STATE_STOPPED2,      // 정차 2 (정차 시간 10분 미만)
    E_M010_STATE_PARKED,        // 주차 (정차 시간 10분 이상)
    E_M010_STATE_FORWARD,       // 전진 중
    E_M010_STATE_REVERSE,       // 후진 중
} T_M010_CarMovementState;

// ====================================================================================================
// 자동차 상태 정보 구조체
// ====================================================================================================
typedef struct {
    T_M010_CarMovementState movementState; // 현재 움직임 상태

    float speed_kmh;               // 현재 속도 (km/h, 가속도 적분 추정)
    float accelX_ms2;             // X축 가속도 (m/s^2)
    float accelY_ms2;             // Y축 가속도 (m/s^2, 전진 기준: 양수-가속, 음수-감속)
    float accelZ_ms2;             // Z축 가속도 (m/s^2, 노면 충격 감지)

    float yawAngle_deg;            // Yaw 각도 (도)
    float yawAngleVelocity_degps;  // Yaw 각속도 (도/초, 양수-우회전, 음수-좌회전)

    float pitchAngle_deg;          // Pitch 각도 (도, 양수-오르막, 음수-내리막)

    bool isSpeedBumpDetected;      // 과속 방지턱 감지 여부 (일시적 플래그)
    bool isEmergencyBraking;       // 급감속 감지 여부 (일시적 플래그)

    unsigned long lastMovementTime_ms; // 마지막 움직임 감지 시간 (ms)
    unsigned long stopStartTime_ms;    // 정차 시작 시간 (ms)
    unsigned long currentStopTime_ms;  // 현재 정차 지속 시간 (ms)
    unsigned long stopStableStartTime_ms; // 정지 상태 안정화 시작 시간 (ms) - 속도 0 보정용
} T_M010_CarStatus;

// ====================================================================================================
// 전역 변수 (g_M010_으로 시작)
// ====================================================================================================
T_M010_CarStatus g_M010_CarStatus; // 자동차 상태 구조체 인스턴스

// MPU6050 DMP 관련
bool     g_M010_dmp_isReady = false; // DMP 초기화 완료 여부
uint8_t  g_M010_mpu_interruptStatus;        // MPU6050 인터럽트 상태
uint8_t  g_M010_dmp_devStatus;       // 장치 상태 (0=성공, >0=오류)
uint16_t g_M010_dmp_packetSize;     // DMP 패킷 크기
uint16_t g_M010_dmp_fifoCount;      // FIFO 바이트 수
uint8_t  g_M010_dmp_fifoBuffer[64];  // FIFO 버퍼

// 쿼터니언 및 오일러 각
Quaternion  g_M010_Quaternion;            // 쿼터니언
VectorFloat g_M010_gravity;               // 중력 벡터 
float       g_M010_ypr[3];            // Yaw, Pitch, Roll (radian)
float       g_M010_yawAngleVelocity_degps;      // Yaw 각속도 (deg/s)

// 가속도 데이터 (필터링)
float g_M010_filteredAx;
float g_M010_filteredAy;
float g_M010_filteredAz; // 필터링 가속도 (m/s^2)

// 시간 관련
unsigned long g_M010_lastSampleTime_ms = 0;      // 마지막 MPU6050 샘플링 시간
unsigned long g_M010_lastSerialPrintTime_ms = 0; // 마지막 시리얼 출력 시간
unsigned long g_M010_lastBumpDetectionTime_ms = 0; // 마지막 방지턱 감지 시간
unsigned long g_M010_lastDecelDetectionTime_ms = 0; // 마지막 급감속 감지 시간

unsigned long g_M010_stateTransitionStartTime_ms = 0; // 상태 전환 조건이 만족된 시작 시간

volatile bool g_M010_mpu_isInterrupt = false; // MPU6050 인터럽트 발생 여부
void M010_dmpDataReady() {
    g_M010_mpu_isInterrupt = true;
}

// ====================================================================================================
// 함수 선언 (프로토타입)
// ====================================================================================================
void M010_defineCarState(unsigned long p_currentTime_ms);

// ====================================================================================================
// 함수 정의 (M010_으로 시작)
// ====================================================================================================

/**
 * @brief MPU6050 및 DMP 초기화.
 */
void M010_setupMPU6050() {
    Wire.begin(); // I2C 시작
    Wire.setClock(400000); // I2C 속도 400kHz


    dbgP1_print_F(F("MPU6050 연결 테스트: "));
    dbgP1_println_F(g_M010_Mpu.testConnection() ? F("성공") : F("실패"));
    dbgP1_println_F(F("DMP 로딩 중..."));

    g_M010_dmp_devStatus = g_M010_Mpu.dmpInitialize();

    if (g_M010_dmp_devStatus == 0) {
        dbgP1_println_F(F("DMP 활성화 중..."));
        dbgP1_print_F(F("MPU6050 인터럽트 핀 (GPIO "));
        dbgP1_print(G_M010_MPU_INTERRUPT_PIN);
        dbgP1_println_F(F(") 설정 중..."));
        
        pinMode(G_M010_MPU_INTERRUPT_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(G_M010_MPU_INTERRUPT_PIN), M010_dmpDataReady, RISING);
        g_M010_mpu_interruptStatus = g_M010_Mpu.getIntStatus();

        g_M010_dmp_packetSize = 42; 

        g_M010_dmp_isReady = true;
        dbgP1_println_F(F("DMP 초기화 완료!"));
    } else {
        dbgP1_printf_F(F("DMP 초기화 실패 (오류 코드: %d)\n"), g_M010_dmp_devStatus);
        while (true); // 오류 시 무한 대기
    }
}

/**
 * @brief MPU6050 데이터 읽기 및 자동차 상태 업데이트.
 */
void M010_updateCarStatus() {
    if (!g_M010_dmp_isReady) return; // DMP 미준비 시 종료

    if (!g_M010_mpu_isInterrupt && g_M010_dmp_fifoCount < g_M010_dmp_packetSize) {
        return; // 인터럽트가 없거나 데이터가 부족하면 함수 종료
    }
    
    g_M010_mpu_isInterrupt = false;

    if (g_M010_Mpu.dmpGetCurrentFIFOPacket(g_M010_dmp_fifoBuffer)) { 
        unsigned long v_currentTime_ms = millis();
        float v_deltaTime_s = (v_currentTime_ms - g_M010_lastSampleTime_ms) / 1000.0f;
        g_M010_lastSampleTime_ms = v_currentTime_ms;

        // 쿼터니언, Yaw/Pitch/Roll 계산
        g_M010_Mpu.dmpGetQuaternion(&g_M010_Quaternion, g_M010_dmp_fifoBuffer);
        g_M010_Mpu.dmpGetGravity(&g_M010_gravity, &g_M010_Quaternion); 
        g_M010_Mpu.dmpGetYawPitchRoll(g_M010_ypr, &g_M010_Quaternion, &g_M010_gravity);

        g_M010_CarStatus.yawAngle_deg = g_M010_ypr[0] * 180 / M_PI;
        g_M010_CarStatus.pitchAngle_deg = g_M010_ypr[1] * 180 / M_PI;

        // 선형 가속도 (중력분 제거) 및 필터링
        VectorInt16 aa; // Raw 가속도 (int16)
        VectorInt16 linAccel; // 선형 가속도 결과를 VectorInt16으로 받음

        g_M010_Mpu.dmpGetAccel(&aa, g_M010_dmp_fifoBuffer);
        g_M010_Mpu.dmpGetLinearAccel(&linAccel, &aa, &g_M010_gravity);

        float v_currentAx_ms2 = (float)linAccel.x * G_M010_GRAVITY_MPS2;
        float v_currentAy_ms2 = (float)linAccel.y * G_M010_GRAVITY_MPS2;
        float v_currentAz_ms2 = (float)linAccel.z * G_M010_GRAVITY_MPS2;

        g_M010_filteredAx = G_M010_ACCEL_ALPHA * g_M010_filteredAx + (1 - G_M010_ACCEL_ALPHA) * v_currentAx_ms2;
        g_M010_filteredAy = G_M010_ACCEL_ALPHA * g_M010_filteredAy + (1 - G_M010_ACCEL_ALPHA) * v_currentAy_ms2;
        g_M010_filteredAz = G_M010_ACCEL_ALPHA * g_M010_filteredAz + (1 - G_M010_ACCEL_ALPHA) * v_currentAz_ms2;

        g_M010_CarStatus.accelX_ms2 = g_M010_filteredAx;
        g_M010_CarStatus.accelY_ms2 = g_M010_filteredAy;
        g_M010_CarStatus.accelZ_ms2 = g_M010_filteredAz;

        // Yaw 각속도 (Z축 자이로)
        VectorInt16 gyr; // 자이로 데이터 (VectorInt16)
        g_M010_Mpu.dmpGetGyro(&gyr, g_M010_dmp_fifoBuffer);

        g_M010_yawAngleVelocity_degps = (float)gyr.z / 131.0f; // 131 LSB/deg/s @ +/-250 deg/s
        g_M010_CarStatus.yawAngleVelocity_degps = g_M010_yawAngleVelocity_degps;

        // 속도 추정 (Y축 가속도 적분, 오차 누적 유의)
        float v_speedChange_mps = g_M010_CarStatus.accelY_ms2 * v_deltaTime_s;
        g_M010_CarStatus.speed_kmh += (v_speedChange_mps * 3.6); // m/s -> km/h

        // 정지 시 속도 드리프트 보정 강화
        // 가속도 및 각속도 변화가 모두 임계값 이하로 충분히 오래 유지될 때만 속도를 0으로 설정
        if (fabs(g_M010_CarStatus.accelY_ms2) < G_M010_ACCEL_STOP_THRESHOLD_MPS2 &&
            fabs(g_M010_CarStatus.yawAngleVelocity_degps) < G_M010_GYRO_STOP_THRESHOLD_DPS) {
            
            if (g_M010_CarStatus.stopStableStartTime_ms == 0) { // 정지 안정화 시작 시간 기록
                g_M010_CarStatus.stopStableStartTime_ms = v_currentTime_ms;
            } else if ((v_currentTime_ms - g_M010_CarStatus.stopStableStartTime_ms) >= G_M010_STOP_STABLE_DURATION_MS) {
                g_M010_CarStatus.speed_kmh = 0.0; // 충분히 안정적인 상태로 판단되면 속도 0으로 보정
            }
        } else {
            g_M010_CarStatus.stopStableStartTime_ms = 0; // 움직임 감지 시 안정화 시작 시간 리셋
        }

        // 자동차 상태 정의
        M010_defineCarState(v_currentTime_ms);
    }
}

/**
 * @brief MPU6050 데이터 기반 자동차 상태 정의. (상태 머신 로직)
 * @param p_currentTime_ms 현재 시간 (millis())
 */
void M010_defineCarState(unsigned long p_currentTime_ms) {
    float v_speed = g_M010_CarStatus.speed_kmh;
    float v_accelY = g_M010_CarStatus.accelY_ms2;
    float v_accelZ = g_M010_CarStatus.accelZ_ms2;

    // 다음 상태를 예측 (기본적으로 현재 상태 유지)
    T_M010_CarMovementState v_nextState = g_M010_CarStatus.movementState;

    // =============================================================================================
    // 1. 과속 방지턱 감지 (일시적 플래그)
    //    속도 조건 및 쿨다운 포함. 메인 상태와 독립적으로 작동.
    // =============================================================================================
    if (fabs(v_accelZ) > G_M010_ACCEL_BUMP_THRESHOLD_MPS2 &&
        fabs(v_speed) > G_M010_BUMP_MIN_SPEED_KMH && // 속도 조건 추가
        (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) > G_M010_BUMP_COOLDOWN_MS) {
        g_M010_CarStatus.isSpeedBumpDetected = true;
        g_M010_lastBumpDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isSpeedBumpDetected &&
               (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) >= G_M010_BUMP_HOLD_DURATION_MS) {
        g_M010_CarStatus.isSpeedBumpDetected = false; // 유지 시간 후 리셋
    }

    // =============================================================================================
    // 2. 급감속 감지 (일시적 플래그)
    //    메인 상태와 독립적으로 작동.
    // =============================================================================================
    if (v_accelY < G_M010_ACCEL_DECEL_THRESHOLD_MPS2) {
        g_M010_CarStatus.isEmergencyBraking = true;
        g_M010_lastDecelDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isEmergencyBraking &&
               (p_currentTime_ms - g_M010_lastDecelDetectionTime_ms) >= G_M010_DECEL_HOLD_DURATION_MS) {
        g_M010_CarStatus.isEmergencyBraking = false; // 유지 시간 후 리셋
    }

    // =============================================================================================
    // 3. 메인 움직임 상태 머신 로직
    //    히스테리시스 및 시간 지연을 통한 안정적인 상태 전환.
    // =============================================================================================
    switch (g_M010_CarStatus.movementState) {
        case E_M010_STATE_UNKNOWN:
        case E_M010_STATE_STOPPED_INIT:
        case E_M010_STATE_SIGNAL_WAIT1:
        case E_M010_STATE_SIGNAL_WAIT2:
        case E_M010_STATE_STOPPED1:
        case E_M010_STATE_STOPPED2:
        case E_M010_STATE_PARKED:
            // 현재가 '정지' 관련 상태일 때, '움직임' 상태로의 전환 조건 확인
            if (v_speed > G_M010_SPEED_FORWARD_THRESHOLD_KMH) { // 전진 시작 조건 (히스테리시스 상단)
                if (g_M010_stateTransitionStartTime_ms == 0) {
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms; // 조건 만족 시작 시간 기록
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= G_M010_MOVE_STABLE_DURATION_MS) {
                    // 전진 조건이 충분히 오래 지속되면 상태 전환
                    v_nextState = E_M010_STATE_FORWARD;
                    g_M010_CarStatus.stopStartTime_ms = 0;        // 정차 시간 리셋
                    g_M010_CarStatus.lastMovementTime_ms = p_currentTime_ms; // 마지막 움직임 시간 기록
                    g_M010_stateTransitionStartTime_ms = 0;      // 전환 완료 후 리셋
                }
            } else if (v_speed < -G_M010_SPEED_REVERSE_THRESHOLD_KMH) { // 후진 시작 조건 (히스테리시스 상단)
                if (g_M010_stateTransitionStartTime_ms == 0) {
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= G_M010_MOVE_STABLE_DURATION_MS) {
                    // 후진 조건이 충분히 오래 지속되면 상태 전환
                    v_nextState = E_M010_STATE_REVERSE;
                    g_M010_CarStatus.stopStartTime_ms = 0;
                    g_M010_CarStatus.lastMovementTime_ms = p_currentTime_ms;
                    g_M010_stateTransitionStartTime_ms = 0;
                }
            } else {
                // 전진/후진 조건 불충족 시 전환 시작 시간 리셋
                g_M010_stateTransitionStartTime_ms = 0;

                // 정차 상태 내에서의 세부 시간 기반 상태 전환 (정지 상태가 유지될 때만 갱신)
                g_M010_CarStatus.currentStopTime_ms = p_currentTime_ms - g_M010_CarStatus.stopStartTime_ms;
                unsigned long v_stopSeconds = g_M010_CarStatus.currentStopTime_ms / 1000;

                if (v_stopSeconds >= G_M010_PARK_SECONDS) {
                    v_nextState = E_M010_STATE_PARKED;
                } else if (v_stopSeconds >= G_M010_STOP2_SECONDS) {
                    v_nextState = E_M010_STATE_STOPPED2;
                } else if (v_stopSeconds >= G_M010_STOP1_SECONDS) {
                    v_nextState = E_M010_STATE_STOPPED1;
                } else if (v_stopSeconds >= G_M010_SIGNAL_WAIT2_SECONDS) {
                    v_nextState = E_M010_STATE_SIGNAL_WAIT2;
                } else if (v_stopSeconds >= G_M010_SIGNAL_WAIT1_SECONDS) {
                    v_nextState = E_M010_STATE_SIGNAL_WAIT1;
                } 
                // E_M010_STATE_STOPPED_INIT은 이보다 낮은 시간 기준이므로, 기본 정차 상태가 됨.
            }
            break;

        case E_M010_STATE_FORWARD:
        case E_M010_STATE_REVERSE:
            // 현재가 '움직임' 상태일 때, '정지' 상태로의 전환 조건 확인
            if (fabs(v_speed) < G_M010_SPEED_STOP_THRESHOLD_KMH) { // 정지 조건 (히스테리시스 하단)
                if (g_M010_stateTransitionStartTime_ms == 0) {
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms; // 조건 만족 시작 시간 기록
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= G_M010_STOP_STABLE_DURATION_MS) {
                    // 정지 조건이 충분히 오래 지속되면 상태 전환
                    v_nextState = E_M010_STATE_STOPPED_INIT; // 정차의 진입점 상태로 전환
                    g_M010_CarStatus.speed_kmh = 0.0;     // 속도 0으로 보정
                    g_M010_CarStatus.stopStartTime_ms = p_currentTime_ms; // 정차 시작 시간 기록
                    g_M010_CarStatus.lastMovementTime_ms = 0; // 움직임 없음
                    g_M010_stateTransitionStartTime_ms = 0; // 전환 완료 후 리셋
                }
            } else {
                // 정지 조건 불충족 시 전환 시작 시간 리셋
                g_M010_stateTransitionStartTime_ms = 0;
            }
            break;
    }
    
    // 최종 상태 업데이트 (상태가 실제로 변경될 때만)
    if (v_nextState != g_M010_CarStatus.movementState) {
        dbgP1_printf_F(F("State transition: %d -> %d\n"), g_M010_CarStatus.movementState, v_nextState);
        g_M010_CarStatus.movementState = v_nextState;
    }
}

/**
 * @brief 자동차 상태 정보 시리얼 출력.
 */
void M010_printCarStatus() {
    dbgP1_println_F(F("\n---- 자동차 현재 상태 ----"));
    dbgP1_print_F(F("상태: "));
    switch (g_M010_CarStatus.movementState) {
        case E_M010_STATE_UNKNOWN: dbgP1_println_F(F("알 수 없음")); break;
        case E_M010_STATE_STOPPED_INIT: dbgP1_println_F(F("정차 중 (초기)")); break;
        case E_M010_STATE_SIGNAL_WAIT1: dbgP1_print_F(F("신호대기 1 (60초 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_SIGNAL_WAIT2: dbgP1_print_F(F("신호대기 2 (120초 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_STOPPED1: dbgP1_print_F(F("정차 1 (5분 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_STOPPED2: dbgP1_print_F(F("정차 2 (10분 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_PARKED: dbgP1_print_F(F("주차 중 (10분 이상), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_FORWARD: dbgP1_println_F(F("전진 중")); break;
        case E_M010_STATE_REVERSE: dbP1_println_F(F("후진 중")); break;
    }
    dbgP1_print_F(F("추정 속도: ")); dbgP1_print(g_M010_CarStatus.speed_kmh, 2); dbgP1_println_F(F(" km/h"));
    dbgP1_print_F(F("가속도(X,Y,Z): "));
    dbgP1_print(g_M010_CarStatus.accelX_ms2, 2); dbgP1_print_F(F(" m/s^2, "));
    dbgP1_print(g_M010_CarStatus.accelY_ms2, 2); dbgP1_print_F(F(" m/s^2, "));
    dbgP1_print(g_M010_CarStatus.accelZ_ms2, 2); dbgP1_println_F(F(" m/s^2"));
    dbgP1_print_F(F("Yaw 각도: ")); dbgP1_print(g_M010_CarStatus.yawAngle_deg, 2); dbgP1_println_F(F(" 도"));
    dbgP1_print_F(F("Pitch 각도: ")); dbgP1_print(g_M010_CarStatus.pitchAngle_deg, 2); dbgP1_println_F(F(" 도"));
    dbgP1_print_F(F("Yaw 각속도: ")); dbgP1_print(g_M010_CarStatus.yawAngleVelocity_degps, 2); dbgP1_println_F(F(" 도/초"));
    dbgP1_print_F(F("급감속: ")); dbgP1_println_F(g_M010_CarStatus.isEmergencyBraking ? F("감지됨") : F("아님"));
    dbgP1_print_F(F("과속 방지턱: ")); dbgP1_println_F(g_M010_CarStatus.isSpeedBumpDetected ? F("감지됨") : F("아님"));
    dbgP1_println_F(F("--------------------------"));
}

/**
 * @brief 초기 설정 (ESP32 시작 시 1회 실행).
 */
void M010_MPU_init() {
    
    M010_setupMPU6050(); // MPU6050 초기화

    // 자동차 상태 구조체 초기화
    g_M010_CarStatus.movementState = E_M010_STATE_UNKNOWN;
    g_M010_CarStatus.speed_kmh = 0.0;
    g_M010_CarStatus.accelX_ms2 = 0.0;
    g_M010_CarStatus.accelY_ms2 = 0.0;
    g_M010_CarStatus.accelZ_ms2 = 0.0;
    g_M010_CarStatus.yawAngle_deg = 0.0;
    g_M010_CarStatus.yawAngleVelocity_degps = 0.0;
    g_M010_CarStatus.pitchAngle_deg = 0.0;
    g_M010_CarStatus.isSpeedBumpDetected = false;
    g_M010_CarStatus.isEmergencyBraking = false;
    g_M010_CarStatus.lastMovementTime_ms = millis();
    g_M010_CarStatus.stopStartTime_ms = 0;
    g_M010_CarStatus.currentStopTime_ms = 0;
    g_M010_CarStatus.stopStableStartTime_ms = 0; // 초기화 추가

    g_M010_lastSampleTime_ms = 0; // millis()는 setup() 시작 시점부터이므로 0으로 초기화하여 첫 샘플 시간 계산에 사용
    g_M010_lastSerialPrintTime_ms = 0; // millis()는 setup() 시작 시점부터이므로 0으로 초기화
    g_M010_lastBumpDetectionTime_ms = 0; // 초기화
    g_M010_lastDecelDetectionTime_ms = 0; // 초기화
    g_M010_stateTransitionStartTime_ms = 0; // 상태 전환 시간 초기화

    dbgP1_println_F(F("Setup 완료!"));
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

    // LED Matrix 업데이트 등 추가 작업 (예: M010_updateLEDMatrix(g_M010_CarStatus.movementState);)
}
