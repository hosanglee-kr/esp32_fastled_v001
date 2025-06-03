// M010_자동차_움직임_감지_시스템
//
// 이 코드는 ESP32와 MPU6050 센서를 사용하여 자동차의 다양한 움직임 상태를 감지하고 시리얼 포트로 출력합니다.
//
// 주요 기능:
// 1. MPU6050 DMP(Digital Motion Processor)를 사용하여 오일러 각도(피치, 롤, 요) 및 가속도 데이터 획득.
// 2. 센서 데이터에 간단한 이동 평균 필터를 적용하여 진동 노이즈 감소.
// 3. 자동차의 개략적인 속도, 가속/감속 정도, 오르막/내리막 경사, 과속 방지턱 통과, 후진 여부, 좌/우회전 각도, 정차/주차 상태를 판단.
// 4. 판단된 자동차 상태 정보를 구조체에 저장하고 5초 간격으로 시리얼 모니터에 출력.
//
// MPU6050 설치 방향:
// - 전방: Y축
// - 위쪽: Z축
//
// 명명 규칙:
// - 전역 상수: G_M010_으로 시작
// - 전역 변수: g_M010_으로 시작
// - 로컬 변수: v_로 시작
// - 함수: M010_로 시작

#include <Wire.h> // I2C 통신 라이브러리
#include <MPU6050_6Axis_MotionApps20.h> // MPU6050 DMP 라이브러리

// MPU6050 객체 생성
MPU6050 mpu;

// MPU6050 DMP 관련 전역 상수
const bool G_M010_FORCE_DMP_INIT = false; // DMP 초기화 강제 여부
const uint16_t G_M010_PACKET_SIZE = 42; // MPU6050 DMP 패킷 크기
const uint8_t G_M010_MPU_INTERRUPT_PIN = 4; // MPU6050 인터럽트 핀 (ESP32 GPIO 4)

// 필터 관련 전역 상수
const int G_M010_FILTER_WINDOW_SIZE = 10; // 이동 평균 필터 윈도우 크기

// 상태 판단을 위한 임계값 상수
const float G_M010_ACCEL_THRESHOLD_STOP_START = 0.2; // 정지/출발 판단 가속도 임계값 (g)
const float G_M010_ACCEL_THRESHOLD_DECEL_ACCEL = 0.5; // 가속/감속 판단 가속도 임계값 (g)
const float G_M010_PITCH_THRESHOLD_SLOPE = 5.0; // 오르막/내리막 판단 피치 각도 임계값 (도)
const float G_M010_PITCH_THRESHOLD_BUMP = 10.0; // 과속 방지턱 판단 피치 각도 임계값 (도)
const float G_M010_YAW_THRESHOLD_TURN = 15.0; // 좌/우회전 판단 요 각도 변화 임계값 (도)
const unsigned long G_M010_STOP_TIME_THRESHOLD_MS = 30000; // 정차/주차 판단 시간 임계값 (ms)
const float G_M010_REVERSE_ACCEL_THRESHOLD = -0.5; // 후진 판단 가속도 임계값 (m/s^2) - Y축 기준

// 시간 관련 전역 상수
const unsigned long G_M010_REPORT_INTERVAL_MS = 5000; // 시리얼 출력 간격 (ms)

// MPU6050 DMP 관련 전역 변수
bool g_M010_dmpReady = false; // DMP 초기화 완료 여부
uint8_t g_M010_mpuIntStatus; // MPU 인터럽트 상태
uint16_t g_M010_fifoCount; // FIFO에 저장된 바이트 수
uint8_t g_M010_fifoBuffer[G_M010_PACKET_SIZE]; // FIFO 버퍼

// MPU6050 데이터 전역 변수
Quaternion g_M010_q; // 쿼터니언 (DMP 출력)
VectorFloat g_M010_gravity; // 중력 벡터
float g_M010_ypr[3]; // 요, 피치, 롤 (오일러 각도)
VectorInt16 g_M010_aa; // 가속도 (raw)
VectorInt16 g_M010_aaReal; // 가속도 (실제 중력 보정)
VectorInt16 g_M010_aaWorld; // 가속도 (월드 좌표계)

