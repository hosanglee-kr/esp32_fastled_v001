# 프로젝트 개요
ESP32와 MPU6050 센서를 활용, 차량 움직임을 감지하여 LED Matrix에 로봇 눈 표정으로 표현하는 후방 장치 프로젝트. MPU6050 데이터 분석으로 차량의 속도, 가감속, 경사, 회전, 정차, 주차, 방지턱 통과 등 다양한 움직임 상태를 정의하고, 이에 맞춰 로봇 눈 표정을 LED Matrix에 출력.
# 세부 요구사항
1. MPU6050 설정 및 데이터 처리
 * 설치 방향: MPU6050 Y축은 차량 전방, Z축은 위쪽.
 * DMP (Digital Motion Processor) 사용: DMP 기능 활용, 데이터 처리 효율 및 정확성 향상. 쿼터니언 기반 오일러 각 계산으로 정확한 자세 정보 획득.
 * SW 필터 적용: 차량 진동 대응을 위한 소프트웨어 필터(예: 이동 평균, 상보 필터) 적용, MPU6050 데이터 안정성 확보. 쿼터니언 자세 추정으로 기본 안정성 확보, 필요시 가속도 데이터 추가 필터 적용.
2. 자동차 움직임 감지 및 상태 정의
차량의 개략적인 움직임 감지 및 상태 정의.
 * 전진/후진/정차/주차:
   * 정차: 속도 0, 일정 시간(예: 2초) 이상 움직임 없음.
   * 신호대기 1: 정차 0초 이상 60초 미만 지속.
   * 신호대기 2: 정차 60초 이상 120초 미만 지속.
   * 정차 1: 정차 120초 이상 5분(300초) 미만 지속.
   * 정차 2: 정차 5분(300초) 이상 10분(600초) 미만 지속.
   * 주차: 정차 10분(600초) 이상 지속.
   * 전진/후진: 속도 및 Y축 가속도 방향으로 판단.
 * 이동 속도(Km/h): MPU6050 가속도 데이터 적분하여 상대적 속도 변화 추정 (절대 속도와 차이, 오차 누적 가능성 유의).
 * 가속도(m/s^2): MPU6050 Y축(전후) 가속도 및 Z축(상하) 가속도 사용.
 * 좌/우회전 각도: MPU6050 Yaw 각속도(v_yawRate_degps)로 회전 여부 및 방향 감지. Yaw 각(v_currentYawAngle_deg)은 절대 방향 지시.
 * 전후 경사도(도): MPU6050 Pitch 각도(v_pitchAngle_deg)로 오르막/내리막 경사 판단.
 * 급감속 상태: Y축 가속도 값이 급격히 음수로 커질 때 감지.
 * 과속 방지턱 통과 여부: Z축 가속도 값이 순간적으로 크게 변화할 때 감지.
3. 자동차 상태 정보 출력
 * 시리얼 출력: 5초 간격으로 현재 자동차 상태 정보 시리얼 모니터로 출력.
4. 자동차 상태 정의 구조체
제공된 구조체 기반으로 M010_CarStatus 정의 및 소스 코드 반영. 정차 시간 관련 상태 변수 추가.
5. 소스 코드 요구사항
 * 전체 소스 코드: 요구사항 반영된 전체 소스 코드 제공.
 * 주석: 소스 코드 상단에 요구 기능 주석 추가, 코드 내 상세 설명 주석 포함.
 * 명명 규칙:
   * 전역 상수: G_M010_으로 시작
   * 전역 변수: g_M010_으로 시작
   * 로컬 변수: v_로 시작
   * 함수: M010_으로 시작
개발 환경 및 라이브러리
 * MCU: ESP32
 * IMU 센서: MPU6050
 * 개발 환경: PlatformIO Arduino Core
 * MPU6050 라이브러리: MPU6050_DMP6 (Jeff Rowberg의 I2Cdevlib 중 MPU6050 라이브러리)
 * 
