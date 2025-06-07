#pragma once
// M010_main2_007.h
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

#include "A01_debug_001.h" // 디버그 출력을 위한 라이브러리 포함

// ====================================================================================================

#include <Wire.h> // I2C 통신을 위한 라이브러리
#include <I2Cdev.h> // I2C 장치 통신을 위한 라이브러리
#include <MPU6050_6Axis_MotionApps612.h> // MPU6050 DMP 기능을 위한 라이브러리

// LittleFS 및 ArduinoJson 라이브러리 추가
#include <FS.h> 
#include <LittleFS.h>
#include <ArduinoJson.h>

// MPU6050 객체 생성
MPU6050 g_M010_Mpu;

// ====================================================================================================
// 전역 설정 변수 (g_M010_CFG_으로 시작) 정의
// ====================================================================================================

// --- 차량 상태 머신 (State Machine) 개요 (기존 설명 유지) ---
/*
 * 주요 상태: 정지 (STOPPED_INIT 및 세부 상태), 전진 (FORWARD), 후진 (REVERSE)
 * 상태 전환은 히스테리시스 (임계값 차등) 및 시간 지연 (조건 지속 시간)을 통해 노이즈에 강인하게 동작합니다.
 *
 * 상태               | 진입 조건                                                          | 전이 조건 (다음 상태)
 * -------------------|--------------------------------------------------------------------|--------------------------------------------------------------------
 * E_M010_STATE_UNKNOWN | (초기 상태)                                                      | - 초기화 시 -> E_M010_STATE_STOPPED_INIT
 * E_M010_STATE_STOPPED_INIT | - 초기 진입                                                  | - 속도 > G_M010_SPEED_FORWARD_THRESHOLD_KMH가 G_M010_MOVE_STABLE_DURATION_MS 이상 지속 -> E_M010_STATE_FORWARD
 * (and sub-states:    | - (FROM FORWARD/REVERSE): 속도 < G_M010_SPEED_STOP_THRESHOLD_KMH가 G_M010_STOP_STABLE_DURATION_MS 이상 지속 | - 속도 < -G_M010_SPEED_REVERSE_THRESHOLD_KMH가 G_M010_MOVE_STABLE_DURATION_MS 이상 지속 -> E_M010_STATE_REVERSE
 * SIGNAL_WAIT1/2,      |                                                                    | - (내부적으로 시간 경과에 따라): SIGNAL_WAIT1/2, STOPPED1/2, PARKED
 * STOPPED1/2, PARKED) |                                                                    |
 * E_M010_STATE_FORWARD | - (FROM STOPPED_INIT): 속도 > G_M010_SPEED_FORWARD_THRESHOLD_KMH가 G_M010_MOVE_STABLE_DURATION_MS 이상 지속 | - 속도 < G_M010_SPEED_STOP_THRESHOLD_KMH가 G_M010_STOP_STABLE_DURATION_MS 이상 지속 -> E_M010_STATE_STOPPED_INIT
 * E_M010_STATE_REVERSE | - (FROM STOPPED_INIT): 속도 < -G_M010_SPEED_REVERSE_THRESHOLD_KMH가 G_M010_MOVE_STABLE_DURATION_MS 이상 지속 | - 속도 > -G_M010_SPEED_STOP_THRESHOLD_KMH가 G_M010_STOP_STABLE_DURATION_MS 이상 지속 -> E_M010_STATE_STOPPED_INIT
 *
 * 특수 감지 플래그 (메인 상태와 독립적으로 동작하며, 해당 조건이 만족될 때만 True):
 * - isSpeedBumpDetected: Z축 가속도 임계값 초과 & 최소 속도 이상 & 쿨다운 시간 경과 시 True. G_M010_BUMP_HOLD_DURATION_MS 동안 유지.
 * - isEmergencyBraking: Y축 가속도 급감속 임계값 미만 시 True. G_M010_DECEL_HOLD_DURATION_MS 동안 유지.
 */
// ---------------------------------------------------

// MPU6050 관련 설정 변수
int g_M010_CFG_MpuInterruptPin = 4; // MPU6050 INT 핀이 ESP32 GPIO 4에 연결

float g_M010_CFG_AccelAlpha = 0.8;   // 가속도 필터링을 위한 상보 필터 계수
float g_M010_CFG_GravityMps2 = 9.80665; // 중력 가속도 값 (m/s^2)

// 자동차 움직임 상태 감지 임계값 (상태 머신 전이 조건)
float g_M010_CFG_SpeedForwardThresholdKmh = 0.8; // 정지 상태에서 전진 상태로 전환하기 위한 최소 속도 (km/h)
float g_M010_CFG_SpeedReverseThresholdKmh = 0.8; // 정지 상태에서 후진 상태로 전환하기 위한 최소 속도 (km/h)
float g_M010_CFG_SpeedStopThresholdKmh = 0.2;    // 전진/후진 상태에서 정지 상태로 전환하기 위한 최대 속도 (km/h)

float g_M010_CFG_AccelStopThresholdMps2 = 0.2; // 차량 정차 여부 판단을 위한 Y축 가속도 변화 임계값 (m/s^2)
float g_M010_CFG_GyroStopThresholdDps = 0.5;   // 차량 정차 여부 판단을 위한 Yaw 각속도 변화 임계값 (deg/s)

unsigned long g_M010_CFG_StopStableDurationMs = 200;  // 정지 상태가 안정적으로 지속되어야 하는 시간 (ms)
unsigned long g_M010_CFG_MoveStableDurationMs = 150;  // 움직임이 안정적으로 감지되어야 하는 시간 (ms)
unsigned long g_M010_CFG_NormalMoveDurationMs = 100; // 특수 상태 해제 후 일반 움직임으로 복귀하는 데 필요한 시간 (ms)

// 급감속/방지턱 감지 관련 임계값
float g_M010_CFG_AccelDecelThresholdMps2 = -3.0; // 급감속 감지를 위한 Y축 가속도 임계값 (음수 값, m/s^2)
float g_M010_CFG_AccelBumpThresholdMps2 = 5.0; // 방지턱 감지를 위한 Z축 가속도 변화 임계값 (m/s^2)
float g_M010_CFG_BumpMinSpeedKmh = 5.0; // 방지턱 감지 시 필요한 최소 속도 (km/h)

unsigned long g_M010_CFG_BumpCooldownMs = 1000; // 방지턱 감지 후 재감지를 방지하는 쿨다운 시간 (ms)
unsigned long g_M010_CFG_DecelHoldDurationMs = 10000; // 급감속 상태가 유지되는 시간 (ms, 10초)
unsigned long g_M010_CFG_BumpHoldDurationMs = 10000;  // 과속방지턱 감지 상태가 유지되는 시간 (ms, 10초)

// 정차/주차 시간 기준 (초)
unsigned long g_M010_CFG_StopGracePeriodMs = 2000; // 2초 이상 움직임 없으면 정차로 간주 (현재 사용되지 않음, 참고용)
unsigned long g_M010_CFG_SignalWait1Seconds = 60;   // 신호대기 1 상태 기준 시간 (60초)
unsigned long g_M010_CFG_SignalWait2Seconds = 120;  // 신호대기 2 상태 기준 시간 (120초)
unsigned long g_M010_CFG_Stop1Seconds = 300;         // 정차 1 상태 기준 시간 (300초 = 5분)
unsigned long g_M010_CFG_Stop2Seconds = 600;         // 정차 2 상태 기준 시간 (600초 = 10분)
unsigned long g_M010_CFG_ParkSeconds = 600;          // 주차 상태 기준 시간 (600초 = 10분)

// 시리얼 출력 주기 (ms)
unsigned long g_M010_CFG_SerialPrintIntervalMs = 5000; // 시리얼 출력 간격 (5초)

// ====================================================================================================
// 새로운 회전 감지 관련 설정 변수 정의
// ====================================================================================================
float g_M010_CFG_TurnNoneThresholdDps = 5.0;     // 이 각속도 이하이면 회전이 없다고 간주 (deg/s)
float g_M010_CFG_TurnSlightThresholdDps = 15.0;  // 약간 회전 감지 임계값 (deg/s)
float g_M010_CFG_TurnModerateThresholdDps = 30.0; // 중간 회전 감지 임계값 (deg/s)
float g_M010_CFG_TurnSharpThresholdDps = 50.0;   // 급격한 회전 감지 임계값 (deg/s)

float g_M010_CFG_TurnMinSpeedKmh = 1.0;          // 회전 감지를 위한 최소 속도 (km/h) - 정지 시 오인 방지
float g_M010_CFG_TurnHighSpeedThresholdKmh = 30.0; // 고속 회전 감지 시 임계값 조정을 위한 기준 속도 (km/h)

unsigned long g_M010_CFG_TurnStableDurationMs = 100; // 회전 상태가 안정적으로 유지되어야 하는 시간 (ms)