// 소프트웨어 필터 버퍼
float g_M010_accelXBuffer[G_M010_FILTER_WINDOW_SIZE];
float g_M010_accelYBuffer[G_M010_FILTER_WINDOW_SIZE];
float g_M010_accelZBuffer[G_M010_FILTER_WINDOW_SIZE];
int g_M010_filterIndex = 0;
float g_M010_filteredAccelX = 0.0;
float g_M010_filteredAccelY = 0.0;
float g_M010_filteredAccelZ = 0.0;

// 시간 측정 전역 변수
unsigned long g_M010_lastReportTime = 0;
unsigned long g_M010_lastMovementTime = 0;
unsigned long g_M010_lastYawUpdateTime = 0;
float g_M010_previousYaw = 0.0;

// 자동차 상태 정의 구조체
struct M010_CarStatus {
    float velocityMPS; // 개략적인 속도 (m/s)
    String accelDecelStatus; // 가속/감속 상태 (정지, 가속, 감속, 일정)
    String slopeStatus; // 오르막/내리막 상태 (평지, 오르막, 내리막)
    bool isSpeedBump; // 과속 방지턱 통과 여부
    bool isReversing; // 후진 여부
    float turnAngle; // 좌, 우회전 각도 (도, 양수:우회전, 음수:좌회전)
    String parkStopStatus; // 정차/주차 여부 (움직임, 정차, 주차)
};

// 자동차 상태 전역 변수
M010_CarStatus g_M010_currentCarStatus;

// MPU6050 인터럽트 서비스 루틴
volatile bool g_M010_mpuInterrupt = false; // MPU 데이터 준비 여부
void M010_dmpDataReady() {
    g_M010_mpuInterrupt = true;
}

// MPU6050 초기화 및 DMP 설정 함수
void M010_setupMPU6050() {
    Serial.println(F("I2C 통신 초기화 중..."));
    Wire.begin(); // I2C 통신 시작
    Wire.setClock(400000); // I2C 클럭 속도 설정 (400kHz)

    Serial.println(F("MPU6050 초기화 중..."));
    mpu.initialize(); // MPU6050 초기화
    pinMode(G_M010_MPU_INTERRUPT_PIN, INPUT); // 인터럽트 핀 설정

    Serial.println(F("MPU6050 연결 확인 중..."));
    if (!mpu.testConnection()) { // MPU6050 연결 확인
        Serial.println(F("MPU6050 연결 실패! 다시 확인하세요."));
        while (1); // 연결 실패 시 무한 루프
    }

    Serial.println(F("DMP 초기화 중..."));
    uint8_t v_devStatus = mpu.dmpInitialize(); // DMP 초기화

    // 자이로 오프셋 설정 (필요시 교정된 값으로 변경)
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788);

    if (v_devStatus == 0) { // DMP 초기화 성공
        Serial.println(F("DMP 활성화 중..."));
        mpu.setDMPEnabled(true); // DMP 활성화

        Serial.println(F("MPU6050 인터럽트 설정 중..."));
        attachInterrupt(digitalPinToInterrupt(G_M010_MPU_INTERRUPT_PIN), M010_dmpDataReady, RISING); // 인터럽트 연결
        g_M010_mpuIntStatus = mpu.getIntStatus(); // 인터럽트 상태 읽기
        g_M010_dmpReady = true;
        Serial.println(F("DMP 초기화 완료!"));
    } else { // DMP 초기화 실패
        Serial.print(F("DMP 초기화 실패 (오류 코드 "));
        Serial.print(v_devStatus);
        Serial.println(F(")"));
        while (1); // 초기화 실패 시 무한 루프
    }
}

