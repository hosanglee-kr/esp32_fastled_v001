
# 프로젝트 개요
이 프로젝트는 ESP32와 MPU6050 센서를 활용하여 자동차의 움직임을 실시간으로 감지하고, 이 정보를 기반으로 LED Matrix에 로봇 눈의 표정을 표현하는 장치를 자동차 후방에 설치하는 것을 목표로 합니다. MPU6050의 가속도계와 자이로스코프 데이터를 분석하여 자동차의 속도, 가속/감속, 경사, 회전, 정차, 주차, 과속 방지턱 통과 등 다양한 움직임 상태를 정의하고, 이 상태에 따라 적절한 로봇 눈 표정을 LED Matrix에 출력합니다.
세부 요구사항
1. MPU6050 설정 및 데이터 처리
 * 설치 방향: MPU6050은 전방(자동차 진행 방향)을 Y축, 위쪽을 Z축으로 하여 설치됩니다.
 * DMP (Digital Motion Processor) 사용: MPU6050의 DMP 기능을 활용하여 데이터 처리의 효율성과 정확성을 높입니다. 쿼터니언(Quaternion) 데이터를 사용하여 오일러 각(Euler Angle)을 계산, 정확한 자세 정보를 얻습니다.
 * SW 필터 적용: 자동차의 진동에 대응할 수 있는 적절한 소프트웨어 필터(예: 이동 평균 필터, 상보 필터 등)를 적용하여 MPU6050 데이터의 안정성을 확보합니다. 여기서는 쿼터니언 기반의 자세 추정으로 기본적인 안정성을 확보하며, 필요시 가속도 데이터에 추가 필터를 적용할 수 있습니다.
2. 자동차 움직임 감지 및 상태 정의
다음과 같은 자동차의 개략적인 움직임을 감지하고 상태를 정의합니다.
 * 전진/후진/정차/주차:
   * 정차: 속도가 0이고, 일정 시간(예: 2초) 이상 움직임이 없는 상태.
   * 신호대기 1: 정차 상태가 0초 이상 60초 미만 지속.
   * 신호대기 2: 정차 상태가 60초 이상 120초 미만 지속.
   * 정차 1: 정차 상태가 120초 이상 5분(300초) 미만 지속.
   * 정차 2: 정차 상태가 5분(300초) 이상 10분(600초) 미만 지속.
   * 주차: 정차 상태가 10분(600초) 이상 지속.
   * 전진/후진: 속도 및 Y축 가속도 방향을 통해 판단합니다.
 * 이동 속도(Km/h): GPS 모듈이 없으므로, MPU6050의 가속도 데이터를 시간에 따라 적분하여 상대적인 속도 변화를 추정할 수 있습니다. 이는 절대적인 속도(Km/h)와는 차이가 있을 수 있으며, 오차가 누적될 수 있음에 유의해야 합니다. 여기서는 편의상 v_currentSpeed_kmh로 명시하지만, 실제 구현에서는 가속도 적분을 통한 속도 변화량으로 해석해야 합니다.
 * 가속도(m/s^2): MPU6050의 Y축(전후) 가속도와 Z축(상하) 가속도를 사용합니다.
 * 좌/우회전 각도: MPU6050의 Yaw 각속도(v_yawRate_degps)를 통해 차량의 회전 여부와 방향을 감지합니다. Yaw 각(v_currentYawAngle_deg)은 절대적인 방향을 나타냅니다.
 * 전후 경사도(도): MPU6050의 Pitch 각도(v_pitchAngle_deg)를 통해 오르막/내리막 경사를 판단합니다.
 * 급감속 상태: Y축 가속도 값이 급격히 음수로 커질 때 감지합니다.
 * 과속 방지턱 통과 여부: Z축 가속도 값이 순간적으로 크게 변화할 때 감지합니다.
3. 자동차 상태 정보 출력
 * 시리얼 출력: 5초 간격으로 현재 자동차의 상태 정보를 시리얼 모니터로 출력합니다.
4. 자동차 상태 정의 구조체
제공해주신 구조체를 바탕으로 자동차의 상태 정보를 종합하여 M010_CarStatus 구조체를 정의하고 소스 코드에 반영합니다. 기존 구조체에 정차 시간 관련 상태 변수를 추가합니다.
5. 소스 코드 요구사항
 * 전체 소스 코드: 요구사항을 모두 반영한 전체 소스 코드를 제공합니다.
 * 주석: 소스 코드 앞 부분에 요구 기능에 대한 주석을 추가하고, 코드 내에 상세 설명을 위한 주석을 충분히 포함합니다.
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
소스 코드
아래는 ESP32와 MPU6050을 사용하여 자동차 움직임을 감지하고 상태를 정의하는 예시 소스 코드입니다. 이 코드는 MPU6050의 DMP 기능을 활용하며, 가속도 및 자이로 데이터를 기반으로 자동차의 다양한 상태를 추정합니다. LED Matrix 관련 코드는 포함되지 않았습니다.
// ====================================================================================================
// 프로젝트: 자동차 후방 로봇 눈
// 설명: ESP32와 MPU6050을 사용하여 자동차의 움직임(속도, 가속/감속, 경사, 회전 등)을 감지하고
//       이 정보를 기반으로 LED Matrix에 로봇 눈의 표정을 표현하는 장치입니다.
//       이 코드는 MPU6050 데이터를 처리하고 자동차 상태를 정의하는 부분을 담당합니다.
// 개발 환경: PlatformIO Arduino Core (ESP32)
// 사용 라이브러리: MPU6050_DMP6 (I2Cdevlib by Jeff Rowberg)
// 최종 수정일: 2025-06-02
// ====================================================================================================

