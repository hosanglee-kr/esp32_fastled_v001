#include <Wire.h>
#include <MPU6050_6Axis_MotionApps_V6_12.h> // MPU6050 라이브러리 (DMP 지원 버전)

// -------------------------------------------------------------------------------------------------
// 요구 기능 주석
// -------------------------------------------------------------------------------------------------

// 요구 기능 1: 전진, 후진, 정차, 주차 상태 감지
// - 자동차의 현재 움직임 상태를 G_M010_STATE_FORWARD, G_M010_STATE_REVERSE, G_M010_STATE_STOPPED, G_M010_STATE_PARKED로 분류합니다.
// - 정차와 주차는 일정 시간(G_M010_PARK_THRESHOLD_SECONDS) 동안 움직임이 없는 기준으로 구분합니다.

// 요구 기능 2: 이동 속도 (Km/h), 가속도 (m/s^2) 추정
// - Y축(전후) 및 Z축(상하) 가속도(m/s^2)를 측정합니다.
// - DMP를 통해 얻은 가속도 데이터를 기반으로 개략적인 속도(km/h)를 추정합니다.
// - 전진 시 양수, 후진 시 음수 속도를 나타냅니다.

// 요구 기능 3: 좌, 우회전 각도 (도)
// - MPU6050의 Yaw 각속도(Yaw Rate)를 사용하여 차량의 회전 정도를 감지하고,
//   DMP를 통해 얻은 Yaw 각(절대 방향)을 추정합니다.

// 요구 기능 4: 전후 경사도 (도)
// - MPU6050의 Pitch 각을 사용하여 자동차의 오르막/내리막 경사도를 추정합니다.

// 요구 기능 5: 과속 방지턱 통과 여부 감지
// - Z축(상하) 가속도의 급격한 변화를 감지하여 과속 방지턱 통과 여부를 판단합니다.

// -------------------------------------------------------------------------------------------------
// 전역 상수 정의
// -------------------------------------------------------------------------------------------------

// MPU6050 I2C 주소
const int G_M010_MPU_ADDRESS = 0x68;

// G_M010_STATE_STOPPED와 G_M010_STATE_PARKED를 구분하는 시간 기준 (초)
const unsigned long G_M010_PARK_THRESHOLD_SECONDS = 60; // 60초 이상 움직임 없으면 주차

// 가속도 센서의 노이즈 제거를 위한 임계값 (m/s^2)
const float G_M010_ACCEL_THRESHOLD_STOPPED_MS2 = 0.1; // 이 값보다 작으면 정지 상태로 간주 (0.1 m/s^2)
const float G_M010_ACCEL_THRESHOLD_MOVEMENT_MS2 = 0.3; // 이 값보다 크면 움직임으로 간주 (0.3 m/s^2)

// Yaw 각속도(회전) 임계값 (deg/s)
const float G_M010_YAW_RATE_THRESHOLD_DEGPS = 5.0; // 이 값보다 크면 회전으로 간주

// 과속 방지턱 감지를 위한 Z축 가속도 임계값 (m/s^2)
const float G_M010_SPEED_BUMP_ACCEL_THRESHOLD_MS2 = 5.0; // Z축 가속도 변화가 이 값보다 크면 과속 방지턱으로 간주
const unsigned long G_M010_SPEED_BUMP_COOLDOWN_MS = 2000; // 과속 방지턱 감지 후 쿨다운 시간 (2초)

// 속도 계산을 위한 시간 간격 (초)
const float G_M010_LOOP_INTERVAL_SEC = 0.01; // 메인 루프 실행 간격 (100Hz 가정)

// -------------------------------------------------------------------------------------------------
// 데이터 구조체 및 열거형 정의
// -------------------------------------------------------------------------------------------------

// 자동차의 현재 움직임 상태를 정의하는 열거형
enum CarMovementState {
    G_M010_STATE_UNKNOWN,   // 초기 상태 또는 알 수 없음
    G_M010_STATE_STOPPED,   // 정차 (잠깐 멈춘 상태, 0km/h)
    G_M010_STATE_PARKED,    // 주차 (일정 시간 이상 움직임 없는 상태)
    G_M010_STATE_FORWARD,   // 전진
    G_M010_STATE_REVERSE    // 후진
};