// 이동 평균 필터 적용 함수
float M010_applyMovingAverageFilter(float v_newValue, float v_buffer[], int& v_index, float& v_filteredValue) {
    // 버퍼에서 가장 오래된 값 제거
    v_filteredValue -= v_buffer[v_index];
    // 새로운 값 추가
    v_buffer[v_index] = v_newValue;
    // 필터된 값에 새로운 값 추가
    v_filteredValue += v_newValue;
    // 인덱스 업데이트
    v_index = (v_index + 1) % G_M010_FILTER_WINDOW_SIZE;
    // 평균 계산 및 반환
    return v_filteredValue / G_M010_FILTER_WINDOW_SIZE;
}

// 자동차 속도 및 가속/감속 정도 판단 함수
void M010_updateSpeedAndAcceleration() {
    // 가속도 벡터의 크기를 사용하여 개략적인 속도 및 가속/감속 판단
    // 가속도 값은 중력 가속도를 제거한 값 (aaReal) 또는 월드 좌표계 가속도 (aaWorld)를 사용.
    // 여기서는 월드 좌표계 가속도를 사용하여 자동차 자체의 움직임 감지.
    float v_totalAccel = sqrt(pow(g_M010_filteredAccelX, 2) + pow(g_M010_filteredAccelY, 2) + pow(g_M010_filteredAccelZ, 2));

    // 이전 가속도 값과 현재 가속도 값을 비교하여 변화율을 계산
    static float v_prevTotalAccel = 0.0;
    float v_accelChange = v_totalAccel - v_prevTotalAccel;
    v_prevTotalAccel = v_totalAccel;

    // 간단한 속도 추정 (적분)
    // 이 부분은 개략적인 추정이며, 정확한 속도 측정을 위해서는 GPS 또는 휠 엔코더 등이 필요합니다.
    // 여기서는 가속도를 시간에 따라 적분하여 속도를 추정합니다.
    static float v_estimatedVelocity = 0.0;
    static unsigned long v_lastVelocityUpdateTime = millis();
    unsigned long v_currentTime = millis();
    float v_deltaTimeSec = (v_currentTime - v_lastVelocityUpdateTime) / 1000.0;
    v_lastVelocityUpdateTime = v_currentTime;

    // 가속도 (G_M010_aaWorld.y (y축:전방)를 사용하고 중력 가속도(9.81m/s^2)를 곱하여 m/s^2로 변환)
    // 이 값은 MPU6050이 측정한 실제 선형 가속도가 아니므로 주의 필요.
    // DMP가 계산한 월드 좌표계 가속도는 중력이 제거된 상태이며, 센서 자체의 선형 가속도를 나타냄.
    float v_forwardAccelMPS2 = g_M010_filteredAccelY * (9.81 / 16384.0); // raw 값 스케일링 (16384 LSB/g)

    // 간단한 속도 적분 (매우 개략적)
    v_estimatedVelocity += v_forwardAccelMPS2 * v_deltaTimeSec;
    if (v_estimatedVelocity < 0) v_estimatedVelocity = 0; // 속도는 음수가 될 수 없음

    g_M010_currentCarStatus.velocityMPS = v_estimatedVelocity;

    // 가속/감속 상태 판단
    if (abs(v_forwardAccelMPS2) < G_M010_ACCEL_THRESHOLD_STOP_START) {
        g_M010_currentCarStatus.accelDecelStatus = "정지";
    } else if (v_forwardAccelMPS2 > G_M010_ACCEL_THRESHOLD_DECEL_ACCEL) {
        g_M010_currentCarStatus.accelDecelStatus = "가속";
    } else if (v_forwardAccelMPS2 < -G_M010_ACCEL_THRESHOLD_DECEL_ACCEL) {
        g_M010_currentCarStatus.accelDecelStatus = "감속";
    } else {
        g_M010_currentCarStatus.accelDecelStatus = "일정";
    }
}

// 오르막, 내리막 정도 판단 함수
void M010_updateSlope() {
    float v_pitchAngle = g_M010_ypr[1] * 180 / M_PI; // 피치 각도 (라디안을 도로 변환)

    if (v_pitchAngle > G_M010_PITCH_THRESHOLD_SLOPE) {
        g_M010_currentCarStatus.slopeStatus = "오르막";
    } else if (v_pitchAngle < -G_M010_PITCH_THRESHOLD_SLOPE) {
        g_M010_currentCarStatus.slopeStatus = "내리막";
    } else {
        g_M010_currentCarStatus.slopeStatus = "평지";
    }
}