#include <Wire.h>             // I2C 통신을 위한 라이브러리
#include <I2Cdev.h>           // I2C 장치 통신을 위한 라이브러리
#include <MPU6050_6Axis_MotionApps20.h> // MPU6050 DMP 기능을 위한 라이브러리 (Jeff Rowberg)

// MPU6050 객체 생성
MPU6050 g_M010_mpu;

// ====================================================================================================
// 전역 상수 정의 (G_M010_으로 시작)
// ====================================================================================================

// MPU6050 관련 상수
const int G_M010_MPU_INTERRUPT_PIN = 4; // MPU6050 INT 핀을 ESP32 GPIO 4에 연결
const float G_M010_ACCEL_ALPHA = 0.8;   // 가속도 데이터 필터링을 위한 상보 필터 계수 (낮을수록 평활)
const float G_M010_GRAVITY_MPS2 = 9.80665; // 중력 가속도 (m/s^2)

// 자동차 상태 감지 임계값
const float G_M010_SPEED_THRESHOLD_KMH = 0.5; // 속도 임계값 (km/h): 이 값 이하면 정차로 간주
const float G_M010_ACCEL_STOP_THRESHOLD_MPS2 = 0.5; // 정차 감지를 위한 가속도 변화 임계값 (m/s^2)
const float G_M010_ACCEL_DECEL_THRESHOLD_MPS2 = -3.0; // 급감속 감지를 위한 Y축 가속도 임계값 (m/s^2)
const float G_M010_ACCEL_BUMP_THRESHOLD_MPS2 = 5.0; // 과속 방지턱 감지를 위한 Z축 가속도 변화 임계값 (m/s^2)
const unsigned long G_M010_BUMP_COOLDOWN_MS = 1000; // 과속 방지턱 감지 후 쿨다운 시간 (ms)

// 정차 및 주차 시간 기준 (초)
const unsigned long G_M010_STOP_GRACE_PERIOD_MS = 2000; // 2초 이상 움직임 없으면 정차로 간주
const unsigned long G_M010_SIGNAL_WAIT1_SECONDS = 60;   // 신호대기 1 기준 (초)
const unsigned long G_M010_SIGNAL_WAIT2_SECONDS = 120;  // 신호대기 2 기준 (초)
const unsigned long G_M010_STOP1_SECONDS = 300;         // 정차 1 기준 (5분)
const unsigned long G_M010_STOP2_SECONDS = 600;         // 정차 2 기준 (10분)
const unsigned long G_M010_PARK_SECONDS = 600;          // 주차 기준 (10분)

// 시리얼 출력 주기 (밀리초)
const unsigned long G_M010_SERIAL_PRINT_INTERVAL_MS = 5000;

// ====================================================================================================
// 자동차의 현재 움직임 상태를 정의하는 열거형 (enum)
// ====================================================================================================
enum CarMovementState {
    G_M010_STATE_UNKNOWN,       // 0: 초기 상태 또는 현재 상태를 알 수 없는 경우
    G_M010_STATE_STOPPED_INIT,  // 1: 정차 초기 (아직 구체적인 정차 시간 미지정)
    G_M010_STATE_SIGNAL_WAIT1,  // 2: 신호대기 1 (60초 미만 정차)
    G_M010_STATE_SIGNAL_WAIT2,  // 3: 신호대기 2 (60초 이상 120초 미만 정차)
    G_M010_STATE_STOPPED1,      // 4: 정차 1 (120초 이상 5분 미만 정차)
    G_M010_STATE_STOPPED2,      // 5: 정차 2 (5분 이상 10분 미만 정차)
    G_M010_STATE_PARKED,        // 6: 주차 중 (10분 이상 정지)
    G_M010_STATE_FORWARD,       // 7: 전진 중
    G_M010_STATE_REVERSE,       // 8: 후진 중
    G_M010_STATE_DECELERATING,  // 9: 급감속 중
    G_M010_STATE_SPEED_BUMP     // 10: 과속 방지턱 통과 중
};

// ====================================================================================================
// 자동차의 현재 상태 정보를 담는 구조체 (struct)
// ====================================================================================================
struct M010_CarStatus {
    // 1. 자동차 움직임 상태 관련
    CarMovementState v_currentMovementState; // 현재 자동차의 움직임 상태