// 자동차의 현재 상태 정보를 담는 구조체
struct M010_CarStatus {
    // 요구기능 1: 전진, 후진, 정차, 주차
    CarMovementState v_currentMovementState; // 현재 자동차의 움직임 상태

    // 요구기능 2: +-이동 속도(Km/h), 가속도(m/s^2)
    float v_currentSpeed_kmh;          // 현재 속도 (km/h, 양수: 전진, 음수: 후진)
    float v_accelerationY_ms2;         // Y축(전후) 가속도 (m/s^2)
    float v_accelerationZ_ms2;         // Z축(상하) 가속도 (m/s^2, 과속 방지턱 감지에 유용)

    // 요구기능 3: 좌, 우회전 각도 (도)
    float v_currentYawAngle_deg;       // 현재 Yaw 각 (도, 차량의 절대 방향)
    float v_yawRate_degps;             // Yaw 각속도 (도/초, 회전 정도 판단)

    // 요구기능 4: 전후 경사도(도)
    float v_pitchAngle_deg;            // Pitch 각 (도, 오르막/내리막 경사도)

    // 요구기능 5: 과속 방지턱 통과 여부
    bool v_speedBumpDetected;          // 과속 방지턱 통과 감지 여부 (true: 감지, false: 미감지)
};

// -------------------------------------------------------------------------------------------------
// 전역 변수 선언
// -------------------------------------------------------------------------------------------------

MPU6050 g_M010_mpu; // MPU6050 객체
bool g_M010_dmpReady = false;  // DMP 초기화 완료 여부
uint8_t g_M010_mpuIntStatus;   // MPU6050 인터럽트 상태 레지스터
uint8_t g_M010_devStatus;      // MPU6050 장치 상태
uint16_t g_M010_packetSize;    // DMP FIFO 패킷 크기
uint16_t g_M010_fifoCount;     // DMP FIFO 버퍼에 있는 바이트 수
uint8_t g_M010_fifoBuffer[64]; // FIFO 버퍼

// MPU6050 DMP 쿼터니언 (자세 추정에 사용)
Quaternion g_M010_q;
VectorFloat g_M010_gravity;
float g_M010_ypr[3]; // yaw, pitch, roll (rad)
VectorInt16 g_M010_aa; // 가속도 (DMP에서 처리된 값)
VectorInt16 g_M010_aaReal; // 실제 가속도 (중력분리 후)
VectorInt16 g_M010_aaWorld; // 월드 좌표계 가속도 (중력분리 후 회전 적용)

// 자동차 상태 구조체 전역 변수
M010_CarStatus g_M010_carStatus;

// 상태 판단을 위한 시간 기록 변수
unsigned long g_M010_lastMovementTime = 0;
unsigned long g_M010_lastSpeedBumpTime = 0; // 과속 방지턱 쿨다운을 위한 시간
float g_M010_previousVelocityY_ms = 0; // 이전 속도 (Y축, m/s)
float g_M010_previousZAccel_ms2 = 0; // 이전 Z축 가속도

// -------------------------------------------------------------------------------------------------
// 함수 선언
// -------------------------------------------------------------------------------------------------

void M010_dmpDataReady();
void M010_setupMPU6050();
void M010_updateCarStatus();
void M010_detectSpeedBump(float v_currentZAccel_ms2);
float M010_lowPassFilter(float v_currentValue, float v_previousValue, float v_alpha);

// -------------------------------------------------------------------------------------------------
// 인터럽트 서비스 루틴 (ISR)
// -------------------------------------------------------------------------------------------------

volatile bool g_M010_mpuInterrupt = false; // MPU6050 인터럽트 플래그
void IRAM_ATTR M010_dmpDataReady() {
    g_M010_mpuInterrupt = true;
}

// -------------------------------------------------------------------------------------------------
// 초기 설정 (setup)
// -------------------------------------------------------------------------------------------------

