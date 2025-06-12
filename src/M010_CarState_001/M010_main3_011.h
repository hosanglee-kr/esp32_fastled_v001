#pragma once
// M010_main3_011.h
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

#define G_M010_STREAM_USE
#ifdef G_M010_STREAM_USE
     #include <StreamUtils.h> 
#endif

// ====================================================================================================
// lib_deps =
//        https://github.com/FastLED/FastLED.git#3.9.20  ;3.9.17
//        ;#fastled/FastLED @ ^3.9.17
//        jrowberg/I2Cdevlib-MPU6050@^1.0.0
//        bblanchon/ArduinoJson@^7.4.1
			
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
 * E_M010_CARMOVESTATE_UNKNOWN | (초기 상태)                                                      | - 초기화 시 -> E_M010_CARMOVESTATE_STOPPED_INIT
 * E_M010_CARMOVESTATE_STOPPED_INIT | - 초기 진입                                                  | - 속도 > G_M010_SPEED_FORWARD_THRESHOLD_KMH가 G_M010_MOVE_STABLE_DURATION_MS 이상 지속 -> E_M010_CARMOVESTATE_FORWARD
 * (and sub-states:    | - (FROM FORWARD/REVERSE): 속도 < G_M010_SPEED_STOP_THRESHOLD_KMH가 G_M010_STOP_STABLE_DURATION_MS 이상 지속 | - 속도 < -G_M010_SPEED_REVERSE_THRESHOLD_KMH가 G_M010_MOVE_STABLE_DURATION_MS 이상 지속 -> E_M010_CARMOVESTATE_REVERSE
 * SIGNAL_WAIT1/2,      |                                                                    | - (내부적으로 시간 경과에 따라): SIGNAL_WAIT1/2, STOPPED1/2, PARKED
 * STOPPED1/2, PARKED) |                                                                    |
 * E_M010_CARMOVESTATE_FORWARD | - (FROM STOPPED_INIT): 속도 > G_M010_SPEED_FORWARD_THRESHOLD_KMH가 G_M010_MOVE_STABLE_DURATION_MS 이상 지속 | - 속도 < G_M010_SPEED_STOP_THRESHOLD_KMH가 G_M010_STOP_STABLE_DURATION_MS 이상 지속 -> E_M010_CARMOVESTATE_STOPPED_INIT
 * E_M010_CARMOVESTATE_REVERSE | - (FROM STOPPED_INIT): 속도 < -G_M010_SPEED_REVERSE_THRESHOLD_KMH가 G_M010_MOVE_STABLE_DURATION_MS 이상 지속 | - 속도 > -G_M010_SPEED_STOP_THRESHOLD_KMH가 G_M010_STOP_STABLE_DURATION_MS 이상 지속 -> E_M010_CARMOVESTATE_STOPPED_INIT
 *
 * 특수 감지 플래그 (메인 상태와 독립적으로 동작하며, 해당 조건이 만족될 때만 True):
 * - isSpeedBumpDetected: Z축 가속도 임계값 초과 & 최소 속도 이상 & 쿨다운 시간 경과 시 True. G_M010_BUMP_HOLD_DURATION_MS 동안 유지.
 * - isEmergencyBraking: Y축 가속도 급감속 임계값 미만 시 True. G_M010_DECEL_HOLD_DURATION_MS 동안 유지.
 */
// ---------------------------------------------------

// MPU6050 관련 상수 (핀 번호는 하드웨어적이므로 const 유지)
const int   G_M010_MPU_INTERRUPT_PIN    = 4; // MPU6050 INT 핀이 ESP32 GPIO 4에 연결

#define     G_M010_CONFIG_JSON_FILE     "/M010_config_002.json"

// 물리 상수 및 변환 계수
const float G_M010_GRAVITY_MPS2         = 9.80665;  // 중력 가속도 (m/s^2)
const float G_M010_RAD_TO_DEG           = 180.0 / M_PI; // 라디안을 도로 변환하는 상수
const float G_M010_GYRO_SCALE_FACTOR    = 131.0;    // MPU6050 자이로 스케일 팩터 (deg/s 변환)
const float G_M010_MPS_TO_KMH_FACTOR    = 3.6;      // m/s를 km/h로 변환하는 상수
const uint16_t G_M010_DMP_PACKET_SIZE   = 42;       // MPU6050 DMP FIFO 패킷 크기 (바이트)
const uint32_t G_M010_I2C_CLOCK_FREQ    = 400000;   // I2C 통신 속도 (Hz)

// 설정값을 담을 구조체 정의
typedef struct {
    float       mvState_accelFilter_Alpha;                  // 가속도 필터링을 위한 상보 필터 계수 (0.0 ~ 1.0)

                                                            // 자동차 움직임 상태 감지 임계값 (상태 머신 전이 조건)
    float       mvState_Forward_speedKmh_Threshold_Min;     // 정지 상태에서 전진 상태로 전환하기 위한 최소 속도 (km/h)
    float       mvState_Reverse_speedKmh_Threshold_Min;     // 정지 상태에서 후진 상태로 전환하기 위한 최소 속도 (km/h)
    float       mvState_Stop_speedKmh_Threshold_Max;        // 전진/후진 상태에서 정지 상태로 전환하기 위한 최대 속도 (km/h) (히스테리시스 하단)
    float       mvState_Stop_accelMps2_Threshold_Max;       // 차량 정차 여부 판단을 위한 Y축 가속도 변화 임계값 (m/s^2) - 속도 드리프트 보정용
    float       mvState_Stop_gyroDps_Threshold_Max;         // 차량 정차 여부 판단을 위한 Yaw 각속도 변화 임계값 (deg/s) - 속도 드리프트 보정용

    u_int32_t   mvState_stop_durationMs_Stable_Min;         // 정지 상태가 안정적으로 지속되어야 하는 최소 시간 (ms)
    u_int32_t   mvState_move_durationMs_Stable_Min;         // 움직임(전진, 후진)이 안정적으로 감지되어야 하는 최소 시간 (ms)
    u_int32_t   mvState_normalMove_durationMs;              // 특수 상태 해제 후 일반 움직임으로 복귀하는 데 필요한 시간 (ms)  (현재사용되지 않음)

                                                            // 급감속/방지턱 감지 관련 임계값
    float       mvState_Decel_accelMps2_Threshold;          // 급감속 감지를 위한 Y축 가속도 임계값 (음수 값, m/s^2)
    float       mvState_Bump_accelMps2_Threshold;           // 방지턱 감지를 위한 Z축 가속도 변화 임계값 (m/s^2)
    float       mvState_Bump_SpeedKmh_Min;                  // 방지턱 감지 시 필요한 최소 속도 (km/h)

    u_int32_t   mvState_Bump_CooldownMs;                    // 방지턱 감지 후 재감지를 방지하는 쿨다운 시간 (ms)
    u_int32_t   mvState_Decel_durationMs_Hold;              // 급감속 상태가 유지되는 시간 (ms)
    u_int32_t   mvState_Bump_durationMs_Hold;               // 과속방지턱 감지 상태가 유지되는 시간 (ms)

                                                            // 정차/주차 시간 기준 (초) - E_M010_CARMOVESTATE_STOPPED_INIT 상태에서 시간 경과에 따라 세부 상태로 전이
    u_int32_t   mvState_PeriodMs_stopGrace;                 // 2초 이상 움직임 없으면 정차로 간주 (현재 사용되지 않음, 참고용)
    u_int32_t   mvState_signalWait1_Seconds;                // 신호대기 1 상태 기준 시간 (초)
    u_int32_t   mvState_signalWait2_Seconds;                // 신호대기 2 상태 기준 시간 (초)
    u_int32_t   mvState_stopped1_Seconds;                   // 정차 1 상태 기준 시간 (초)
    u_int32_t   mvState_stopped2_Seconds;                   // 정차 2 상태 기준 시간 (초)
    u_int32_t   mvState_park_Seconds;                       // 주차 상태 기준 시간 (초)

    u_int32_t   serialPrint_intervalMs;                     // 시리얼 출력 간격 (ms)

                                                                    // 회전 감지 관련 상수 (Yaw 각속도 기반)
    float       turnState_Center_yawAngleVelocityDegps_Thresold;    // 이 각속도 이하이면 회전이 없다고 간주 (deg/s)
    float       turnState_LR_1_yawAngleVelocityDegps_Thresold;      // 약간 회전 감지 임계값 (deg/s)
    float       turnState_LR_2_yawAngleVelocityDegps_Thresold;      // 중간 회전 감지 임계값 (deg/s)
    float       turnState_LR_3_yawAngleVelocityDegps_Thresold;      // 급격한 회전 감지 임계값 (deg/s)
    float       turnState_speedKmh_MinSpeed;                        // 회전 감지를 위한 최소 속도 (km/h) - 정지 시 오인 방지
    float       turnState_speedKmh_HighSpeed_Threshold;             // 고속 회전 감지 시 임계값 조정을 위한 기준 속도 (km/h)
    u_int32_t   turnState_StableDurationMs;                         // 회전 상태가 안정적으로 유지되어야 하는 시간 (ms)
    
} T_M010_Config;

// 전역 설정 변수 선언
T_M010_Config g_M010_Config;


// ====================================================================================================
// 자동차 움직임 상태 열거형 (State Machine State) 정의
// ====================================================================================================
typedef enum {
    E_M010_CARMOVESTATE_UNKNOWN,        // 초기/알 수 없는 상태 (시스템 부팅 직후)
    E_M010_CARMOVESTATE_STOPPED_INIT,   // 기본 정차 상태 (다른 정차 세부 상태 진입점, 움직임 없음)
    E_M010_CARMOVESTATE_SIGNAL_WAIT1,   // 신호대기 1 (짧은 정차 시간, 예: 60초 미만)
    E_M010_CARMOVESTATE_SIGNAL_WAIT2,   // 신호대기 2 (중간 정차 시간, 예: 120초 미만)
    E_M010_CARMOVESTATE_STOPPED1,       // 정차 1 (긴 정차 시간, 예: 5분 미만)
    E_M010_CARMOVESTATE_STOPPED2,       // 정차 2 (더 긴 정차 시간, 예: 10분 미만)
    E_M010_CARMOVESTATE_PARKED,         // 주차 (가장 긴 정차 시간, 예: 10분 이상)
    E_M010_CARMOVESTATE_FORWARD,        // 전진 중
    E_M010_CARMOVESTATE_REVERSE,        // 후진 중
} T_M010_CarMovementState;