    // 2. 속도 및 가속도 관련
    float v_currentSpeed_kmh;      // 현재 자동차의 속도 (km/h 단위, 가속도 적분을 통한 상대 속도 변화)
                                   // 양수(+)는 전진, 음수(-)는 후진 (추정)
    float v_accelerationX_ms2;     // 자동차의 좌우(X축) 가속도(m/s^2)
    float v_accelerationY_ms2;     // 자동차의 전후(Y축) 가속도(m/s^2), 양수: 가속, 음수: 감속 (전진 기준)
    float v_accelerationZ_ms2;     // 자동차의 상하(Z축) 가속도(m/s^2), 노면 충격 감지에 사용

    // 3. 회전 관련
    float v_currentYawAngle_deg;   // 현재 자동차의 Yaw 각도 (도)
    float v_yawRate_degps;         // 현재 자동차의 Yaw 각속도 (도/초), 양수: 우회전, 음수: 좌회전

    // 4. 경사도 관련
    float v_pitchAngle_deg;        // 현재 자동차의 Pitch 각도 (도), 양수: 오르막, 음수: 내리막

    // 5. 특이 상황 관련
    bool v_speedBumpDetected;      // 과속 방지턱 통과 감지 여부 (true: 감지)
    bool v_isEmergencyBraking;     // 급감속 상태 감지 여부 (true: 감지)

    // 6. 시간 관련 (정차 및 주차 상태 판별용)
    unsigned long v_lastMovementTime_ms; // 마지막으로 움직임이 감지된 시간 (ms)
    unsigned long v_stopStartTime_ms;    // 정차 시작 시간 (ms), 움직이다가 멈췄을 때 기록
    unsigned long v_currentStopTime_ms;  // 현재 정차 지속 시간 (ms)
};

// ====================================================================================================
// 전역 변수 정의 (g_M010_으로 시작)
// ====================================================================================================
M010_CarStatus g_M010_carStatus; // 자동차 상태 구조체 인스턴스

// MPU6050 DMP 관련 변수
bool g_M010_dmpReady = false;  // DMP 초기화 완료 여부
uint8_t g_M010_mpuIntStatus;   // MPU6050 인터럽트 상태 레지스터 값
uint8_t g_M010_devStatus;     // 장치 상태 (0 = 성공, >0 = 오류)
uint16_t g_M010_packetSize;   // DMP 패킷 크기
uint16_t g_M010_fifoCount;    // FIFO의 바이트 수
uint8_t g_M010_fifoBuffer[64]; // FIFO 버퍼

// 쿼터니언 및 오일러 각 변수
Quaternion g_M010_q;          // 쿼터니언
VectorFloat g_M010_gravity;   // 중력 벡터
float g_M010_ypr[3];          // Yaw, Pitch, Roll (radian)
float g_M010_yawRateDegPs;    // Yaw 각속도 (deg/s)

// 가속도 데이터 변수 (raw 및 필터링)
int16_t g_M010_ax, g_M010_ay, g_M010_az; // Raw 가속도
float g_M010_filteredAx, g_M010_filteredAy, g_M010_filteredAz; // 필터링된 가속도 (m/s^2)

// 시간 관련 변수
unsigned long g_M010_lastSampleTime_ms = 0; // 마지막 MPU6050 샘플링 시간
unsigned long g_M010_lastSerialPrintTime_ms = 0; // 마지막 시리얼 출력 시간
unsigned long g_M010_lastBumpDetectionTime_ms = 0; // 마지막 과속 방지턱 감지 시간

volatile bool g_M010_mpuInterrupt = false; // MPU6050 인터럽트 발생 여부
void M010_dmpDataReady() {
    g_M010_mpuInterrupt = true;
}

// ====================================================================================================
// 함수 정의 (M010_으로 시작)
// ====================================================================================================

/**
 * @brief MPU6050을 초기화하고 DMP를 설정합니다.
 */
void M010_setupMPU6050() {
    // I2C 버스 시작 (SDA=GPIO21, SCL=GPIO22 for ESP32 default)
    Wire.begin();
    Wire.setClock(400000); // I2C 속도를 400kHz로 설정

    Serial.println(F("MPU6050 초기화 중..."));

    // MPU6050 연결 테스트
    g_M010_mpu.initialize();
    Serial.print(F("MPU6050 연결 테스트: "));
    Serial.println(g_M010_mpu.testConnection() ? F("성공") : F("실패"));

    // DMP 로드
    Serial.println(F("DMP 로딩 중..."));
    g_M010_devStatus = g_M010_mpu.dmpInitialize();

    // 공급 전압/오프셋 설정 (필요시 교정)
    // g_M010_mpu.setXGyroOffset(220);
    // g_M010_mpu.setYGyroOffset(76);
    // g_M010_mpu.setZGyroOffset(-85);
    // g_M010_mpu.setZAccelOffset(1788);

    if (g_M010_devStatus == 0) {
        // DMP 활성화
        Serial.println(F("DMP 활성화 중..."));
        g_M010_mpu.setDMPEnabled(true);

        // 인터럽트 핀 설정
        Serial.print(F("MPU6050 인터럽트 핀 (GPIO "));
        Serial.print(G_M010_MPU_INTERRUPT_PIN);
        Serial.println(F(") 설정 중..."));
        pinMode(G_M010_MPU_INTERRUPT_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(G_M010_MPU_INTERRUPT_PIN), M010_dmpDataReady, RISING);
        g_M010_mpuIntStatus = g_M010_mpu.getIntStatus();

        // DMP 패킷 크기 확인
        g_M010_packetSize = g_M010_mpu.dmpGetFIFOPacketSize();
        g_M010_dmpReady = true;

        Serial.println(F("DMP 초기화 완료!"));
    } else {
        // DMP 초기화 실패 오류 코드 출력
        Serial.print(F("DMP 초기화 실패 (오류 코드: "));
        Serial.print(g_M010_devStatus);
        Serial.println(F(")"));
        while (true); // 오류 발생 시 무한 대기
    }
}