void setup() {
    // 시리얼 통신 초기화
    Serial.begin(115200);
    while (!Serial); // 시리얼 연결 대기

    // I2C 통신 초기화
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C 클럭

    Serial.println(F("MPU6050 초기화 중..."));

    // MPU6050 초기 설정 함수 호출
    M010_setupMPU6050();

    // 자동차 상태 구조체 초기화
    g_M010_carStatus.v_currentMovementState = G_M010_STATE_UNKNOWN;
    g_M010_carStatus.v_currentSpeed_kmh = 0.0;
    g_M010_carStatus.v_accelerationY_ms2 = 0.0;
    g_M010_carStatus.v_accelerationZ_ms2 = 0.0;
    g_M010_carStatus.v_currentYawAngle_deg = 0.0;
    g_M010_carStatus.v_yawRate_degps = 0.0;
    g_M010_carStatus.v_pitchAngle_deg = 0.0;
    g_M010_carStatus.v_speedBumpDetected = false;

    // 초기 시간 기록
    g_M010_lastMovementTime = millis();
    g_M010_lastSpeedBumpTime = millis();
}

// -------------------------------------------------------------------------------------------------
// 메인 루프 (loop)
// -------------------------------------------------------------------------------------------------

void loop() {
    // DMP가 준비되지 않았으면 아무것도 하지 않음
    if (!g_M010_dmpReady) return;

    // MPU6050 인터럽트가 발생하지 않았거나 FIFO 버퍼에 충분한 데이터가 없으면 대기
    // (MPU6050_FIFO_COUNT_INTERRUPT_THRESHOLD는 라이브러리 내에서 정의됨)
    if (!g_M010_mpuInterrupt && g_M010_fifoCount < g_M010_packetSize) return;

    // 인터럽트 플래그 리셋
    g_M010_mpuInterrupt = false;

    // FIFO 버퍼에서 패킷 읽기
    g_M010_fifoCount = g_M010_mpu.getFIFOCount();
    if (g_M010_fifoCount == 0) {
        // Serial.println(F("FIFO Count is 0."));
        return; // FIFO가 비어있으면 다음 루프 대기
    }
    
    // FIFO 버퍼 오버플로우 처리
    if (g_M010_fifoCount == 1024 || g_M010_fifoCount % g_M010_packetSize != 0) {
        // FIFO 오버플로우 또는 패킷 불완전: FIFO 리셋
        g_M010_mpu.resetFIFO();
        Serial.println(F("FIFO Overflow! Resetting FIFO."));
        g_M010_fifoCount = g_M010_mpu.getFIFOCount(); // Reset 후 다시 카운트
        if (g_M010_fifoCount == 0) return; // Reset 후에도 데이터 없으면 대기
    }
    
    // 여러 패킷이 있을 경우 마지막 패킷을 읽어 최신 데이터 사용
    while (g_M010_fifoCount >= g_M010_packetSize) {
        g_M010_mpu.getFIFOBytes(g_M010_fifoBuffer, g_M010_packetSize);
        g_M010_fifoCount -= g_M010_packetSize;
    }

    // MPU6050 DMP 데이터 처리 및 자동차 상태 업데이트
    M010_updateCarStatus();

    // 시리얼 모니터로 현재 상태 출력 (디버깅용)
    // Serial.print("상태: ");
    // if (g_M010_carStatus.v_currentMovementState == G_M010_STATE_FORWARD) Serial.print("전진");
    // else if (g_M010_carStatus.v_currentMovementState == G_M010_STATE_REVERSE) Serial.print("후진");
    // else if (g_M010_carStatus.v_currentMovementState == G_M010_STATE_STOPPED) Serial.print("정차");
    // else if (g_M010_carStatus.v_currentMovementState == G_M010_STATE_PARKED) Serial.print("주차");
    // else Serial.print("알 수 없음");

    // Serial.print(", 속도: "); Serial.print(g_M010_carStatus.v_currentSpeed_kmh, 1); Serial.print(" km/h");
    // Serial.print(", AccY: "); Serial.print(g_M010_carStatus.v_accelerationY_ms2, 2); Serial.print(" m/s^2");
    // Serial.print(", AccZ: "); Serial.print(g_M010_carStatus.v_accelerationZ_ms2, 2); Serial.print(" m/s^2");
    // Serial.print(", Pitch: "); Serial.print(g_M010_carStatus.v_pitchAngle_deg, 1); Serial.print(" deg");
    // Serial.print(", Yaw: "); Serial.print(g_M010_carStatus.v_currentYawAngle_deg, 1); Serial.print(" deg");
    // Serial.print(", YawRate: "); Serial.print(g_M010_carStatus.v_yawRate_degps, 1); Serial.print(" deg/s");
    // Serial.print(", 방지턱: "); Serial.println(g_M010_carStatus.v_speedBumpDetected ? "감지" : "미감지");
}