// 과속 방지턱 통과 상황 판단 함수
void M010_updateSpeedBump() {
    // 순간적인 큰 피치 각도 변화 (움직임) 또는 Z축 가속도 변화로 판단
    // 여기서는 Z축 가속도(차량 위아래 움직임) 변화를 주로 사용
    float v_zAccel = g_M010_filteredAccelZ; // Z축 가속도 (월드 좌표계)

    // 임계값보다 큰 순간적인 Z축 가속도 변화 감지
    // 과속 방지턱 통과 시 상하 진동이 발생하므로 Z축 가속도가 크게 변동
    if (abs(v_zAccel) > 1.5) { // 이 값은 실험을 통해 조정해야 함
        g_M010_currentCarStatus.isSpeedBump = true;
    } else {
        g_M010_currentCarStatus.isSpeedBump = false;
    }
}

// 후진 여부 및 후진 속도 판단 함수
void M010_updateReverseStatus() {
    // Y축 가속도 (전방 기준)가 음수이고 일정 임계값 이하일 때 후진으로 판단
    // 자동차가 뒤로 움직이면 Y축 가속도 (DMP 기준)가 음수가 됨
    float v_yAccelMPS2 = g_M010_filteredAccelY * (9.81 / 16384.0); // raw 값 스케일링 (m/s^2)

    if (v_yAccelMPS2 < G_M010_REVERSE_ACCEL_THRESHOLD && g_M010_currentCarStatus.velocityMPS > 0.1) { // 0.1m/s 이상의 속도에서 후진
        g_M010_currentCarStatus.isReversing = true;
        // 후진 속도는 개략적인 속도(velocityMPS)를 그대로 사용하거나 별도 계산
    } else {
        g_M010_currentCarStatus.isReversing = false;
    }
}

// 좌, 우회전 각도 판단 함수
void M010_updateTurnAngle() {
    float v_currentYaw = g_M010_ypr[0] * 180 / M_PI; // 요 각도 (라디안을 도로 변환)

    // 요 각도 변화를 누적하여 회전 각도 계산
    // 롤오버 방지를 위해 이전 요 각도와 현재 요 각도 간의 차이를 계산
    float v_yawDiff = v_currentYaw - g_M010_previousYaw;

    // 각도 차이가 -180도를 넘거나 +180도를 넘는 경우 처리 (쿼터니언의 특성)
    if (v_yawDiff > 180) {
        v_yawDiff -= 360;
    } else if (v_yawDiff < -180) {
        v_yawDiff += 360;
    }

    g_M010_currentCarStatus.turnAngle = v_yawDiff; // 현재 사이클에서의 회전 각도
    g_M010_previousYaw = v_currentYaw; // 이전 요 각도 업데이트
}

// 정차, 주차 여부 판단 함수
void M010_updateParkStopStatus() {
    // 가속도 벡터의 크기를 사용하여 움직임 여부 판단
    float v_totalAccelMagnitude = sqrt(pow(g_M010_filteredAccelX, 2) + pow(g_M010_filteredAccelY, 2) + pow(g_M010_filteredAccelZ, 2));

    if (v_totalAccelMagnitude > G_M010_ACCEL_THRESHOLD_STOP_START) {
        g_M010_currentCarStatus.parkStopStatus = "움직임";
        g_M010_lastMovementTime = millis(); // 움직임 감지 시 시간 업데이트
    } else {
        // 움직임이 없을 때 시간 확인
        if (millis() - g_M010_lastMovementTime > G_M010_STOP_TIME_THRESHOLD_MS) {
            g_M010_currentCarStatus.parkStopStatus = "주차";
        } else {
            g_M010_currentCarStatus.parkStopStatus = "정차";
        }
    }
}