/**
 * @brief MPU6050 데이터를 읽고 처리하여 자동차 상태를 업데이트합니다.
 */
void M010_updateCarStatus() {
    // DMP가 준비되지 않았으면 아무것도 하지 않음
    if (!g_M010_dmpReady) return;

    // 인터럽트가 발생하지 않았거나 FIFO에 데이터가 없으면 대기
    while (!g_M010_mpuInterrupt && g_M010_fifoCount < g_M010_packetSize) {
        // yield(); // ESP32의 다른 작업을 위해 양보
    }

    g_M010_mpuInterrupt = false;
    g_M010_mpuIntStatus = g_M010_mpu.getIntStatus();
    g_M010_fifoCount = g_M010_mpu.getFIFOCount();

    // FIFO 버퍼 오버플로우 처리
    if ((g_M010_mpuIntStatus & 0x10) || g_M010_fifoCount == 1024) {
        g_M010_mpu.resetFIFO();
        Serial.println(F("FIFO 오버플로우!"));
    }
    // DMP 데이터 준비 완료
    else if (g_M010_mpuIntStatus & 0x02) {
        // FIFO에서 패킷 읽기
        while (g_M010_fifoCount < g_M010_packetSize) g_M010_fifoCount = g_M010_mpu.getFIFOCount();
        g_M010_mpu.getFIFOBytes(g_M010_fifoBuffer, g_M010_packetSize);
        g_M010_fifoCount -= g_M010_packetSize;

        // 현재 시간 기록 (샘플링 시간)
        unsigned long v_currentTime_ms = millis();
        float v_deltaTime_s = (v_currentTime_ms - g_M010_lastSampleTime_ms) / 1000.0f;
        g_M010_lastSampleTime_ms = v_currentTime_ms;

        // 쿼터니언 데이터 가져오기
        g_M010_mpu.dmpGetQuaternion(&g_M010_q, g_M010_fifoBuffer);
        // Yaw, Pitch, Roll 각도 계산 (radian)
        g_M010_mpu.dmpGetGravity(&g_M010_gravity, &g_M010_q);
        g_M010_mpu.dmpGetYawPitchRoll(g_M010_ypr, &g_M010_q, &g_M010_gravity);

        // 오일러 각도 (도)로 변환
        g_M010_carStatus.v_currentYawAngle_deg = g_M010_ypr[0] * 180 / M_PI;
        g_M010_carStatus.v_pitchAngle_deg = g_M010_ypr[1] * 180 / M_PI;
        // Roll 각도는 자동차 기울기 판단에 크게 중요하지 않으므로 사용 안 함

        // 가속도 데이터 가져오기 (m/s^2 단위로 변환 및 필터링)
        g_M010_mpu.dmpGetAccel(&g_M010_ax, &g_M010_ay, &g_M010_az, g_M010_fifoBuffer);
        // 가속도계 데이터는 기본적으로 g 단위로 나오므로, m/s^2로 변환 (MPU6050_6Axis_MotionApps20 라이브러리에서 스케일링을 이미 처리함)
        // 여기서는 Raw 값에서 스케일링된 값을 받아오므로 중력가속도로 나누어 m/s^2로 변환합니다.
        // MPU6050의 가속도 데이터는 대략 16384 LSB/g 이므로, raw_accel / 16384.0 * G_M010_GRAVITY_MPS2 로 변환 가능.
        // Jeff Rowberg 라이브러리의 dmpGetAccel은 이미 적절히 스케일링된 값을 제공하므로, 여기서는 바로 사용.
        // 단, MPU6050의 가속도계는 중력 가속도를 포함하고 있으므로, 이를 제거해야 순수 가속도(linear acceleration)를 얻을 수 있습니다.
        // DMP의 getLinearAccel() 함수를 사용하면 중력분이 제거된 선형 가속도를 얻을 수 있습니다.
        VectorInt16 aa;
        VectorInt16 gv;
        VectorFloat linAccel;
        g_M010_mpu.dmpGetAccel(&aa, g_M010_fifoBuffer);
        g_M010_mpu.dmpGetGravity(&gv, &g_M010_q); // 이 함수는 내부적으로 G_M010_gravity를 설정하지 않고 중력벡터만 계산합니다.
        g_M010_mpu.dmpGetLinearAccel(&linAccel, &aa, &gv); // aa: raw accel, gv: gravity vector

        // DMP에서 얻은 선형 가속도는 G 단위이므로 m/s^2으로 변환합니다.
        float v_currentAx_ms2 = linAccel.x * G_M010_GRAVITY_MPS2;
        float v_currentAy_ms2 = linAccel.y * G_M010_GRAVITY_MPS2;
        float v_currentAz_ms2 = linAccel.z * G_M010_GRAVITY_MPS2;

        // 가속도 데이터 필터링 (간단한 이동 평균 필터 또는 상보 필터)
        // 여기서는 이전 값과의 상보 필터를 적용하여 노이즈를 줄입니다.
        g_M010_filteredAx = G_M010_ACCEL_ALPHA * g_M010_filteredAx + (1 - G_M010_ACCEL_ALPHA) * v_currentAx_ms2;
        g_M010_filteredAy = G_M010_ACCEL_ALPHA * g_M010_filteredAy + (1 - G_M010_ACCEL_ALPHA) * v_currentAy_ms2;
        g_M010_filteredAz = G_M010_ACCEL_ALPHA * g_M010_filteredAz + (1 - G_M010_ACCEL_ALPHA) * v_currentAz_ms2;

        g_M010_carStatus.v_accelerationX_ms2 = g_M010_filteredAx;
        g_M010_carStatus.v_accelerationY_ms2 = g_M010_filteredAy;
        g_M010_carStatus.v_accelerationZ_ms2 = g_M010_filteredAz;

        // 자이로스코프 데이터 (각속도) 가져오기
        int16_t v_gx, v_gy, v_gz;
        g_M010_mpu.dmpGetGyro(&v_gx, &v_gy, &v_gz, g_M010_fifoBuffer);
        // 각속도 데이터 스케일링 (MPU6050의 자이로 스케일 팩터는 131 LSB/deg/s @ +/-250 deg/s)
        // Yaw 각속도는 Z축에 해당
        g_M010_yawRateDegPs = (float)v_gz / 131.0f;
        g_M010_carStatus.v_yawRate_degps = g_M010_yawRateDegPs;

        // 속도 추정 (Y축 가속도 적분, 초기 속도 0으로 가정, 오차 누적 가능)
        // MPU6050이 전방 Y축으로 설치되었으므로 Y축 가속도 사용
        // 속도 = 이전 속도 + 가속도 * 시간
        // 이 속도 추정은 드리프트가 발생하므로, GPS 등의 외부 센서가 없으면 정확도가 떨어집니다.
        // 여기서는 '상대적인 속도 변화'를 의미한다고 이해하는 것이 좋습니다.
        float v_speedChange_mps = g_M010_carStatus.v_accelerationY_ms2 * v_deltaTime_s;
        g_M010_carStatus.v_currentSpeed_kmh += (v_speedChange_mps * 3.6); // m/s -> km/h

        // MPU6050이 거의 정지 상태일 때 속도 드리프트 보정
        // 작은 가속도 변화는 무시하여 속도 0으로 수렴시킴
        if (fabs(g_M010_carStatus.v_accelerationY_ms2) < G_M010_ACCEL_STOP_THRESHOLD_MPS2 &&
            fabs(g_M010_carStatus.v_yawRate_degps) < 1.0) { // Yaw 각속도도 낮으면 정지
            g_M010_carStatus.v_currentSpeed_kmh = 0.0;
        }

        // 자동차 상태 정의
        M010_defineCarState(v_currentTime_ms);
    }
}

