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

// MPU6050 객체 생성
MPU6050 g_M010_Mpu;

// ====================================================================================================
// 전역 상수 (G_M010_으로 시작) 정의
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

// MPU6050 관련 상수
const int G_M010_MPU_INTERRUPT_PIN = 4; // MPU6050 INT 핀이 ESP32 GPIO 4에 연결

const float G_M010_ACCEL_ALPHA = 0.8;   // 가속도 필터링을 위한 상보 필터 계수
const float G_M010_GRAVITY_MPS2 = 9.80665; // 중력 가속도 값 (m/s^2)

// 자동차 움직임 상태 감지 임계값 (상태 머신 전이 조건)
const float G_M010_SPEED_FORWARD_THRESHOLD_KMH = 0.8; // 정지 상태에서 전진 상태로 전환하기 위한 최소 속도 (km/h)
const float G_M010_SPEED_REVERSE_THRESHOLD_KMH = 0.8; // 정지 상태에서 후진 상태로 전환하기 위한 최소 속도 (km/h)
const float G_M010_SPEED_STOP_THRESHOLD_KMH = 0.2;    // 전진/후진 상태에서 정지 상태로 전환하기 위한 최대 속도 (km/h)

const float G_M010_ACCEL_STOP_THRESHOLD_MPS2 = 0.2; // 차량 정차 여부 판단을 위한 Y축 가속도 변화 임계값 (m/s^2)
const float G_M010_GYRO_STOP_THRESHOLD_DPS = 0.5;   // 차량 정차 여부 판단을 위한 Yaw 각속도 변화 임계값 (deg/s)

const unsigned long G_M010_STOP_STABLE_DURATION_MS = 200;  // 정지 상태가 안정적으로 지속되어야 하는 시간 (ms)
const unsigned long G_M010_MOVE_STABLE_DURATION_MS = 150;  // 움직임이 안정적으로 감지되어야 하는 시간 (ms)
const unsigned long G_M010_NORMAL_MOVE_DURATION_MS = 100; // 특수 상태 해제 후 일반 움직임으로 복귀하는 데 필요한 시간 (ms)

// 급감속/방지턱 감지 관련 임계값
const float G_M010_ACCEL_DECEL_THRESHOLD_MPS2 = -3.0; // 급감속 감지를 위한 Y축 가속도 임계값 (음수 값, m/s^2)
const float G_M010_ACCEL_BUMP_THRESHOLD_MPS2 = 5.0; // 방지턱 감지를 위한 Z축 가속도 변화 임계값 (m/s^2)
const float G_M010_BUMP_MIN_SPEED_KMH = 5.0; // 방지턱 감지 시 필요한 최소 속도 (km/h)

const unsigned long G_M010_BUMP_COOLDOWN_MS = 1000; // 방지턱 감지 후 재감지를 방지하는 쿨다운 시간 (ms)
const unsigned long G_M010_DECEL_HOLD_DURATION_MS = 10000; // 급감속 상태가 유지되는 시간 (ms, 10초)
const unsigned long G_M010_BUMP_HOLD_DURATION_MS = 10000;  // 과속방지턱 감지 상태가 유지되는 시간 (ms, 10초)

// 정차/주차 시간 기준 (초)
const unsigned long G_M010_STOP_GRACE_PERIOD_MS = 2000; // 2초 이상 움직임 없으면 정차로 간주 (현재 사용되지 않음, 참고용)
const unsigned long G_M010_SIGNAL_WAIT1_SECONDS = 60;   // 신호대기 1 상태 기준 시간 (60초)
const unsigned long G_M010_SIGNAL_WAIT2_SECONDS = 120;  // 신호대기 2 상태 기준 시간 (120초)
const unsigned long G_M010_STOP1_SECONDS = 300;         // 정차 1 상태 기준 시간 (300초 = 5분)
const unsigned long G_M010_STOP2_SECONDS = 600;         // 정차 2 상태 기준 시간 (600초 = 10분)
const unsigned long G_M010_PARK_SECONDS = 600;          // 주차 상태 기준 시간 (600초 = 10분)

// 시리얼 출력 주기 (ms)
const unsigned long G_M010_SERIAL_PRINT_INTERVAL_MS = 5000; // 시리얼 출력 간격 (5초)