// -------------------------------------------------------------------------------------------------
// MPU6050 초기 설정 함수
// -------------------------------------------------------------------------------------------------

void M010_setupMPU6050() {
    // MPU6050 연결 테스트
    Serial.println(F("MPU6050 연결 테스트 중..."));
    g_M010_mpu.initialize();
    Serial.print(F("MPU6050 테스트 결과: "));
    Serial.println(g_M010_mpu.testConnection() ? F("성공") : F("실패"));

    // DMP 로드
    Serial.println(F("DMP 초기화 중..."));
    g_M010_devStatus = g_M010_mpu.dmpInitialize();

    // MPU 오프셋 설정 (환경에 따라 보정 필요)
    // 이 값들은 MPU6050_DMP6_EXAMPLE의 'MPU6050_CALIBRATION' 섹션을 참조하여 얻을 수 있습니다.
    // 여기서는 예시 값을 사용합니다. 실제 사용 시에는 환경에 맞게 보정해야 합니다.
    g_M010_mpu.setXGyroOffset(220);
    g_M010_mpu.setYGyroOffset(76);
    g_M010_mpu.setZGyroOffset(-85);
    g_M010_mpu.setZAccelOffset(1788); // 16384 (1g) - 실제 측정값으로 보정

    // DMP 초기화 상태 확인
    if (g_M010_devStatus == 0) {
        // DMP 활성화
        Serial.println(F("DMP 활성화 중..."));
        g_M010_mpu.setDMPEnabled(true);

        // MPU6050 인터럽트 핀 설정 (GPIO23 사용, ESP32 기준)
        Serial.println(F("인터럽트 핀 설정 중... (핀 23)"));
        pinMode(23, INPUT); // ESP32의 D23 핀 사용
        attachInterrupt(digitalPinToInterrupt(23), M010_dmpDataReady, RISING);
        g_M010_mpuIntStatus = g_M010_mpu.getIntStatus();

        // DMP FIFO 패킷 크기 얻기
        g_M010_packetSize = g_M010_mpu.dmpGetFIFOPacketSize();
        g_M010_dmpReady = true;
        Serial.println(F("DMP 초기화 완료!"));
    } else {
        // 오류 코드 출력
        Serial.print(F("DMP 초기화 실패 (코드: "));
        Serial.print(g_M010_devStatus);
        Serial.println(F(")"));
    }
}

// -------------------------------------------------------------------------------------------------
// 자동차 상태 업데이트 함수
// -------------------------------------------------------------------------------------------------