// ====================================================================================================
// 자동차 움직임 상태 열거형 (State Machine State) 정의
// ====================================================================================================
typedef enum {
    E_M010_STATE_UNKNOWN,       // 초기/알 수 없는 상태
    E_M010_STATE_STOPPED_INIT,  // 기본 정차 상태 (다른 정차 세부 상태 진입점)
    E_M010_STATE_SIGNAL_WAIT1,  // 신호대기 1 (정차 시간 60초 미만)
    E_M010_STATE_SIGNAL_WAIT2,  // 신호대기 2 (정차 시간 120초 미만)
    E_M010_STATE_STOPPED1,      // 정차 1 (정차 시간 5분 미만)
    E_M010_STATE_STOPPED2,      // 정차 2 (정차 시간 10분 미만)
    E_M010_STATE_PARKED,        // 주차 (정차 시간 10분 이상)
    E_M010_STATE_FORWARD,       // 전진 중
    E_M010_STATE_REVERSE,       // 후진 중
} T_M010_CarMovementState;

// ====================================================================================================
// 새로운 자동차 회전 상태 열거형 정의
// ====================================================================================================
typedef enum {
    E_M010_TURN_NONE,           // 회전 없음 또는 미미한 회전
    E_M010_TURN_SLIGHT_LEFT,    // 약간 좌회전
    E_M010_TURN_MODERATE_LEFT,  // 중간 좌회전
    E_M010_TURN_SHARP_LEFT,     // 급격한 좌회전
    E_M010_TURN_SLIGHT_RIGHT,   // 약간 우회전
    E_M010_TURN_MODERATE_RIGHT, // 중간 우회전
    E_M010_TURN_SHARP_RIGHT     // 급격한 우회전
} T_M010_CarTurnState;