// ====================================================================================================
// 새로운 회전 감지 관련 상수 정의
// ====================================================================================================
const float G_M010_TURN_NONE_THRESHOLD_DPS = 5.0;     // 이 각속도 이하이면 회전이 없다고 간주 (deg/s)
const float G_M010_TURN_SLIGHT_THRESHOLD_DPS = 15.0;  // 약간 회전 감지 임계값 (deg/s)
const float G_M010_TURN_MODERATE_THRESHOLD_DPS = 30.0; // 중간 회전 감지 임계값 (deg/s)
const float G_M010_TURN_SHARP_THRESHOLD_DPS = 50.0;   // 급격한 회전 감지 임계값 (deg/s)

const float G_M010_TURN_MIN_SPEED_KMH = 1.0;          // 회전 감지를 위한 최소 속도 (km/h) - 정지 시 오인 방지
const float G_M010_TURN_HIGH_SPEED_THRESHOLD_KMH = 30.0; // 고속 회전 감지 시 임계값 조정을 위한 기준 속도 (km/h)

const unsigned long G_M010_TURN_STABLE_DURATION_MS = 100; // 회전 상태가 안정적으로 유지되어야 하는 시간 (ms)

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

// ====================================================================================================
// 함수 정의 (M010_으로 시작)
// ====================================================================================================

/**
 * @brief MPU6050 센서 및 DMP (Digital Motion Processor)를 초기화합니다.
 */