/**
 * @brief 현재 MPU6050 데이터 및 추정된 속도를 기반으로 자동차의 상태를 정의합니다.
 * @param p_currentTime_ms 현재 시간 (millis())
 */
void M010_defineCarState(unsigned long p_currentTime_ms) {
    float v_speed = g_M010_carStatus.v_currentSpeed_kmh;
    float v_accelY = g_M010_carStatus.v_accelerationY_ms2;
    float v_accelZ = g_M010_carStatus.v_accelerationZ_ms2;

    // 과속 방지턱 감지 (Z축 가속도의 급격한 변화)
    if (fabs(v_accelZ) > G_M010_ACCEL_BUMP_THRESHOLD_MPS2 &&
        (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) > G_M010_BUMP_COOLDOWN_MS) {
        g_M010_carStatus.v_speedBumpDetected = true;
        g_M010_lastBumpDetectionTime_ms = p_currentTime_ms;
        g_M010_carStatus.v_currentMovementState = G_M010_STATE_SPEED_BUMP;
        return; // 과속 방지턱 감지 시 다른 상태보다 우선
    } else if (g_M010_carStatus.v_speedBumpDetected &&
               (p_currentTime_ms - g_M010_lastBumpDetectionTime_ms) > G_M010_BUMP_COOLDOWN_MS) {
        g_M010_carStatus.v_speedBumpDetected = false; // 쿨다운 후 리셋
    }

    // 급감속 감지 (Y축 가속도의 급격한 음수 변화)
    if (v_accelY < G_M010_ACCEL_DECEL_THRESHOLD_MPS2) {
        g_M010_carStatus.v_isEmergencyBraking = true;
        g_M010_carStatus.v_currentMovementState = G_M010_STATE_DECELERATING;
        return; // 급감속 감지 시 다른 상태보다 우선
    } else {
        g_M010_carStatus.v_isEmergencyBraking = false;
    }

    // 전진, 후진, 정차, 주차 판단
    if (fabs(v_speed) < G_M010_SPEED_THRESHOLD_KMH) { // 정차 또는 주차 상태
        if (g_M010_carStatus.v_currentMovementState != G_M010_STATE_STOPPED_INIT &&
            g_M010_carStatus.v_currentMovementState != G_M010_STATE_SIGNAL_WAIT1 &&
            g_M010_carStatus.v_currentMovementState != G_M010_STATE_SIGNAL_WAIT2 &&
            g_M010_carStatus.v_currentMovementState != G_M010_STATE_STOPPED1 &&
            g_M010_carStatus.v_currentMovementState != G_M010_STATE_STOPPED2 &&
            g_M010_carStatus.v_currentMovementState != G_M010_STATE_PARKED) {
            // 움직이다가 처음 멈춘 경우 정차 시작 시간 기록
            g_M010_carStatus.v_stopStartTime_ms = p_currentTime_ms;
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_STOPPED_INIT;
        }

        // 정차 시간 계산
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
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_STOPPED_INIT; // 짧은 정차
        }
        g_M010_carStatus.v_lastMovementTime_ms = 0; // 정지 중이므로 마지막 움직임 시간 리셋
    } else { // 움직이는 상태
        g_M010_carStatus.v_lastMovementTime_ms = p_currentTime_ms; // 마지막 움직임 시간 업데이트
        g_M010_carStatus.v_stopStartTime_ms = 0; // 움직이면 정차 시작 시간 리셋

        if (v_speed > G_M010_SPEED_THRESHOLD_KMH) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_FORWARD;
        } else if (v_speed < -G_M010_SPEED_THRESHOLD_KMH) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_REVERSE;
        } else {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_UNKNOWN; // 이 경우 발생하면 안 됨
        }
    }
}