void M010_updateCarStatus() {
    // DMP에서 가속도 및 자세 데이터 얻기
    g_M010_mpu.dmpGetQuaternion(&g_M010_q, g_M010_fifoBuffer);
    g_M010_mpu.dmpGetAccel(&g_M010_aa, g_M010_fifoBuffer);
    g_M010_mpu.dmpGetGravity(&g_M010_gravity, &g_M010_q);
    g_M010_mpu.dmpGetLinearAccel(&g_M010_aaReal, &g_M010_aa, &g_M010_gravity); // 중력 분리 가속도
    g_M010_mpu.dmpGetLinearAccelInWorld(&g_M010_aaWorld, &g_M010_aaReal, &g_M010_q); // 월드 좌표계 가속도
    g_M010_mpu.dmpGetYawPitchRoll(g_M010_ypr, &g_M010_q, &g_M010_gravity); // Yaw, Pitch, Roll (라디안)
    
    // MPU6050의 가속도 스케일은 1g = 16384 (FS_SEL_0 기준)
    // 1g = 9.80665 m/s^2
    const float v_accelScale = 9.80665 / 16384.0; // m/s^2 per LSB

    // 가속도 값 업데이트 (Y축: 전후, Z축: 상하)
    // Y축 가속도: 전후 움직임 (양수: 전진 가속, 음수: 후진 가속)
    // Z축 가속도: 상하 움직임 (양수: 위로 가속, 음수: 아래로 가속. 과속 방지턱 감지에 유용)
    float v_currentAccelY_ms2 = g_M010_aaReal.y * v_accelScale; // Y축은 전후방, Z축은 상하방
    float v_currentAccelZ_ms2 = g_M010_aaReal.z * v_accelScale; // Z축은 상하방, Y축은 전후방

    // 저역 통과 필터 적용 (진동 완화)
    // 알파 값은 0과 1 사이이며, 1에 가까울수록 필터링이 적게 되고, 0에 가까울수록 많이 됨.
    // 0.1~0.5 범위가 일반적으로 적절합니다.
    float v_alpha = 0.3; // 가속도 필터링 강도
    g_M010_carStatus.v_accelerationY_ms2 = M010_lowPassFilter(v_currentAccelY_ms2, g_M010_carStatus.v_accelerationY_ms2, v_alpha);
    g_M010_carStatus.v_accelerationZ_ms2 = M010_lowPassFilter(v_currentAccelZ_ms2, g_M010_carStatus.v_accelerationZ_ms2, v_alpha);

    // 피치 및 요 각도 업데이트 (라디안 -> 도 변환)
    g_M010_carStatus.v_pitchAngle_deg = g_M010_ypr[1] * 180 / M_PI; // Pitch (오르막/내리막)
    g_M010_carStatus.v_currentYawAngle_deg = g_M010_ypr[0] * 180 / M_PI; // Yaw (절대 방향)
    
    // Yaw 각속도 계산 (정확한 yaw rate는 자이로스코프에서 직접 얻는 것이 좋지만, DMP에서 얻기 어려우므로 변화량으로 추정)
    // MPU6050_6Axis_MotionApps_V6_12.h 라이브러리는 직접적인 자이로 Yaw Rate를 월드 좌표계로 제공하지 않으므로,
    // 여기서는 간단히 yaw 각의 변화량으로 yaw rate를 추정합니다.
    // 더 정확한 yaw rate를 얻으려면 getRotation() 함수 등을 통해 자이로 데이터를 직접 처리해야 합니다.
    float v_currentYawRate_degps = g_M010_mpu.getRotationZ() / 131.0; // deg/s
    g_M010_carStatus.v_yawRate_degps = M010_lowPassFilter(v_currentYawRate_degps, g_M010_carStatus.v_yawRate_degps, v_alpha);

    // 속도 추정 (Y축 가속도 적분)
    // 간단한 적분 방식으로 오차 누적 가능성 있음.
    // 실제 속도 센서와 병합하거나 칼만 필터 등 고급 필터링 필요.
    float v_deltaT = G_M010_LOOP_INTERVAL_SEC; // 루프 실행 간격 (초)
    float v_currentVelocityY_ms = g_M010_previousVelocityY_ms + g_M010_carStatus.v_accelerationY_ms2 * v_deltaT;
    
    // 정지 상태일 경우 속도 초기화 (드리프트 방지)
    if (fabs(g_M010_carStatus.v_accelerationY_ms2) < G_M010_ACCEL_THRESHOLD_STOPPED_MS2) {
        v_currentVelocityY_ms = 0.0;
    }
    g_M010_previousVelocityY_ms = v_currentVelocityY_ms; // 다음 계산을 위해 현재 속도 저장

    // km/h로 변환 (1 m/s = 3.6 km/h)
    g_M010_carStatus.v_currentSpeed_kmh = v_currentVelocityY_ms * 3.6;

    // 움직임 상태 감지 및 업데이트
    unsigned long v_currentTime = millis();
    float v_absAccelY = fabs(g_M010_carStatus.v_accelerationY_ms2);
    float v_absSpeed = fabs(g_M010_carStatus.v_currentSpeed_kmh);

    if (v_absAccelY < G_M010_ACCEL_THRESHOLD_STOPPED_MS2 && v_absSpeed < 0.5) { // 거의 움직임 없음
        // 움직임이 멈췄을 때
        if (g_M010_carStatus.v_currentMovementState != G_M010_STATE_STOPPED &&
            g_M010_carStatus.v_currentMovementState != G_M010_STATE_PARKED) {
            g_M010_lastMovementTime = v_currentTime; // 정차 시작 시간 기록
        }
        
        // 정차/주차 구분
        if ((v_currentTime - g_M010_lastMovementTime) / 1000 >= G_M010_PARK_THRESHOLD_SECONDS) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_PARKED; // 주차
        } else {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_STOPPED; // 정차
        }
        g_M010_carStatus.v_currentSpeed_kmh = 0.0; // 정지 상태에서는 속도 0으로 강제 설정
    } else if (v_absAccelY >= G_M010_ACCEL_THRESHOLD_MOVEMENT_MS2 || v_absSpeed >= 0.5) { // 움직임 감지
        g_M010_lastMovementTime = v_currentTime; // 마지막 움직임 시간 업데이트

        if (g_M010_carStatus.v_currentSpeed_kmh > 0.5) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_FORWARD; // 전진
        } else if (g_M010_carStatus.v_currentSpeed_kmh < -0.5) {
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_REVERSE; // 후진
        } else {
            // 미세한 움직임이 있으나 전진/후진으로 분류하기 어려운 경우
            g_M010_carStatus.v_currentMovementState = G_M010_STATE_UNKNOWN; 
        }
    }

    // 과속 방지턱 감지
    M010_detectSpeedBump(g_M010_carStatus.v_accelerationZ_ms2);
}