void M010_setupMPU6050() {
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
        float v_currentAx_ms2 = (float)linAccel.x * G_M010_GRAVITY_MPS2;
        float v_currentAy_ms2 = (float)linAccel.y * G_M010_GRAVITY_MPS2;
        float v_currentAz_ms2 = (float)linAccel.z * G_M010_GRAVITY_MPS2;

        // 상보 필터를 사용하여 가속도 데이터 평활화
        g_M010_filteredAx = G_M010_ACCEL_ALPHA * g_M010_filteredAx + (1 - G_M010_ACCEL_ALPHA) * v_currentAx_ms2;
        g_M010_filteredAy = G_M010_ACCEL_ALPHA * g_M010_filteredAy + (1 - G_M010_ACCEL_ALPHA) * v_currentAy_ms2;
        g_M010_filteredAz = G_M010_ACCEL_ALPHA * g_M010_filteredAz + (1 - G_M010_ACCEL_ALPHA) * v_currentAz_ms2;

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
        if (fabs(g_M010_CarStatus.accelY_ms2) < G_M010_ACCEL_STOP_THRESHOLD_MPS2 &&
            fabs(g_M010_CarStatus.yawAngleVelocity_degps) < G_M010_GYRO_STOP_THRESHOLD_DPS) {
            
            if (g_M010_CarStatus.stopStableStartTime_ms == 0) { // 정지 안정화 시작 시간 기록
                g_M010_CarStatus.stopStableStartTime_ms = v_currentTime_ms;
            } else if ((v_currentTime_ms - g_M010_CarStatus.stopStableStartTime_ms) >= G_M010_STOP_STABLE_DURATION_MS) {
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
    if (fabs(v_accelZ) > G_M010_ACCEL_BUMP_THRESHOLD_MPS2 && // Z축 가속도 임계값 초과
        fabs(v_speed) > G_M010_BUMP_MIN_SPEED_KMH &&         // 최소 속도 이상
        (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) > G_M010_BUMP_COOLDOWN_MS) { // 쿨다운 시간 경과
        g_M010_CarStatus.isSpeedBumpDetected = true;
        g_M010_lastBumpDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isSpeedBumpDetected &&
               (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) >= G_M010_BUMP_HOLD_DURATION_MS) {
        g_M010_CarStatus.isSpeedBumpDetected = false; // 유지 시간 후 플래그 리셋
    }

    // =============================================================================================
    // 2. 급감속 감지 (일시적 플래그, 메인 상태와 독립적으로 작동)
    // =============================================================================================
    if (v_accelY < G_M010_ACCEL_DECEL_THRESHOLD_MPS2) { // Y축 가속도가 급감속 임계값 미만
        g_M010_CarStatus.isEmergencyBraking = true;
        g_M010_lastDecelDetectionTime_ms = p_currentTime_ms; // 감지 시간 업데이트
    } else if (g_M010_CarStatus.isEmergencyBraking &&
               (p_currentTime_ms - g_M010_lastDecelDetectionTime_ms) >= G_M010_DECEL_HOLD_DURATION_MS) {
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
            if (v_speed > G_M010_SPEED_FORWARD_THRESHOLD_KMH) { // 전진 시작 조건 (히스테리시스 상단)
                if (g_M010_stateTransitionStartTime_ms == 0) { // 조건 만족 시작 시간 기록
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
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

        // 현재가 '움직임' 상태일 때, '정지' 상태로의 전환 조건 확인
        case E_M010_STATE_FORWARD:
        case E_M010_STATE_REVERSE:
            if (fabs(v_speed) < G_M010_SPEED_STOP_THRESHOLD_KMH) { // 정지 조건 (히스테리시스 하단)
                if (g_M010_stateTransitionStartTime_ms == 0) { // 조건 만족 시작 시간 기록
                    g_M010_stateTransitionStartTime_ms = p_currentTime_ms;
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
    float v_turnNoneThreshold = G_M010_TURN_NONE_THRESHOLD_DPS;
    float v_turnSlightThreshold = G_M010_TURN_SLIGHT_THRESHOLD_DPS;
    float v_turnModerateThreshold = G_M010_TURN_MODERATE_THRESHOLD_DPS;
    float v_turnSharpThreshold = G_M010_TURN_SHARP_THRESHOLD_DPS;

    // 고속 주행 시 임계값을 약간 낮춰 작은 각속도 변화에도 민감하게 반응
    if (v_speed > G_M010_TURN_HIGH_SPEED_THRESHOLD_KMH) {
        v_turnNoneThreshold *= 0.8; 
        v_turnSlightThreshold *= 0.8;
        v_turnModerateThreshold *= 0.8;
        v_turnSharpThreshold *= 0.8;
    } 
    // 저속 주행 시(G_M010_TURN_MIN_SPEED_KMH 에 가까울수록) 임계값을 높여 불필요한 감지 방지
    // (예: 정지 상태에서 핸들만 돌리는 경우 등)
    else if (v_speed < G_M010_TURN_MIN_SPEED_KMH + 3.0) { // 최소 속도 + 3km/h 범위
        v_turnNoneThreshold *= 1.2; 
        v_turnSlightThreshold *= 1.2;
        v_turnModerateThreshold *= 1.2;
        v_turnSharpThreshold *= 1.2;
    }


    // 최소 속도 이상일 때만 회전 감지 로직 활성화
    if (v_speed >= G_M010_TURN_MIN_SPEED_KMH) {
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
        if ((p_currentTime_ms - s_turnStateStartTime_ms) >= G_M010_TURN_STABLE_DURATION_MS) {
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
        case E_M010_STATE_SIGNAL_WAIT1: dbgP1_print_F(F("신호대기 1 (60초 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_SIGNAL_WAIT2: dbgP1_print_F(F("신호대기 2 (120초 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_STOPPED1: dbgP1_print_F(F("정차 1 (5분 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_STOPPED2: dbgP1_print_F(F("정차 2 (10분 미만), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
        case E_M010_STATE_PARKED: dbgP1_print_F(F("주차 중 (10분 이상), 시간: ")); dbgP1_print(g_M010_CarStatus.currentStopTime_ms / 1000); dbgP1_println_F(F("s")); break;
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
 * @brief ESP32가 시작될 때 한 번 실행되는 초기 설정 함수입니다.
 * MPU6050 초기화 및 모든 전역 상태 변수를 설정합니다.
 */
void M010_MPU_init() {
    
    M010_setupMPU6050(); // MPU6050 센서 초기화

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

    dbgP1_println_F(F("Setup 완료!"));
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
    // 5초 간격으로 자동차 상태를 시리얼 출력
    if (millis() - g_M010_lastSerialPrintTime_ms >= G_M010_SERIAL_PRINT_INTERVAL_MS) {
        M010_printCarStatus();
        g_M010_lastSerialPrintTime_ms = millis();
    }

    // 이 위치에 LED Matrix 업데이트 등 추가 작업을 구현할 수 있습니다.
    // 예: M010_updateLEDMatrix(g_M010_CarStatus.movementState, g_M010_CarStatus.turnState);
}
