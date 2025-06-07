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

#include <ArduinoJson.h> // JSON 파싱 및 생성을 위한 라이브러리 추가
#include <LittleFS.h>    // LittleFS 파일 시스템 라이브러리 추가

// MPU6050 객체 생성
MPU6050 g_M010_Mpu;

// ====================================================================================================
// 전역 변수 (G_M010_으로 시작) 정의 - 이제 설정값도 여기로 이동
// ====================================================================================================

// --- 차량 상태 머신 (State Machine) 개요 ---
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

// MPU6050 관련 변수
int g_M010_MpuInterruptPin = 4; // MPU6050 INT 핀이 ESP32 GPIO 4에 연결 (상수 -> 변수)

float g_M010_AccelAlpha = 0.8;   // 가속도 필터링을 위한 상보 필터 계수 (상수 -> 변수)
float g_M010_GravityMps2 = 9.80665; // 중력 가속도 값 (m/s^2) (상수 -> 변수)

// 자동차 움직임 상태 감지 임계값 (상태 머신 전이 조건)
float g_M010_SpeedForwardThresholdKmh = 0.8; // 정지 상태에서 전진 상태로 전환하기 위한 최소 속도 (km/h) (상수 -> 변수)
float g_M010_SpeedReverseThresholdKmh = 0.8; // 정지 상태에서 후진 상태로 전환하기 위한 최소 속도 (km/h) (상수 -> 변수)
float g_M010_SpeedStopThresholdKmh = 0.2;    // 전진/후진 상태에서 정지 상태로 전환하기 위한 최대 속도 (km/h) (상수 -> 변수)

float g_M010_AccelStopThresholdMps2 = 0.2; // 차량 정차 여부 판단을 위한 Y축 가속도 변화 임계값 (m/s^2) (상수 -> 변수)
float g_M010_GyroStopThresholdDps = 0.5;   // 차량 정차 여부 판단을 위한 Yaw 각속도 변화 임계값 (deg/s) (상수 -> 변수)

unsigned long g_M010_StopStableDurationMs = 200;  // 정지 상태가 안정적으로 지속되어야 하는 시간 (ms) (상수 -> 변수)
unsigned long g_M010_MoveStableDurationMs = 150;  // 움직임이 안정적으로 감지되어야 하는 시간 (ms) (상수 -> 변수)
unsigned long g_M010_NormalMoveDurationMs = 100; // 특수 상태 해제 후 일반 움직임으로 복귀하는 데 필요한 시간 (ms) (상수 -> 변수)

// 급감속/방지턱 감지 관련 임계값
float g_M010_AccelDecelThresholdMps2 = -3.0; // 급감속 감지를 위한 Y축 가속도 임계값 (음수 값, m/s^2) (상수 -> 변수)
float g_M010_AccelBumpThresholdMps2 = 5.0; // 방지턱 감지를 위한 Z축 가속도 변화 임계값 (m/s^2) (상수 -> 변수)
float g_M010_BumpMinSpeedKmh = 5.0; // 방지턱 감지 시 필요한 최소 속도 (km/h) (상수 -> 변수)

unsigned long g_M010_BumpCooldownMs = 1000; // 방지턱 감지 후 재감지를 방지하는 쿨다운 시간 (ms) (상수 -> 변수)
unsigned long g_M010_DecelHoldDurationMs = 10000; // 급감속 상태가 유지되는 시간 (ms, 10초) (상수 -> 변수)
unsigned long g_M010_BumpHoldDurationMs = 10000;  // 과속방지턱 감지 상태가 유지되는 시간 (ms, 10초) (상수 -> 변수)

// 정차/주차 시간 기준 (초)
unsigned long g_M010_StopGracePeriodMs = 2000; // 2초 이상 움직임 없으면 정차로 간주 (현재 사용되지 않음, 참고용) (상수 -> 변수)
unsigned long g_M010_SignalWait1Seconds = 60;   // 신호대기 1 상태 기준 시간 (60초) (상수 -> 변수)
unsigned long g_M010_SignalWait2Seconds = 120;  // 신호대기 2 상태 기준 시간 (120초) (상수 -> 변수)
unsigned long g_M010_Stop1Seconds = 300;         // 정차 1 상태 기준 시간 (300초 = 5분) (상수 -> 변수)
unsigned long g_M010_Stop2Seconds = 600;         // 정차 2 상태 기준 시간 (600초 = 10분) (상수 -> 변수)
unsigned long g_M010_ParkSeconds = 600;          // 주차 상태 기준 시간 (600초 = 10분) (상수 -> 변수)

// 시리얼 출력 주기 (ms)
unsigned long g_M010_SerialPrintIntervalMs = 5000; // 시리얼 출력 간격 (5초) (상수 -> 변수)