void setup() {
    Serial.begin(115200); // 시리얼 통신 시작

    // MPU6050 초기화 및 DMP 설정
    M010_setupMPU6050();

    // 필터 버퍼 초기화
    for (int v_i = 0; v_i < G_M010_FILTER_WINDOW_SIZE; v_i++) {
        g_M010_accelXBuffer[v_i] = 0.0;
        g_M010_accelYBuffer[v_i] = 0.0;
        g_M010_accelZBuffer[v_i] = 0.0;
    }

    g_M010_lastMovementTime = millis(); // 초기값 설정
    g_M010_lastYawUpdateTime = millis();
}

void loop() {
    if (!g_M010_dmpReady) return; // DMP가 준비되지 않았으면 루프 실행 안함

    if (g_M010_mpu.dmpGetCurrentFIFOPacket(g_M010_fifoBuffer)) { // 최신 FIFO 패킷 읽기
        // 쿼터니언 데이터 처리
        mpu.dmpGetQuaternion(&g_M010_q, g_M010_fifoBuffer);
        // 중력 벡터 계산
        mpu.dmpGetGravity(&g_M010_gravity, &g_M010_q);
        // 요, 피치, 롤 (오일러 각도) 계산
        mpu.dmpGetYawPitchRoll(g_M010_ypr, &g_M010_q, &g_M010_gravity);
        // 월드 좌표계 가속도 계산
        mpu.dmpGetAccel(&g_M010_aa, g_M010_fifoBuffer);
        mpu.dmpGetLinearAccel(&g_M010_aaReal, &g_M010_aa, &g_M010_gravity); // 중력 제거된 가속도
        mpu.dmpGetLinearAccelInWorld(&g_M010_aaWorld, &g_M010_aaReal, &g_M010_q); // 월드 좌표계 가속도

        // 이동 평균 필터 적용 (월드 좌표계 가속도에 적용)
        g_M010_filteredAccelX = M010_applyMovingAverageFilter(g_M010_aaWorld.x, g_M010_accelXBuffer, g_M010_filterIndex, g_M010_filteredAccelX);
        g_M010_filteredAccelY = M010_applyMovingAverageFilter(g_M010_aaWorld.y, g_M010_accelYBuffer, g_M010_filterIndex, g_M010_filteredAccelY);
        g_M010_filteredAccelZ = M010_applyMovingAverageFilter(g_M010_aaWorld.z, g_M010_accelZBuffer, g_M010_filterIndex, g_M010_filteredAccelZ);

        // 자동차 상태 업데이트
        M010_updateSpeedAndAcceleration(); // 속도 및 가속/감속
        M010_updateSlope(); // 오르막/내리막
        M010_updateSpeedBump(); // 과속 방지턱
        M010_updateReverseStatus(); // 후진 여부
        M010_updateTurnAngle(); // 좌, 우회전 각도
        M010_updateParkStopStatus(); // 정차/주차

        // 5초 간격으로 시리얼 출력
        if (millis() - g_M010_lastReportTime > G_M010_REPORT_INTERVAL_MS) {
            Serial.println("--- 자동차 상태 ---");
            Serial.print("개략적인 속도: "); Serial.print(g_M010_currentCarStatus.velocityMPS); Serial.println(" m/s");
            Serial.print("가속/감속 상태: "); Serial.println(g_M010_currentCarStatus.accelDecelStatus);
            Serial.print("경사 상태: "); Serial.println(g_M010_currentCarStatus.slopeStatus);
            Serial.print("과속 방지턱: "); Serial.println(g_M010_currentCarStatus.isSpeedBump ? "통과 중" : "아님");
            Serial.print("후진 여부: "); Serial.println(g_M010_currentCarStatus.isReversing ? "후진 중" : "전진 또는 정지");
            Serial.print("회전 각도: "); Serial.print(g_M010_currentCarStatus.turnAngle); Serial.println(" 도");
            Serial.print("정차/주차 상태: "); Serial.println(g_M010_currentCarStatus.parkStopStatus);
            Serial.println("--------------------");
            g_M010_lastReportTime = millis();
        }
    }
}