// ====================================================================================================
// 새로운 자동차 회전 상태 열거형 정의
// ====================================================================================================
typedef enum {
    E_M010_CARTURNSTATE_CENTER,     // 회전 없음 또는 미미한 회전 (직진)
    E_M010_CARTURNSTATE_LEFT_1,     // 약간 좌회전
    E_M010_CARTURNSTATE_LEFT_2,     // 중간 좌회전
    E_M010_CARTURNSTATE_LEFT_3,     // 급격한 좌회전
    E_M010_CARTURNSTATE_RIGHT_1,    // 약간 우회전
    E_M010_CARTURNSTATE_RIGHT_2,    // 중간 우회전
    E_M010_CARTURNSTATE_RIGHT_3     // 급격한 우회전
} T_M010_CarTurnState;

// ====================================================================================================
// 자동차 상태 정보 구조체 정의
// MPU6050 데이터를 기반으로 현재 차량의 움직임 및 회전 상태를 저장하고 추정된 물리량을 담습니다.
// ====================================================================================================
typedef struct {
    T_M010_CarMovementState carMovementState;   // 현재 움직임 상태 (정지, 전진, 후진 등)
    T_M010_CarTurnState     carTurnState;       // 현재 회전 상태 (직진, 좌/우회전 정도)

    float       speed_kmh;                      // 현재 추정 속도 (km/h, Y축 가속도 적분 추정)
    float       accelX_ms2;                     // X축 선형 가속도 (m/s^2)
    float       accelY_ms2;                     // Y축 선형 가속도 (m/s^2, 전진 기준: 양수-가속, 음수-감속)
    float       accelZ_ms2;                     // Z축 선형 가속도 (m/s^2, 노면 충격 감지용)

    float       yawAngle_deg;                   // Yaw 각도 (도) - 북쪽 기준 방위각, 0~360 또는 -180~180
    float       yawAngleVelocity_degps;         // Yaw 각속도 (도/초, 양수-우회전, 음수-좌회전)

    float       pitchAngle_deg;                 // Pitch 각도 (도, 양수-오르막, 음수-내리막)

    bool        isSpeedBumpDetected;            // 과속 방지턱 감지 여부 (일시적 플래그)
    bool        isEmergencyBraking;             // 급감속 감지 여부 (일시적 플래그)

    u_int32_t   lastMovementTime_ms;            // 마지막 움직임 (속도가 0이 아닐 때) 감지 시간 (ms)
    u_int32_t   stopStartTime_ms;               // 정차 상태가 시작된 시간 (ms) - 이 시간부터 정차 지속 시간 계산
    u_int32_t   currentStopTime_ms;             // 현재 정차 지속 시간 (ms)
    u_int32_t   stopStableStartTime_ms;         // 속도 드리프트 보정을 위해 정지 조건이 안정적으로 유지되기 시작한 시간 (ms)
} T_M010_CarStatus;


bool     g_M010_mpu_isDataReady = false; // MPU6050 DMP 데이터가 새로 준비되었는지 여부
// ====================================================================================================
// 전역 변수 (g_M010_으로 시작) 정의
// ====================================================================================================
T_M010_CarStatus g_M010_CarStatus; // 자동차 상태 구조체 인스턴스

// MPU6050 DMP 관련 전역 변수
bool        g_M010_dmp_isReady          = false;    // DMP 초기화 완료 여부 플래그
uint8_t     g_M010_mpu_interruptStatus;             // MPU6050 인터럽트 상태 레지스터 값
uint8_t     g_M010_dmp_devStatus;                   // 장치 상태 (0=성공, >0=오류 코드)
uint16_t    g_M010_dmp_fifoCount;                   // FIFO에 저장된 바이트 수
uint8_t     g_M010_dmp_fifoBuffer[64];              // FIFO 버퍼 (최대 64바이트)
uint16_t    g_M010_dmp_packetSize;                  // DMP 패킷의 크기 (바이트)


// 쿼터니언 및 오일러 각 관련 변수
Quaternion  g_M010_Quaternion;                      // MPU6050에서 계산된 쿼터니언 데이터
VectorFloat g_M010_gravity;                         // 중력 벡터 (쿼터니언에서 파생)
float       g_M010_ypr[3];                          // Yaw, Pitch, Roll 각도 (라디안, ypr[0]=Yaw, ypr[1]=Pitch, ypr[2]=Roll)
float       g_M010_yawAngleVelocity_degps;          // Yaw 각속도 (도/초, Raw 자이로 Z축에서 변환)

// 가속도 데이터 (필터링) 변수
float       g_M010_filteredAx;                      // 필터링된 X축 가속도 (m/s^2)
float       g_M010_filteredAy;                      // 필터링된 Y축 가속도 (m/s^2)
float       g_M010_filteredAz;                      // 필터링된 Z축 가속도 (m/s^2)

// 시간 관련 전역 변수
u_int32_t   g_M010_lastSampleTime_ms            = 0; // 마지막 MPU6050 데이터 샘플링 시간 (ms)
u_int32_t   g_M010_lastSerialPrintTime_ms       = 0; // 마지막 시리얼 출력 시간 (ms)
u_int32_t   g_M010_lastBumpDetectionTime_ms     = 0; // 마지막 방지턱 감지 시간 (ms) - 쿨다운 및 홀드 시간 계산용
u_int32_t   g_M010_lastDecelDetectionTime_ms    = 0; // 마지막 급감속 감지 시간 (ms) - 홀드 시간 계산용

u_int32_t   g_M010_stateTransitionStartTime_ms = 0; // 메인 상태 전환 조건이 만족되기 시작한 시간 (ms) - 히스테리시스용

// 회전 상태 안정화를 위한 전역 변수 (static으로 선언하여 파일 내에서만 유효)
static T_M010_CarTurnState  g_M010_potentialTurnState    = E_M010_CARTURNSTATE_CENTER;   // 잠재적 회전 상태
static u_int32_t            g_M010_turnStateStartTime_ms = 0;                    // 잠재적 회전 상태가 시작된 시간 (ms)

// MPU6050 인터럽트 발생 여부 플래그 및 인터럽트 서비스 루틴 (ISR)
volatile bool g_M010_mpu_isInterrupt = false; // MPU6050 인터럽트 발생 여부 (true = 데이터 준비됨)
/**
 * @brief MPU6050 데이터 준비 인터럽트 서비스 루틴 (ISR).
 * MPU6050에서 새로운 데이터가 준비되면 호출됩니다.
 * 이 함수는 가능한 한 짧고 빠르게 g_M010_mpu_isInterrupt 플래그만 설정합니다.
 */
void M010_dmpDataReady() {
    g_M010_mpu_isInterrupt = true; // 인터럽트 발생 시 플래그 설정
}

// ====================================================================================================
// 함수 선언 (프로토타입)
// ====================================================================================================
void M010_updateCarStatus(u_int32_t* p_currentTime_ms);     // MPU6050 데이터를 읽고 자동차 상태를 업데이트하는 함수
void M010_defineCarState(u_int32_t p_currentTime_ms);       // 차량 움직임 상태 정의 함수 (정지, 전진, 후진 등)
void M010_defineCarTurnState(u_int32_t p_currentTime_ms);   // 차량 회전 상태 정의 함수 (직진, 좌/우회전 정도)

// 설정 관련 함수 선언
void M010_Config_initDefaults();                            // 설정값 기본값 초기화
bool M010_Config_load();                                    // LittleFS에서 설정값 로드
bool M010_Config_save();                                    // LittleFS에 설정값 저장
void M010_Config_print();                                   // 현재 설정값 시리얼 출력
void M010_Config_handleSerialInput();                       // 시리얼 입력 처리 (설정 변경/저장/로드)

void M010_GlobalVar_init();                                 // 전역 변수 초기화

// ====================================================================================================
// 함수 정의 (M010_으로 시작)
// ====================================================================================================

/**
 * @brief MPU6050 센서 및 DMP (Digital Motion Processor)를 초기화합니다.
 * I2C 통신을 시작하고, MPU6050 연결을 테스트한 후, DMP를 초기화하고 인터럽트를 설정합니다.
 * DMP 초기화 실패 시 시스템을 무한 대기 상태로 만듭니다.
 */
void M010_MPU6050_init() {
    Wire.begin(); // I2C 통신 시작
    Wire.setClock(G_M010_I2C_CLOCK_FREQ); // I2C 통신 속도를 400kHz로 설정 (G_M010_I2C_CLOCK_FREQ 상수 사용)

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
        // RISING 모드는 MPU6050의 INT 핀이 High로 올라갈 때 인터럽트를 발생시킵니다.
        pinMode(G_M010_MPU_INTERRUPT_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(G_M010_MPU_INTERRUPT_PIN), M010_dmpDataReady, RISING);
        g_M010_mpu_interruptStatus = g_M010_Mpu.getIntStatus(); // 현재 인터럽트 상태 가져오기

        g_M010_dmp_packetSize = G_M010_DMP_PACKET_SIZE; // DMP에서 출력하는 FIFO 패킷의 크기 (MotionApps612 기준, 상수 사용)

        g_M010_dmp_isReady = true; // DMP 초기화 완료 플래그 설정
        dbgP1_println_F(F("DMP 초기화 완료!"));
    } else { // DMP 초기화 실패 시
        dbgP1_printf_F(F("DMP 초기화 실패 (오류 코드: %d)\n"), g_M010_dmp_devStatus);
        while (true); // 치명적인 오류이므로 무한 대기 (시스템 정지)
    }
}

/**
 * @brief 설정값 구조체 (g_M010_Config)에 기본값을 초기화합니다.
 * config.json 파일이 없거나 로드에 실패할 경우 이 기본값이 사용됩니다.
 * 시스템의 안정적인 동작을 위한 초기값 또는 복구 값으로 활용됩니다.
 */