# 소스 코드
```cpp
// ====================================================================================================
// 프로젝트: 자동차 후방 로봇 눈
// 설명: ESP32 + MPU6050 활용, 차량 움직임 감지 및 LED Matrix 로봇 눈 표정 표현.
//       본 코드는 MPU6050 데이터 처리 및 자동차 상태 정의.
// 개발 환경: PlatformIO Arduino Core (ESP32)
// 사용 라이브러리: MPU6050_DMP6 (I2Cdevlib by Jeff Rowberg)
// ====================================================================================================

#include <Wire.h> // I2C 통신
#include <I2Cdev.h> // I2C 장치 통신
#include <MPU6050_6Axis_MotionApps20.h> // MPU6050 DMP 기능

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
int16_t g_M010_ax, g_M010_ay, g_M010_az; // Raw 가속도
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

    if (g_M010_devStatus == 0) {
        Serial.println(F("DMP 활성화 중..."));
        g_M010_mpu.setDMPEnabled(true);

        Serial.print(F("MPU6050 인터럽트 핀 (GPIO "));
        Serial.print(G_M010_MPU_INTERRUPT_PIN);
        Serial.println(F(") 설정 중..."));
        pinMode(G_M010_MPU_INTERRUPT_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(G_M010_MPU_INTERRUPT_PIN), M010_dmpDataReady, RISING);
        g_M010_mpuIntStatus = g_M010_mpu.getIntStatus();

        g_M010_packetSize = g_M010_mpu.dmpGetFIFOPacketSize();
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

    while (!g_M010_mpuInterrupt && g_M010_fifoCount < g_M010_packetSize) {} // 데이터 대기

    g_M010_mpuInterrupt = false;
    g_M010_mpuIntStatus = g_M010_mpu.getIntStatus();
    g_M010_fifoCount = g_M010_mpu.getFIFOCount();

    if ((g_M010_mpuIntStatus & 0x10) || g_M010_fifoCount == 1024) {
        g_M010_mpu.resetFIFO();
        Serial.println(F("FIFO 오버플로우!"));
    } else if (g_M010_mpuIntStatus & 0x02) {
        while (g_M010_fifoCount < g_M010_packetSize) g_M010_fifoCount = g_M010_mpu.getFIFOCount();
        g_M010_mpu.getFIFOBytes(g_M010_fifoBuffer, g_M010_packetSize);
        g_M010_fifoCount -= g_M010_packetSize;

        unsigned long v_currentTime_ms = millis();
        float v_deltaTime_s = (v_currentTime_ms - g_M010_lastSampleTime_ms) / 1000.0f;
        g_M010_lastSampleTime_ms = v_currentTime_ms;

        // 쿼터니언, Yaw/Pitch/Roll 계산
        g_M010_mpu.dmpGetQuaternion(&g_M010_q, g_M010_fifoBuffer);
        g_M010_mpu.dmpGetGravity(&g_M010_gravity, &g_M010_q);
        g_M010_mpu.dmpGetYawPitchRoll(g_M010_ypr, &g_M010_q, &g_M010_gravity);
        g_M010_carStatus.v_currentYawAngle_deg = g_M010_ypr[0] * 180 / M_PI;
        g_M010_carStatus.v_pitchAngle_deg = g_M010_ypr[1] * 180 / M_PI;

        // 선형 가속도 (중력분 제거) 및 필터링
        VectorInt16 aa;
        VectorInt16 gv;
        VectorFloat linAccel;
        g_M010_mpu.dmpGetAccel(&aa, g_M010_fifoBuffer);
        g_M010_mpu.dmpGetGravity(&gv, &g_M010_q);
        g_M010_mpu.dmpGetLinearAccel(&linAccel, &aa, &gv);

        float v_currentAx_ms2 = linAccel.x * G_M010_GRAVITY_MPS2;
        float v_currentAy_ms2 = linAccel.y * G_M010_GRAVITY_MPS2;
        float v_currentAz_ms2 = linAccel.z * G_M010_GRAVITY_MPS2;

        g_M010_filteredAx = G_M010_ACCEL_ALPHA * g_M010_filteredAx + (1 - G_M010_ACCEL_ALPHA) * v_currentAx_ms2;
        g_M010_filteredAy = G_M010_ACCEL_ALPHA * g_M010_filteredAy + (1 - G_M010_ACCEL_ALPHA) * v_currentAy_ms2;
        g_M010_filteredAz = G_M010_ACCEL_ALPHA * g_M010_filteredAz + (1 - G_M010_ACCEL_ALPHA) * v_currentAz_ms2;

        g_M010_carStatus.v_accelerationX_ms2 = g_M010_filteredAx;
        g_M010_carStatus.v_accelerationY_ms2 = g_M010_filteredAy;
        g_M010_carStatus.v_accelerationZ_ms2 = g_M010_filteredAz;

        // Yaw 각속도 (Z축 자이로)
        int16_t v_gx, v_gy, v_gz;
        g_M010_mpu.dmpGetGyro(&v_gx, &v_gy, &v_gz, g_M010_fifoBuffer);
        g_M010_yawRateDegPs = (float)v_gz / 131.0f; // 131 LSB/deg/s @ +/-250 deg/s
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
void setup() {
    Serial.begin(115200); // 시리얼 통신 시작
    while (!Serial); // 시리얼 모니터 연결 대기

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
void loop() {
    M010_updateCarStatus(); // MPU6050 데이터 및 상태 업데이트

    // 5초 간격 시리얼 출력
    if (millis() - g_M010_lastSerialPrintTime_ms >= G_M010_SERIAL_PRINT_INTERVAL_MS) {
        M010_printCarStatus();
        g_M010_lastSerialPrintTime_ms = millis();
    }

    // LED Matrix 업데이트 등 추가 작업 (예: M010_updateLEDMatrix(g_M010_carStatus.v_currentMovementState);)
}
```

# 추가 고려사항 및 개선 방향 (간결한 어체)
 * MPU6050 센서 퓨전 및 드리프트:
   * MPU6050만으로는 속도/위치 추정의 한계 (오차 누적). 장거리 운행 시 GPS 등 외부 센서와의 센서 퓨전 고려.
   * Yaw 각도 드리프트 발생 가능. 자기장 센서 포함 IMU(예: MPU9250) 사용 시 Yaw 각 절대 방향 유지에 유리.
 * LED Matrix 제어:
   * LED Matrix 라이브러리(LedControl, FastLED 등) 추가 및 M010_updateLEDMatrix 함수 구현 필요.
   * g_M010_carStatus.v_currentMovementState 값에 따라 로봇 눈 표정을 매핑하여 LED Matrix에 표시하는 로직 작성.
 * 오프셋 및 교정:
   * MPU6050 가속도/자이로 오프셋 교정 필요 (setXGyroOffset, setZAccelOffset 등).
   * MPU6050_DMP6 라이브러리 예제 참고하여 오프셋 값 찾기.
 * 임계값 튜닝:
   * 소스 코드 내 임계값 상수들(예: G_M010_SPEED_THRESHOLD_KMH)은 실제 차량 및 주행 환경에 맞춰 테스트를 통한 조정이 필수.
 * 전력 관리:
   * ESP32와 MPU6050 전력 소모 고려. 장시간 사용 시 안정적인 전원 공급 필수.
이 코드가 프로젝트 시작에 도움이 되길 바랍니다. 추가 문의 사항이 있다면 언제든지 알려주세요.