// ====================================================================================================
// 자동차 상태 정보 구조체 정의
// ====================================================================================================
typedef struct {
    T_M010_CarMovementState movementState; // 현재 움직임 상태
    T_M010_CarTurnState turnState;         // 현재 회전 상태 (새로 추가)

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
// LittleFS config 저장용 구조체 정의
// ====================================================================================================
struct T_M010_Config {
    int mpuInterruptPin;
    float accelAlpha;
    float gravityMps2;
    float speedForwardThresholdKmh;
    float speedReverseThresholdKmh;
    float speedStopThresholdKmh;
    float accelStopThresholdMps2;
    float gyroStopThresholdDps;
    unsigned long stopStableDurationMs;
    unsigned long moveStableDurationMs;
    unsigned long normalMoveDurationMs;
    float accelDecelThresholdMps2;
    float accelBumpThresholdMps2;
    float bumpMinSpeedKmh;
    unsigned long bumpCooldownMs;
    unsigned long decelHoldDurationMs;
    unsigned long bumpHoldDurationMs;
    unsigned long stopGracePeriodMs;
    unsigned long signalWait1Seconds;
    unsigned long signalWait2Seconds;
    unsigned long stop1Seconds;
    unsigned long stop2Seconds;
    unsigned long parkSeconds;
    unsigned long serialPrintIntervalMs;
    float turnNoneThresholdDps;
    float turnSlightThresholdDps;
    float turnModerateThresholdDps;
    float turnSharpThresholdDps;
    float turnMinSpeedKmh;
    float turnHighSpeedThresholdKmh;
    unsigned long turnStableDurationMs;
};

// ====================================================================================================
// 전역 변수 (g_M010_으로 시작) 정의
// ====================================================================================================
T_M010_CarStatus g_M010_CarStatus; // 자동차 상태 구조체 인스턴스

// MPU6050 DMP 관련 전역 변수
bool     g_M010_dmp_isReady = false; // DMP 초기화 완료 여부 플래그
uint8_t  g_M010_mpu_interruptStatus; // MPU6050 인터럽트 상태 레지스터 값
uint8_t  g_M010_dmp_devStatus;       // 장치 상태 (0=성공, >0=오류 코드)
uint16_t g_M010_dmp_packetSize;     // DMP 패킷의 크기 (바이트)
uint16_t g_M010_dmp_fifoCount;      // FIFO에 저장된 바이트 수
uint8_t  g_M010_dmp_fifoBuffer[64];  // FIFO 버퍼 (최대 64바이트)

// 쿼터니언 및 오일러 각 관련 변수
Quaternion  g_M010_Quaternion;            // MPU6050에서 계산된 쿼터니언 데이터
VectorFloat g_M010_gravity;               // 중력 벡터 (쿼터니언에서 파생)
float       g_M010_ypr[3];            // Yaw, Pitch, Roll 각도 (라디안)
float       g_M010_yawAngleVelocity_degps;      // Yaw 각속도 (도/초)

// 가속도 데이터 (필터링) 변수
float g_M010_filteredAx; // 필터링된 X축 가속도 (m/s^2)
float g_M010_filteredAy; // 필터링된 Y축 가속도 (m/s^2)
float g_M010_filteredAz; // 필터링된 Z축 가속도 (m/s^2)

// 시간 관련 전역 변수
unsigned long g_M010_lastSampleTime_ms = 0;      // 마지막 MPU6050 데이터 샘플링 시간 (ms)
unsigned long g_M010_lastSerialPrintTime_ms = 0; // 마지막 시리얼 출력 시간 (ms)
unsigned long g_M010_lastBumpDetectionTime_ms = 0; // 마지막 방지턱 감지 시간 (ms)
unsigned long g_M010_lastDecelDetectionTime_ms = 0; // 마지막 급감속 감지 시간 (ms)

unsigned long g_M010_stateTransitionStartTime_ms = 0; // 메인 상태 전환 조건이 만족되기 시작한 시간 (ms)

bool     g_M010_mpu_isDataReady = false;

// 회전 상태 안정화를 위한 전역 변수 (static으로 선언하여 파일 내에서만 유효)
static T_M010_CarTurnState s_potentialTurnState = E_M010_TURN_NONE; // 잠재적 회전 상태
static unsigned long s_turnStateStartTime_ms = 0; // 잠재적 회전 상태가 시작된 시간 (ms)

// MPU6050 인터럽트 발생 여부 플래그 및 인터럽트 서비스 루틴 (ISR)
volatile bool g_M010_mpu_isInterrupt = false; // MPU6050 인터럽트 발생 여부
void M010_dmpDataReady() {
    g_M010_mpu_isInterrupt = true; // 인터럽트 발생 시 플래그 설정
}

// ====================================================================================================
// 함수 선언 (프로토타입)
// ====================================================================================================
void M010_defineCarState(unsigned long p_currentTime_ms);     // 차량 움직임 상태 정의 함수
void M010_defineCarTurnState(unsigned long p_currentTime_ms); // 차량 회전 상태 정의 함수 (새로 추가)

// 설정 관련 함수
void M010_loadConfig(); // config.json 로드 함수
void M010_saveConfig(); // config.json 저장 함수
void M010_setDefaultConfig(); // 기본 설정 값 적용 함수
void M010_printConfig(); // 현재 설정 값 시리얼 출력
void M010_handleSerialInput(); // 시리얼 입력 처리 함수

// ====================================================================================================
// 함수 정의 (M010_으로 시작)
// ====================================================================================================

/**
 * @brief MPU6050 센서 및 DMP (Digital Motion Processor)를 초기화합니다.
 */
void M010_MPU6050_init() {
    Wire.begin(); // I2C 통신 시작
    Wire.setClock(400000); // I2C 통신 속도를 400kHz로 설정

    dbgP1_print_F(F("MPU6050 연결 테스트: "));
    dbgP1_println_F(g_M010_Mpu.testConnection() ? F("성공") : F("실패")); // MPU6050 연결 테스트 결과 출력
    dbgP1_println_F(F("DMP 로딩 중..."));

    // DMP 초기화 시도
    g_M010_dmp_devStatus = g_M010_Mpu.dmpInitialize();

    if (g_M010_dmp_devStatus == 0) { // DMP 초기화 성공 시
        dbgP1_println_F(F("DMP 활성화 중..."));
        dbgP1_print_F(F("MPU6050 인터럽트 핀 (GPIO "));
        dbgP1_print(g_M010_CFG_MpuInterruptPin); // 변경된 전역 설정 변수 사용
        dbgP1_println_F(F(") 설정 중..."));
        
        // MPU6050 인터럽트 핀 설정 및 ISR 연결
        pinMode(g_M010_CFG_MpuInterruptPin, INPUT); // 변경된 전역 설정 변수 사용
        attachInterrupt(digitalPinToInterrupt(g_M010_CFG_MpuInterruptPin), M010_dmpDataReady, RISING); // 변경된 전역 설정 변수 사용
        g_M010_mpu_interruptStatus = g_M010_Mpu.getIntStatus(); // 현재 인터럽트 상태 가져오기

        g_M010_dmp_packetSize = 42; // DMP에서 출력하는 FIFO 패킷의 크기 (MotionApps612 기준)

        g_M010_dmp_isReady = true; // DMP 초기화 완료 플래그 설정
        dbgP1_println_F(F("DMP 초기화 완료!"));
    } else { // DMP 초기화 실패 시
        dbgP1_printf_F(F("DMP 초기화 실패 (오류 코드: %d)\n"), g_M010_dmp_devStatus);
        // DMP 초기화 실패 시에는 강제로 무한 대기하지 않고, 계속 진행하여 시리얼 입력을 받을 수 있도록 변경.
        // 대신, MPU6050 관련 기능은 정상 동작하지 않음을 사용자에게 알려야 합니다.
        // while (true); 
    }
}

/**
 * @brief MPU6050 데이터를 읽고 자동차 상태를 업데이트합니다.
 * 주기적으로 호출되어 센서 데이터를 처리하고 차량의 상태를 갱신합니다.
 */
void M010_updateCarStatus() {
    if (!g_M010_dmp_isReady) return; // DMP가 준비되지 않았으면 함수 종료

    // 인터럽트가 발생하지 않았거나 FIFO에 충분한 데이터가 없으면 종료
    if (!g_M010_mpu_isInterrupt && g_M010_dmp_fifoCount < g_M010_dmp_packetSize) {
        return; 
    }
    
    g_M010_mpu_isInterrupt = false; // 인터럽트 플래그 초기화

    // DMP FIFO에서 최신 패킷을 읽어옴
    if (g_M010_Mpu.dmpGetCurrentFIFOPacket(g_M010_dmp_fifoBuffer)) { 
        unsigned long v_currentTime_ms = millis(); // 현재 시간 가져오기
        // 샘플링 시간 간격 계산 (이전 샘플링 시간과 현재 시간의 차이)
        float v_deltaTime_s = (v_currentTime_ms - g_M010_lastSampleTime_ms) / 1000.0f;
        g_M010_lastSampleTime_ms = v_currentTime_ms; // 마지막 샘플링 시간 업데이트

        // 쿼터니언, Yaw/Pitch/Roll 및 중력 벡터 계산
        g_M010_Mpu.dmpGetQuaternion(&g_M010_Quaternion, g_M010_dmp_fifoBuffer);
        g_M010_Mpu.dmpGetGravity(&g_M010_gravity, &g_M010_Quaternion); 
        g_M010_Mpu.dmpGetYawPitchRoll(g_M010_ypr, &g_M010_Quaternion, &g_M010_gravity);

        // Yaw, Pitch 각도를 도로 변환하여 저장
        g_M010_CarStatus.yawAngle_deg = g_M010_ypr[0] * 180 / M_PI;
        g_M010_CarStatus.pitchAngle_deg = g_M010_ypr[1] * 180 / M_PI;

        // 선형 가속도 (중력분 제거) 계산 및 필터링
        VectorInt16 aa; // Raw 가속도 (MPU6050 내부 데이터 형식)
        VectorInt16 linAccel; // 중력분이 제거된 선형 가속도 결과

        g_M010_Mpu.dmpGetAccel(&aa, g_M010_dmp_fifoBuffer); // Raw 가속도 얻기
        g_M010_Mpu.dmpGetLinearAccel(&linAccel, &aa, &g_M010_gravity); // 선형 가속도 계산

        // 계산된 선형 가속도를 m/s^2 단위로 변환
        float v_currentAx_ms2 = (float)linAccel.x * g_M010_CFG_GravityMps2; // 변경된 전역 설정 변수 사용
        float v_currentAy_ms2 = (float)linAccel.y * g_M010_CFG_GravityMps2; // 변경된 전역 설정 변수 사용
        float v_currentAz_ms2 = (float)linAccel.z * g_M010_CFG_GravityMps2; // 변경된 전역 설정 변수 사용

        // 상보 필터를 사용하여 가속도 데이터 평활화
        g_M010_filteredAx = g_M010_CFG_AccelAlpha * g_M010_filteredAx + (1 - g_M010_CFG_AccelAlpha) * v_currentAx_ms2; // 변경된 전역 설정 변수 사용
        g_M010_filteredAy = g_M010_CFG_AccelAlpha * g_M010_filteredAy + (1 - g_M010_CFG_AccelAlpha) * v_currentAy_ms2; // 변경된 전역 설정 변수 사용
        g_M010_filteredAz = g_M010_CFG_AccelAlpha * g_M010_filteredAz + (1 - g_M010_CFG_AccelAlpha) * v_currentAz_ms2; // 변경된 전역 설정 변수 사용

        // 필터링된 가속도 값을 자동차 상태 구조체에 저장
        g_M010_CarStatus.accelX_ms2 = g_M010_filteredAx;
        g_M010_CarStatus.accelY_ms2 = g_M010_filteredAy;
        g_M010_CarStatus.accelZ_ms2 = g_M010_filteredAz;

        // Yaw 각속도 (Z축 자이로 데이터) 계산
        VectorInt16 gyr; // 자이로 데이터
        g_M010_Mpu.dmpGetGyro(&gyr, g_M010_dmp_fifoBuffer); // Raw 자이로 데이터 얻기

        // 자이로 스케일 팩터(131.0f)를 이용하여 deg/s 단위로 변환
        g_M010_yawAngleVelocity_degps = (float)gyr.z / 131.0f; 
        g_M010_CarStatus.yawAngleVelocity_degps = g_M010_yawAngleVelocity_degps;

        // 속도 추정 (Y축 가속도 적분, 오차 누적 가능성에 유의)
        float v_speedChange_mps = g_M010_CarStatus.accelY_ms2 * v_deltaTime_s;
        g_M010_CarStatus.speed_kmh += (v_speedChange_mps * 3.6); // m/s를 km/h로 변환하여 누적

        // 정지 시 속도 드리프트 보정 강화 로직
        // 가속도 및 각속도 변화가 모두 임계값 이하로 충분히 오래 유지될 때만 속도를 0으로 설정
        if (fabs(g_M010_CarStatus.accelY_ms2) < g_M010_CFG_AccelStopThresholdMps2 && // 변경된 전역 설정 변수 사용
            fabs(g_M010_CarStatus.yawAngleVelocity_degps) < g_M010_CFG_GyroStopThresholdDps) { // 변경된 전역 설정 변수 사용
            
            if (g_M010_CarStatus.stopStableStartTime_ms == 0) { // 정지 안정화 시작 시간 기록
                g_M010_CarStatus.stopStableStartTime_ms = v_currentTime_ms;
            } else if ((v_currentTime_ms - g_M010_CarStatus.stopStableStartTime_ms) >= g_M010_CFG_StopStableDurationMs) { // 변경된 전역 설정 변수 사용
                g_M010_CarStatus.speed_kmh = 0.0; // 충분히 안정적인 정지 상태로 판단되면 속도 0으로 보정
            }
        } else {
            g_M010_CarStatus.stopStableStartTime_ms = 0; // 움직임 감지 시 안정화 시작 시간 리셋
        }

		g_M010_mpu_isDataReady = true;
    }
}

/**
 * @brief MPU6050 데이터 기반 자동차 움직임 상태를 정의합니다. (주요 상태 머신 로직)
 * 히스테리시스와 시간 지연을 통해 안정적인 상태 전환을 수행합니다.
 * @param p_currentTime_ms 현재 시간 (millis() 값)
 */
void M010_defineCarState(unsigned long p_currentTime_ms) {
    float v_speed = g_M010_CarStatus.speed_kmh;
    float v_accelY = g_M010_CarStatus.accelY_ms2;
    float v_accelZ = g_M010_CarStatus.accelZ_ms2;

    // 다음 상태를 예측 (기본적으로 현재 상태를 유지)
    T_M010_CarMovementState v_nextState = g_M010_CarStatus.movementState;

    // =============================================================================================
    // 1. 과속 방지턱 감지 (일시적 플래그, 메인 상태와 독립적으로 작동)
    // =============================================================================================
    if (fabs(v_accelZ) > g_M010_CFG_AccelBumpThresholdMps2 && // 변경된 전역 설정 변수 사용
        fabs(v_speed) > g_M010_CFG_BumpMinSpeedKmh &&         // 변경된 전역 설정 변수 사용
        (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) > g_M010_CFG_BumpCooldownMs) { // 변경된 전역 설정 변수 사용
        g_M010_CarStatus.isSpeedBumpDetected = true;
        g_M010_lastBumpDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isSpeedBumpDetected &&
               (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) >= g_M010_CFG_BumpHoldDurationMs) { // 변경된 전역 설정 변수 사용
        g_M010_CarStatus.isSpeedBumpDetected = false; // 유지 시간 후 플래그 리셋
    }

    // =============================================================================================
    // 2. 급감속 감지 (일시적 플래그, 메인 상태와 독립적으로 작동)
    // =============================================================================================
    if (v_accelY < g_M010_CFG_AccelDecelThresholdMps2) { // 변경된 전역 설정 변수 사용
        g_M010_CarStatus.isEmergencyBraking = true;
        g_M010_lastDecelDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isEmergencyBraking &&
               (p_currentTime_ms - g_M010_lastDecelDetectionTime_ms) >= g_M010_CFG_DecelHoldDurationMs) { // 변경된 전역 설정 변수 사용
        g_M010_CarStatus.isEmergencyBraking = false; // 유지 시간 후 플래그 리셋
    }

    // =============================================================================================
    // 3. 메인 움직임 상태 머신 로직
    // =============================================================================================
    switch (g_M010_CarStatus.movementState) {
        // 현재가 '정지' 관련 상태일 때, '움직임' 상태로의 전환 조건 확인
        case E_M010_STATE_UNKNOWN:
        case E_M010_STATE_STOPPED_INIT:
        case E_M010_STATE_SIGNAL_WAIT1:
        case E_M010_STATE_SIGNAL_WAIT2:
        case E_M010_STATE_STOPPED1:
        case E_M010_STATE_STOPPED2:
        case E_M010_STATE_PARKED:
            if (v_speed > g_M010_CFG_SpeedForwardThresholdKmh) { // 변경된 전역 설정 변수 사용
                if (g_M010_stateTransitionStartTime_ms == 0) { // 조건 만족 시작 시간 기록
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_CFG_MoveStableDurationMs) { // 변경된 전역 설정 변수 사용
                    // 전진 조건이 충분히 오래 지속되면 상태 전환
                    v_nextState = E_M010_STATE_FORWARD;
                    g_M010_CarStatus.stopStartTime_ms = 0;        // 정차 시간 리셋
                    g_M010_CarStatus.lastMovementTime_ms = p_currentTime_ms; // 마지막 움직임 시간 기록
                    g_M010_stateTransitionStartTime_ms = 0;      // 전환 완료 후 리셋
                }
            } else if (v_speed < -g_M010_CFG_SpeedReverseThresholdKmh) { // 변경된 전역 설정 변수 사용
                if (g_M010_stateTransitionStartTime_ms == 0) {
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_CFG_MoveStableDurationMs) { // 변경된 전역 설정 변수 사용
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

                if (v_stopSeconds >= g_M010_CFG_ParkSeconds) { // 변경된 전역 설정 변수 사용
                    v_nextState = E_M010_STATE_PARKED;
                } else if (v_stopSeconds >= g_M010_CFG_Stop2Seconds) { // 변경된 전역 설정 변수 사용
                    v_nextState = E_M010_STATE_STOPPED2;
                } else if (v_stopSeconds >= g_M010_CFG_Stop1Seconds) { // 변경된 전역 설정 변수 사용
                    v_nextState = E_M010_STATE_STOPPED1;
                } else if (v_stopSeconds >= g_M010_CFG_SignalWait2Seconds) { // 변경된 전역 설정 변수 사용
                    v_nextState = E_M010_STATE_SIGNAL_WAIT2;
                } else if (v_stopSeconds >= g_M010_CFG_SignalWait1Seconds) { // 변경된 전역 설정 변수 사용
                    v_nextState = E_M010_STATE_SIGNAL_WAIT1;
                } 
                // E_M010_STATE_STOPPED_INIT은 이보다 낮은 시간 기준이므로, 기본 정차 상태가 됨.
            }
            break;

        // 현재가 '움직임' 상태일 때, '정지' 상태로의 전환 조건 확인
        case E_M010_STATE_FORWARD:
        case E_M010_STATE_REVERSE:
            if (fabs(v_speed) < g_M010_CFG_SpeedStopThresholdKmh) { // 변경된 전역 설정 변수 사용
                if (g_M010_stateTransitionStartTime_ms == 0) { // 조건 만족 시작 시간 기록
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_CFG_StopStableDurationMs) { // 변경된 전역 설정 변수 사용
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
    
    // 최종 상태 업데이트 (상태가 실제로 변경될 때만 디버그 출력)
    if (v_nextState != g_M010_CarStatus.movementState) {
        dbgP1_printf_F(F("State transition: %d -> %d\n"), g_M010_CarStatus.movementState, v_nextState);
        g_M010_CarStatus.movementState = v_nextState;
    }
}

/**
 * @brief Yaw 각속도와 차량 이동 속도를 기반으로 자동차의 회전 상태를 정의합니다.
 * 속도에 따라 회전 감지 임계값을 동적으로 조정하여 정확도를 높입니다.
 * @param p_currentTime_ms 현재 시간 (millis() 값)
 */
void M010_defineCarTurnState(unsigned long p_currentTime_ms) {
    float v_speed = fabs(g_M010_CarStatus.speed_kmh); // 속도는 항상 양수 절댓값으로 처리
    float v_yawRate = g_M010_CarStatus.yawAngleVelocity_degps; // 현재 Yaw 각속도
    
    // 현재 프레임에서 감지된 회전 상태를 저장할 임시 변수
    T_M010_CarTurnState v_currentDetectedTurnState = E_M010_TURN_NONE;

    // 속도에 따른 Yaw 각속도 임계값 동적 조정 (전역 설정 변수 사용)
    float v_turnNoneThreshold = g_M010_CFG_TurnNoneThresholdDps;
    float v_turnSlightThreshold = g_M010_CFG_TurnSlightThresholdDps;
    float v_turnModerateThreshold = g_M010_CFG_TurnModerateThresholdDps;
    float v_turnSharpThreshold = g_M010_CFG_TurnSharpThresholdDps;

    // 고속 주행 시 임계값을 약간 낮춰 작은 각속도 변화에도 민감하게 반응
    if (v_speed > g_M010_CFG_TurnHighSpeedThresholdKmh) { // 변경된 전역 설정 변수 사용
        v_turnNoneThreshold *= 0.8; 
        v_turnSlightThreshold *= 0.8;
        v_turnModerateThreshold *= 0.8;
        v_turnSharpThreshold *= 0.8;
    } 
    // 저속 주행 시(g_M010_CFG_TurnMinSpeedKmh 에 가까울수록) 임계값을 높여 불필요한 감지 방지
    // (예: 정지 상태에서 핸들만 돌리는 경우 등)
    else if (v_speed < g_M010_CFG_TurnMinSpeedKmh + 3.0) { // 변경된 전역 설정 변수 사용
        v_turnNoneThreshold *= 1.2; 
        v_turnSlightThreshold *= 1.2;
        v_turnModerateThreshold *= 1.2;
        v_turnSharpThreshold *= 1.2;
    }

    // 최소 속도 이상일 때만 회전 감지 로직 활성화
    if (v_speed >= g_M010_CFG_TurnMinSpeedKmh) { // 변경된 전역 설정 변수 사용
        if (v_yawRate > v_turnNoneThreshold) { // 우회전 감지 (양의 각속도)
            if (v_yawRate >= v_turnSharpThreshold) {
                v_currentDetectedTurnState = E_M010_TURN_SHARP_RIGHT;
            } else if (v_yawRate >= v_turnModerateThreshold) {
                v_currentDetectedTurnState = E_M010_TURN_MODERATE_RIGHT;
            } else if (v_yawRate >= v_turnSlightThreshold) {
                v_currentDetectedTurnState = E_M010_TURN_SLIGHT_RIGHT;
            } else { // 임계값 이하지만 0이 아니므로 미미한 회전
                v_currentDetectedTurnState = E_M010_TURN_NONE; 
            }
        } else if (v_yawRate < -v_turnNoneThreshold) { // 좌회전 감지 (음의 각속도)
            if (v_yawRate <= -v_turnSharpThreshold) {
                v_currentDetectedTurnState = E_M010_TURN_SHARP_LEFT;
            } else if (v_yawRate <= -v_turnModerateThreshold) {
                v_currentDetectedTurnState = E_M010_TURN_MODERATE_LEFT;
            } else if (v_yawRate <= -v_turnSlightThreshold) {
                v_currentDetectedTurnState = E_M010_TURN_SLIGHT_LEFT;
            } else { // 임계값 이하지만 0이 아니므로 미미한 회전
                v_currentDetectedTurnState = E_M010_TURN_NONE; 
            }
        } else { // 각속도가 '회전 없음' 임계값 범위 내에 있을 경우
            v_currentDetectedTurnState = E_M010_TURN_NONE;
        }
    } else { // 속도가 최소 회전 감지 속도보다 낮으면 무조건 회전 없음으로 간주
        v_currentDetectedTurnState = E_M010_TURN_NONE;
    }

    // 회전 상태 안정화 로직 (히스테리시스 적용)
    if (v_currentDetectedTurnState != s_potentialTurnState) {
        // 감지된 상태가 이전 잠재적 상태와 다르면, 새로운 잠재적 상태로 설정하고 시간 기록 리셋
        s_potentialTurnState = v_currentDetectedTurnState;
        s_turnStateStartTime_ms = p_currentTime_ms;
    } else {
        // 감지된 상태가 충분히 오래 지속되었는지 확인
        if ((p_currentTime_ms - s_turnStateStartTime_ms) >= g_M010_CFG_TurnStableDurationMs) { // 변경된 전역 설정 변수 사용
            // 잠재적 상태가 현재 확정된 상태와 다르고 충분히 오래 지속되었다면, 상태 전환
            if (s_potentialTurnState != g_M010_CarStatus.turnState) {
                 dbgP1_printf_F(F("Turn State transition: %d -> %d\n"), g_M010_CarStatus.turnState, s_potentialTurnState);
            }
            g_M010_CarStatus.turnState = s_potentialTurnState; // 회전 상태 확정
        }
    }
}


/**
 * @brief 자동차 상태 정보를 시리얼 모니터로 출력합니다.
 * 디버깅 및 현재 상태 확인을 위해 사용됩니다.
 */
void M010_printCarStatus() {
    dbgP1_println_F(F("\n---- 자동차 현재 상태 ----"));
    dbgP1_print_F(F("상태: "));
    switch (g_M010_CarStatus.movementState) {
        case E_M010_STATE_UNKNOWN: dbgP1_println_F(F("알 수 없음")); break;
        case E_M010_STATE_STOPPED_INIT: dbgP1_println_F(F("정차 중 (초기)")); break;
        case E_M010_STATE_SIGNAL_WAIT1: dbgP1_print_F(F("신호대기 1 (")); dbgP1_print(g_M010_CFG_SignalWait1Seconds); dbgP1_print_F(F("초 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_SIGNAL_WAIT2: dbgP1_print_F(F("신호대기 2 (")); dbgP1_print(g_M010_CFG_SignalWait2Seconds); dbgP1_print_F(F("초 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_STOPPED1: dbgP1_print_F(F("정차 1 (")); dbgP1_print(g_M010_CFG_Stop1Seconds/60); dbgP1_print_F(F("분 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_STOPPED2: dbgP1_print_F(F("정차 2 (")); dbgP1_print(g_M010_CFG_Stop2Seconds/60); dbgP1_print_F(F("분 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_PARKED: dbgP1_print_F(F("주차 중 (")); dbgP1_print(g_M010_CFG_ParkSeconds/60); dbgP1_print_F(F("분 이상), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_FORWARD: dbgP1_println_F(F("전진 중")); break;
        case E_M010_STATE_REVERSE: dbgP1_println_F(F("후진 중")); break;
    }
    // 새로 추가된 회전 상태 출력
    dbgP1_print_F(F("회전 상태: "));
    switch (g_M010_CarStatus.turnState) {
        case E_M010_TURN_NONE: dbgP1_println_F(F("직진 또는 정지")); break;
        case E_M010_TURN_SLIGHT_LEFT: dbgP1_println_F(F("약간 좌회전")); break;
        case E_M010_TURN_MODERATE_LEFT: dbgP1_println_F(F("중간 좌회전")); break;
        case E_M010_TURN_SHARP_LEFT: dbgP1_println_F(F("급격한 좌회전")); break;
        case E_M010_TURN_SLIGHT_RIGHT: dbgP1_println_F(F("약간 우회전")); break;
        case E_M010_TURN_MODERATE_RIGHT: dbgP1_println_F(F("중간 우회전")); break;
        case E_M010_TURN_SHARP_RIGHT: dbgP1_println_F(F("급격한 우회전")); break;
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

// ====================================================================================================
// 설정 관련 함수 구현
// ====================================================================================================

/**
 * @brief 기본 설정 값을 전역 설정 변수에 적용합니다.
 * config.json 파일 로드에 실패하거나 파일이 없을 경우 호출됩니다.
 */
void M010_setDefaultConfig() {
    dbgP1_println_F(F("기본 설정 값을 적용합니다."));

    g_M010_CFG_MpuInterruptPin = 4;
    g_M010_CFG_AccelAlpha = 0.8;
    g_M010_CFG_GravityMps2 = 9.80665;
    g_M010_CFG_SpeedForwardThresholdKmh = 0.8;
    g_M010_CFG_SpeedReverseThresholdKmh = 0.8;
    g_M010_CFG_SpeedStopThresholdKmh = 0.2;
    g_M010_CFG_AccelStopThresholdMps2 = 0.2;
    g_M010_CFG_GyroStopThresholdDps = 0.5;
    g_M010_CFG_StopStableDurationMs = 200;
    g_M010_CFG_MoveStableDurationMs = 150;
    g_M010_CFG_NormalMoveDurationMs = 100;
    g_M010_CFG_AccelDecelThresholdMps2 = -3.0;
    g_M010_CFG_AccelBumpThresholdMps2 = 5.0;
    g_M010_CFG_BumpMinSpeedKmh = 5.0;
    g_M010_CFG_BumpCooldownMs = 1000;
    g_M010_CFG_DecelHoldDurationMs = 10000;
    g_M010_CFG_BumpHoldDurationMs = 10000;
    g_M010_CFG_StopGracePeriodMs = 2000;
    g_M010_CFG_SignalWait1Seconds = 60;
    g_M010_CFG_SignalWait2Seconds = 120;
    g_M010_CFG_Stop1Seconds = 300;
    g_M010_CFG_Stop2Seconds = 600;
    g_M010_CFG_ParkSeconds = 600;
    g_M010_CFG_SerialPrintIntervalMs = 5000;
    g_M010_CFG_TurnNoneThresholdDps = 5.0;
    g_M010_CFG_TurnSlightThresholdDps = 15.0;
    g_M010_CFG_TurnModerateThresholdDps = 30.0;
    g_M010_CFG_TurnSharpThresholdDps = 50.0;
    g_M010_CFG_TurnMinSpeedKmh = 1.0;
    g_M010_CFG_TurnHighSpeedThresholdKmh = 30.0;
    g_M010_CFG_TurnStableDurationMs = 100;
}

/**
 * @brief LittleFS에서 config.json 파일을 읽어와 설정 값을 로드합니다.
 * 파일이 없거나 유효하지 않으면 기본값을 적용합니다.
 */
void M010_loadConfig() {
    if (!LittleFS.begin(true)) { // LittleFS 초기화 시도, true는 포맷 시도
        dbgP1_println_F(F("LittleFS 마운트 실패! 기본 설정 적용."));
        M010_setDefaultConfig();
        return;
    }

    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        dbgP1_println_F(F("config.json 파일을 찾을 수 없습니다. 기본 설정 적용."));
        M010_setDefaultConfig();
        // 기본값으로 초기화된 후 바로 저장하여 다음 부팅 시 사용 가능하도록 함
        M010_saveConfig(); 
        return;
    }

    size_t size = configFile.size();
    if (size > 2048) { // 파일 크기 제한 (필요에 따라 조정)
        dbgP1_println_F(F("config.json 파일이 너무 큽니다! 기본 설정 적용."));
        configFile.close();
        M010_setDefaultConfig();
        return;
    }

    // JSON 데이터 로드
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    configFile.close();

    StaticJsonDocument<2048> doc; // JSON 문서 버퍼 크기 (필요에 따라 조정)
    DeserializationError error = deserializeJson(doc, buf.get());

    if (error) {
        dbgP1_printf_F(F("config.json 파싱 실패! (%s) 기본 설정 적용.\n"), error.c_str());
        M010_setDefaultConfig();
        return;
    }

    dbgP1_println_F(F("config.json 로드 성공!"));
    // JSON에서 값 읽어와 전역 설정 변수에 적용
    g_M010_CFG_MpuInterruptPin = doc["mpuInterruptPin"] | g_M010_CFG_MpuInterruptPin;
    g_M010_CFG_AccelAlpha = doc["accelAlpha"] | g_M010_CFG_AccelAlpha;
    g_M010_CFG_GravityMps2 = doc["gravityMps2"] | g_M010_CFG_GravityMps2;
    g_M010_CFG_SpeedForwardThresholdKmh = doc["speedForwardThresholdKmh"] | g_M010_CFG_SpeedForwardThresholdKmh;
    g_M010_CFG_SpeedReverseThresholdKmh = doc["speedReverseThresholdKmh"] | g_M010_CFG_SpeedReverseThresholdKmh;
    g_M010_CFG_SpeedStopThresholdKmh = doc["speedStopThresholdKmh"] | g_M010_CFG_SpeedStopThresholdKmh;
    g_M010_CFG_AccelStopThresholdMps2 = doc["accelStopThresholdMps2"] | g_M010_CFG_AccelStopThresholdMps2;
    g_M010_CFG_GyroStopThresholdDps = doc["gyroStopThresholdDps"] | g_M010_CFG_GyroStopThresholdDps;
    g_M010_CFG_StopStableDurationMs = doc["stopStableDurationMs"] | g_M010_CFG_StopStableDurationMs;
    g_M010_CFG_MoveStableDurationMs = doc["moveStableDurationMs"] | g_M010_CFG_MoveStableDurationMs;
    g_M010_CFG_NormalMoveDurationMs = doc["normalMoveDurationMs"] | g_M010_CFG_NormalMoveDurationMs;
    g_M010_CFG_AccelDecelThresholdMps2 = doc["accelDecelThresholdMps2"] | g_M010_CFG_AccelDecelThresholdMps2;
    g_M010_CFG_AccelBumpThresholdMps2 = doc["accelBumpThresholdMps2"] | g_M010_CFG_AccelBumpThresholdMps2;
    g_M010_CFG_BumpMinSpeedKmh = doc["bumpMinSpeedKmh"] | g_M010_CFG_BumpMinSpeedKmh;
    g_M010_CFG_BumpCooldownMs = doc["bumpCooldownMs"] | g_M010_CFG_BumpCooldownMs;
    g_M010_CFG_DecelHoldDurationMs = doc["decelHoldDurationMs"] | g_M010_CFG_DecelHoldDurationMs;
    g_M010_CFG_BumpHoldDurationMs = doc["bumpHoldDurationMs"] | g_M010_CFG_BumpHoldDurationMs;
    g_M010_CFG_StopGracePeriodMs = doc["stopGracePeriodMs"] | g_M010_CFG_StopGracePeriodMs;
    g_M010_CFG_SignalWait1Seconds = doc["signalWait1Seconds"] | g_M010_CFG_SignalWait1Seconds;
    g_M010_CFG_SignalWait2Seconds = doc["signalWait2Seconds"] | g_M010_CFG_SignalWait2Seconds;
    g_M010_CFG_Stop1Seconds = doc["stop1Seconds"] | g_M010_CFG_Stop1Seconds;
    g_M010_CFG_Stop2Seconds = doc["stop2Seconds"] | g_M010_CFG_Stop2Seconds;
    g_M010_CFG_ParkSeconds = doc["parkSeconds"] | g_M010_CFG_ParkSeconds;
    g_M010_CFG_SerialPrintIntervalMs = doc["serialPrintIntervalMs"] | g_M010_CFG_SerialPrintIntervalMs;
    g_M010_CFG_TurnNoneThresholdDps = doc["turnNoneThresholdDps"] | g_M010_CFG_TurnNoneThresholdDps;
    g_M010_CFG_TurnSlightThresholdDps = doc["turnSlightThresholdDps"] | g_M010_CFG_TurnSlightThresholdDps;
    g_M010_CFG_TurnModerateThresholdDps = doc["turnModerateThresholdDps"] | g_M010_CFG_TurnModerateThresholdDps;
    g_M010_CFG_TurnSharpThresholdDps = doc["turnSharpThresholdDps"] | g_M010_CFG_TurnSharpThresholdDps;
    g_M010_CFG_TurnMinSpeedKmh = doc["turnMinSpeedKmh"] | g_M010_CFG_TurnMinSpeedKmh;
    g_M010_CFG_TurnHighSpeedThresholdKmh = doc["turnHighSpeedThresholdKmh"] | g_M010_CFG_TurnHighSpeedThresholdKmh;
    g_M010_CFG_TurnStableDurationMs = doc["turnStableDurationMs"] | g_M010_CFG_TurnStableDurationMs;

    // 로드된 설정 값을 시리얼로 출력하여 확인
    M010_printConfig();
}

/**
 * @brief 현재 전역 설정 변수 값을 config.json 파일에 저장합니다.
 */
void M010_saveConfig() {
    StaticJsonDocument<2048> doc; // JSON 문서 버퍼 크기 (필요에 따라 조정)

    // 전역 설정 변수 값을 JSON에 저장
    doc["mpuInterruptPin"] = g_M010_CFG_MpuInterruptPin;
    doc["accelAlpha"] = g_M010_CFG_AccelAlpha;
    doc["gravityMps2"] = g_M010_CFG_GravityMps2;
    doc["speedForwardThresholdKmh"] = g_M010_CFG_SpeedForwardThresholdKmh;
    doc["speedReverseThresholdKmh"] = g_M010_CFG_SpeedReverseThresholdKmh;
    doc["speedStopThresholdKmh"] = g_M010_CFG_SpeedStopThresholdKmh;
    doc["accelStopThresholdMps2"] = g_M010_CFG_AccelStopThresholdMps2;
    doc["gyroStopThresholdDps"] = g_M010_CFG_GyroStopThresholdDps;
    doc["stopStableDurationMs"] = g_M010_CFG_StopStableDurationMs;
    doc["moveStableDurationMs"] = g_M010_CFG_MoveStableDurationMs;
    doc["normalMoveDurationMs"] = g_M010_CFG_NormalMoveDurationMs;
    doc["accelDecelThresholdMps2"] = g_M010_CFG_AccelDecelThresholdMps2;
    doc["accelBumpThresholdMps2"] = g_M010_CFG_AccelBumpThresholdMps2;
    doc["bumpMinSpeedKmh"] = g_M010_CFG_BumpMinSpeedKmh;
    doc["bumpCooldownMs"] = g_M010_CFG_BumpCooldownMs;
    doc["decelHoldDurationMs"] = g_M010_CFG_DecelHoldDurationMs;
    doc["bumpHoldDurationMs"] = g_M010_CFG_BumpHoldDurationMs;
    doc["stopGracePeriodMs"] = g_M010_CFG_StopGracePeriodMs;
    doc["signalWait1Seconds"] = g_M010_CFG_SignalWait1Seconds;
    doc["signalWait2Seconds"] = g_M010_CFG_SignalWait2Seconds;
    doc["stop1Seconds"] = g_M010_CFG_Stop1Seconds;
    doc["stop2Seconds"] = g_M010_CFG_Stop2Seconds;
    doc["parkSeconds"] = g_M010_CFG_ParkSeconds;
    doc["serialPrintIntervalMs"] = g_M010_CFG_SerialPrintIntervalMs;
    doc["turnNoneThresholdDps"] = g_M010_CFG_TurnNoneThresholdDps;
    doc["turnSlightThresholdDps"] = g_M010_CFG_TurnSlightThresholdDps;
    doc["turnModerateThresholdDps"] = g_M010_CFG_TurnModerateThresholdDps;
    doc["turnSharpThresholdDps"] = g_M010_CFG_TurnSharpThresholdDps;
    doc["turnMinSpeedKmh"] = g_M010_CFG_TurnMinSpeedKmh;
    doc["turnHighSpeedThresholdKmh"] = g_M010_CFG_TurnHighSpeedThresholdKmh;
    doc["turnStableDurationMs"] = g_M010_CFG_TurnStableDurationMs;

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        dbgP1_println_F(F("config.json 파일 쓰기 오류!"));
        return;
    }

    // JSON을 파일에 직렬화
    if (serializeJson(doc, configFile) == 0) {
        dbgP1_println_F(F("config.json 파일 직렬화 실패!"));
    } else {
        dbgP1_println_F(F("config.json 저장 성공!"));
    }
    configFile.close();
}

/**
 * @brief 현재 설정 값들을 시리얼 모니터로 출력합니다.
 */
void M010_printConfig() {
    dbgP1_println_F(F("\n---- 현재 설정 값 ----"));
    dbgP1_printf_F(F("mpuInterruptPin: %d\n"), g_M010_CFG_MpuInterruptPin);
    dbgP1_printf_F(F("accelAlpha: %.2f\n"), g_M010_CFG_AccelAlpha);
    dbgP1_printf_F(F("gravityMps2: %.3f\n"), g_M010_CFG_GravityMps2);
    dbgP1_printf_F(F("speedForwardThresholdKmh: %.2f\n"), g_M010_CFG_SpeedForwardThresholdKmh);
    dbgP1_printf_F(F("speedReverseThresholdKmh: %.2f\n"), g_M010_CFG_SpeedReverseThresholdKmh);
    dbgP1_printf_F(F("speedStopThresholdKmh: %.2f\n"), g_M010_CFG_SpeedStopThresholdKmh);
    dbgP1_printf_F(F("accelStopThresholdMps2: %.2f\n"), g_M010_CFG_AccelStopThresholdMps2);
    dbgP1_printf_F(F("gyroStopThresholdDps: %.2f\n"), g_M010_CFG_GyroStopThresholdDps);
    dbgP1_printf_F(F("stopStableDurationMs: %lu\n"), g_M010_CFG_StopStableDurationMs);
    dbgP1_printf_F(F("moveStableDurationMs: %lu\n"), g_M010_CFG_MoveStableDurationMs);
    dbgP1_printf_F(F("normalMoveDurationMs: %lu\n"), g_M010_CFG_NormalMoveDurationMs);
    dbgP1_printf_F(F("accelDecelThresholdMps2: %.2f\n"), g_M010_CFG_AccelDecelThresholdMps2);
    dbgP1_printf_F(F("accelBumpThresholdMps2: %.2f\n"), g_M010_CFG_AccelBumpThresholdMps2);
    dbgP1_printf_F(F("bumpMinSpeedKmh: %.2f\n"), g_M010_CFG_BumpMinSpeedKmh);
    dbgP1_printf_F(F("bumpCooldownMs: %lu\n"), g_M010_CFG_BumpCooldownMs);
    dbgP1_printf_F(F("decelHoldDurationMs: %lu\n"), g_M010_CFG_DecelHoldDurationMs);
    dbgP1_printf_F(F("bumpHoldDurationMs: %lu\n"), g_M010_CFG_BumpHoldDurationMs);
    dbgP1_printf_F(F("stopGracePeriodMs: %lu\n"), g_M010_CFG_StopGracePeriodMs);
    dbgP1_printf_F(F("signalWait1Seconds: %lu\n"), g_M010_CFG_SignalWait1Seconds);
    dbgP1_printf_F(F("signalWait2Seconds: %lu\n"), g_M010_CFG_SignalWait2Seconds);
    dbgP1_printf_F(F("stop1Seconds: %lu\n"), g_M010_CFG_Stop1Seconds);
    dbgP1_printf_F(F("stop2Seconds: %lu\n"), g_M010_CFG_Stop2Seconds);
    dbgP1_printf_F(F("parkSeconds: %lu\n"), g_M010_CFG_ParkSeconds);
    dbgP1_printf_F(F("serialPrintIntervalMs: %lu\n"), g_M010_CFG_SerialPrintIntervalMs);
    dbgP1_printf_F(F("turnNoneThresholdDps: %.2f\n"), g_M010_CFG_TurnNoneThresholdDps);
    dbgP1_printf_F(F("turnSlightThresholdDps: %.2f\n"), g_M010_CFG_TurnSlightThresholdDps);
    dbgP1_printf_F(F("turnModerateThresholdDps: %.2f\n"), g_M010_CFG_TurnModerateThresholdDps);
    dbgP1_printf_F(F("turnSharpThresholdDps: %.2f\n"), g_M010_CFG_TurnSharpThresholdDps);
    dbgP1_printf_F(F("turnMinSpeedKmh: %.2f\n"), g_M010_CFG_TurnMinSpeedKmh);
    dbgP1_printf_F(F("turnHighSpeedThresholdKmh: %.2f\n"), g_M010_CFG_TurnHighSpeedThresholdKmh);
    dbgP1_printf_F(F("turnStableDurationMs: %lu\n"), g_M010_CFG_TurnStableDurationMs);
    dbgP1_println_F(F("------------------------"));
    dbgP1_println_F(F("설정 변경: [설정명]=[값] (예: accelAlpha=0.7)"));
    dbgP1_println_F(F("모든 설정 출력: config_print"));
    dbgP1_println_F(F("설정 저장: config_save"));
    dbgP1_println_F(F("초기 설정 로드: config_default"));
}

/**
 * @brief 시리얼 입력을 처리하여 설정 값을 변경하고 저장합니다.
 * 시리얼 모니터에서 "설정명=값" 형식으로 입력받습니다.
 * "config_print" 입력 시 현재 설정을 출력하고, "config_save" 입력 시 설정을 저장합니다.
 * "config_default" 입력 시 기본 설정으로 초기화합니다.
 */
void M010_handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim(); // 공백 제거

        dbgP1_printf_F(F("시리얼 입력: %s\n"), input.c_str());

        if (input.equalsIgnoreCase("config_print")) {
            M010_printConfig();
        } else if (input.equalsIgnoreCase("config_save")) {
            M010_saveConfig();
        } else if (input.equalsIgnoreCase("config_default")) {
            M010_setDefaultConfig();
            M010_saveConfig(); // 기본값으로 설정 후 바로 저장
        } else {
            int equalsIndex = input.indexOf('=');
            if (equalsIndex > 0) {
                String paramName = input.substring(0, equalsIndex);
                String paramValueStr = input.substring(equalsIndex + 1);

                // 설정 값 업데이트 (각 설정 변수별로 조건문 추가)
                if (paramName.equalsIgnoreCase("mpuInterruptPin")) {
                    g_M010_CFG_MpuInterruptPin = paramValueStr.toInt();
                    dbgP1_printf_F(F("mpuInterruptPin이 %d로 변경되었습니다.\n"), g_M010_CFG_MpuInterruptPin);
                } else if (paramName.equalsIgnoreCase("accelAlpha")) {
                    g_M010_CFG_AccelAlpha = paramValueStr.toFloat();
                    dbgP1_printf_F(F("accelAlpha가 %.2f로 변경되었습니다.\n"), g_M010_CFG_AccelAlpha);
                } else if (paramName.equalsIgnoreCase("gravityMps2")) {
                    g_M010_CFG_GravityMps2 = paramValueStr.toFloat();
                    dbgP1_printf_F(F("gravityMps2가 %.3f로 변경되었습니다.\n"), g_M010_CFG_GravityMps2);
                } else if (paramName.equalsIgnoreCase("speedForwardThresholdKmh")) {
                    g_M010_CFG_SpeedForwardThresholdKmh = paramValueStr.toFloat();
                    dbgP1_printf_F(F("speedForwardThresholdKmh가 %.2f로 변경되었습니다.\n"), g_M010_CFG_SpeedForwardThresholdKmh);
                } else if (paramName.equalsIgnoreCase("speedReverseThresholdKmh")) {
                    g_M010_CFG_SpeedReverseThresholdKmh = paramValueStr.toFloat();
                    dbgP1_printf_F(F("speedReverseThresholdKmh가 %.2f로 변경되었습니다.\n"), g_M010_CFG_SpeedReverseThresholdKmh);
                } else if (paramName.equalsIgnoreCase("speedStopThresholdKmh")) {
                    g_M010_CFG_SpeedStopThresholdKmh = paramValueStr.toFloat();
                    dbgP1_printf_F(F("speedStopThresholdKmh가 %.2f로 변경되었습니다.\n"), g_M010_CFG_SpeedStopThresholdKmh);
                } else if (paramName.equalsIgnoreCase("accelStopThresholdMps2")) {
                    g_M010_CFG_AccelStopThresholdMps2 = paramValueStr.toFloat();
                    dbgP1_printf_F(F("accelStopThresholdMps2가 %.2f로 변경되었습니다.\n"), g_M010_CFG_AccelStopThresholdMps2);
                } else if (paramName.equalsIgnoreCase("gyroStopThresholdDps")) {
                    g_M010_CFG_GyroStopThresholdDps = paramValueStr.toFloat();
                    dbgP1_printf_F(F("gyroStopThresholdDps가 %.2f로 변경되었습니다.\n"), g_M010_CFG_GyroStopThresholdDps);
                } else if (paramName.equalsIgnoreCase("stopStableDurationMs")) {
                    g_M010_CFG_StopStableDurationMs = paramValueStr.toInt();
                    dbgP1_printf_F(F("stopStableDurationMs가 %lu로 변경되었습니다.\n"), g_M010_CFG_StopStableDurationMs);
                } else if (paramName.equalsIgnoreCase("moveStableDurationMs")) {
                    g_M010_CFG_MoveStableDurationMs = paramValueStr.toInt();
                    dbgP1_printf_F(F("moveStableDurationMs가 %lu로 변경되었습니다.\n"), g_M010_CFG_MoveStableDurationMs);
                } else if (paramName.equalsIgnoreCase("normalMoveDurationMs")) {
                    g_M010_CFG_NormalMoveDurationMs = paramValueStr.toInt();
                    dbgP1_printf_F(F("normalMoveDurationMs가 %lu로 변경되었습니다.\n"), g_M010_CFG_NormalMoveDurationMs);
                } else if (paramName.equalsIgnoreCase("accelDecelThresholdMps2")) {
                    g_M010_CFG_AccelDecelThresholdMps2 = paramValueStr.toFloat();
                    dbgP1_printf_F(F("accelDecelThresholdMps2가 %.2f로 변경되었습니다.\n"), g_M010_CFG_AccelDecelThresholdMps2);
                } else if (paramName.equalsIgnoreCase("accelBumpThresholdMps2")) {
                    g_M010_CFG_AccelBumpThresholdMps2 = paramValueStr.toFloat();
                    dbgP1_printf_F(F("accelBumpThresholdMps2가 %.2f로 변경되었습니다.\n"), g_M010_CFG_AccelBumpThresholdMps2);
                } else if (paramName.equalsIgnoreCase("bumpMinSpeedKmh")) {
                    g_M010_CFG_BumpMinSpeedKmh = paramValueStr.toFloat();
                    dbgP1_printf_F(F("bumpMinSpeedKmh가 %.2f로 변경되었습니다.\n"), g_M010_CFG_BumpMinSpeedKmh);
                } else if (paramName.equalsIgnoreCase("bumpCooldownMs")) {
                    g_M010_CFG_BumpCooldownMs = paramValueStr.toInt();
                    dbgP1_printf_F(F("bumpCooldownMs가 %lu로 변경되었습니다.\n"), g_M010_CFG_BumpCooldownMs);
                } else if (paramName.equalsIgnoreCase("decelHoldDurationMs")) {
                    g_M010_CFG_DecelHoldDurationMs = paramValueStr.toInt();
                    dbgP1_printf_F(F("decelHoldDurationMs가 %lu로 변경되었습니다.\n"), g_M010_CFG_DecelHoldDurationMs);
                } else if (paramName.equalsIgnoreCase("bumpHoldDurationMs")) {
                    g_M010_CFG_BumpHoldDurationMs = paramValueStr.toInt();
                    dbgP1_printf_F(F("bumpHoldDurationMs가 %lu로 변경되었습니다.\n"), g_M010_CFG_BumpHoldDurationMs);
                } else if (paramName.equalsIgnoreCase("stopGracePeriodMs")) {
                    g_M010_CFG_StopGracePeriodMs = paramValueStr.toInt();
                    dbgP1_printf_F(F("stopGracePeriodMs가 %lu로 변경되었습니다.\n"), g_M010_CFG_StopGracePeriodMs);
                } else if (paramName.equalsIgnoreCase("signalWait1Seconds")) {
                    g_M010_CFG_SignalWait1Seconds = paramValueStr.toInt();
                    dbgP1_printf_F(F("signalWait1Seconds가 %lu로 변경되었습니다.\n"), g_M010_CFG_SignalWait1Seconds);
                } else if (paramName.equalsIgnoreCase("signalWait2Seconds")) {
                    g_M010_CFG_SignalWait2Seconds = paramValueStr.toInt();
                    dbgP1_printf_F(F("signalWait2Seconds가 %lu로 변경되었습니다.\n"), g_M010_CFG_SignalWait2Seconds);
                } else if (paramName.equalsIgnoreCase("stop1Seconds")) {
                    g_M010_CFG_Stop1Seconds = paramValueStr.toInt();
                    dbgP1_printf_F(F("stop1Seconds가 %lu로 변경되었습니다.\n"), g_M010_CFG_Stop1Seconds);
                } else if (paramName.equalsIgnoreCase("stop2Seconds")) {
                    g_M010_CFG_Stop2Seconds = paramValueStr.toInt();
                    dbgP1_printf_F(F("stop2Seconds가 %lu로 변경되었습니다.\n"), g_M010_CFG_Stop2Seconds);
                } else if (paramName.equalsIgnoreCase("parkSeconds")) {
                    g_M010_CFG_ParkSeconds = paramValueStr.toInt();
                    dbgP1_printf_F(F("parkSeconds가 %lu로 변경되었습니다.\n"), g_M010_CFG_ParkSeconds);
                } else if (paramName.equalsIgnoreCase("serialPrintIntervalMs")) {
                    g_M010_CFG_SerialPrintIntervalMs = paramValueStr.toInt();
                    dbgP1_printf_F(F("serialPrintIntervalMs가 %lu로 변경되었습니다.\n"), g_M010_CFG_SerialPrintIntervalMs);
                } else if (paramName.equalsIgnoreCase("turnNoneThresholdDps")) {
                    g_M010_CFG_TurnNoneThresholdDps = paramValueStr.toFloat();
                    dbgP1_printf_F(F("turnNoneThresholdDps가 %.2f로 변경되었습니다.\n"), g_M010_CFG_TurnNoneThresholdDps);
                } else if (paramName.equalsIgnoreCase("turnSlightThresholdDps")) {
                    g_M010_CFG_TurnSlightThresholdDps = paramValueStr.toFloat();
                    dbgP1_printf_F(F("turnSlightThresholdDps가 %.2f로 변경되었습니다.\n"), g_M010_CFG_TurnSlightThresholdDps);
                } else if (paramName.equalsIgnoreCase("turnModerateThresholdDps")) {
                    g_M010_CFG_TurnModerateThresholdDps = paramValueStr.toFloat();
                    dbgP1_printf_F(F("turnModerateThresholdDps가 %.2f로 변경되었습니다.\n"), g_M010_CFG_TurnModerateThresholdDps);
                } else if (paramName.equalsIgnoreCase("turnSharpThresholdDps")) {
                    g_M010_CFG_TurnSharpThresholdDps = paramValueStr.toFloat();
                    dbgP1_printf_F(F("turnSharpThresholdDps가 %.2f로 변경되었습니다.\n"), g_M010_CFG_TurnSharpThresholdDps);
                } else if (paramName.equalsIgnoreCase("turnMinSpeedKmh")) {
                    g_M010_CFG_TurnMinSpeedKmh = paramValueStr.toFloat();
                    dbgP1_printf_F(F("turnMinSpeedKmh가 %.2f로 변경되었습니다.\n"), g_M010_CFG_TurnMinSpeedKmh);
                } else if (paramName.equalsIgnoreCase("turnHighSpeedThresholdKmh")) {
                    g_M010_CFG_TurnHighSpeedThresholdKmh = paramValueStr.toFloat();
                    dbgP1_printf_F(F("turnHighSpeedThresholdKmh가 %.2f로 변경되었습니다.\n"), g_M010_CFG_TurnHighSpeedThresholdKmh);
                } else if (paramName.equalsIgnoreCase("turnStableDurationMs")) {
                    g_M010_CFG_TurnStableDurationMs = paramValueStr.toInt();
                    dbgP1_printf_F(F("turnStableDurationMs가 %lu로 변경되었습니다.\n"), g_M010_CFG_TurnStableDurationMs);
                }
                 else {
                    dbgP1_printf_F(F("알 수 없는 설정명: %s\n"), paramName.c_str());
                }
                M010_saveConfig(); // 변경된 설정 저장
            } else {
                dbgP1_println_F(F("잘못된 입력 형식입니다. [설정명]=[값] 또는 'config_print', 'config_save', 'config_default'를 사용하세요."));
            }
        }
    }
}


void M010_GlobalVar_init(){
    // 자동차 상태 구조체 초기화
    g_M010_CarStatus.movementState = E_M010_STATE_UNKNOWN; // 초기 움직임 상태를 알 수 없음으로 설정
    g_M010_CarStatus.turnState = E_M010_TURN_NONE;         // 초기 회전 상태를 회전 없음으로 설정 (새로 추가)
    g_M010_CarStatus.speed_kmh = 0.0;
    g_M010_CarStatus.accelX_ms2 = 0.0;
    g_M010_CarStatus.accelY_ms2 = 0.0;
    g_M010_CarStatus.accelZ_ms2 = 0.0;
    g_M010_CarStatus.yawAngle_deg = 0.0;
    g_M010_CarStatus.yawAngleVelocity_degps = 0.0;
    g_M010_CarStatus.pitchAngle_deg = 0.0;
    g_M010_CarStatus.isSpeedBumpDetected = false;
    g_M010_CarStatus.isEmergencyBraking = false;
    g_M010_CarStatus.lastMovementTime_ms = millis(); // 초기 움직임 시간 설정
    g_M010_CarStatus.stopStartTime_ms = 0;
    g_M010_CarStatus.currentStopTime_ms = 0;
    g_M010_CarStatus.stopStableStartTime_ms = 0; // 정지 안정화 시작 시간 초기화

    // 시간 관련 전역 변수 초기화
    g_M010_lastSampleTime_ms = 0;      
    g_M010_lastSerialPrintTime_ms = 0; 
    g_M010_lastBumpDetectionTime_ms = 0; 
    g_M010_lastDecelDetectionTime_ms = 0; 
    g_M010_stateTransitionStartTime_ms = 0; 
    
    // 회전 상태 안정화를 위한 전역 변수 초기화 (새로 추가)
    s_potentialTurnState = E_M010_TURN_NONE;
    s_turnStateStartTime_ms = 0;

}

/**
 * @brief ESP32가 시작될 때 한 번 실행되는 초기 설정 함수입니다.
 * MPU6050 초기화 및 모든 전역 상태 변수를 설정합니다.
 */
void M010_MPU_init() {
    // LittleFS 및 config 로드 (MPU 초기화 전에 먼저 수행)
    M010_loadConfig();

    M010_MPU6050_init(); // MPU6050 센서 초기화 (MPUInterruptPin은 config에서 로드된 값 사용)

    M010_GlobalVar_init();
	
    dbgP1_println_F(F("Setup 완료!"));
    M010_printConfig(); // 초기 설정 값을 시리얼로 출력
    dbgP1_println_F(F("시리얼 모니터에 'config_print' 또는 '설정명=값'을 입력하여 설정을 변경할 수 있습니다."));
}

/**
 * @brief ESP32 메인 루프에서 반복적으로 실행되는 함수입니다.
 * MPU6050 데이터를 지속적으로 업데이트하고, 주기적으로 상태를 시리얼 출력합니다.
 * 시리얼 입력을 주기적으로 확인하여 설정 변경 명령을 처리합니다.
 */
void M010_MPU_run() {
    M010_updateCarStatus(); // MPU6050 데이터 읽기 및 자동차 상태 업데이트

	if(g_M010_mpu_isDataReady==true){
		unsigned long v_currentTime_ms = millis(); 
		// 자동차 움직임 상태 및 회전 상태 정의 함수 호출
        M010_defineCarState(v_currentTime_ms);
        M010_defineCarTurnState(v_currentTime_ms); // 새로 추가된 회전 상태 정의 함수 호출
		g_M010_mpu_isDataReady = false;
  
	}
    // 시리얼 입력 처리 (자주 호출하여 입력 즉시 반응)
    M010_handleSerialInput();

    // 설정된 주기(g_M010_CFG_SerialPrintIntervalMs)에 따라 자동차 상태를 시리얼 출력
    if (millis() - g_M010_lastSerialPrintTime_ms >= g_M010_CFG_SerialPrintIntervalMs) {
        M010_printCarStatus();
        g_M010_lastSerialPrintTime_ms = millis();
    }
}