// ====================================================================================================
// 새로운 회전 감지 관련 변수 정의
// ====================================================================================================
float g_M010_TurnNoneThresholdDps = 5.0;     // 이 각속도 이하이면 회전이 없다고 간주 (deg/s) (상수 -> 변수)
float g_M010_TurnSlightThresholdDps = 15.0;  // 약간 회전 감지 임계값 (deg/s) (상수 -> 변수)
float g_M010_TurnModerateThresholdDps = 30.0; // 중간 회전 감지 임계값 (deg/s) (상수 -> 변수)
float g_M010_TurnSharpThresholdDps = 50.0;   // 급격한 회전 감지 임계값 (deg/s) (상수 -> 변수)

float g_M010_TurnMinSpeedKmh = 1.0;          // 회전 감지를 위한 최소 속도 (km/h) - 정지 시 오인 방지 (상수 -> 변수)
float g_M010_TurnHighSpeedThresholdKmh = 30.0; // 고속 회전 감지 시 임계값 조정을 위한 기준 속도 (km/h) (상수 -> 변수)

unsigned long g_M010_TurnStableDurationMs = 100; // 회전 상태가 안정적으로 유지되어야 하는 시간 (ms) (상수 -> 변수)

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


bool     g_M010_mpu_isDataReady = false;
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