void M010_Config_initDefaults() {
    dbgP1_println_F(F("설정 기본값 초기화 중..."));
    g_M010_Config.mvState_accelFilter_Alpha                 = 0.8; // 가속도 필터링의 부드러움 조절 (높을수록 이전 값 영향 큼)

    g_M010_Config.mvState_Forward_speedKmh_Threshold_Min    = 0.8; // 전진 시작 속도 (km/h)
    g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min    = 0.8; // 후진 시작 속도 (km/h)
    g_M010_Config.mvState_Stop_speedKmh_Threshold_Max       = 0.2; // 정지 판단 최대 속도 (km/h)
    g_M010_Config.mvState_Stop_accelMps2_Threshold_Max      = 0.2; // 정지 판단 가속도 변화 임계값 (m/s^2)
    g_M010_Config.mvState_Stop_gyroDps_Threshold_Max        = 0.5; // 정지 판단 Yaw 각속도 변화 임계값 (deg/s)

    g_M010_Config.mvState_stop_durationMs_Stable_Min        = 200; // 정지 상태 유지 최소 시간 (ms)
    g_M010_Config.mvState_move_durationMs_Stable_Min        = 150; // 움직임 상태 유지 최소 시간 (ms)
    g_M010_Config.mvState_normalMove_durationMs             = 100; // (현재 사용되지 않음)

    g_M010_Config.mvState_Decel_accelMps2_Threshold         = -3.0; // 급감속 감지 Y축 가속도 임계값 (m/s^2, 음수)
    g_M010_Config.mvState_Bump_accelMps2_Threshold          = 5.0;  // 방지턱 감지 Z축 가속도 임계값 (m/s^2)
    g_M010_Config.mvState_Bump_SpeedKmh_Min                 = 5.0;  // 방지턱 감지 최소 속도 (km/h)

    g_M010_Config.mvState_Bump_CooldownMs                   = 1000; // 방지턱 재감지 쿨다운 시간 (ms)
    g_M010_Config.mvState_Decel_durationMs_Hold             = 10000; // 급감속 상태 유지 시간 (ms)
    g_M010_Config.mvState_Bump_durationMs_Hold              = 10000; // 방지턱 감지 상태 유지 시간 (ms)

    g_M010_Config.mvState_PeriodMs_stopGrace                = 2000; // (현재 사용되지 않음)
    g_M010_Config.mvState_signalWait1_Seconds               = 60;   // 신호대기 1 기준 시간 (초)
    g_M010_Config.mvState_signalWait2_Seconds               = 120;  // 신호대기 2 기준 시간 (초)
    g_M010_Config.mvState_stopped1_Seconds                  = 300;  // 정차 1 기준 시간 (초, 5분)
    g_M010_Config.mvState_stopped2_Seconds                  = 600;  // 정차 2 기준 시간 (초, 10분)
    g_M010_Config.mvState_park_Seconds                      = 600;  // 주차 기준 시간 (초, 10분) - stopped2와 동일하게 시작, 주차는 더 긴 시간으로 설정 가능

    g_M010_Config.serialPrint_intervalMs                    = 5000; // 시리얼 출력 주기 (ms)

    g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold   = 5.0;  // 회전 없음 판단 Yaw 각속도 임계값 (deg/s)
    g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold     = 15.0; // 약간 회전 판단 Yaw 각속도 임계값 (deg/s)
    g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold     = 30.0; // 중간 회전 판단 Yaw 각속도 임계값 (deg/s)
    g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold     = 50.0; // 급격한 회전 판단 Yaw 각속도 임계값 (deg/s)
    g_M010_Config.turnState_speedKmh_MinSpeed                       = 1.0;  // 회전 감지 최소 속도 (km/h)
    g_M010_Config.turnState_speedKmh_HighSpeed_Threshold            = 30.0; // 고속 회전 판단 기준 속도 (km/h)
    g_M010_Config.turnState_StableDurationMs                        = 100;  // 회전 상태 유지 최소 시간 (ms)
}

/**
 * @brief LittleFS 파일 시스템에서 'config.json' 파일을 로드하여 g_M010_Config 구조체에 저장합니다.
 * 파일이 없거나 JSON 파싱에 실패하면 false를 반환하고, 성공하면 true를 반환합니다.
 * 기존에 저장된 설정값을 불러와 시스템을 이전 상태로 복원하는 데 사용됩니다.
 * @return 로드 성공 시 true, 실패 시 false
 */
bool M010_Config_load() {
    if (!LittleFS.begin()) {
        dbgP1_println_F(F("LittleFS 마운트 실패"));
        return false;
    }
    
    File v_configFile = LittleFS.open(G_M010_CONFIG_JSON_FILE, "r");
    if (!v_configFile) {
        dbgP1_println_F(F("config.json 파일 없음, 기본값 로드."));
        LittleFS.end();
        return false; // 파일이 없으면 로드 실패로 간주
    }

    // ArduinoJson 라이브러리를 사용하여 JSON 문서 파싱을 위한 메모리 할당
    // 적절한 STATIC_JSON_BUFFER_SIZE 또는 DynamicJsonDocument 사용
    // 여기서는 DynamicJsonDocument를 사용 (필요에 따라 메모리 할당)
    JsonDocument v_config_doc; 

    #ifdef G_M010_STREAM_USE
        ReadBufferingStream v_buffered_configFile(v_configFile, 256);
        Serial.println("Reading JSON with buffering...");
        DeserializationError error = deserializeJson(v_config_doc, v_buffered_configFile);
        //StaticJsonDocument<256> readDoc;

        //DeserializationError error = deserializeJson(readDoc, bufferedFile);
    #elif
        DeserializationError error = deserializeJson(v_config_doc, v_configFile);
    #endif
  
    v_configFile.close();
    LittleFS.end();

    if (error) {
        dbgP1_printf_F(F("config.json 파싱 실패: %s\n"), error.c_str());
        return false;
    }

    dbgP1_println_F(F("config.json 로드 성공."));

    // JSON 문서에서 값을 읽어 g_M010_Config 구조체에 저장
    // JSON 필드가 없으면 현재 g_M010_Config의 값을 유지 (OR 연산자 | 사용)
    g_M010_Config.mvState_accelFilter_Alpha = v_config_doc["mvState_accelFilter_Alpha"] | g_M010_Config.mvState_accelFilter_Alpha;

    g_M010_Config.mvState_Forward_speedKmh_Threshold_Min    = v_config_doc["mvState_Forward_speedKmh_Threshold_Min"]    | g_M010_Config.mvState_Forward_speedKmh_Threshold_Min;
    g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min    = v_config_doc["mvState_Reverse_speedKmh_Threshold_Min"]    | g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min;
    g_M010_Config.mvState_Stop_speedKmh_Threshold_Max       = v_config_doc["mvState_Stop_speedKmh_Threshold_Max"]       | g_M010_Config.mvState_Stop_speedKmh_Threshold_Max;
    g_M010_Config.mvState_Stop_accelMps2_Threshold_Max      = v_config_doc["mvState_Stop_accelMps2_Threshold_Max"]      | g_M010_Config.mvState_Stop_accelMps2_Threshold_Max;
    g_M010_Config.mvState_Stop_gyroDps_Threshold_Max        = v_config_doc["mvState_Stop_gyroDps_Threshold_Max"]        | g_M010_Config.mvState_Stop_gyroDps_Threshold_Max;

    g_M010_Config.mvState_stop_durationMs_Stable_Min    = v_config_doc["mvState_stop_durationMs_Stable_Min"]            | g_M010_Config.mvState_stop_durationMs_Stable_Min;
    g_M010_Config.mvState_move_durationMs_Stable_Min    = v_config_doc["mvState_move_durationMs_Stable_Min"]            | g_M010_Config.mvState_move_durationMs_Stable_Min;
    g_M010_Config.mvState_normalMove_durationMs         = v_config_doc["mvState_normalMove_durationMs"]                 | g_M010_Config.mvState_normalMove_durationMs;

    g_M010_Config.mvState_Decel_accelMps2_Threshold     = v_config_doc["mvState_Decel_accelMps2_Threshold"]             | g_M010_Config.mvState_Decel_accelMps2_Threshold;
    g_M010_Config.mvState_Bump_accelMps2_Threshold      = v_config_doc["mvState_Bump_accelMps2_Threshold"]              | g_M010_Config.mvState_Bump_accelMps2_Threshold;
    g_M010_Config.mvState_Bump_SpeedKmh_Min             = v_config_doc["mvState_Bump_SpeedKmh_Min"]                     | g_M010_Config.mvState_Bump_SpeedKmh_Min;

    g_M010_Config.mvState_Bump_CooldownMs               = v_config_doc["mvState_Bump_CooldownMs"]                       | g_M010_Config.mvState_Bump_CooldownMs;
    g_M010_Config.mvState_Decel_durationMs_Hold         = v_config_doc["mvState_Decel_durationMs_Hold"]                 | g_M010_Config.mvState_Decel_durationMs_Hold;
    g_M010_Config.mvState_Bump_durationMs_Hold          = v_config_doc["mvState_Bump_durationMs_Hold"]                  | g_M010_Config.mvState_Bump_durationMs_Hold;

    g_M010_Config.mvState_PeriodMs_stopGrace            = v_config_doc["mvState_PeriodMs_stopGrace"]                    | g_M010_Config.mvState_PeriodMs_stopGrace;
    g_M010_Config.mvState_signalWait1_Seconds           = v_config_doc["mvState_signalWait1_Seconds"]                   | g_M010_Config.mvState_signalWait1_Seconds;
    g_M010_Config.mvState_signalWait2_Seconds           = v_config_doc["mvState_signalWait2_Seconds"]                   | g_M010_Config.mvState_signalWait2_Seconds;
    g_M010_Config.mvState_stopped1_Seconds              = v_config_doc["mvState_stopped1_Seconds"]                      | g_M010_Config.mvState_stopped1_Seconds;
    g_M010_Config.mvState_stopped2_Seconds              = v_config_doc["mvState_stopped2_Seconds"]                      | g_M010_Config.mvState_stopped2_Seconds;
    g_M010_Config.mvState_park_Seconds                  = v_config_doc["mvState_park_Seconds"]                          | g_M010_Config.mvState_park_Seconds;

    g_M010_Config.serialPrint_intervalMs                = v_config_doc["serialPrint_intervalMs"]                        | g_M010_Config.serialPrint_intervalMs;

    g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold   = v_config_doc["turnState_Center_yawAngleVelocityDegps_Thresold"]   | g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold;
    g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold     = v_config_doc["turnState_LR_1_yawAngleVelocityDegps_Thresold"]     | g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold;
    g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold     = v_config_doc["turnState_LR_2_yawAngleVelocityDegps_Thresold"]     | g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold;
    g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold     = v_config_doc["turnState_LR_3_yawAngleVelocityDegps_Thresold"]     | g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold;
    g_M010_Config.turnState_speedKmh_MinSpeed                       = v_config_doc["turnState_speedKmh_MinSpeed"]                       | g_M010_Config.turnState_speedKmh_MinSpeed;
    g_M010_Config.turnState_speedKmh_HighSpeed_Threshold            = v_config_doc["turnState_speedKmh_HighSpeed_Threshold"]            | g_M010_Config.turnState_speedKmh_HighSpeed_Threshold;
    g_M010_Config.turnState_StableDurationMs                        = v_config_doc["turnState_StableDurationMs"]                        | g_M010_Config.turnState_StableDurationMs;

    return true;
}

