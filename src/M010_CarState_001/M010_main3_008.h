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
#include <LittleFS.h> // ESP32 파일 시스템
#include <ArduinoJson.h> // JSON 직렬화/역직렬화

// MPU6050 객체 생성
MPU6050 g_M010_Mpu;

// ====================================================================================================
// 전역 설정 구조체 (T_M010_Config) 정의 및 인스턴스 (g_M010_Config) 선언
// ====================================================================================================

// --- 차량 상태 머신 (State Machine) 개요 (이 부분은 코드 로직에 대한 설명이므로 상수로 변경되지 않음) ---
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

// MPU6050 관련 상수 (핀 번호는 하드웨어적이므로 const 유지)
const int G_M010_MPU_INTERRUPT_PIN = 4; // MPU6050 INT 핀이 ESP32 GPIO 4에 연결

// 설정값을 담을 구조체 정의
typedef struct {
    float accelAlpha;                       // 가속도 필터링을 위한 상보 필터 계수
    float gravityMps2;                      // 중력 가속도 값 (m/s^2)

    // 자동차 움직임 상태 감지 임계값 (상태 머신 전이 조건)
    float speedForwardThresholdKmh;         // 정지 상태에서 전진 상태로 전환하기 위한 최소 속도 (km/h)
    float speedReverseThresholdKmh;         // 정지 상태에서 후진 상태로 전환하기 위한 최소 속도 (km/h)
    float speedStopThresholdKmh;            // 전진/후진 상태에서 정지 상태로 전환하기 위한 최대 속도 (km/h)
    float accelStopThresholdMps2;           // 차량 정차 여부 판단을 위한 Y축 가속도 변화 임계값 (m/s^2)
    float gyroStopThresholdDps;             // 차량 정차 여부 판단을 위한 Yaw 각속도 변화 임계값 (deg/s)

    unsigned long stopStableDurationMs;     // 정지 상태가 안정적으로 지속되어야 하는 시간 (ms)
    unsigned long moveStableDurationMs;     // 움직임이 안정적으로 감지되어야 하는 시간 (ms)
    unsigned long normalMoveDurationMs;     // 특수 상태 해제 후 일반 움직임으로 복귀하는 데 필요한 시간 (ms)

    // 급감속/방지턱 감지 관련 임계값
    float accelDecelThresholdMps2;          // 급감속 감지를 위한 Y축 가속도 임계값 (음수 값, m/s^2)
    float accelBumpThresholdMps2;           // 방지턱 감지를 위한 Z축 가속도 변화 임계값 (m/s^2)
    float bumpMinSpeedKmh;                  // 방지턱 감지 시 필요한 최소 속도 (km/h)

    unsigned long bumpCooldownMs;           // 방지턱 감지 후 재감지를 방지하는 쿨다운 시간 (ms)
    unsigned long decelHoldDurationMs;      // 급감속 상태가 유지되는 시간 (ms, 10초)
    unsigned long bumpHoldDurationMs;       // 과속방지턱 감지 상태가 유지되는 시간 (ms, 10초)

    // 정차/주차 시간 기준 (초)
    unsigned long stopGracePeriodMs;        // 2초 이상 움직임 없으면 정차로 간주 (현재 사용되지 않음, 참고용)
    unsigned long signalWait1Seconds;       // 신호대기 1 상태 기준 시간 (60초)
    unsigned long signalWait2Seconds;       // 신호대기 2 상태 기준 시간 (120초)
    unsigned long stopped1Seconds;          // 정차 1 상태 기준 시간 (300초 = 5분)
    unsigned long stopped2Seconds;          // 정차 2 상태 기준 시간 (600초 = 10분)
    unsigned long parkSeconds;              // 주차 상태 기준 시간 (600초 = 10분)

    unsigned long serialPrintIntervalMs;    // 시리얼 출력 간격 (5초)

    // 새로운 회전 감지 관련 상수
    float turnNoneThresholdDps;             // 이 각속도 이하이면 회전이 없다고 간주 (deg/s)
    float turnSlightThresholdDps;           // 약간 회전 감지 임계값 (deg/s)
    float turnModerateThresholdDps;         // 중간 회전 감지 임계값 (deg/s)
    float turnSharpThresholdDps;            // 급격한 회전 감지 임계값 (deg/s)
    float turnMinSpeedKmh;                  // 회전 감지를 위한 최소 속도 (km/h) - 정지 시 오인 방지
    float turnHighSpeedThresholdKmh;        // 고속 회전 감지 시 임계값 조정을 위한 기준 속도 (km/h)
    unsigned long turnStableDurationMs;     // 회전 상태가 안정적으로 유지되어야 하는 시간 (ms)
} T_M010_Config;

// 전역 설정 변수 선언
T_M010_Config g_M010_Config;