// --- 새로운 함수 선언 ---
void M010_loadConfig();       // 설정 파일 로드 함수
void M010_saveConfig();       // 설정 파일 저장 함수
void M010_handleSerialInput(); // 시리얼 입력 처리 함수
void M010_printCurrentSettings(); // 현재 설정값 출력 함수

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
        dbgP1_print(g_M010_MpuInterruptPin); // g_M010_MpuInterruptPin 변수 사용
        dbgP1_println_F(F(") 설정 중..."));
        
        // MPU6050 인터럽트 핀 설정 및 ISR 연결
        pinMode(g_M010_MpuInterruptPin, INPUT); // g_M010_MpuInterruptPin 변수 사용
        attachInterrupt(digitalPinToInterrupt(g_M010_MpuInterruptPin), M010_dmpDataReady, RISING); // g_M010_MpuInterruptPin 변수 사용
        g_M010_mpu_interruptStatus = g_M010_Mpu.getIntStatus(); // 현재 인터럽트 상태 가져오기

        g_M010_dmp_packetSize = 42; // DMP에서 출력하는 FIFO 패킷의 크기 (MotionApps612 기준)

        g_M010_dmp_isReady = true; // DMP 초기화 완료 플래그 설정
        dbgP1_println_F(F("DMP 초기화 완료!"));
    } else { // DMP 초기화 실패 시
        dbgP1_printf_F(F("DMP 초기화 실패 (오류 코드: %d)\n"), g_M010_dmp_devStatus);
        while (true); // 무한 대기 (오류 상태 유지)
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
        float v_currentAx_ms2 = (float)linAccel.x * g_M010_GravityMps2; // g_M010_GravityMps2 변수 사용
        float v_currentAy_ms2 = (float)linAccel.y * g_M010_GravityMps2; // g_M010_GravityMps2 변수 사용
        float v_currentAz_ms2 = (float)linAccel.z * g_M010_GravityMps2; // g_M010_GravityMps2 변수 사용

        // 상보 필터를 사용하여 가속도 데이터 평활화
        g_M010_filteredAx = g_M010_AccelAlpha * g_M010_filteredAx + (1 - g_M010_AccelAlpha) * v_currentAx_ms2; // g_M010_AccelAlpha 변수 사용
        g_M010_filteredAy = g_M010_AccelAlpha * g_M010_filteredAy + (1 - g_M010_AccelAlpha) * v_currentAy_ms2; // g_M010_AccelAlpha 변수 사용
        g_M010_filteredAz = g_M010_AccelAlpha * g_M010_filteredAz + (1 - g_M010_AccelAlpha) * v_currentAz_ms2; // g_M010_AccelAlpha 변수 사용

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
        if (fabs(g_M010_CarStatus.accelY_ms2) < g_M010_AccelStopThresholdMps2 && // g_M010_AccelStopThresholdMps2 변수 사용
            fabs(g_M010_CarStatus.yawAngleVelocity_degps) < g_M010_GyroStopThresholdDps) { // g_M010_GyroStopThresholdDps 변수 사용
            
            if (g_M010_CarStatus.stopStableStartTime_ms == 0) { // 정지 안정화 시작 시간 기록
                g_M010_CarStatus.stopStableStartTime_ms = v_currentTime_ms;
            } else if ((v_currentTime_ms - g_M010_CarStatus.stopStableStartTime_ms) >= g_M010_StopStableDurationMs) { // g_M010_StopStableDurationMs 변수 사용
                g_M010_CarStatus.speed_kmh = 0.0; // 충분히 안정적인 정지 상태로 판단되면 속도 0으로 보정
            }
        } else {
            g_M010_CarStatus.stopStableStartTime_ms = 0; // 움직임 감지 시 안정화 시작 시간 리셋
        }

		g_M010_mpu_isDataReady = true;

        // 자동차 움직임 상태 및 회전 상태 정의 함수 호출
        // M010_defineCarState(v_currentTime_ms);
        // M010_defineCarTurnState(v_currentTime_ms); // 새로 추가된 회전 상태 정의 함수 호출
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
    if (fabs(v_accelZ) > g_M010_AccelBumpThresholdMps2 && // g_M010_AccelBumpThresholdMps2 변수 사용
        fabs(v_speed) > g_M010_BumpMinSpeedKmh &&         // g_M010_BumpMinSpeedKmh 변수 사용
        (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) > g_M010_BumpCooldownMs) { // g_M010_BumpCooldownMs 변수 사용
        g_M010_CarStatus.isSpeedBumpDetected = true;
        g_M010_lastBumpDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isSpeedBumpDetected &&
               (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) >= g_M010_BumpHoldDurationMs) { // g_M010_BumpHoldDurationMs 변수 사용
        g_M010_CarStatus.isSpeedBumpDetected = false; // 유지 시간 후 플래그 리셋
    }

    // =============================================================================================
    // 2. 급감속 감지 (일시적 플래그, 메인 상태와 독립적으로 작동)
    // =============================================================================================
    if (v_accelY < g_M010_AccelDecelThresholdMps2) { // g_M010_AccelDecelThresholdMps2 변수 사용
        g_M010_CarStatus.isEmergencyBraking = true;
        g_M010_lastDecelDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isEmergencyBraking &&
               (p_currentTime_ms - g_M010_lastDecelDetectionTime_ms) >= g_M010_DecelHoldDurationMs) { // g_M010_DecelHoldDurationMs 변수 사용
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
            if (v_speed > g_M010_SpeedForwardThresholdKmh) { // 전진 시작 조건 (히스테리시스 상단) // g_M010_SpeedForwardThresholdKmh 변수 사용
                if (g_M010_stateTransitionStartTime_ms == 0) { // 조건 만족 시작 시간 기록
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_MoveStableDurationMs) { // g_M010_MoveStableDurationMs 변수 사용
                    // 전진 조건이 충분히 오래 지속되면 상태 전환
                    v_nextState = E_M010_STATE_FORWARD;
                    g_M010_CarStatus.stopStartTime_ms = 0;        // 정차 시간 리셋
                    g_M010_CarStatus.lastMovementTime_ms = p_currentTime_ms; // 마지막 움직임 시간 기록
                    g_M010_stateTransitionStartTime_ms = 0;      // 전환 완료 후 리셋
                }
            } else if (v_speed < -g_M010_SpeedReverseThresholdKmh) { // 후진 시작 조건 (히스테리시스 상단) // g_M010_SpeedReverseThresholdKmh 변수 사용
                if (g_M010_stateTransitionStartTime_ms == 0) {
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_MoveStableDurationMs) { // g_M010_MoveStableDurationMs 변수 사용
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

                if (v_stopSeconds >= g_M010_ParkSeconds) { // g_M010_ParkSeconds 변수 사용
                    v_nextState = E_M010_STATE_PARKED;
                } else if (v_stopSeconds >= g_M010_Stop2Seconds) { // g_M010_Stop2Seconds 변수 사용
                    v_nextState = E_M010_STATE_STOPPED2;
                } else if (v_stopSeconds >= g_M010_Stop1Seconds) { // g_M010_Stop1Seconds 변수 사용
                    v_nextState = E_M010_STATE_STOPPED1;
                } else if (v_stopSeconds >= g_M010_SignalWait2Seconds) { // g_M010_SignalWait2Seconds 변수 사용
                    v_nextState = E_M010_STATE_SIGNAL_WAIT2;
                } else if (v_stopSeconds >= g_M010_SignalWait1Seconds) { // g_M010_SignalWait1Seconds 변수 사용
                    v_nextState = E_M010_STATE_SIGNAL_WAIT1;
                } 
                // E_M010_STATE_STOPPED_INIT은 이보다 낮은 시간 기준이므로, 기본 정차 상태가 됨.
            }
            break;

        // 현재가 '움직임' 상태일 때, '정지' 상태로의 전환 조건 확인
        case E_M010_STATE_FORWARD:
        case E_M010_STATE_REVERSE:
            if (fabs(v_speed) < g_M010_SpeedStopThresholdKmh) { // 정지 조건 (히스테리시스 하단) // g_M010_SpeedStopThresholdKmh 변수 사용
                if (g_M010_stateTransitionStartTime_ms == 0) { // 조건 만족 시작 시간 기록
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_StopStableDurationMs) { // g_M010_StopStableDurationMs 변수 사용
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

    // 속도에 따른 Yaw 각속도 임계값 동적 조정
    float v_turnNoneThreshold = g_M010_TurnNoneThresholdDps; // g_M010_TurnNoneThresholdDps 변수 사용
    float v_turnSlightThreshold = g_M010_TurnSlightThresholdDps; // g_M010_TurnSlightThresholdDps 변수 사용
    float v_turnModerateThreshold = g_M010_TurnModerateThresholdDps; // g_M010_TurnModerateThresholdDps 변수 사용
    float v_turnSharpThreshold = g_M010_TurnSharpThresholdDps; // g_M010_TurnSharpThresholdDps 변수 사용

    // 고속 주행 시 임계값을 약간 낮춰 작은 각속도 변화에도 민감하게 반응
    if (v_speed > g_M010_TurnHighSpeedThresholdKmh) { // g_M010_TurnHighSpeedThresholdKmh 변수 사용
        v_turnNoneThreshold *= 0.8; 
        v_turnSlightThreshold *= 0.8;
        v_turnModerateThreshold *= 0.8;
        v_turnSharpThreshold *= 0.8;
    } 
    // 저속 주행 시(g_M010_TurnMinSpeedKmh 에 가까울수록) 임계값을 높여 불필요한 감지 방지
    // (예: 정지 상태에서 핸들만 돌리는 경우 등)
    else if (v_speed < g_M010_TurnMinSpeedKmh + 3.0) { // g_M010_TurnMinSpeedKmh 변수 사용
        v_turnNoneThreshold *= 1.2; 
        v_turnSlightThreshold *= 1.2;
        v_turnModerateThreshold *= 1.2;
        v_turnSharpThreshold *= 1.2;
    }


    // 최소 속도 이상일 때만 회전 감지 로직 활성화
    if (v_speed >= g_M010_TurnMinSpeedKmh) { // g_M010_TurnMinSpeedKmh 변수 사용
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
            } else { // 임계값 이지만 0이 아니므로 미미한 회전
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
        if ((p_currentTime_ms - s_turnStateStartTime_ms) >= g_M010_TurnStableDurationMs) { // g_M010_TurnStableDurationMs 변수 사용
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
        case E_M010_STATE_SIGNAL_WAIT1: dbgP1_print_F(F("신호대기 1 (")); dbgP1_print(g_M010_SignalWait1Seconds); dbgP1_print_F(F("s 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break; // 변수 사용
        case E_M010_STATE_SIGNAL_WAIT2: dbgP1_print_F(F("신호대기 2 (")); dbgP1_print(g_M010_SignalWait2Seconds); dbgP1_print_F(F("s 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break; // 변수 사용
        case E_M010_STATE_STOPPED1: dbgP1_print_F(F("정차 1 (")); dbgP1_print(g_M010_Stop1Seconds/60); dbgP1_print_F(F("분 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break; // 변수 사용
        case E_M010_STATE_STOPPED2: dbgP1_print_F(F("정차 2 (")); dbgP1_print(g_M010_Stop2Seconds/60); dbgP1_print_F(F("분 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break; // 변수 사용
        case E_M010_STATE_PARKED: dbgP1_print_F(F("주차 중 (")); dbgP1_print(g_M010_ParkSeconds/60); dbgP1_print_F(F("분 이상), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break; // 변수 사용
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

/**
 * @brief 현재 전역 설정 변수들을 시리얼 모니터로 출력합니다.
 * 시리얼 입력을 통해 변경 가능한 설정들을 보여줍니다.
 */
void M010_printCurrentSettings() {
    dbgP1_println_F(F("\n---- 현재 설정값 (변경 가능) ----"));
    dbgP1_printf_F(F("1. SpeedForwardThreshold: %.2f km/h\n"), g_M010_SpeedForwardThresholdKmh);
    dbgP1_printf_F(F("2. SpeedReverseThreshold: %.2f km/h\n"), g_M010_SpeedReverseThresholdKmh);
    dbgP1_printf_F(F("3. SpeedStopThreshold: %.2f km/h\n"), g_M010_SpeedStopThresholdKmh);
    dbgP1_printf_F(F("4. AccelStopThreshold: %.2f m/s^2\n"), g_M010_AccelStopThresholdMps2);
    dbgP1_printf_F(F("5. GyroStopThreshold: %.2f deg/s\n"), g_M010_GyroStopThresholdDps);
    dbgP1_printf_F(F("6. StopStableDuration: %lu ms\n"), g_M010_StopStableDurationMs);
    dbgP1_printf_F(F("7. MoveStableDuration: %lu ms\n"), g_M010_MoveStableDurationMs);
    dbgP1_printf_F(F("8. AccelDecelThreshold: %.2f m/s^2\n"), g_M010_AccelDecelThresholdMps2);
    dbgP1_printf_F(F("9. AccelBumpThreshold: %.2f m/s^2\n"), g_M010_AccelBumpThresholdMps2);
    dbgP1_printf_F(F("10. BumpMinSpeed: %.2f km/h\n"), g_M010_BumpMinSpeedKmh);
    dbgP1_printf_F(F("11. BumpCooldown: %lu ms\n"), g_M010_BumpCooldownMs);
    dbgP1_printf_F(F("12. DecelHoldDuration: %lu ms\n"), g_M010_DecelHoldDurationMs);
    dbgP1_printf_F(F("13. BumpHoldDuration: %lu ms\n"), g_M010_BumpHoldDurationMs);
    dbgP1_printf_F(F("14. SignalWait1Seconds: %lu s\n"), g_M010_SignalWait1Seconds);
    dbgP1_printf_F(F("15. SignalWait2Seconds: %lu s\n"), g_M010_SignalWait2Seconds);
    dbgP1_printf_F(F("16. Stop1Seconds: %lu s\n"), g_M010_Stop1Seconds);
    dbgP1_printf_F(F("17. Stop2Seconds: %lu s\n"), g_M010_Stop2Seconds);
    dbgP1_printf_F(F("18. ParkSeconds: %lu s\n"), g_M010_ParkSeconds);
    dbgP1_printf_F(F("19. SerialPrintInterval: %lu ms\n"), g_M010_SerialPrintIntervalMs);
    dbgP1_printf_F(F("20. TurnNoneThreshold: %.2f deg/s\n"), g_M010_TurnNoneThresholdDps);
    dbgP1_printf_F(F("21. TurnSlightThreshold: %.2f deg/s\n"), g_M010_TurnSlightThresholdDps);
    dbgP1_printf_F(F("22. TurnModerateThreshold: %.2f deg/s\n"), g_M010_TurnModerateThresholdDps);
    dbgP1_printf_F(F("23. TurnSharpThreshold: %.2f deg/s\n"), g_M010_TurnSharpThresholdDps);
    dbgP1_printf_F(F("24. TurnMinSpeed: %.2f km/h\n"), g_M010_TurnMinSpeedKmh);
    dbgP1_printf_F(F("25. TurnHighSpeedThreshold: %.2f km/h\n"), g_M010_TurnHighSpeedThresholdKmh);
    dbgP1_printf_F(F("26. TurnStableDuration: %lu ms\n"), g_M010_TurnStableDurationMs);
    dbgP1_printf_F(F("27. MpuInterruptPin: %d\n"), g_M010_MpuInterruptPin);
    dbgP1_printf_F(F("28. AccelAlpha: %.2f\n"), g_M010_AccelAlpha);
    dbgP1_printf_F(F("29. GravityMps2: %.3f\n"), g_M010_GravityMps2);
    dbgP1_println_F(F("--------------------------------"));
    dbgP1_println_F(F("설정 변경: set <번호> <값> (예: set 1 1.0)"));
    dbgP1_println_F(F("설정 저장: save"));
    dbgP1_println_F(F("설정 목록: list"));
    dbgP1_println_F(F("--------------------------------"));
}


/**
 * @brief LittleFS에서 설정 파일을 로드합니다.
 * config.json 파일이 없거나 유효하지 않으면 기본값을 사용합니다.
 */
void M010_loadConfig() {
    dbgP1_println_F(F("Loading config from LittleFS..."));
    if (!LittleFS.begin(true)) { // true는 포맷 실패 시 포맷을 시도합니다.
        dbgP1_println_F(F("LittleFS Mount Failed or Formatted"));
        return;
    }

    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        dbgP1_println_F(F("Failed to open config file. Using default values."));
        M010_saveConfig(); // 파일이 없으면 현재 기본값으로 저장
        return;
    }

    // JSON 문서 크기 (필요에 따라 더 크게 설정할 수 있습니다)
    StaticJsonDocument<1500> doc; // 모든 설정값을 담을 충분한 크기로 조정

    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        dbgP1_printf_F(F("Failed to read file, using default configuration: %s\n"), error.c_str());
        configFile.close();
        M010_saveConfig(); // 오류 발생 시 현재 기본값으로 저장
        return;
    }

    g_M010_SpeedForwardThresholdKmh = doc["SpeedForwardThresholdKmh"] | g_M010_SpeedForwardThresholdKmh;
    g_M010_SpeedReverseThresholdKmh = doc["SpeedReverseThresholdKmh"] | g_M010_SpeedReverseThresholdKmh;
    g_M010_SpeedStopThresholdKmh = doc["SpeedStopThresholdKmh"] | g_M010_SpeedStopThresholdKmh;
    g_M010_AccelStopThresholdMps2 = doc["AccelStopThresholdMps2"] | g_M010_AccelStopThresholdMps2;
    g_M010_GyroStopThresholdDps = doc["GyroStopThresholdDps"] | g_M010_GyroStopThresholdDps;
    g_M010_StopStableDurationMs = doc["StopStableDurationMs"] | g_M010_StopStableDurationMs;
    g_M010_MoveStableDurationMs = doc["MoveStableDurationMs"] | g_M010_MoveStableDurationMs;
    g_M010_AccelDecelThresholdMps2 = doc["AccelDecelThresholdMps2"] | g_M010_AccelDecelThresholdMps2;
    g_M010_AccelBumpThresholdMps2 = doc["AccelBumpThresholdMps2"] | g_M010_AccelBumpThresholdMps2;
    g_M010_BumpMinSpeedKmh = doc["BumpMinSpeedKmh"] | g_M010_BumpMinSpeedKmh;
    g_M010_BumpCooldownMs = doc["BumpCooldownMs"] | g_M010_BumpCooldownMs;
    g_M010_DecelHoldDurationMs = doc["DecelHoldDurationMs"] | g_M010_DecelHoldDurationMs;
    g_M010_BumpHoldDurationMs = doc["BumpHoldDurationMs"] | g_M010_BumpHoldDurationMs;
    g_M010_SignalWait1Seconds = doc["SignalWait1Seconds"] | g_M010_SignalWait1Seconds;
    g_M010_SignalWait2Seconds = doc["SignalWait2Seconds"] | g_M010_SignalWait2Seconds;
    g_M010_Stop1Seconds = doc["Stop1Seconds"] | g_M010_Stop1Seconds;
    g_M010_Stop2Seconds = doc["Stop2Seconds"] | g_M010_Stop2Seconds;
    g_M010_ParkSeconds = doc["ParkSeconds"] | g_M010_ParkSeconds;
    g_M010_SerialPrintIntervalMs = doc["SerialPrintIntervalMs"] | g_M010_SerialPrintIntervalMs;
    g_M010_TurnNoneThresholdDps = doc["TurnNoneThresholdDps"] | g_M010_TurnNoneThresholdDps;
    g_M010_TurnSlightThresholdDps = doc["TurnSlightThresholdDps"] | g_M010_TurnSlightThresholdDps;
    g_M010_TurnModerateThresholdDps = doc["TurnModerateThresholdDps"] | g_M010_TurnModerateThresholdDps;
    g_M010_TurnSharpThresholdDps = doc["TurnSharpThresholdDps"] | g_M010_TurnSharpThresholdDps;
    g_M010_TurnMinSpeedKmh = doc["TurnMinSpeedKmh"] | g_M010_TurnMinSpeedKmh;
    g_M010_TurnHighSpeedThresholdKmh = doc["TurnHighSpeedThresholdKmh"] | g_M010_TurnHighSpeedThresholdKmh;
    g_M010_TurnStableDurationMs = doc["TurnStableDurationMs"] | g_M010_TurnStableDurationMs;
    g_M010_MpuInterruptPin = doc["MpuInterruptPin"] | g_M010_MpuInterruptPin;
    g_M010_AccelAlpha = doc["AccelAlpha"] | g_M010_AccelAlpha;
    g_M010_GravityMps2 = doc["GravityMps2"] | g_M010_GravityMps2;


    configFile.close();
    dbgP1_println_F(F("Config loaded successfully."));
    M010_printCurrentSettings(); // 로드된 설정 출력
}

/**
 * @brief 현재 전역 설정 변수들을 LittleFS의 설정 파일에 저장합니다.
 */
void M010_saveConfig() {
    dbgP1_println_F(F("Saving config to LittleFS..."));
    if (!LittleFS.begin(true)) {
        dbgP1_println_F(F("LittleFS Mount Failed or Formatted"));
        return;
    }

    StaticJsonDocument<1500> doc; // 모든 설정값을 담을 충분한 크기로 조정

    doc["SpeedForwardThresholdKmh"] = g_M010_SpeedForwardThresholdKmh;
    doc["SpeedReverseThresholdKmh"] = g_M010_SpeedReverseThresholdKmh;
    doc["SpeedStopThresholdKmh"] = g_M010_SpeedStopThresholdKmh;
    doc["AccelStopThresholdMps2"] = g_M010_AccelStopThresholdMps2;
    doc["GyroStopThresholdDps"] = g_M010_GyroStopThresholdDps;
    doc["StopStableDurationMs"] = g_M010_StopStableDurationMs;
    doc["MoveStableDurationMs"] = g_M010_MoveStableDurationMs;
    doc["AccelDecelThresholdMps2"] = g_M010_AccelDecelThresholdMps2;
    doc["AccelBumpThresholdMps2"] = g_M010_AccelBumpThresholdMps2;
    doc["BumpMinSpeedKmh"] = g_M010_BumpMinSpeedKmh;
    doc["BumpCooldownMs"] = g_M010_BumpCooldownMs;
    doc["DecelHoldDurationMs"] = g_M010_DecelHoldDurationMs;
    doc["BumpHoldDurationMs"] = g_M010_BumpHoldDurationMs;
    doc["SignalWait1Seconds"] = g_M010_SignalWait1Seconds;
    doc["SignalWait2Seconds"] = g_M010_SignalWait2Seconds;
    doc["Stop1Seconds"] = g_M010_Stop1Seconds;
    doc["Stop2Seconds"] = g_M010_Stop2Seconds;
    doc["ParkSeconds"] = g_M010_ParkSeconds;
    doc["SerialPrintIntervalMs"] = g_M010_SerialPrintIntervalMs;
    doc["TurnNoneThresholdDps"] = g_M010_TurnNoneThresholdDps;
    doc["TurnSlightThresholdDps"] = g_M010_TurnSlightThresholdDps;
    doc["TurnModerateThresholdDps"] = g_M010_TurnModerateThresholdDps;
    doc["TurnSharpThresholdDps"] = g_M010_TurnSharpThresholdDps;
    doc["TurnMinSpeedKmh"] = g_M010_TurnMinSpeedKmh;
    doc["TurnHighSpeedThresholdKmh"] = g_M010_TurnHighSpeedThresholdKmh;
    doc["TurnStableDurationMs"] = g_M010_TurnStableDurationMs;
    doc["MpuInterruptPin"] = g_M010_MpuInterruptPin;
    doc["AccelAlpha"] = g_M010_AccelAlpha;
    doc["GravityMps2"] = g_M010_GravityMps2;


    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        dbgP1_println_F(F("Failed to open config file for writing."));
        return;
    }

    if (serializeJsonPretty(doc, configFile) == 0) { // 예쁘게 포맷팅하여 저장
        dbgP1_println_F(F("Failed to write to file."));
    } else {
        dbgP1_println_F(F("Config saved successfully."));
    }
    configFile.close();
}


/**
 * @brief 시리얼 입력을 처리하여 설정값을 변경하고 저장합니다.
 * "set <번호> <값>" 또는 "save", "list" 명령어를 인식합니다.
 */
void M010_handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim(); // 공백 제거
        dbgP1_printf_F(F("Received command: %s\n"), input.c_str());

        if (input.startsWith("set ")) {
            int firstSpace = input.indexOf(' ');
            int secondSpace = input.indexOf(' ', firstSpace + 1);

            if (firstSpace != -1 && secondSpace != -1) {
                String numStr = input.substring(firstSpace + 1, secondSpace);
                String valStr = input.substring(secondSpace + 1);

                int settingNum = numStr.toInt();
                float settingValue = valStr.toFloat();
                unsigned long settingULongValue = valStr.toInt(); // unsigned long 값도 받을 수 있도록

                bool changed = false;
                switch (settingNum) {
                    case 1: g_M010_SpeedForwardThresholdKmh = settingValue; changed = true; break;
                    case 2: g_M010_SpeedReverseThresholdKmh = settingValue; changed = true; break;
                    case 3: g_M010_SpeedStopThresholdKmh = settingValue; changed = true; break;
                    case 4: g_M010_AccelStopThresholdMps2 = settingValue; changed = true; break;
                    case 5: g_M010_GyroStopThresholdDps = settingValue; changed = true; break;
                    case 6: g_M010_StopStableDurationMs = settingULongValue; changed = true; break;
                    case 7: g_M010_MoveStableDurationMs = settingULongValue; changed = true; break;
                    case 8: g_M010_AccelDecelThresholdMps2 = settingValue; changed = true; break;
                    case 9: g_M010_AccelBumpThresholdMps2 = settingValue; changed = true; break;
                    case 10: g_M010_BumpMinSpeedKmh = settingValue; changed = true; break;
                    case 11: g_M010_BumpCooldownMs = settingULongValue; changed = true; break;
                    case 12: g_M010_DecelHoldDurationMs = settingULongValue; changed = true; break;
                    case 13: g_M010_BumpHoldDurationMs = settingULongValue; changed = true; break;
                    case 14: g_M010_SignalWait1Seconds = settingULongValue; changed = true; break;
                    case 15: g_M010_SignalWait2Seconds = settingULongValue; changed = true; break;
                    case 16: g_M010_Stop1Seconds = settingULongValue; changed = true; break;
                    case 17: g_M010_Stop2Seconds = settingULongValue; changed = true; break;
                    case 18: g_M010_ParkSeconds = settingULongValue; changed = true; break;
                    case 19: g_M010_SerialPrintIntervalMs = settingULongValue; changed = true; break;
                    case 20: g_M010_TurnNoneThresholdDps = settingValue; changed = true; break;
                    case 21: g_M010_TurnSlightThresholdDps = settingValue; changed = true; break;
                    case 22: g_M010_TurnModerateThresholdDps = settingValue; changed = true; break;
                    case 23: g_M010_TurnSharpThresholdDps = settingValue; changed = true; break;
                    case 24: g_M010_TurnMinSpeedKmh = settingValue; changed = true; break;
                    case 25: g_M010_TurnHighSpeedThresholdKmh = settingValue; changed = true; break;
                    case 26: g_M010_TurnStableDurationMs = settingULongValue; changed = true; break;
                    // MPU 인터럽트 핀은 하드웨어 연결이므로 런타임 변경은 권장되지 않지만, 예시로 포함.
                    // 변경 시 MPU 초기화 재실행이 필요할 수 있습니다.
                    case 27: g_M010_MpuInterruptPin = (int)settingULongValue; changed = true; break; 
                    case 28: g_M010_AccelAlpha = settingValue; changed = true; break;
                    case 29: g_M010_GravityMps2 = settingValue; changed = true; break;
                    default:
                        dbgP1_println_F(F("Invalid setting number."));
                        break;
                }
                if (changed) {
                    dbgP1_println_F(F("Setting updated in memory. Use 'save' to write to file."));
                    M010_printCurrentSettings();
                }
            } else {
                dbgP1_println_F(F("Invalid 'set' command format. Use: set <number> <value>"));
            }
        } else if (input == "save") {
            M010_saveConfig();
        } else if (input == "list") {
            M010_printCurrentSettings();
        } else {
            dbgP1_println_F(F("Unknown command. Available commands: set <number> <value>, save, list"));
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
    
    // 1. 기본 전역 변수 초기화 (상수 대신 변수 사용)
    // 이전에 const로 선언된 값들의 초기값은 이 함수 밖에서 전역변수 선언 시 기본값으로 설정하거나
    // M010_loadConfig()를 호출하기 전에 여기서 수동으로 설정해줄 수 있습니다.
    // 여기서는 파일에서 로드하기 전에 기본값으로 설정하는 부분은 생략하고,
    // 파일 로드 실패 시 '|| 기본값' 연산자를 사용하여 대체하도록 합니다.
    M010_GlobalVar_init(); // 모든 상태 관련 전역 변수 초기화

    // 2. LittleFS에서 설정 로드
    M010_loadConfig(); // 저장된 설정값을 불러와 전역변수에 적용

    // 3. MPU6050 센서 초기화 (이제 g_M010_MpuInterruptPin 변수 사용)
    M010_MPU6050_init(); 
	
    dbgP1_println_F(F("Setup 완료!"));
    M010_printCurrentSettings(); // 초기화 후 현재 설정 출력
}

/**
 * @brief ESP32 메인 루프에서 반복적으로 실행되는 함수입니다.
 * MPU6050 데이터를 지속적으로 업데이트하고, 주기적으로 상태를 시리얼 출력합니다.
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
    // 시리얼 입력 처리 (새로 추가)
    M010_handleSerialInput();

    // 설정된 시리얼 출력 주기(g_M010_SerialPrintIntervalMs)에 따라 자동차 상태를 시리얼 출력
    if (millis() - g_M010_lastSerialPrintTime_ms >= g_M010_SerialPrintIntervalMs) { // g_M010_SerialPrintIntervalMs 변수 사용
        M010_printCarStatus();
        g_M010_lastSerialPrintTime_ms = millis();
    }

    // 이 위치에 LED Matrix 업데이트 등 추가 작업을 구현할 수 있습니다.
    // 예: M010_updateLEDMatrix(g_M010_CarStatus.movementState, g_M010_CarStatus.turnState);
}