/**
 * @brief 현재 자동차 상태 정보를 시리얼 모니터로 출력합니다.
 */
void M010_printCarStatus() {
    Serial.println(F("\n--- 자동차 현재 상태 ---"));
    Serial.print(F("  상태: "));
    switch (g_M010_carStatus.v_currentMovementState) {
        case G_M010_STATE_UNKNOWN: Serial.println(F("알 수 없음")); break;
        case G_M010_STATE_STOPPED_INIT: Serial.println(F("정차 중 (초기)")); break;
        case G_M010_STATE_SIGNAL_WAIT1: Serial.print(F("신호대기 1 (60초 미만 정차), 시간: ")); Serial.print(g_M010_carStatus.v_currentStopTime_ms / 1000); Serial.println(F("s")); break;
        case G_M010_STATE_SIGNAL_WAIT2: Serial.print(F("신호대기 2 (120초 미만 정차), 시간: ")); Serial.print(g_M010_carStatus.v_currentStopTime_ms / 1000); Serial.println(F("s")); break;
        case G_M010_STATE_STOPPED1: Serial.print(F("정차 1 (5분 미만 정차), 시간: ")); Serial.print(g_M010_carStatus.v_currentStopTime_ms / 1000); Serial.println(F("s")); break;
        case G_M010_STATE_STOPPED2: Serial.print(F("정차 2 (10분 미만 정차), 시간: ")); Serial.print(g_M010_carStatus.v_currentStopTime_ms / 1000); Serial.println(F("s")); break;
        case G_M010_STATE_PARKED: Serial.print(F("주차 중 (10분 이상 정차), 시간: ")); Serial.print(g_M010_carStatus.v_currentStopTime_ms / 1000); Serial.println(F("s")); break;
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
 * @brief 초기 설정 함수입니다. ESP32 보드가 시작될 때 한 번 실행됩니다.
 */
void setup() {
    Serial.begin(115200); // 시리얼 통신 시작
    while (!Serial); // 시리얼 모니터가 연결될 때까지 대기 (디버깅용)

    // MPU6050 초기화
    M010_setupMPU6050();

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
    g_M010_carStatus.v_lastMovementTime_ms = millis(); // 초기 시간 설정
    g_M010_carStatus.v_stopStartTime_ms = 0; // 초기에는 움직였다고 가정
    g_M010_carStatus.v_currentStopTime_ms = 0;

    g_M010_lastSampleTime_ms = millis();
    g_M010_lastSerialPrintTime_ms = millis();

    Serial.println(F("Setup 완료!"));
}

/**
 * @brief 메인 루프 함수입니다. ESP32 보드에서 계속 반복 실행됩니다.
 */
void loop() {
    // MPU6050 데이터 업데이트
    M010_updateCarStatus();

    // 5초 간격으로 시리얼 출력
    if (millis() - g_M010_lastSerialPrintTime_ms >= G_M010_SERIAL_PRINT_INTERVAL_MS) {
        M010_printCarStatus();
        g_M010_lastSerialPrintTime_ms = millis();
    }

    // 다른 작업 (LED Matrix 업데이트 등)은 여기에 추가
    // 예: M010_updateLEDMatrix(g_M010_carStatus.v_currentMovementState);
}

소스 코드 설명
1. 라이브러리 및 전역 변수 설정
 * Wire.h: ESP32의 I2C 통신을 위해 사용됩니다.
 * I2Cdev.h, MPU6050_6Axis_MotionApps20.h: Jeff Rowberg의 MPU6050 라이브러리로, 특히 DMP 기능을 활용하기 위해 필요합니다. PlatformIO에서 MPU6050_DMP6 라이브러리를 설치하면 이 파일들이 포함됩니다.
 * g_M010_mpu: MPU6050 객체입니다.
 * G_M010_MPU_INTERRUPT_PIN: MPU6050의 인터럽트 핀을 ESP32에 연결할 GPIO 핀 번호입니다.
 * MPU6050 상수: G_M010_ACCEL_ALPHA는 가속도 데이터 필터링에 사용되는 상보 필터 계수입니다.
 * 자동차 상태 감지 임계값: G_M010_SPEED_THRESHOLD_KMH, G_M010_ACCEL_STOP_THRESHOLD_MPS2, G_M010_ACCEL_DECEL_THRESHOLD_MPS2, G_M010_ACCEL_BUMP_THRESHOLD_MPS2, G_M010_BUMP_COOLDOWN_MS 등은 각 자동차 상태를 판단하기 위한 임계값들입니다. 실제 차량에 맞게 조정이 필요할 수 있습니다.
 * 정차 및 주차 시간 기준: G_M010_STOP_GRACE_PERIOD_MS, G_M010_SIGNAL_WAIT1_SECONDS 등은 정차 및 주차 상태를 세분화하는 시간 기준입니다.
 * G_M010_SERIAL_PRINT_INTERVAL_MS: 시리얼 출력 주기입니다.
 * CarMovementState 열거형: 자동차의 다양한 움직임 상태를 정의합니다.
 * M010_CarStatus 구조체: 자동차의 현재 상태를 종합적으로 나타내는 구조체입니다. 제공해주신 구조체에 정차 시간 추적을 위한 v_lastMovementTime_ms, v_stopStartTime_ms, v_currentStopTime_ms가 추가되었습니다.
 * g_M010_carStatus: M010_CarStatus 구조체의 전역 인스턴스로, 현재 자동차 상태 정보를 저장합니다.
 * MPU6050 DMP 관련 변수: DMP 초기화 여부, FIFO 버퍼, 쿼터니언, 오일러 각 등을 저장하는 변수들입니다.
2. 함수 설명
 * M010_dmpDataReady(): MPU6050의 인터럽트 발생 시 호출되는 ISR(인터럽트 서비스 루틴)입니다. g_M010_mpuInterrupt 플래그를 설정하여 메인 루프에 데이터 준비를 알립니다.
 * M010_setupMPU6050():
   * I2C 통신(Wire.begin())을 시작하고, 통신 속도를 설정합니다.
   * MPU6050 연결을 테스트합니다.
   * g_M010_mpu.dmpInitialize()를 호출하여 MPU6050의 DMP를 초기화합니다.
   * 성공적으로 초기화되면 DMP를 활성화하고, MPU6050 인터럽트 핀에 ISR을 연결합니다.
   * DMP 패킷 크기를 가져와 g_M010_packetSize에 저장합니다.
 * M010_updateCarStatus():
   * MPU6050 인터럽트가 발생하고 FIFO에 충분한 데이터가 있을 때까지 기다립니다.
   * FIFO에서 쿼터니언 데이터를 읽어와 g_M010_q에 저장합니다.
   * 쿼터니언을 사용하여 Yaw, Pitch, Roll 각도(g_M010_ypr)를 계산합니다. v_currentYawAngle_deg와 v_pitchAngle_deg를 업데이트합니다.
   * dmpGetLinearAccel()을 사용하여 중력분이 제거된 선형 가속도 값을 가져옵니다. 이 값에 중력 가속도(G_M010_GRAVITY_MPS2)를 곱하여 m/s^2 단위로 변환합니다.
   * 가속도 데이터에 간단한 상보 필터를 적용하여 노이즈를 줄입니다 (g_M010_filteredAx, g_M010_filteredAy, g_M010_filteredAz).
   * 자이로스코프 데이터를 읽어 Yaw 각속도(v_yawRate_degps)를 계산합니다.
   * Y축 가속도를 시간에 따라 적분하여 v_currentSpeed_kmh (상대 속도 변화)를 추정합니다. 정지 상태에서는 드리프트를 보정하기 위해 속도를 0으로 설정합니다.
   * M010_defineCarState() 함수를 호출하여 현재 데이터를 기반으로 자동차의 상태를 정의합니다.
 * M010_defineCarState(unsigned long p_currentTime_ms):
   * 현재 속도, Y축 가속도, Z축 가속도 값을 사용하여 자동차의 상세 상태를 판단합니다.
   * 과속 방지턱: v_accelZ의 급격한 변화를 감지하여 v_speedBumpDetected를 설정하고, G_M010_STATE_SPEED_BUMP 상태로 전환합니다. 쿨다운 시간을 두어 중복 감지를 방지합니다.
   * 급감속: v_accelY가 G_M010_ACCEL_DECEL_THRESHOLD_MPS2보다 작을 때 (-3.0m/s^2 미만) v_isEmergencyBraking을 설정하고, G_M010_STATE_DECELERATING 상태로 전환합니다.
   * 전진/후진/정차/주차: v_currentSpeed_kmh의 값에 따라 G_M010_STATE_FORWARD, G_M010_STATE_REVERSE 또는 정차 관련 상태로 전환합니다. 정차 시 v_stopStartTime_ms를 기록하고, 이 시간의 경과에 따라 G_M010_STATE_SIGNAL_WAIT1, G_M010_STATE_SIGNAL_WAIT2, G_M010_STATE_STOPPED1, G_M010_STATE_STOPPED2, G_M010_STATE_PARKED 중 하나로 세분화합니다.
 * M010_printCarStatus(): g_M010_carStatus 구조체에 저장된 모든 자동차 상태 정보를 시리얼 모니터로 출력합니다.
 * setup():
   * 시리얼 통신을 초기화합니다.
   * M010_setupMPU6050()를 호출하여 MPU6050을 설정합니다.
   * g_M010_carStatus 구조체의 초기값을 설정합니다.
 * loop():
   * M010_updateCarStatus()를 지속적으로 호출하여 MPU6050 데이터를 업데이트하고 자동차 상태를 분석합니다.
   * G_M010_SERIAL_PRINT_INTERVAL_MS에 정의된 주기(5초)마다 M010_printCarStatus()를 호출하여 현재 상태를 시리얼 출력합니다.
   * LED Matrix 제어 로직은 이 loop() 함수 내에서 g_M010_carStatus.v_currentMovementState 값에 따라 구현하면 됩니다.
추가 고려사항 및 개선 방향
 * MPU6050 센서 퓨전 및 드리프트:
   * MPU6050의 가속도계만으로는 정확한 속도나 위치를 추정하기 어렵습니다. 특히 적분으로 인한 오차(드리프트)가 누적됩니다.
   * 자동차의 정차 상태를 명확히 감지하여 속도를 0으로 초기화하는 로직을 강화했지만, 장거리 운행 시에는 GPS 모듈 등 외부 센서와의 센서 퓨전이 필요할 수 있습니다.
   * Yaw 각도 또한 시간에 따라 드리프트가 발생할 수 있습니다. 자기장 센서(나침반)가 포함된 IMU(예: MPU9250)를 사용하면 Yaw 각의 절대적인 방향을 더 정확하게 유지할 수 있습니다.
 * LED Matrix 제어:
   * LED Matrix(예: MAX7219 기반 도트 매트릭스 또는 WS2812B 기반 주소 지정 가능 LED)를 제어하는 라이브러리(예: LedControl, FastLED 등)를 추가하여 M010_updateLEDMatrix와 같은 함수를 구현해야 합니다.
   * g_M010_carStatus.v_currentMovementState 값에 따라 해당하는 로봇 눈 표정을 매핑하여 LED Matrix에 표시하는 로직을 작성해야 합니다.
 * 오프셋 및 교정:
   * MPU6050은 환경에 따라 가속도계 및 자이로스코프의 오프셋이 다를 수 있습니다. g_M010_mpu.setXGyroOffset(), g_M010_mpu.setZAccelOffset() 등의 함수를 사용하여 센서 교정을 수행해야 더 정확한 데이터를 얻을 수 있습니다.
   * MPU6050_DMP6 라이브러리의 예제 중 MPU6050_DMP6/Examples/DMP6/DMP6.ino를 참고하여 오프셋을 찾는 과정을 수행해 보세요.
 * 임계값 튜닝:
   * 소스 코드 내의 다양한 임계값 상수들(예: G_M010_SPEED_THRESHOLD_KMH, G_M010_ACCEL_DECEL_THRESHOLD_MPS2, G_M010_ACCEL_BUMP_THRESHOLD_MPS2)은 실제 차량과 주행 환경에 따라 최적화되어야 합니다. 실제 테스트를 통해 값을 조정하는 것이 중요합니다.
 * 전력 관리:
   * 자동차에 상시 전원을 연결할 경우, ESP32와 MPU6050의 전력 소모를 고려해야 합니다. 장시간 사용 시에는 안정적인 전원 공급이 필수입니다.
이 코드가 프로젝트의 좋은 시작점이 되기를 바랍니다. 궁금한 점이 있거나 추가적인 기능 구현이 필요하면 언제든지 질문해주세요!