/**
 * @brief g_M010_Config 구조체의 현재 설정값을 LittleFS의 'config.json' 파일에 저장합니다.
 * 이는 시스템 재부팅 후에도 설정값이 유지되도록 합니다.
 * @return 저장 성공 시 true, 실패 시 false
 */
bool M010_Config_save() {
    if (!LittleFS.begin()) {
        dbgP1_println_F(F("LittleFS 마운트 실패"));
        return false;
    }

    File v_configFile = LittleFS.open(G_M010_CONFIG_JSON_FILE, "w"); // 쓰기 모드로 열기 (기존 파일 덮어쓰기)
    if (!v_configFile) {
        dbgP1_println_F(F("config.json 파일 생성 실패"));
        LittleFS.end();
        return false;
    }

    // JSON 문서에 g_M010_Config 구조체의 값을 복사
    JsonDocument v_config_doc;

    v_config_doc["mvState_accelFilter_Alpha"]                   = g_M010_Config.mvState_accelFilter_Alpha;

    v_config_doc["mvState_Forward_speedKmh_Threshold_Min"]      = g_M010_Config.mvState_Forward_speedKmh_Threshold_Min;
    v_config_doc["mvState_Reverse_speedKmh_Threshold_Min"]      = g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min;
    v_config_doc["mvState_Stop_speedKmh_Threshold_Max"]         = g_M010_Config.mvState_Stop_speedKmh_Threshold_Max;
    v_config_doc["mvState_Stop_accelMps2_Threshold_Max"]        = g_M010_Config.mvState_Stop_accelMps2_Threshold_Max;
    v_config_doc["mvState_Stop_gyroDps_Threshold_Max"]          = g_M010_Config.mvState_Stop_gyroDps_Threshold_Max;

    v_config_doc["mvState_stop_durationMs_Stable_Min"]          = g_M010_Config.mvState_stop_durationMs_Stable_Min;
    v_config_doc["mvState_move_durationMs_Stable_Min"]          = g_M010_Config.mvState_move_durationMs_Stable_Min;
    v_config_doc["mvState_normalMove_durationMs"]               = g_M010_Config.mvState_normalMove_durationMs;

    v_config_doc["mvState_Decel_accelMps2_Threshold"]           = g_M010_Config.mvState_Decel_accelMps2_Threshold;
    v_config_doc["mvState_Bump_accelMps2_Threshold"]            = g_M010_Config.mvState_Bump_accelMps2_Threshold;
    v_config_doc["mvState_Bump_SpeedKmh_Min"]                   = g_M010_Config.mvState_Bump_SpeedKmh_Min;

    v_config_doc["mvState_Bump_CooldownMs"]                     = g_M010_Config.mvState_Bump_CooldownMs;
    v_config_doc["mvState_Decel_durationMs_Hold"]               = g_M010_Config.mvState_Decel_durationMs_Hold;
    v_config_doc["mvState_Bump_durationMs_Hold"]                = g_M010_Config.mvState_Bump_durationMs_Hold;

    v_config_doc["mvState_PeriodMs_stopGrace"]                  = g_M010_Config.mvState_PeriodMs_stopGrace;
    v_config_doc["mvState_signalWait1_Seconds"]                 = g_M010_Config.mvState_signalWait1_Seconds;
    v_config_doc["mvState_signalWait2_Seconds"]                 = g_M010_Config.mvState_signalWait2_Seconds;
    v_config_doc["mvState_stopped1_Seconds"]                    = g_M010_Config.mvState_stopped1_Seconds;
    v_config_doc["mvState_stopped2_Seconds"]                    = g_M010_Config.mvState_stopped2_Seconds;
    v_config_doc["mvState_park_Seconds"]                        = g_M010_Config.mvState_park_Seconds;

    v_config_doc["serialPrint_intervalMs"]                      = g_M010_Config.serialPrint_intervalMs;

    v_config_doc["turnState_Center_yawAngleVelocityDegps_Thresold"] = g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold;
    v_config_doc["turnState_LR_1_yawAngleVelocityDegps_Thresold"]   = g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold;
    v_config_doc["turnState_LR_2_yawAngleVelocityDegps_Thresold"]   = g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold;
    v_config_doc["turnState_LR_3_yawAngleVelocityDegps_Thresold"]   = g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold;
    v_config_doc["turnState_speedKmh_MinSpeed"]                     = g_M010_Config.turnState_speedKmh_MinSpeed;
    v_config_doc["turnState_speedKmh_HighSpeed_Threshold"]          = g_M010_Config.turnState_speedKmh_HighSpeed_Threshold;
    v_config_doc["turnState_StableDurationMs"]                      = g_M010_Config.turnState_StableDurationMs;


    #ifdef G_M010_STREAM_USE
        WriteBufferingStream v_buffered_configFile(v_configFile, 256);

        Serial.println("Reading JSON with buffering...");
        
        if(SerializeJson(v_config_doc, v_buffered_configFile) == 0){
            dbgP1_println_F(F("config.json 직렬화 실패"));
            
            v_configFile.close();
            LittleFS.end();

            return false;
        }

    #elif
        // JSON 문서를 파일에 직렬화하여 저장
        if (serializeJson(v_config_doc, v_configFile) == 0) {
            dbgP1_println_F(F("config.json 직렬화 실패"));
            v_configFile.close();
            LittleFS.end();
            return false;
        }
    #endif



    v_configFile.close();
    LittleFS.end();
    
    dbgP1_println_F(F("config.json 저장 성공."));
    return true;
}

/**
 * @brief 현재 활성화된 설정값을 시리얼 모니터로 자세히 출력합니다.
 * 사용자에게 현재 시스템의 동작 임계값 및 파라미터를 시각적으로 확인시켜 줍니다.
 * 또한 시리얼 명령어를 통한 설정 변경 방법을 안내합니다.
 */
void M010_Config_print() {
    dbgP1_println_F(F("\n---- 현재 설정값 ----"));
    dbgP1_print_F(F("mvState_accelFilter_Alpha: "               )); dbgP1_println(g_M010_Config.mvState_accelFilter_Alpha, 3);
    dbgP1_print_F(F("mvState_Forward_speedKmh_Threshold_Min: "  )); dbgP1_println(g_M010_Config.mvState_Forward_speedKmh_Threshold_Min, 2);
    dbgP1_print_F(F("mvState_Reverse_speedKmh_Threshold_Min: "  )); dbgP1_println(g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min, 2);
    dbgP1_print_F(F("mvState_Stop_speedKmh_Threshold_Max: "     )); dbgP1_println(g_M010_Config.mvState_Stop_speedKmh_Threshold_Max, 2);
    dbgP1_print_F(F("mvState_Stop_accelMps2_Threshold_Max: "    )); dbgP1_println(g_M010_Config.mvState_Stop_accelMps2_Threshold_Max, 2);
    dbgP1_print_F(F("mvState_Stop_gyroDps_Threshold_Max: "      )); dbgP1_println(g_M010_Config.mvState_Stop_gyroDps_Threshold_Max, 2);
    dbgP1_print_F(F("mvState_stop_durationMs_Stable_Min: "      )); dbgP1_println(g_M010_Config.mvState_stop_durationMs_Stable_Min);
    dbgP1_print_F(F("mvState_move_durationMs_Stable_Min: "      )); dbgP1_println(g_M010_Config.mvState_move_durationMs_Stable_Min);
    dbgP1_print_F(F("mvState_normalMove_durationMs: "           )); dbgP1_println(g_M010_Config.mvState_normalMove_durationMs);
    dbgP1_print_F(F("mvState_Decel_accelMps2_Threshold: "       )); dbgP1_println(g_M010_Config.mvState_Decel_accelMps2_Threshold, 2);
    dbgP1_print_F(F("mvState_Bump_accelMps2_Threshold: "        )); dbgP1_println(g_M010_Config.mvState_Bump_accelMps2_Threshold, 2);
    dbgP1_print_F(F("mvState_Bump_SpeedKmh_Min: "               )); dbgP1_println(g_M010_Config.mvState_Bump_SpeedKmh_Min, 2);
    dbgP1_print_F(F("mvState_Bump_CooldownMs: "                 )); dbgP1_println(g_M010_Config.mvState_Bump_CooldownMs);
    dbgP1_print_F(F("mvState_Decel_durationMs_Hold: "           )); dbgP1_println(g_M010_Config.mvState_Decel_durationMs_Hold);
    dbgP1_print_F(F("mvState_Bump_durationMs_Hold: "            )); dbgP1_println(g_M010_Config.mvState_Bump_durationMs_Hold);
    dbgP1_print_F(F("mvState_PeriodMs_stopGrace: "              )); dbgP1_println(g_M010_Config.mvState_PeriodMs_stopGrace);
    dbgP1_print_F(F("mvState_signalWait1_Seconds: "             )); dbgP1_println(g_M010_Config.mvState_signalWait1_Seconds);
    dbgP1_print_F(F("mvState_signalWait2_Seconds: "             )); dbgP1_println(g_M010_Config.mvState_signalWait2_Seconds);
    dbgP1_print_F(F("mvState_stopped1_Seconds: "                )); dbgP1_println(g_M010_Config.mvState_stopped1_Seconds);
    dbgP1_print_F(F("mvState_stopped2_Seconds: "                )); dbgP1_println(g_M010_Config.mvState_stopped2_Seconds);
    dbgP1_print_F(F("mvState_park_Seconds: "                    )); dbgP1_println(g_M010_Config.mvState_park_Seconds);
    dbgP1_print_F(F("serialPrint_intervalMs: "                  )); dbgP1_println(g_M010_Config.serialPrint_intervalMs);
    dbgP1_print_F(F("turnState_Center_yawAngleVelocityDegps_Thresold: " )); dbgP1_println(g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold, 2);
    dbgP1_print_F(F("turnState_LR_1_yawAngleVelocityDegps_Thresold: "   )); dbgP1_println(g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold, 2);
    dbgP1_print_F(F("turnState_LR_2_yawAngleVelocityDegps_Thresold: "   )); dbgP1_println(g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold, 2);
    dbgP1_print_F(F("turnState_LR_3_yawAngleVelocityDegps_Thresold: "   )); dbgP1_println(g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold, 2);
    dbgP1_print_F(F("turnState_speedKmh_MinSpeed: "                     )); dbgP1_println(g_M010_Config.turnState_speedKmh_MinSpeed, 2);
    dbgP1_print_F(F("turnState_speedKmh_HighSpeed_Threshold: "          )); dbgP1_println(g_M010_Config.turnState_speedKmh_HighSpeed_Threshold, 2);
    dbgP1_print_F(F("turnState_StableDurationMs: "                      )); dbgP1_println(g_M010_Config.turnState_StableDurationMs);
    dbgP1_println_F(F("--------------------------"));
    dbgP1_println_F(F("설정 변경: set <항목이름> <값> (예: set mvState_accelFilter_Alpha 0.9)"));
    dbgP1_println_F(F("설정 저장: saveconfig"));
    dbgP1_println_F(F("설정 불러오기: loadconfig"));
    dbgP1_println_F(F("설정 초기화: resetconfig"));
    dbgP1_println_F(F("설정 보기: printconfig"));
}