// -------------------------------------------------------------------------------------------------
// 과속 방지턱 감지 함수
// -------------------------------------------------------------------------------------------------

void M010_detectSpeedBump(float v_currentZAccel_ms2) {
    unsigned long v_currentTime = millis();

    // 쿨다운 시간 동안은 재감지하지 않음
    if (v_currentTime - g_M010_lastSpeedBumpTime < G_M010_SPEED_BUMP_COOLDOWN_MS) {
        g_M010_carStatus.v_speedBumpDetected = false; // 쿨다운 중에는 false로 유지
        return;
    }

    // Z축 가속도의 변화량 계산 (이전 값과의 차이)
    // 과속 방지턱은 짧은 시간 동안 큰 상하 충격을 유발함
    float v_zAccelChange = fabs(v_currentZAccel_ms2 - g_M010_previousZAccel_ms2);

    if (v_zAccelChange > G_M010_SPEED_BUMP_ACCEL_THRESHOLD_MS2) {
        g_M010_carStatus.v_speedBumpDetected = true;
        g_M010_lastSpeedBumpTime = v_currentTime; // 감지 시간 업데이트
    } else {
        g_M010_carStatus.v_speedBumpDetected = false;
    }
    
    g_M010_previousZAccel_ms2 = v_currentZAccel_ms2; // 현재 Z가속도 저장
}

// -------------------------------------------------------------------------------------------------
// 간단한 저역 통과 필터 함수 (Low-Pass Filter)
// -------------------------------------------------------------------------------------------------
// `v_alpha` 값은 0에서 1 사이.
// `v_alpha`가 1에 가까울수록 필터링 효과가 적어 현재 값에 더 많이 의존하고,
// `v_alpha`가 0에 가까울수록 필터링 효과가 커서 이전 값의 영향을 많이 받습니다.
// 진동이 심한 환경에서는 낮은 `v_alpha` 값을 사용하면 좋습니다. (예: 0.1 ~ 0.5)
float M010_lowPassFilter(float v_currentValue, float v_previousValue, float v_alpha) {
    return v_previousValue + v_alpha * (v_currentValue - v_previousValue);
}