// ====================================================================================================
// 자동차 움직임 상태 열거형 (State Machine State) 정의
// ====================================================================================================
typedef enum {
    E_M010_STATE_UNKNOWN,       // 초기/알 수 없는 상태
    E_M010_STATE_STOPPED_INIT,  // 기본 정차 상태 (다른 정차 세부 상태 진입점)
    E_M010_STATE_SIGNAL_WAIT1,  // 신호대기 1 (정차 시간 60초 미만)
    E_M010_STATE_SIGNAL_WAIT2,  // 신호대기 2 (120초 미만)
    E_M010_STATE_STOPPED1,      // 정차 1 (정차 시간 5분 미만)
    E_M010_STATE_STOPPED2,      // 정차 2 (10분 미만)
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

// 설정 관련 함수 선언
void M010_Config_initDefaults(); // 설정값 기본값 초기화
bool M010_Config_load();         // LittleFS에서 설정값 로드
bool M010_Config_save();         // LittleFS에 설정값 저장
void M010_Config_print();        // 현재 설정값 시리얼 출력
void M010_Config_handleSerialInput(); // 시리얼 입력 처리

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
        dbgP1_print(G_M010_MPU_INTERRUPT_PIN);
        dbgP1_println_F(F(") 설정 중..."));
        
        // MPU6050 인터럽트 핀 설정 및 ISR 연결
        pinMode(G_M010_MPU_INTERRUPT_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(G_M010_MPU_INTERRUPT_PIN), M010_dmpDataReady, RISING);
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
void M010_updateCarStatus(unsigned long* p_currentTime_ms) {
    if (!g_M010_dmp_isReady) return; // DMP가 준비되지 않았으면 함수 종료

    // 인터럽트가 발생하지 않았거나 FIFO에 충분한 데이터가 없으면 종료
    if (!g_M010_mpu_isInterrupt && g_M010_dmp_fifoCount < g_M010_dmp_packetSize) {
        return; 
    }
    
    g_M010_mpu_isInterrupt = false; // 인터럽트 플래그 초기화

    // DMP FIFO에서 최신 패킷을 읽어옴
    if (g_M010_Mpu.dmpGetCurrentFIFOPacket(g_M010_dmp_fifoBuffer)) { 
		p_currentTime_ms = millis(); // 현재 시간 가져오기
        // unsigned long v_currentTime_ms = millis(); // 현재 시간 가져오기
		
        // 샘플링 시간 간격 계산 (이전 샘플링 시간과 현재 시간의 차이)
		float v_deltaTime_s = (p_currentTime_ms - g_M010_lastSampleTime_ms) / 1000.0f;
        g_M010_lastSampleTime_ms = p_currentTime_ms; // 마지막 샘플링 시간 업데이트
        // float v_deltaTime_s = (v_currentTime_ms - g_M010_lastSampleTime_ms) / 1000.0f;
        // g_M010_lastSampleTime_ms = v_currentTime_ms; // 마지막 샘플링 시간 업데이트

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
        float v_currentAx_ms2 = (float)linAccel.x * g_M010_Config.gravityMps2;
        float v_currentAy_ms2 = (float)linAccel.y * g_M010_Config.gravityMps2;
        float v_currentAz_ms2 = (float)linAccel.z * g_M010_Config.gravityMps2;

        // 상보 필터를 사용하여 가속도 데이터 평활화
        g_M010_filteredAx = g_M010_Config.accelAlpha * g_M010_filteredAx + (1 - g_M010_Config.accelAlpha) * v_currentAx_ms2;
        g_M010_filteredAy = g_M010_Config.accelAlpha * g_M010_filteredAy + (1 - g_M010_Config.accelAlpha) * v_currentAy_ms2;
        g_M010_filteredAz = g_M010_Config.accelAlpha * g_M010_filteredAz + (1 - g_M010_Config.accelAlpha) * v_currentAz_ms2;

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
        if (fabs(g_M010_CarStatus.accelY_ms2) < g_M010_Config.accelStopThresholdMps2 &&
            fabs(g_M010_CarStatus.yawAngleVelocity_degps) < g_M010_Config.gyroStopThresholdDps) {
            
            if (g_M010_CarStatus.stopStableStartTime_ms == 0) { // 정지 안정화 시작 시간 기록
                g_M010_CarStatus.stopStableStartTime_ms = v_currentTime_ms;
            } else if ((v_currentTime_ms - g_M010_CarStatus.stopStableStartTime_ms) >= g_M010_Config.stopStableDurationMs) {
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
    if (fabs(v_accelZ) > g_M010_Config.accelBumpThresholdMps2 && // Z축 가속도 임계값 초과
        fabs(v_speed) > g_M010_Config.bumpMinSpeedKmh &&         // 최소 속도 이상
        (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) > g_M010_Config.bumpCooldownMs) { // 쿨다운 시간 경과
        g_M010_CarStatus.isSpeedBumpDetected = true;
        g_M010_lastBumpDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isSpeedBumpDetected &&
               (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) >= g_M010_Config.bumpHoldDurationMs) {
        g_M010_CarStatus.isSpeedBumpDetected = false; // 유지 시간 후 플래그 리셋
    }

    // =============================================================================================
    // 2. 급감속 감지 (일시적 플래그, 메인 상태와 독립적으로 작동)
    // =============================================================================================
    if (v_accelY < g_M010_Config.accelDecelThresholdMps2) { // Y축 가속도가 급감속 임계값 미만
        g_M010_CarStatus.isEmergencyBraking = true;
        g_M010_lastDecelDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isEmergencyBraking &&
               (p_currentTime_ms - g_M010_lastDecelDetectionTime_ms) >= g_M010_Config.decelHoldDurationMs) {
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
            if (v_speed > g_M010_Config.speedForwardThresholdKmh) { // 전진 시작 조건 (히스테리시스 상단)
                if (g_M010_stateTransitionStartTime_ms == 0) { // 조건 만족 시작 시간 기록
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_Config.moveStableDurationMs) {
                    // 전진 조건이 충분히 오래 지속되면 상태 전환
                    v_nextState = E_M010_STATE_FORWARD;
                    g_M010_CarStatus.stopStartTime_ms = 0;        // 정차 시간 리셋
                    g_M010_CarStatus.lastMovementTime_ms = p_currentTime_ms; // 마지막 움직임 시간 기록
                    g_M010_stateTransitionStartTime_ms = 0;      // 전환 완료 후 리셋
                }
            } else if (v_speed < -g_M010_Config.speedReverseThresholdKmh) { // 후진 시작 조건 (히스테리시스 상단)
                if (g_M010_stateTransitionStartTime_ms == 0) {
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_Config.moveStableDurationMs) {
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

                if (v_stopSeconds >= g_M010_Config.parkSeconds) {
                    v_nextState = E_M010_STATE_PARKED;
                } else if (v_stopSeconds >= g_M010_Config.stopped2Seconds) {
                    v_nextState = E_M010_STATE_STOPPED2;
                } else if (v_stopSeconds >= g_M010_Config.stopped1Seconds) {
                    v_nextState = E_M010_STATE_STOPPED1;
                } else if (v_stopSeconds >= g_M010_Config.signalWait2Seconds) {
                    v_nextState = E_M010_STATE_SIGNAL_WAIT2;
                } else if (v_stopSeconds >= g_M010_Config.signalWait1Seconds) {
                    v_nextState = E_M010_STATE_SIGNAL_WAIT1;
                } 
                // E_M010_STATE_STOPPED_INIT은 이보다 낮은 시간 기준이므로, 기본 정차 상태가 됨.
            }
            break;

        // 현재가 '움직임' 상태일 때, '정지' 상태로의 전환 조건 확인
        case E_M010_STATE_FORWARD:
        case E_M010_STATE_REVERSE:
            if (fabs(v_speed) < g_M010_Config.speedStopThresholdKmh) { // 정지 조건 (히스테리시스 하단)
                if (g_M010_stateTransitionStartTime_ms == 0) { // 조건 만족 시작 시간 기록
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_Config.stopStableDurationMs) {
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
    float v_turnNoneThreshold = g_M010_Config.turnNoneThresholdDps;
    float v_turnSlightThreshold = g_M010_Config.turnSlightThresholdDps;
    float v_turnModerateThreshold = g_M010_Config.turnModerateThresholdDps;
    float v_turnSharpThreshold = g_M010_Config.turnSharpThresholdDps;

    // 고속 주행 시 임계값을 약간 낮춰 작은 각속도 변화에도 민감하게 반응
    if (v_speed > g_M010_Config.turnHighSpeedThresholdKmh) {
        v_turnNoneThreshold *= 0.8; 
        v_turnSlightThreshold *= 0.8;
        v_turnModerateThreshold *= 0.8;
        v_turnSharpThreshold *= 0.8;
    } 
    // 저속 주행 시(G_M010_TURN_MIN_SPEED_KMH 에 가까울수록) 임계값을 높여 불필요한 감지 방지
    // (예: 정지 상태에서 핸들만 돌리는 경우 등)
    else if (v_speed < g_M010_Config.turnMinSpeedKmh + 3.0) { // 최소 속도 + 3km/h 범위
        v_turnNoneThreshold *= 1.2; 
        v_turnSlightThreshold *= 1.2;
        v_turnModerateThreshold *= 1.2;
        v_turnSharpThreshold *= 1.2;
    }

    // 최소 속도 이상일 때만 회전 감지 로직 활성화
    if (v_speed >= g_M010_Config.turnMinSpeedKmh) {
        if (v_yawRate > v_turnNoneThreshold) { // 우회전 감지 (양의 각속도)
            if (v_yawRate >= v_turnSharpThreshold) {
                v_currentDetectedTurnState = E_M010_TURN_SHARP_RIGHT;
            } else if (v_yawRate >= v_turnModerateThreshold) {
                v_currentDetectedTurnState = E_M010_TURN_MODERATE_RIGHT;
            } else if (v_yawRate >= v_turnSlightThreshold) {
                v_currentDetectedTurnState = E_M010_TURN_SLIGHT_RIGHT;
            } else { // 임계값 이지만 0이 아니므로 미미한 회전
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
        if ((p_currentTime_ms - s_turnStateStartTime_ms) >= g_M010_Config.turnStableDurationMs) {
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
        case E_M010_STATE_SIGNAL_WAIT1: dbgP1_print_F(F("신호대기 1 (")); dbgP1_print(g_M010_Config.signalWait1Seconds); dbgP1_print_F(F("s 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_SIGNAL_WAIT2: dbgP1_print_F(F("신호대기 2 (")); dbgP1_print(g_M010_Config.signalWait2Seconds); dbgP1_print_F(F("s 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_STOPPED1: dbgP1_print_F(F("정차 1 (")); dbgP1_print(g_M010_Config.stopped1Seconds / 60); dbgP1_print_F(F("분 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_STOPPED2: dbgP1_print_F(F("정차 2 (")); dbgP1_print(g_M010_Config.stopped2Seconds / 60); dbgP1_print_F(F("분 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_PARKED: dbgP1_print_F(F("주차 중 (")); dbgP1_print(g_M010_Config.parkSeconds / 60); dbgP1_print_F(F("분 이상), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
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
 * @brief 설정값 구조체에 기본값을 초기화합니다.
 * config.json 파일이 없거나 로드에 실패할 경우 이 기본값이 사용됩니다.
 */
void M010_Config_initDefaults() {
    dbgP1_println_F(F("설정 기본값 초기화 중..."));
    g_M010_Config.accelAlpha = 0.8;
    g_M010_Config.gravityMps2 = 9.80665;

    g_M010_Config.speedForwardThresholdKmh = 0.8;
    g_M010_Config.speedReverseThresholdKmh = 0.8;
    g_M010_Config.speedStopThresholdKmh = 0.2;
    g_M010_Config.accelStopThresholdMps2 = 0.2;
    g_M010_Config.gyroStopThresholdDps = 0.5;

    g_M010_Config.stopStableDurationMs = 200;
    g_M010_Config.moveStableDurationMs = 150;
    g_M010_Config.normalMoveDurationMs = 100;

    g_M010_Config.accelDecelThresholdMps2 = -3.0;
    g_M010_Config.accelBumpThresholdMps2 = 5.0;
    g_M010_Config.bumpMinSpeedKmh = 5.0;

    g_M010_Config.bumpCooldownMs = 1000;
    g_M010_Config.decelHoldDurationMs = 10000;
    g_M010_Config.bumpHoldDurationMs = 10000;

    g_M010_Config.stopGracePeriodMs = 2000;
    g_M010_Config.signalWait1Seconds = 60;
    g_M010_Config.signalWait2Seconds = 120;
    g_M010_Config.stopped1Seconds = 300;
    g_M010_Config.stopped2Seconds = 600;
    g_M010_Config.parkSeconds = 600;

    g_M010_Config.serialPrintIntervalMs = 5000;

    g_M010_Config.turnNoneThresholdDps = 5.0;
    g_M010_Config.turnSlightThresholdDps = 15.0;
    g_M010_Config.turnModerateThresholdDps = 30.0;
    g_M010_Config.turnSharpThresholdDps = 50.0;
    g_M010_Config.turnMinSpeedKmh = 1.0;
    g_M010_Config.turnHighSpeedThresholdKmh = 30.0;
    g_M010_Config.turnStableDurationMs = 100;
}

/**
 * @brief LittleFS에서 config.json 파일을 로드하여 g_M010_Config 구조체에 저장합니다.
 * @return 로드 성공 시 true, 실패 시 false
 */
bool M010_Config_load() {
    if (!LittleFS.begin()) {
        dbgP1_println_F(F("LittleFS 마운트 실패"));
        return false;
    }

    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        dbgP1_println_F(F("config.json 파일 없음, 기본값 로드."));
        LittleFS.end();
        return false; // 파일이 없으면 로드 실패로 간주
    }

    JsonDocument doc; 

    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    LittleFS.end();

    if (error) {
        dbgP1_printf_F(F("config.json 파싱 실패: %s\n"), error.c_str());
        return false;
    }

    dbgP1_println_F(F("config.json 로드 성공."));

    // JSON 문서에서 값을 읽어 g_M010_Config 구조체에 저장
    g_M010_Config.accelAlpha = doc["accelAlpha"] | g_M010_Config.accelAlpha; // 기본값과 OR 연산하여 없으면 기본값 유지
    g_M010_Config.gravityMps2 = doc["gravityMps2"] | g_M010_Config.gravityMps2;

    g_M010_Config.speedForwardThresholdKmh = doc["speedForwardThresholdKmh"] | g_M010_Config.speedForwardThresholdKmh;
    g_M010_Config.speedReverseThresholdKmh = doc["speedReverseThresholdKmh"] | g_M010_Config.speedReverseThresholdKmh;
    g_M010_Config.speedStopThresholdKmh = doc["speedStopThresholdKmh"] | g_M010_Config.speedStopThresholdKmh;
    g_M010_Config.accelStopThresholdMps2 = doc["accelStopThresholdMps2"] | g_M010_Config.accelStopThresholdMps2;
    g_M010_Config.gyroStopThresholdDps = doc["gyroStopThresholdDps"] | g_M010_Config.gyroStopThresholdDps;

    g_M010_Config.stopStableDurationMs = doc["stopStableDurationMs"] | g_M010_Config.stopStableDurationMs;
    g_M010_Config.moveStableDurationMs = doc["moveStableDurationMs"] | g_M010_Config.moveStableDurationMs;
    g_M010_Config.normalMoveDurationMs = doc["normalMoveDurationMs"] | g_M010_Config.normalMoveDurationMs;

    g_M010_Config.accelDecelThresholdMps2 = doc["accelDecelThresholdMps2"] | g_M010_Config.accelDecelThresholdMps2;
    g_M010_Config.accelBumpThresholdMps2 = doc["accelBumpThresholdMps2"] | g_M010_Config.accelBumpThresholdMps2;
    g_M010_Config.bumpMinSpeedKmh = doc["bumpMinSpeedKmh"] | g_M010_Config.bumpMinSpeedKmh;

    g_M010_Config.bumpCooldownMs = doc["bumpCooldownMs"] | g_M010_Config.bumpCooldownMs;
    g_M010_Config.decelHoldDurationMs = doc["decelHoldDurationMs"] | g_M010_Config.decelHoldDurationMs;
    g_M010_Config.bumpHoldDurationMs = doc["bumpHoldDurationMs"] | g_M010_Config.bumpHoldDurationMs;

    g_M010_Config.stopGracePeriodMs = doc["stopGracePeriodMs"] | g_M010_Config.stopGracePeriodMs;
    g_M010_Config.signalWait1Seconds = doc["signalWait1Seconds"] | g_M010_Config.signalWait1Seconds;
    g_M010_Config.signalWait2Seconds = doc["signalWait2Seconds"] | g_M010_Config.signalWait2Seconds;
    g_M010_Config.stopped1Seconds = doc["stopped1Seconds"] | g_M010_Config.stopped1Seconds;
    g_M010_Config.stopped2Seconds = doc["stopped2Seconds"] | g_M010_Config.stopped2Seconds;
    g_M010_Config.parkSeconds = doc["parkSeconds"] | g_M010_Config.parkSeconds;

    g_M010_Config.serialPrintIntervalMs = doc["serialPrintIntervalMs"] | g_M010_Config.serialPrintIntervalMs;

    g_M010_Config.turnNoneThresholdDps = doc["turnNoneThresholdDps"] | g_M010_Config.turnNoneThresholdDps;
    g_M010_Config.turnSlightThresholdDps = doc["turnSlightThresholdDps"] | g_M010_Config.turnSlightThresholdDps;
    g_M010_Config.turnModerateThresholdDps = doc["turnModerateThresholdDps"] | g_M010_Config.turnModerateThresholdDps;
    g_M010_Config.turnSharpThresholdDps = doc["turnSharpThresholdDps"] | g_M010_Config.turnSharpThresholdDps;
    g_M010_Config.turnMinSpeedKmh = doc["turnMinSpeedKmh"] | g_M010_Config.turnMinSpeedKmh;
    g_M010_Config.turnHighSpeedThresholdKmh = doc["turnHighSpeedThresholdKmh"] | g_M010_Config.turnHighSpeedThresholdKmh;
    g_M010_Config.turnStableDurationMs = doc["turnStableDurationMs"] | g_M010_Config.turnStableDurationMs;

    return true;
}

/**
 * @brief g_M010_Config 구조체의 현재 설정값을 LittleFS의 config.json 파일에 저장합니다.
 * @return 저장 성공 시 true, 실패 시 false
 */
bool M010_Config_save() {
    if (!LittleFS.begin()) {
        dbgP1_println_F(F("LittleFS 마운트 실패"));
        return false;
    }

    File configFile = LittleFS.open("/config.json", "w"); // 쓰기 모드로 열기 (기존 파일 덮어쓰기)
    if (!configFile) {
        dbgP1_println_F(F("config.json 파일 생성 실패"));
        LittleFS.end();
        return false;
    }

    JsonDocument doc;

    // g_M010_Config 구조체에서 JSON 문서로 값을 복사
    doc["accelAlpha"] = g_M010_Config.accelAlpha;
    doc["gravityMps2"] = g_M010_Config.gravityMps2;

    doc["speedForwardThresholdKmh"] = g_M010_Config.speedForwardThresholdKmh;
    doc["speedReverseThresholdKmh"] = g_M010_Config.speedReverseThresholdKmh;
    doc["speedStopThresholdKmh"] = g_M010_Config.speedStopThresholdKmh;
    doc["accelStopThresholdMps2"] = g_M010_Config.accelStopThresholdMps2;
    doc["gyroStopThresholdDps"] = g_M010_Config.gyroStopThresholdDps;

    doc["stopStableDurationMs"] = g_M010_Config.stopStableDurationMs;
    doc["moveStableDurationMs"] = g_M010_Config.moveStableDurationMs;
    doc["normalMoveDurationMs"] = g_M010_Config.normalMoveDurationMs;

    doc["accelDecelThresholdMps2"] = g_M010_Config.accelDecelThresholdMps2;
    doc["accelBumpThresholdMps2"] = g_M010_Config.accelBumpThresholdMps2;
    doc["bumpMinSpeedKmh"] = g_M010_Config.bumpMinSpeedKmh;

    doc["bumpCooldownMs"] = g_M010_Config.bumpCooldownMs;
    doc["decelHoldDurationMs"] = g_M010_Config.decelHoldDurationMs;
    doc["bumpHoldDurationMs"] = g_M010_Config.bumpHoldDurationMs;

    doc["stopGracePeriodMs"] = g_M010_Config.stopGracePeriodMs;
    doc["signalWait1Seconds"] = g_M010_Config.signalWait1Seconds;
    doc["signalWait2Seconds"] = g_M010_Config.signalWait2Seconds;
    doc["stopped1Seconds"] = g_M010_Config.stopped1Seconds;
    doc["stopped2Seconds"] = g_M010_Config.stopped2Seconds;
    doc["parkSeconds"] = g_M010_Config.parkSeconds;

    doc["serialPrintIntervalMs"] = g_M010_Config.serialPrintIntervalMs;

    doc["turnNoneThresholdDps"] = g_M010_Config.turnNoneThresholdDps;
    doc["turnSlightThresholdDps"] = g_M010_Config.turnSlightThresholdDps;
    doc["turnModerateThresholdDps"] = g_M010_Config.turnModerateThresholdDps;
    doc["turnSharpThresholdDps"] = g_M010_Config.turnSharpThresholdDps;
    doc["turnMinSpeedKmh"] = g_M010_Config.turnMinSpeedKmh;
    doc["turnHighSpeedThresholdKmh"] = g_M010_Config.turnHighSpeedThresholdKmh;
    doc["turnStableDurationMs"] = g_M010_Config.turnStableDurationMs;

    if (serializeJson(doc, configFile) == 0) {
        dbgP1_println_F(F("config.json 직렬화 실패"));
        configFile.close();
        LittleFS.end();
        return false;
    }

    configFile.close();
    LittleFS.end();
    dbgP1_println_F(F("config.json 저장 성공."));
    return true;
}

/**
 * @brief 현재 설정값을 시리얼 모니터로 출력합니다.
 */
void M010_Config_print() {
    dbgP1_println_F(F("\n---- 현재 설정값 ----"));
    dbgP1_print_F(F("accelAlpha: ")); dbgP1_println(g_M010_Config.accelAlpha, 3);
    dbgP1_print_F(F("gravityMps2: ")); dbgP1_println(g_M010_Config.gravityMps2, 3);
    dbgP1_print_F(F("speedForwardThresholdKmh: ")); dbgP1_println(g_M010_Config.speedForwardThresholdKmh, 2);
    dbgP1_print_F(F("speedReverseThresholdKmh: ")); dbgP1_println(g_M010_Config.speedReverseThresholdKmh, 2);
    dbgP1_print_F(F("speedStopThresholdKmh: ")); dbgP1_println(g_M010_Config.speedStopThresholdKmh, 2);
    dbgP1_print_F(F("accelStopThresholdMps2: ")); dbgP1_println(g_M010_Config.accelStopThresholdMps2, 2);
    dbgP1_print_F(F("gyroStopThresholdDps: ")); dbgP1_println(g_M010_Config.gyroStopThresholdDps, 2);
    dbgP1_print_F(F("stopStableDurationMs: ")); dbgP1_println(g_M010_Config.stopStableDurationMs);
    dbgP1_print_F(F("moveStableDurationMs: ")); dbgP1_println(g_M010_Config.moveStableDurationMs);
    dbgP1_print_F(F("normalMoveDurationMs: ")); dbgP1_println(g_M010_Config.normalMoveDurationMs);
    dbgP1_print_F(F("accelDecelThresholdMps2: ")); dbgP1_println(g_M010_Config.accelDecelThresholdMps2, 2);
    dbgP1_print_F(F("accelBumpThresholdMps2: ")); dbgP1_println(g_M010_Config.accelBumpThresholdMps2, 2);
    dbgP1_print_F(F("bumpMinSpeedKmh: ")); dbgP1_println(g_M010_Config.bumpMinSpeedKmh, 2);
    dbgP1_print_F(F("bumpCooldownMs: ")); dbgP1_println(g_M010_Config.bumpCooldownMs);
    dbgP1_print_F(F("decelHoldDurationMs: ")); dbgP1_println(g_M010_Config.decelHoldDurationMs);
    dbgP1_print_F(F("bumpHoldDurationMs: ")); dbgP1_println(g_M010_Config.bumpHoldDurationMs);
    dbgP1_print_F(F("stopGracePeriodMs: ")); dbgP1_println(g_M010_Config.stopGracePeriodMs);
    dbgP1_print_F(F("signalWait1Seconds: ")); dbgP1_println(g_M010_Config.signalWait1Seconds);
    dbgP1_print_F(F("signalWait2Seconds: ")); dbgP1_println(g_M010_Config.signalWait2Seconds);
    dbgP1_print_F(F("stopped1Seconds: ")); dbgP1_println(g_M010_Config.stopped1Seconds);
    dbgP1_print_F(F("stopped2Seconds: ")); dbgP1_println(g_M010_Config.stopped2Seconds);
    dbgP1_print_F(F("parkSeconds: ")); dbgP1_println(g_M010_Config.parkSeconds);
    dbgP1_print_F(F("serialPrintIntervalMs: ")); dbgP1_println(g_M010_Config.serialPrintIntervalMs);
    dbgP1_print_F(F("turnNoneThresholdDps: ")); dbgP1_println(g_M010_Config.turnNoneThresholdDps, 2);
    dbgP1_print_F(F("turnSlightThresholdDps: ")); dbgP1_println(g_M010_Config.turnSlightThresholdDps, 2);
    dbgP1_print_F(F("turnModerateThresholdDps: ")); dbgP1_println(g_M010_Config.turnModerateThresholdDps, 2);
    dbgP1_print_F(F("turnSharpThresholdDps: ")); dbgP1_println(g_M010_Config.turnSharpThresholdDps, 2);
    dbgP1_print_F(F("turnMinSpeedKmh: ")); dbgP1_println(g_M010_Config.turnMinSpeedKmh, 2);
    dbgP1_print_F(F("turnHighSpeedThresholdKmh: ")); dbgP1_println(g_M010_Config.turnHighSpeedThresholdKmh, 2);
    dbgP1_print_F(F("turnStableDurationMs: ")); dbgP1_println(g_M010_Config.turnStableDurationMs);
    dbgP1_println_F(F("--------------------------"));
    dbgP1_println_F(F("설정 변경: set <항목이름> <값> (예: set accelAlpha 0.9)"));
    dbgP1_println_F(F("설정 저장: saveconfig"));
    dbgP1_println_F(F("설정 불러오기: loadconfig"));
    dbgP1_println_F(F("설정 초기화: resetconfig"));
    dbgP1_println_F(F("설정 보기: printconfig"));
}

/**
 * @brief 시리얼 포트로부터 명령어를 읽어 설정값을 변경하거나 저장/로드합니다.
 * 명령어 형식: "set <항목이름> <값>", "saveconfig", "loadconfig", "printconfig", "resetconfig"
 */
void M010_Config_handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim(); // 공백 제거
        dbgP1_printf_F(F("시리얼 입력: %s\n"), input.c_str());

        if (input.startsWith("set ")) {
            int firstSpace = input.indexOf(' ');
            int secondSpace = input.indexOf(' ', firstSpace + 1);
            if (firstSpace != -1 && secondSpace != -1) {
                String paramName = input.substring(firstSpace + 1, secondSpace);
                String valueStr = input.substring(secondSpace + 1);
                
                // 문자열을 적절한 타입으로 변환하여 설정값 업데이트
                if (paramName.equals("accelAlpha")) g_M010_Config.accelAlpha = valueStr.toFloat();
                else if (paramName.equals("gravityMps2")) g_M010_Config.gravityMps2 = valueStr.toFloat();
                else if (paramName.equals("speedForwardThresholdKmh")) g_M010_Config.speedForwardThresholdKmh = valueStr.toFloat();
                else if (paramName.equals("speedReverseThresholdKmh")) g_M010_Config.speedReverseThresholdKmh = valueStr.toFloat();
                else if (paramName.equals("speedStopThresholdKmh")) g_M010_Config.speedStopThresholdKmh = valueStr.toFloat();
                else if (paramName.equals("accelStopThresholdMps2")) g_M010_Config.accelStopThresholdMps2 = valueStr.toFloat();
                else if (paramName.equals("gyroStopThresholdDps")) g_M010_Config.gyroStopThresholdDps = valueStr.toFloat();
                else if (paramName.equals("stopStableDurationMs")) g_M010_Config.stopStableDurationMs = valueStr.toInt();
                else if (paramName.equals("moveStableDurationMs")) g_M010_Config.moveStableDurationMs = valueStr.toInt();
                else if (paramName.equals("normalMoveDurationMs")) g_M010_Config.normalMoveDurationMs = valueStr.toInt();
                else if (paramName.equals("accelDecelThresholdMps2")) g_M010_Config.accelDecelThresholdMps2 = valueStr.toFloat();
                else if (paramName.equals("accelBumpThresholdMps2")) g_M010_Config.accelBumpThresholdMps2 = valueStr.toFloat();
                else if (paramName.equals("bumpMinSpeedKmh")) g_M010_Config.bumpMinSpeedKmh = valueStr.toFloat();
                else if (paramName.equals("bumpCooldownMs")) g_M010_Config.bumpCooldownMs = valueStr.toInt();
                else if (paramName.equals("decelHoldDurationMs")) g_M010_Config.decelHoldDurationMs = valueStr.toInt();
                else if (paramName.equals("bumpHoldDurationMs")) g_M010_Config.bumpHoldDurationMs = valueStr.toInt();
                else if (paramName.equals("stopGracePeriodMs")) g_M010_Config.stopGracePeriodMs = valueStr.toInt();
                else if (paramName.equals("signalWait1Seconds")) g_M010_Config.signalWait1Seconds = valueStr.toInt();
                else if (paramName.equals("signalWait2Seconds")) g_M010_Config.signalWait2Seconds = valueStr.toInt();
                else if (paramName.equals("stopped1Seconds")) g_M010_Config.stopped1Seconds = valueStr.toInt();
                else if (paramName.equals("stopped2Seconds")) g_M010_Config.stopped2Seconds = valueStr.toInt();
                else if (paramName.equals("parkSeconds")) g_M010_Config.parkSeconds = valueStr.toInt();
                else if (paramName.equals("serialPrintIntervalMs")) g_M010_Config.serialPrintIntervalMs = valueStr.toInt();
                else if (paramName.equals("turnNoneThresholdDps")) g_M010_Config.turnNoneThresholdDps = valueStr.toFloat();
                else if (paramName.equals("turnSlightThresholdDps")) g_M010_Config.turnSlightThresholdDps = valueStr.toFloat();
                else if (paramName.equals("turnModerateThresholdDps")) g_M010_Config.turnModerateThresholdDps = valueStr.toFloat();
                else if (paramName.equals("turnSharpThresholdDps")) g_M010_Config.turnSharpThresholdDps = valueStr.toFloat();
                else if (paramName.equals("turnMinSpeedKmh")) g_M010_Config.turnMinSpeedKmh = valueStr.toFloat();
                else if (paramName.equals("turnHighSpeedThresholdKmh")) g_M010_Config.turnHighSpeedThresholdKmh = valueStr.toFloat();
                else if (paramName.equals("turnStableDurationMs")) g_M010_Config.turnStableDurationMs = valueStr.toInt();
                else {
                    dbgP1_printf_F(F("알 수 없는 설정 항목: %s\n"), paramName.c_str());
                    return;
                }
                dbgP1_printf_F(F("설정 변경됨: %s = %s\n"), paramName.c_str(), valueStr.c_str());
            } else {
                dbgP1_println_F(F("잘못된 'set' 명령어 형식. 예: set accelAlpha 0.9"));
            }
        } else if (input.equals("saveconfig")) {
            if (M010_Config_save()) {
                dbgP1_println_F(F("설정값이 LittleFS에 저장되었습니다."));
            } else {
                dbgP1_println_F(F("설정값 저장 실패!"));
            }
        } else if (input.equals("loadconfig")) {
            if (M010_Config_load()) {
                dbgP1_println_F(F("설정값이 LittleFS에서 로드되었습니다."));
                M010_Config_print(); // 로드 후 현재 설정값 출력
            } else {
                dbgP1_println_F(F("설정값 로드 실패! 기본값이 사용됩니다."));
                M010_Config_initDefaults();
                M010_Config_print();
            }
        } else if (input.equals("printconfig")) {
            M010_Config_print();
        } else if (input.equals("resetconfig")) {
            M010_Config_initDefaults();
            if (M010_Config_save()) {
                 dbgP1_println_F(F("설정값이 기본값으로 초기화되고 저장되었습니다."));
            } else {
                dbgP1_println_F(F("설정 초기화 후 저장 실패!"));
            }
            M010_Config_print();
        } else {
            dbgP1_println_F(F("알 수 없는 명령어입니다. 'help'를 입력하여 명령어 목록을 보세요."));
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
    // 설정값 로드 시도
    // config.json 파일이 없거나 로드에 실패하면 기본값으로 초기화합니다.
    if (!M010_Config_load()) {
        M010_Config_initDefaults();
        M010_Config_save(); // 기본값을 파일에 저장
    }
    M010_Config_print(); // 현재 활성화된 설정값 출력

    M010_MPU6050_init(); // MPU6050 센서 초기화

    M010_GlobalVar_init();
	
    dbgP1_println_F(F("Setup 완료!"));
}

/**
 * @brief ESP32 메인 루프에서 반복적으로 실행되는 함수입니다.
 * MPU6050 데이터를 지속적으로 업데이트하고, 주기적으로 상태를 시리얼 출력합니다.
 */
void M010_MPU_run() {
	unsigned long v_currentTime_ms;
	
    M010_updateCarStatus(&v_currentTime_ms); // MPU6050 데이터 읽기 및 자동차 상태 업데이트

	if(g_M010_mpu_isDataReady==true){
		//unsigned long v_currentTime_ms = millis(); 
		// 자동차 움직임 상태 및 회전 상태 정의 함수 호출
        M010_defineCarState(v_currentTime_ms);
        M010_defineCarTurnState(v_currentTime_ms); // 새로 추가된 회전 상태 정의 함수 호출
		g_M010_mpu_isDataReady = false;
	}
    
    // 시리얼 입력 처리 함수 호출
    M010_Config_handleSerialInput();

    // 설정된 주기(g_M010_Config.serialPrintIntervalMs)에 따라 자동차 상태를 시리얼 출력
    if (millis() - g_M010_lastSerialPrintTime_ms >= g_M010_Config.serialPrintIntervalMs) {
        M010_printCarStatus();
        g_M010_lastSerialPrintTime_ms = millis();
    }

    // 이 위치에 LED Matrix 업데이트 등 추가 작업을 구현할 수 있습니다.
    // 예: M010_updateLEDMatrix(g_M010_CarStatus.movementState, g_M010_CarStatus.turnState);
}