/**
 * @brief 시리얼 포트로부터 명령어를 읽어 설정값을 변경하거나 저장/로드/초기화합니다.
 * 사용자가 시리얼 모니터를 통해 시스템 파라미터를 동적으로 제어할 수 있도록 합니다.
 * 명령어 형식: "set <항목이름> <값>", "saveconfig", "loadconfig", "printconfig", "resetconfig"
 */
void M010_Config_handleSerialInput() {
    if (Serial.available()) {
        String v_serial_input = Serial.readStringUntil('\n');
        v_serial_input.trim(); // 앞뒤 공백 제거
        dbgP1_printf_F(F("시리얼 입력: %s\n"), v_serial_input.c_str());

        if (v_serial_input.startsWith("set ")) {
            int firstSpace = v_serial_input.indexOf(' ');
            int secondSpace = v_serial_input.indexOf(' ', firstSpace + 1);
            if (firstSpace != -1 && secondSpace != -1) {
                String paramName = v_serial_input.substring(firstSpace + 1, secondSpace);
                String valueStr = v_serial_input.substring(secondSpace + 1);
                
                // 문자열을 적절한 타입으로 변환하여 설정값 업데이트
                // 각 설정 항목에 대해 문자열 비교 및 타입 변환을 수행
                if      (paramName.equals("mvState_accelFilter_Alpha"               )) g_M010_Config.mvState_accelFilter_Alpha = valueStr.toFloat();
                else if (paramName.equals("mvState_Forward_speedKmh_Threshold_Min"  )) g_M010_Config.mvState_Forward_speedKmh_Threshold_Min = valueStr.toFloat();
                else if (paramName.equals("mvState_Reverse_speedKmh_Threshold_Min"  )) g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min = valueStr.toFloat();
                else if (paramName.equals("mvState_Stop_speedKmh_Threshold_Max"     )) g_M010_Config.mvState_Stop_speedKmh_Threshold_Max = valueStr.toFloat();
                else if (paramName.equals("mvState_Stop_accelMps2_Threshold_Max"    )) g_M010_Config.mvState_Stop_accelMps2_Threshold_Max = valueStr.toFloat();
                else if (paramName.equals("mvState_Stop_gyroDps_Threshold_Max"      )) g_M010_Config.mvState_Stop_gyroDps_Threshold_Max = valueStr.toFloat();
                else if (paramName.equals("mvState_stop_durationMs_Stable_Min"      )) g_M010_Config.mvState_stop_durationMs_Stable_Min = valueStr.toInt();
                else if (paramName.equals("mvState_move_durationMs_Stable_Min"      )) g_M010_Config.mvState_move_durationMs_Stable_Min = valueStr.toInt();
                else if (paramName.equals("mvState_normalMove_durationMs"           )) g_M010_Config.mvState_normalMove_durationMs = valueStr.toInt();
                else if (paramName.equals("mvState_Decel_accelMps2_Threshold"       )) g_M010_Config.mvState_Decel_accelMps2_Threshold = valueStr.toFloat();
                else if (paramName.equals("mvState_Bump_accelMps2_Threshold"        )) g_M010_Config.mvState_Bump_accelMps2_Threshold = valueStr.toFloat();
                else if (paramName.equals("mvState_Bump_SpeedKmh_Min"               )) g_M010_Config.mvState_Bump_SpeedKmh_Min = valueStr.toFloat();
                else if (paramName.equals("mvState_Bump_CooldownMs"                 )) g_M010_Config.mvState_Bump_CooldownMs = valueStr.toInt();
                else if (paramName.equals("mvState_Decel_durationMs_Hold"           )) g_M010_Config.mvState_Decel_durationMs_Hold = valueStr.toInt();
                else if (paramName.equals("mvState_Bump_durationMs_Hold"            )) g_M010_Config.mvState_Bump_durationMs_Hold = valueStr.toInt();
                else if (paramName.equals("mvState_PeriodMs_stopGrace"              )) g_M010_Config.mvState_PeriodMs_stopGrace = valueStr.toInt();
                else if (paramName.equals("mvState_signalWait1_Seconds"             )) g_M010_Config.mvState_signalWait1_Seconds = valueStr.toInt();
                else if (paramName.equals("mvState_signalWait2_Seconds"             )) g_M010_Config.mvState_signalWait2_Seconds = valueStr.toInt();
                else if (paramName.equals("mvState_stopped1_Seconds"                )) g_M010_Config.mvState_stopped1_Seconds = valueStr.toInt();
                else if (paramName.equals("mvState_stopped2_Seconds"                )) g_M010_Config.mvState_stopped2_Seconds = valueStr.toInt();
                else if (paramName.equals("mvState_park_Seconds"                    )) g_M010_Config.mvState_park_Seconds = valueStr.toInt();
                else if (paramName.equals("serialPrint_intervalMs"                  )) g_M010_Config.serialPrint_intervalMs = valueStr.toInt();
                else if (paramName.equals("turnState_Center_yawAngleVelocityDegps_Thresold" )) g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold = valueStr.toFloat();
                else if (paramName.equals("turnState_LR_1_yawAngleVelocityDegps_Thresold"   )) g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold = valueStr.toFloat();
                else if (paramName.equals("turnState_LR_2_yawAngleVelocityDegps_Thresold"   )) g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold = valueStr.toFloat();
                else if (paramName.equals("turnState_LR_3_yawAngleVelocityDegps_Thresold"   )) g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold = valueStr.toFloat();
                else if (paramName.equals("turnState_speedKmh_MinSpeed"             )) g_M010_Config.turnState_speedKmh_MinSpeed = valueStr.toFloat();
                else if (paramName.equals("turnState_speedKmh_HighSpeed_Threshold"  )) g_M010_Config.turnState_speedKmh_HighSpeed_Threshold = valueStr.toFloat();
                else if (paramName.equals("turnState_StableDurationMs"              )) g_M010_Config.turnState_StableDurationMs = valueStr.toInt();
                else {
                    dbgP1_printf_F(F("알 수 없는 설정 항목: %s\n"), paramName.c_str());
                    return;
                }
                dbgP1_printf_F(F("설정 변경됨: %s = %s\n"), paramName.c_str(), valueStr.c_str());
            } else {
                dbgP1_println_F(F("잘못된 'set' 명령어 형식. 예: set mvState_accelFilter_Alpha 0.9"));
            }
        } else if (v_serial_input.equals("saveconfig")) {
            if (M010_Config_save()) {
                dbgP1_println_F(F("설정값이 LittleFS에 저장되었습니다."));
            } else {
                dbgP1_println_F(F("설정값 저장 실패!"));
            }
        } else if (v_serial_input.equals("loadconfig")) {
            if (M010_Config_load()) {
                dbgP1_println_F(F("설정값이 LittleFS에서 로드되었습니다."));
                M010_Config_print(); // 로드 후 현재 설정값 출력
            } else {
                dbgP1_println_F(F("설정값 로드 실패! 기본값이 사용됩니다."));
                M010_Config_initDefaults(); // 로드 실패 시 기본값으로 초기화
                M010_Config_print();
            }
        } else if (v_serial_input.equals("printconfig")) {
            M010_Config_print();
        } else if (v_serial_input.equals("resetconfig")) {
            M010_Config_initDefaults(); // 설정값을 기본값으로 초기화
            if (M010_Config_save()) { // 초기화된 기본값을 파일에 저장
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

/**
 * @brief 모든 전역 변수를 초기 상태로 설정합니다.
 * 시스템이 시작될 때 또는 특정 조건에서 상태를 리셋할 때 호출됩니다.
 */
void M010_GlobalVar_init(){
    // 자동차 상태 구조체 초기화
    g_M010_CarStatus.carMovementState       = E_M010_CARMOVESTATE_UNKNOWN; // 초기 움직임 상태를 알 수 없음으로 설정
    g_M010_CarStatus.carTurnState           = E_M010_CARTURNSTATE_CENTER;  // 초기 회전 상태를 회전 없음으로 설정
    g_M010_CarStatus.speed_kmh              = 0.0;
    g_M010_CarStatus.accelX_ms2             = 0.0;
    g_M010_CarStatus.accelY_ms2             = 0.0;
    g_M010_CarStatus.accelZ_ms2             = 0.0;
    g_M010_CarStatus.yawAngle_deg           = 0.0;
    g_M010_CarStatus.yawAngleVelocity_degps = 0.0;
    g_M010_CarStatus.pitchAngle_deg         = 0.0;
    g_M010_CarStatus.isSpeedBumpDetected    = false;
    g_M010_CarStatus.isEmergencyBraking     = false;
    g_M010_CarStatus.lastMovementTime_ms    = millis(); // 초기 움직임 시간 설정 (시작 시점)
    g_M010_CarStatus.stopStartTime_ms       = millis(); // **정지 상태 시작 시간 초기화 (추가/개선)**
    g_M010_CarStatus.currentStopTime_ms     = 0;
    g_M010_CarStatus.stopStableStartTime_ms = 0; // 정지 안정화 시작 시간 초기화

    // MPU6050 데이터 필터링 관련 변수 초기화
    g_M010_filteredAx                       = 0.0;
    g_M010_filteredAy                       = 0.0;
    g_M010_filteredAz                       = 0.0;

    // 시간 관련 전역 변수 초기화
    g_M010_lastSampleTime_ms                = 0;      
    g_M010_lastSerialPrintTime_ms           = 0; 
    g_M010_lastBumpDetectionTime_ms         = 0; 
    g_M010_lastDecelDetectionTime_ms        = 0; 
    g_M010_stateTransitionStartTime_ms      = 0; 
    
    // 회전 상태 안정화를 위한 전역 변수 초기화
    g_M010_potentialTurnState               = E_M010_CARTURNSTATE_CENTER;
    g_M010_turnStateStartTime_ms            = 0;
}

/**
 * @brief ESP32가 시작될 때 한 번 실행되는 초기 설정 함수입니다.
 * LittleFS를 통한 설정값 로드 및 MPU6050 센서 초기화를 담당합니다.
 * `setup()` 함수에서 호출되어야 합니다.
 */
void M010_MPU_init() {
    // 설정값 로드 시도. config.json 파일이 없거나 로드에 실패하면 기본값으로 초기화합니다.
    if (!M010_Config_load()) {
        dbgP1_println_F(F("설정 파일 로드 실패. 기본값으로 초기화 후 저장합니다."));
        M010_Config_initDefaults(); // 기본값으로 설정 초기화
        M010_Config_save();         // 초기화된 기본값을 파일에 저장
    }
    M010_Config_print(); // 현재 활성화된 설정값 출력

    M010_MPU6050_init(); // MPU6050 센서 초기화 (DMP 포함)

    M010_GlobalVar_init(); // 모든 전역 변수를 초기 상태로 설정
	
    dbgP1_println_F(F("Setup 완료!"));
}

/**
 * @brief MPU6050 데이터를 읽고 자동차의 가속도, 속도, 각도, 각속도 등을 업데이트합니다.
 * DMP FIFO 버퍼에서 데이터를 가져와 쿼터니언, 오일러 각, 선형 가속도 등을 계산합니다.
 * 상보 필터 및 속도 드리프트 보정 로직이 적용됩니다.
 * @param p_currentTime_ms 현재 시간을 저장할 u_int32_t 포인터 (millis() 값으로 업데이트 됨)
 */
void M010_updateCarStatus(u_int32_t* p_currentTime_ms) {
    if (!g_M010_dmp_isReady) return; // DMP가 준비되지 않았으면 데이터 처리 스킵

    // MPU6050 인터럽트가 발생했거나, FIFO 버퍼에 최소 한 개 이상의 완전한 DMP 패킷이 있는지 확인
    // (g_M010_mpu_isInterrupt는 ISR에서 설정, g_M010_dmp_fifoCount는 MPU 내부 FIFO의 바이트 수)
    if (!g_M010_mpu_isInterrupt && g_M010_dmp_fifoCount < g_M010_dmp_packetSize) {
        return; // 데이터가 없으면 함수 종료
    }
    
    g_M010_mpu_isInterrupt = false; // 인터럽트 플래그 초기화 (다음 인터럽트를 위해)

    // DMP FIFO에서 최신 패킷을 읽어옴
    if (g_M010_Mpu.dmpGetCurrentFIFOPacket(g_M010_dmp_fifoBuffer)) { 
		*p_currentTime_ms           = millis(); // 현재 시간 가져오기
		
        // 샘플링 시간 간격 계산 (이전 샘플링 시간과 현재 시간의 차이). 속도 추정 등에 사용.
		float v_deltaTime_s         = (*p_currentTime_ms - g_M010_lastSampleTime_ms) / 1000.0f;
        g_M010_lastSampleTime_ms    = *p_currentTime_ms; // 마지막 샘플링 시간 업데이트

        // 쿼터니언 (방향 정보를 효율적으로 표현), Yaw/Pitch/Roll (오일러 각), 중력 벡터 계산
        g_M010_Mpu.dmpGetQuaternion(&g_M010_Quaternion, g_M010_dmp_fifoBuffer);
        g_M010_Mpu.dmpGetGravity(&g_M010_gravity, &g_M010_Quaternion); 
        g_M010_Mpu.dmpGetYawPitchRoll(g_M010_ypr, &g_M010_Quaternion, &g_M010_gravity);

        // Yaw, Pitch 각도를 라디안에서 도로 변환하여 저장
        g_M010_CarStatus.yawAngle_deg   = g_M010_ypr[0] * G_M010_RAD_TO_DEG;
        g_M010_CarStatus.pitchAngle_deg = g_M010_ypr[1] * G_M010_RAD_TO_DEG;

        // 선형 가속도 (중력분 제거) 계산
        VectorInt16 v_accel_raw;        // Raw 가속도 (MPU6050 내부 데이터 형식)
        VectorInt16 v_accel_linear;     // 중력분이 제거된 선형 가속도 결과

        g_M010_Mpu.dmpGetAccel(&v_accel_raw, g_M010_dmp_fifoBuffer);                        // Raw 가속도 얻기
        g_M010_Mpu.dmpGetLinearAccel(&v_accel_linear, &v_accel_raw, &g_M010_gravity);       // 선형 가속도 계산

        // 계산된 선형 가속도를 m/s^2 단위로 변환 (중력 가속도 G_M010_GRAVITY_MPS2 곱함)
        float v_currentAx_ms2 = (float)v_accel_linear.x * (G_M010_GRAVITY_MPS2 / 16384.0f); // MPU6050 가속도 센서 2g 스케일 기준 (16384 LSB/g)
        float v_currentAy_ms2 = (float)v_accel_linear.y * (G_M010_GRAVITY_MPS2 / 16384.0f); 
        float v_currentAz_ms2 = (float)v_accel_linear.z * (G_M010_GRAVITY_MPS2 / 16384.0f); 

        // 상보 필터를 사용하여 가속도 데이터 평활화 (노이즈 감소)
        // alpha 값이 높을수록 이전 값의 영향이 크고, 낮을수록 현재 값의 영향이 커짐.
        g_M010_filteredAx = g_M010_Config.mvState_accelFilter_Alpha * g_M010_filteredAx + (1 - g_M010_Config.mvState_accelFilter_Alpha) * v_currentAx_ms2;
        g_M010_filteredAy = g_M010_Config.mvState_accelFilter_Alpha * g_M010_filteredAy + (1 - g_M010_Config.mvState_accelFilter_Alpha) * v_currentAy_ms2;
        g_M010_filteredAz = g_M010_Config.mvState_accelFilter_Alpha * g_M010_filteredAz + (1 - g_M010_Config.mvState_accelFilter_Alpha) * v_currentAz_ms2;

        // 필터링된 가속도 값을 자동차 상태 구조체에 저장
        g_M010_CarStatus.accelX_ms2 = g_M010_filteredAx;
        g_M010_CarStatus.accelY_ms2 = g_M010_filteredAy;
        g_M010_CarStatus.accelZ_ms2 = g_M010_filteredAz;

        // Yaw 각속도 (Z축 자이로 데이터) 계산
        VectorInt16 v_Gyro_raw; // 자이로 데이터
        g_M010_Mpu.dmpGetGyro(&v_Gyro_raw, g_M010_dmp_fifoBuffer); // Raw 자이로 데이터 얻기

        // 자이로 스케일 팩터 (G_M010_GYRO_SCALE_FACTOR)를 이용하여 deg/s 단위로 변환
        g_M010_yawAngleVelocity_degps           = (float)v_Gyro_raw.z / G_M010_GYRO_SCALE_FACTOR; 
        g_M010_CarStatus.yawAngleVelocity_degps = g_M010_yawAngleVelocity_degps;

        // 속도 추정 (Y축 가속도 적분)
        // 이 방법은 오차 누적(드리프트) 가능성이 있으므로, 정지 시 보정 로직이 필수적입니다.
        float v_speedChange_mps     = g_M010_CarStatus.accelY_ms2 * v_deltaTime_s;
        g_M010_CarStatus.speed_kmh += (v_speedChange_mps * G_M010_MPS_TO_KMH_FACTOR); // m/s를 km/h로 변환하여 누적

        // 정지 시 속도 드리프트 보정 강화 로직
        // 가속도 및 각속도 변화가 모두 임계값 이하로 충분히 오래 유지될 때만 속도를 0으로 설정
        if (fabs(g_M010_CarStatus.accelY_ms2) < g_M010_Config.mvState_Stop_accelMps2_Threshold_Max &&
            fabs(g_M010_CarStatus.yawAngleVelocity_degps) < g_M010_Config.mvState_Stop_gyroDps_Threshold_Max) {
            
            if (g_M010_CarStatus.stopStableStartTime_ms == 0) { // 정지 안정화 조건 만족 시작 시간 기록
				g_M010_CarStatus.stopStableStartTime_ms = *p_currentTime_ms;
            } else if ((*p_currentTime_ms - g_M010_CarStatus.stopStableStartTime_ms) >= g_M010_Config.mvState_stop_durationMs_Stable_Min) {
                // 충분히 안정적인 정지 상태로 판단되면 속도 0으로 보정 (드리프트 방지)
                g_M010_CarStatus.speed_kmh = 0.0; 
            }
        } else {
            g_M010_CarStatus.stopStableStartTime_ms = 0; // 움직임 감지 시 안정화 시작 시간 리셋
        }

        // 추정 속도가 음수로 작아지면 0으로 보정 (뒤로 가지 않는 상황 가정)
        if (g_M010_CarStatus.speed_kmh < 0 && g_M010_CarStatus.carMovementState != E_M010_CARMOVESTATE_REVERSE) {
            g_M010_CarStatus.speed_kmh = 0.0;
        }

		g_M010_mpu_isDataReady = true; // 새로운 데이터가 처리되었음을 알림
    }
}

/**
 * @brief MPU6050 데이터 기반으로 자동차 움직임 상태를 정의합니다. (주요 상태 머신 로직)
 * 히스테리시스 (임계값 차등)와 시간 지연 (조건 지속 시간)을 통해 노이즈에 강인한 상태 전환을 수행합니다.
 * @param p_currentTime_ms 현재 시간 (millis() 값)
 */
void M010_defineCarState(u_int32_t p_currentTime_ms) {
    float v_speed_kmh   = g_M010_CarStatus.speed_kmh;
    float v_accelY      = g_M010_CarStatus.accelY_ms2;
    float v_accelZ      = g_M010_CarStatus.accelZ_ms2;

    // 다음 상태를 예측 (기본적으로 현재 상태를 유지)
    T_M010_CarMovementState v_CarMovement_nextState = g_M010_CarStatus.carMovementState;

    // =============================================================================================
    // 1. 과속 방지턱 감지 (일시적 플래그, 메인 움직임 상태와 독립적으로 작동)
    // Z축 가속도 변화와 최소 속도, 그리고 쿨다운 시간을 고려하여 방지턱을 감지합니다.
    // =============================================================================================
    if (fabs(v_accelZ) > g_M010_Config.mvState_Bump_accelMps2_Threshold && // Z축 가속도 임계값 초과
        fabs(v_speed_kmh) > g_M010_Config.mvState_Bump_SpeedKmh_Min &&     // 최소 속도 이상에서만 감지
        (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) > g_M010_Config.mvState_Bump_CooldownMs) { // 쿨다운 시간 경과
        g_M010_CarStatus.isSpeedBumpDetected = true;
        g_M010_lastBumpDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isSpeedBumpDetected &&
               (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) >= g_M010_Config.mvState_Bump_durationMs_Hold) {
        g_M010_CarStatus.isSpeedBumpDetected = false; // 유지 시간 후 플래그 리셋
    }

    // =============================================================================================
    // 2. 급감속 감지 (일시적 플래그, 메인 움직임 상태와 독립적으로 작동)
    // Y축 가속도가 급격한 음수 값 (감속) 임계값 미만일 때 감지합니다.
    // =============================================================================================
    if (v_accelY < g_M010_Config.mvState_Decel_accelMps2_Threshold) { // Y축 가속도가 급감속 임계값 미만
        g_M010_CarStatus.isEmergencyBraking = true;
        g_M010_lastDecelDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isEmergencyBraking &&
               (p_currentTime_ms - g_M010_lastDecelDetectionTime_ms) >= g_M010_Config.mvState_Decel_durationMs_Hold) {
        g_M010_CarStatus.isEmergencyBraking = false; // 유지 시간 후 플래그 리셋
    }

    // =============================================================================================
    // 3. 메인 움직임 상태 머신 로직
    // 현재 차량의 MovementState에 따라 다음 상태를 결정합니다.
    // 히스테리시스 (전이 조건의 지속 시간)를 적용하여 상태 전환의 안정성을 확보합니다.
    // =============================================================================================
    switch (g_M010_CarStatus.carMovementState) {
        // 현재가 '정지' 관련 상태일 때, '움직임' 상태로의 전환 조건 확인
        case E_M010_CARMOVESTATE_UNKNOWN:
        case E_M010_CARMOVESTATE_STOPPED_INIT:
        case E_M010_CARMOVESTATE_SIGNAL_WAIT1:
        case E_M010_CARMOVESTATE_SIGNAL_WAIT2:
        case E_M010_CARMOVESTATE_STOPPED1:
        case E_M010_CARMOVESTATE_STOPPED2:
        case E_M010_CARMOVESTATE_PARKED:
            if (v_speed_kmh > g_M010_Config.mvState_Forward_speedKmh_Threshold_Min) { // 전진 시작 조건 (히스테리시스 상단)
                if (g_M010_stateTransitionStartTime_ms == 0) { // 조건 만족 시작 시간 기록
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_Config.mvState_move_durationMs_Stable_Min) {
                    // 전진 조건이 충분히 오래 지속되면 상태 전환
                    v_CarMovement_nextState = E_M010_CARMOVESTATE_FORWARD;
                    g_M010_CarStatus.stopStartTime_ms = 0;        // 정차 시간 리셋 (움직이기 시작했으므로)
                    g_M010_CarStatus.lastMovementTime_ms = p_currentTime_ms; // 마지막 움직임 시간 기록
                    g_M010_stateTransitionStartTime_ms = 0;      // 전환 완료 후 리셋
                }
            } else if (v_speed_kmh < -g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min) { // 후진 시작 조건 (히스테리시스 상단)
                if (g_M010_stateTransitionStartTime_ms == 0) {
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_Config.mvState_move_durationMs_Stable_Min) {
                    // 후진 조건이 충분히 오래 지속되면 상태 전환
                    v_CarMovement_nextState = E_M010_CARMOVESTATE_REVERSE;
                    g_M010_CarStatus.stopStartTime_ms = 0;        // 정차 시간 리셋
                    g_M010_CarStatus.lastMovementTime_ms = p_currentTime_ms;
                    g_M010_stateTransitionStartTime_ms = 0;
                }
            } else {
                // 전진/후진 조건 불충족 시 전환 시작 시간 리셋
                g_M010_stateTransitionStartTime_ms = 0;

                // 정차 상태 내에서의 세부 시간 기반 상태 전환 (정지 상태가 유지될 때만 갱신)
                // 현재 정차 지속 시간 계산
                g_M010_CarStatus.currentStopTime_ms = p_currentTime_ms - g_M010_CarStatus.stopStartTime_ms;
                u_int32_t v_stopSeconds = g_M010_CarStatus.currentStopTime_ms / 1000;

                // 정차 시간에 따라 세부 상태로 전이
                if (v_stopSeconds >= g_M010_Config.mvState_park_Seconds) {
                    v_CarMovement_nextState = E_M010_CARMOVESTATE_PARKED;
                } else if (v_stopSeconds >= g_M010_Config.mvState_stopped2_Seconds) {
                    v_CarMovement_nextState = E_M010_CARMOVESTATE_STOPPED2;
                } else if (v_stopSeconds >= g_M010_Config.mvState_stopped1_Seconds) {
                    v_CarMovement_nextState = E_M010_CARMOVESTATE_STOPPED1;
                } else if (v_stopSeconds >= g_M010_Config.mvState_signalWait2_Seconds) {
                    v_CarMovement_nextState = E_M010_CARMOVESTATE_SIGNAL_WAIT2;
                } else if (v_stopSeconds >= g_M010_Config.mvState_signalWait1_Seconds) {
                    v_CarMovement_nextState = E_M010_CARMOVESTATE_SIGNAL_WAIT1;
                } else {
                    // 모든 조건에 해당하지 않으면 기본 정차 상태로 유지 (시간이 짧을 경우)
                    v_CarMovement_nextState = E_M010_CARMOVESTATE_STOPPED_INIT;
                }
            }
            break;

        // 현재가 '움직임' 상태일 때, '정지' 상태로의 전환 조건 확인
        case E_M010_CARMOVESTATE_FORWARD:
        case E_M010_CARMOVESTATE_REVERSE:
            if (fabs(v_speed_kmh) < g_M010_Config.mvState_Stop_speedKmh_Threshold_Max) { // 정지 조건 (히스테리시스 하단)
                if (g_M010_stateTransitionStartTime_ms == 0) { // 조건 만족 시작 시간 기록
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
                } else if ((p_currentTime_ms - g_M010_stateTransitionStartTime_ms) >= g_M010_Config.mvState_stop_durationMs_Stable_Min) {
                    // 정지 조건이 충분히 오래 지속되면 상태 전환
                    v_CarMovement_nextState = E_M010_CARMOVESTATE_STOPPED_INIT; // 정차의 진입점 상태로 전환
                    g_M010_CarStatus.speed_kmh = 0.0;     // 속도 0으로 보정 (정지했으므로)
                    g_M010_CarStatus.stopStartTime_ms = p_currentTime_ms; // **새로운 정차 시작 시간 기록 (개선)**
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
    if (v_CarMovement_nextState != g_M010_CarStatus.carMovementState) {
        dbgP1_printf_F(F("State transition: %d -> %d\n"), g_M010_CarStatus.carMovementState, v_CarMovement_nextState);
        g_M010_CarStatus.carMovementState = v_CarMovement_nextState;
    }
}

/**
 * @brief Yaw 각속도와 차량 이동 속도를 기반으로 자동차의 회전 상태를 정의합니다.
 * 속도에 따라 회전 감지 임계값을 동적으로 조정하여 정확도를 높입니다.
 * 회전 상태 전이에도 히스테리시스가 적용되어 안정적인 감지를 목표로 합니다.
 * @param p_currentTime_ms 현재 시간 (millis() 값)
 */
void M010_defineCarTurnState(u_int32_t p_currentTime_ms) {

    float v_speed_kmh_abs               = fabs(g_M010_CarStatus.speed_kmh); // 속도는 항상 양수 절댓값으로 처리
    float v_yawAngleVelocity_degps      = g_M010_CarStatus.yawAngleVelocity_degps; // 현재 Yaw 각속도 (deg/s)
    
    // 현재 프레임에서 감지된 회전 상태를 저장할 임시 변수
    T_M010_CarTurnState v_currentDetectedTurnState = E_M010_CARTURNSTATE_CENTER;

    // 속도에 따른 Yaw 각속도 임계값 동적 조정
    // 차량 속도에 따라 회전 감지 민감도를 조절하여 오인식을 줄이고 정확도를 높입니다.
    float v_turnCenter_yawAngleVelocityDegps_Thresold      = g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold;
    float v_turnLR_1_yawAngleVelocityDegps_Thresold        = g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold;
    float v_turnLR_2_yawAngleVelocityDegps_Thresold        = g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold;
    float v_turnLR_3_yawAngleVelocityDegps_Thresold        = g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold;

    // 고속 주행 시 임계값을 약간 낮춰 작은 각속도 변화에도 민감하게 반응하도록 함
    if (v_speed_kmh_abs > g_M010_Config.turnState_speedKmh_HighSpeed_Threshold) {
        v_turnCenter_yawAngleVelocityDegps_Thresold    *= 0.8; 
        v_turnLR_1_yawAngleVelocityDegps_Thresold      *= 0.8;
        v_turnLR_2_yawAngleVelocityDegps_Thresold      *= 0.8;
        v_turnLR_3_yawAngleVelocityDegps_Thresold      *= 0.8;
    } 
    // 저속 주행 시 (특히 MinSpeed에 가까울수록) 임계값을 높여 불필요한 감지 방지
    // (예: 정지 상태에서 핸들만 돌리는 경우, 주차 시 저속 회전 등 오인식 방지)
    else if (v_speed_kmh_abs < g_M010_Config.turnState_speedKmh_MinSpeed + 3.0) { // 최소 속도 + 3km/h 범위 내에서 조정
        v_turnCenter_yawAngleVelocityDegps_Thresold    *= 1.2; 
        v_turnLR_1_yawAngleVelocityDegps_Thresold      *= 1.2;
        v_turnLR_2_yawAngleVelocityDegps_Thresold      *= 1.2;
        v_turnLR_3_yawAngleVelocityDegps_Thresold      *= 1.2;
    }

    // 최소 속도 이상일 때만 회전 감지 로직 활성화
    if (v_speed_kmh_abs >= g_M010_Config.turnState_speedKmh_MinSpeed) {
        if (v_yawAngleVelocity_degps > v_turnCenter_yawAngleVelocityDegps_Thresold) { // 우회전 감지 (양의 각속도)
            if (v_yawAngleVelocity_degps >= v_turnLR_3_yawAngleVelocityDegps_Thresold) {
                v_currentDetectedTurnState = E_M010_CARTURNSTATE_RIGHT_3; // 급격한 우회전
            } else if (v_yawAngleVelocity_degps >= v_turnLR_2_yawAngleVelocityDegps_Thresold) {
                v_currentDetectedTurnState = E_M010_CARTURNSTATE_RIGHT_2; // 중간 우회전
            } else if (v_yawAngleVelocity_degps >= v_turnLR_1_yawAngleVelocityDegps_Thresold) {
                v_currentDetectedTurnState = E_M010_CARTURNSTATE_RIGHT_1; // 약간 우회전
            } else { 
                v_currentDetectedTurnState = E_M010_CARTURNSTATE_CENTER; // 임계값 이하의 미미한 우회전은 직진으로 간주
            }
        } else if (v_yawAngleVelocity_degps < -v_turnCenter_yawAngleVelocityDegps_Thresold) { // 좌회전 감지 (음의 각속도)
            if (v_yawAngleVelocity_degps <= -v_turnLR_3_yawAngleVelocityDegps_Thresold) {
                v_currentDetectedTurnState = E_M010_CARTURNSTATE_LEFT_3; // 급격한 좌회전
            } else if (v_yawAngleVelocity_degps <= -v_turnLR_2_yawAngleVelocityDegps_Thresold) {
                v_currentDetectedTurnState = E_M010_CARTURNSTATE_LEFT_2; // 중간 좌회전
            } else if (v_yawAngleVelocity_degps <= -v_turnLR_1_yawAngleVelocityDegps_Thresold) {
                v_currentDetectedTurnState = E_M010_CARTURNSTATE_LEFT_1; // 약간 좌회전
            } else { 
                v_currentDetectedTurnState = E_M010_CARTURNSTATE_CENTER; // 임계값 이하의 미미한 좌회전은 직진으로 간주
            }
        } else { // 각속도가 '회전 없음' 임계값 범위 내에 있을 경우
            v_currentDetectedTurnState = E_M010_CARTURNSTATE_CENTER;
        }
    } else { // 속도가 최소 회전 감지 속도보다 낮으면 무조건 회전 없음으로 간주 (정지 상태 핸들 조작 등 무시)
        v_currentDetectedTurnState = E_M010_CARTURNSTATE_CENTER;
    }

    // 회전 상태 안정화 로직 (히스테리시스 적용)
    // 감지된 상태가 충분히 오래 지속되어야 실제 상태로 확정합니다.
    if (v_currentDetectedTurnState != g_M010_potentialTurnState) {
        // 감지된 상태가 이전 잠재적 상태와 다르면, 새로운 잠재적 상태로 설정하고 시간 기록 리셋
        g_M010_potentialTurnState = v_currentDetectedTurnState;
        g_M010_turnStateStartTime_ms = p_currentTime_ms;
    } else {
        // 감지된 상태가 충분히 오래 (StableDurationMs) 지속되었는지 확인
        if ((p_currentTime_ms - g_M010_turnStateStartTime_ms) >= g_M010_Config.turnState_StableDurationMs) {
            // 잠재적 상태가 현재 확정된 상태와 다르고 충분히 오래 지속되었다면, 상태 전환
            if (g_M010_potentialTurnState != g_M010_CarStatus.carTurnState) {
                 dbgP1_printf_F(F("Turn State transition: %d -> %d\n"), g_M010_CarStatus.carTurnState, g_M010_potentialTurnState);
            }
            g_M010_CarStatus.carTurnState = g_M010_potentialTurnState; // 회전 상태 확정
        }
    }
}

/**
 * @brief 자동차 상태 정보를 시리얼 모니터로 자세히 출력합니다.
 * 디버깅 및 현재 차량의 움직임, 회전, 감지 상태를 실시간으로 확인하는 데 사용됩니다.
 */
void M010_printCarStatus() {
    dbgP1_println_F(F("\n---- 자동차 현재 상태 ----"));
    dbgP1_print_F(F("상태: "));
    switch (g_M010_CarStatus.carMovementState) {
        case E_M010_CARMOVESTATE_UNKNOWN:      dbgP1_println_F(F("알 수 없음")); break;
        case E_M010_CARMOVESTATE_STOPPED_INIT: dbgP1_println_F(F("정차 중 (기본)")); break;
        case E_M010_CARMOVESTATE_SIGNAL_WAIT1: dbgP1_print_F(F("신호대기 1 (<"));  dbgP1_print(g_M010_Config.mvState_signalWait1_Seconds)      ; dbgP1_print_F(F("s), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_CARMOVESTATE_SIGNAL_WAIT2: dbgP1_print_F(F("신호대기 2 (<"));  dbgP1_print(g_M010_Config.mvState_signalWait2_Seconds)      ; dbgP1_print_F(F("s), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_CARMOVESTATE_STOPPED1:     dbgP1_print_F(F("정차 1 (<"));     dbgP1_print(g_M010_Config.mvState_stopped1_Seconds / 60)     ; dbgP1_print_F(F("분), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_CARMOVESTATE_STOPPED2:     dbgP1_print_F(F("정차 2 (<"));    dbgP1_print(g_M010_Config.mvState_stopped2_Seconds / 60)    ; dbgP1_print_F(F("분), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_CARMOVESTATE_PARKED:       dbgP1_print_F(F("주차 중 (>= "));  dbgP1_print(g_M010_Config.mvState_park_Seconds / 60)        ; dbgP1_print_F(F("분), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_CARMOVESTATE_FORWARD:      dbgP1_println_F(F("전진 중")); break;
        case E_M010_CARMOVESTATE_REVERSE:      dbgP1_println_F(F("후진 중")); break;
    }
    // 새로 추가된 회전 상태 출력
    dbgP1_print_F(F("회전 상태: "));
    switch (g_M010_CarStatus.carTurnState) {
        case E_M010_CARTURNSTATE_CENTER:            dbgP1_println_F(F("직진 또는 정지")); break;
        case E_M010_CARTURNSTATE_LEFT_1:            dbgP1_println_F(F("약간 좌회전")); break;
        case E_M010_CARTURNSTATE_LEFT_2:            dbgP1_println_F(F("중간 좌회전")); break;
        case E_M010_CARTURNSTATE_LEFT_3:            dbgP1_println_F(F("급격한 좌회전")); break;
        case E_M010_CARTURNSTATE_RIGHT_1:           dbgP1_println_F(F("약간 우회전")); break;
        case E_M010_CARTURNSTATE_RIGHT_2:           dbgP1_println_F(F("중간 우회전")); break;
        case E_M010_CARTURNSTATE_RIGHT_3:           dbgP1_println_F(F("급격한 우회전")); break;
    }
    dbgP1_print_F(F("추정 속도: "));                dbgP1_print(g_M010_CarStatus.speed_kmh, 2);         dbgP1_println_F(F(" km/h"));
    dbgP1_print_F(F("가속도(X,Y,Z): "));
    dbgP1_print(g_M010_CarStatus.accelX_ms2, 2);   dbgP1_print_F(F(" m/s^2, "));
    dbgP1_print(g_M010_CarStatus.accelY_ms2, 2);   dbgP1_print_F(F(" m/s^2, "));
    dbgP1_print(g_M010_CarStatus.accelZ_ms2, 2);   dbgP1_println_F(F(" m/s^2"));
    dbgP1_print_F(F("Yaw 각도: "));                 dbgP1_print(g_M010_CarStatus.yawAngle_deg, 2);              dbgP1_println_F(F(" 도"));
    dbgP1_print_F(F("Pitch 각도: "));               dbgP1_print(g_M010_CarStatus.pitchAngle_deg, 2);            dbgP1_println_F(F(" 도"));
    dbgP1_print_F(F("Yaw 각속도: "));               dbgP1_print(g_M010_CarStatus.yawAngleVelocity_degps, 2);    dbgP1_println_F(F(" 도/초"));
    dbgP1_print_F(F("급감속: "));                   dbgP1_println_F(g_M010_CarStatus.isEmergencyBraking ? F("감지됨") : F("아님"));
    dbgP1_print_F(F("과속 방지턱: "));               dbgP1_println_F(g_M010_CarStatus.isSpeedBumpDetected ? F("감지됨") : F("아님"));
    dbgP1_println_F(F("--------------------------"));
}


/**
 * @brief ESP32 메인 루프에서 반복적으로 실행되는 함수입니다.
 * MPU6050 데이터를 지속적으로 읽고, 차량의 움직임 및 회전 상태를 갱신합니다.
 * 또한 시리얼 명령어를 처리하고, 설정된 주기에 따라 현재 상태를 시리얼 출력합니다.
 * `loop()` 함수에서 호출되어야 합니다.
 */
void M010_MPU_run() {
	u_int32_t v_currentTime_ms = 0; // 현재 시간을 저장할 변수

    // MPU6050 데이터 읽기 및 자동차 상태 업데이트
    // 이 함수 내에서 v_currentTime_ms 값이 갱신됩니다.
    M010_updateCarStatus(&v_currentTime_ms); 

    // 새로운 MPU 데이터가 준비되었을 때만 상태 정의 함수들을 호출
    if(g_M010_mpu_isDataReady == true){
        // 자동차 움직임 상태 정의 함수 호출
        M010_defineCarState(v_currentTime_ms);
        // 자동차 회전 상태 정의 함수 호출
        M010_defineCarTurnState(v_currentTime_ms); 
        g_M010_mpu_isDataReady = false; // 데이터 처리 완료 플래그 리셋
    }
    
    // 시리얼 입력 처리 함수 호출 (설정 변경/저장/로드 등)
    M010_Config_handleSerialInput();

    // 설정된 주기(g_M010_Config.serialPrint_intervalMs)에 따라 자동차 상태를 시리얼 출력
    if (millis() - g_M010_lastSerialPrintTime_ms >= g_M010_Config.serialPrint_intervalMs) {
        M010_printCarStatus();
        g_M010_lastSerialPrintTime_ms = millis(); // 마지막 출력 시간 업데이트
    }

    // 이 위치에 LED Matrix 업데이트 등 추가 작업을 구현할 수 있습니다.
    // 예: M010_updateLEDMatrix(g_M010_CarStatus.carMovementState, g_M010_CarStatus.carTurnState);
}
