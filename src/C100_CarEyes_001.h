//====================================================
// 자동차 후방 로봇 눈 프로젝트 - Refactoring 및 전체 구현
// ESP32 + MPU6050 + FastLED (2x 8x8 WS2812B Matrix, 직렬 연결)
//
// 기능 요구사항:
// - MPU6050 센서 데이터(가속도, 각속도) 및 시간 정보만 사용
// - 차량 신호 입력 없음
// - MPU6050 데이터 노이즈 필터링 포함 (LPF, 기울기 계산 시 보정)
// - 차량 상태 추론 (정지, 주행, 가속, 감속, 회전, 충격, 정지 시 기울어짐)
// - 추론된 상태에 따른 로봇 눈 표현 (FastLED 사용)
// - 8x8 RGB LED 매트릭스 2개 사용 (좌/우 눈), WS2812B 직렬 연결 (지그재그 배선 가정)
// - 네이밍 컨벤션 적용:
//   - 전역 변수: g_C100_
//   - 전역 상수: G_C100_
//   - 함수명: C100_
//   - 로컬 변수: v_
//   - 함수 파라미터: p_
//====================================================

#include <Wire.h>               // I2C 통신 라이브러리
#include <Adafruit_MPU6050.h>   // MPU6050 센서 라이브러리
#include <FastLED.h>            // WS2812B 등 LED 제어 라이브러리
#include <math.h>               // sqrt, atan2 등을 위한 수학 함수

// --- 하드웨어 설정 (전역 상수) ---
#define G_C100_MPU_I2C_SDA 21         // MPU6050 I2C SDA 핀 (ESP32 기본값 또는 사용자 지정)
#define G_C100_MPU_I2C_SCL 22         // MPU6050 I2C SCL 핀 (ESP32 기본값 또는 사용자 지정)
#define G_C100_LED_DATA_PIN 23        // FastLED 데이터 핀 (사용자 지정 ESP32 GPIO 핀)

#define G_C100_COLOR_ORDER GRB        // LED 색상 순서 (WS2812B는 GRB가 많음, 확인 필요)
#define G_C100_LED_TYPE    WS2812B    // 사용 LED 칩 타입

#define G_C100_MATRIX_WIDTH 8         // 매트릭스 가로 픽셀 수
#define G_C100_MATRIX_HEIGHT 8        // 매트릭스 세로 픽셀 수
#define G_C100_NUM_MATRICES 2         // 사용 매트릭스 개수 (좌/우 눈)
#define G_C100_NUM_LEDS_PER_MATRIX (G_C100_MATRIX_WIDTH * G_C100_MATRIX_HEIGHT)
#define G_C100_TOTAL_NUM_LEDS (G_C100_NUM_LEDS_PER_MATRIX * G_C100_NUM_MATRICES) // 전체 LED 개수

// --- LED 배열 선언 (전역 변수) ---
CRGB g_C100_leds[G_C100_TOTAL_NUM_LEDS]; // 전체 LED를 담을 배열

// --- MPU6050 객체 선언 (전역 변수) ---
Adafruit_MPU6050 g_C100_mpu;
sensors_event_t g_C100_a, g_C100_g, g_C100_temp; // 가속도, 자이로, 온도 이벤트 데이터 구조체

// --- 노이즈 필터링 관련 변수 (전역 변수) ---
// 간단한 LPF 계수 (0.0 ~ 1.0, 작을수록 필터링 강함, 튜닝 필요)
float g_C100_alpha = 0.1;
// 필터링된 가속도 및 자이로 값 저장 변수
float g_C100_filtered_ax, g_C100_filtered_ay, g_C100_filtered_az;
float g_C100_filtered_gx, g_C100_filtered_gy, g_C100_filtered_gz;

// 기울기 계산을 위한 변수 (Roll, Pitch) - 정지 시 기울어짐 판단에 사용
// 가속도 센서만으로 계산 (정지 시에만 유효)
float g_C100_accel_roll = 0.0, g_C100_accel_pitch = 0.0;
// 상보 필터 사용 시 변수 (더 안정적인 기울기 측정 가능하나 구현 복잡성 증가)
// float g_C100_comp_roll = 0.0, g_C100_comp_pitch = 0.0;
// unsigned long g_C100_last_filter_time = 0; // 상보 필터 시간 간격 계산용

// --- 차량 상태 정의 (Enum 및 전역 변수) ---
enum CarState { // Enum 타입 이름은 규칙에서 제외
  G_C100_STATE_UNKNOWN,          // 초기 또는 알 수 없는 상태
  G_C100_STATE_STOPPED,          // 정지
  G_C100_STATE_MOVING_NORMAL,    // 일반 주행
  G_C100_STATE_ACCELERATING,     // 가속 중
  G_C100_STATE_BRAKING,          // 감속 중
  G_C100_STATE_TURNING_LEFT,     // 좌회전 중
  G_C100_STATE_TURNING_RIGHT,    // 우회전 중
  G_C100_STATE_BUMP,             // 충격 또는 요철 통과
  G_C100_STATE_TILTED            // 정지 상태에서 기울어짐
  // G_C100_STATE_REVERSING,       // MPU 단독 판단 어려움, 미사용
};
CarState g_C100_currentCarState = G_C100_STATE_UNKNOWN;
CarState g_C100_previousCarState = G_C100_STATE_UNKNOWN; // 상태 변화 감지용

// --- 로봇 눈 표현 상태 정의 (Enum 및 전역 변수) ---
enum EyeState { // Enum 타입 이름은 규칙에서 제외
  G_C100_E_NEUTRAL,       // 기본 눈
  G_C100_E_BLINK,         // 깜빡임 (애니메이션)
  G_C100_E_LOOK_LEFT,     // 왼쪽 보기
  G_C100_E_LOOK_RIGHT,    // 오른쪽 보기
  G_C100_E_LOOK_UP,       // 위 보기
  G_C100_E_LOOK_DOWN,     // 아래 보기
  G_C100_E_SQUINT,        // 약하게 찡그림
  G_C100_E_SQUINT_TIGHT,  // 강하게 찡그림 (악!)
  G_C100_E_ANGRY,         // 화남 (감속/충격 시)
  G_C100_E_ABSURD,        // 황당/놀람 (감속/충격 시)
  G_C100_E_GLARING,       // 불쾌함/노려봄 (충격 시)
  G_C100_E_SLEEPY,        // 졸림 (매우 긴 정지 시)
  G_C100_E_CONFUSED,      // 혼란스러움 (예비 상태)
  // 추가적인 눈 모양 정의 가능
};
EyeState g_C100_currentEyeState = G_C100_E_NEUTRAL; // 현재 로봇 눈 표현 상태

// --- 상태 지속 시간 및 타이머 (전역 변수) ---
unsigned long g_C100_state_start_time = 0; // 현재 차량 상태가 시작된 시간 (millis())
unsigned long g_C100_stop_duration = 0;    // 정지 상태 지속 시간 (ms)

unsigned long g_C100_last_blink_time = 0;    // 마지막 깜빡임/둘러보기 시작 시간
unsigned long g_C100_blink_interval = 5000;  // 다음 깜빡임/둘러보기까지 간격 (ms, 랜덤 가능)
unsigned long g_C100_blink_duration = 200;   // 깜빡임 애니메이션 지속 시간 (ms)
unsigned long g_C100_look_animation_duration = 500; // 둘러보기 애니메이션 지속 시간 (ms)

// --- 임계값 설정 (전역 상수!!! 실제 차량/장착 환경에서 튜닝 필수) ---
// 가속도 및 자이로 값 단위는 MPU6050 라이브러리 및 설정에 따라 다름 (Adafruit 라이브러리는 가속도 m/s^2, 자이로 deg/s 기본)
// 여기서는 MPU6050_RANGE_8_G (가속도 약 8*9.8 m/s^2), MPU6050_RANGE_500_DEG (자이로 500 deg/s) 기준 임계값 예시
// 실제 MPU6050.setAccelerometerRange, setGyroRange 설정에 맞춰 임계값 조정 필요!
// 또한 필터링 강도(g_C100_alpha)에 따라서도 임계값 조정 필요!

const float G_C100_ACCEL_IDLE_THRESHOLD_G = 0.2; // 정지 판별 가속도 변화 임계값 (g 단위 환산값 근처)
// Adafruit 라이브러리는 m/s^2 이므로 약 9.8m/s^2이 중력가속도.
// 정지 시 가속도 크기는 약 9.8 m/s^2 근처.
// abs(accel_magnitude - 9.8) < G_C100_ACCEL_IDLE_THRESHOLD_MS2 로 사용
const float G_C100_ACCEL_IDLE_THRESHOLD_MS2 = 9.8 * G_C100_ACCEL_IDLE_THRESHOLD_G; // 예시: 0.2g -> 약 1.96 m/s^2

const float G_C100_GYRO_IDLE_THRESHOLD_DEGS = 3.0; // 정지 판별 자이로 변화 임계값 (deg/s)

const float G_C100_ACCEL_FORWARD_THRESHOLD_MS2 = 2.5; // 전진 가속 판별 Y축 임계값 (m/s^2)
const float G_C100_ACCEL_BRAKE_THRESHOLD_MS2 = -3.0;  // 급감속 판별 Y축 임계값 (m/s^2)

const float G_C100_GYRO_TURN_THRESHOLD_DEGS = 15.0; // 회전 판별 Z축 임계값 (deg/s)

// 충격/요철 판별 가속도 변화 임계값 (필터링 후 사용, 튜닝 매우 중요)
// 이전 필터링 값과의 차이 또는 순간적인 절대값 크기 등으로 판단 가능
// 여기서는 순간적인 필터링된 가속도 절대값 크기 임계값으로 예시 (다른 상태와 겹치지 않게 로직 필요)
const float G_C100_ACCEL_BUMP_THRESHOLD_MS2 = 5.0; // 예시: 5 m/s^2 이상의 순간 가속도

const float G_C100_TILT_ANGLE_THRESHOLD_DEG = 5.0; // 정지 시 기울어짐 판별 각도 임계값 (도)

const unsigned long G_C100_SHORT_STOP_DURATION = 4000;  // 짧은 정지 시간 기준 (ms)
const unsigned long G_C100_LONG_STOP_DURATION = 20000; // 긴 정지 시간 기준 (ms)

// PI 값 정의 (math.h에 있을 수 있으나 명시적으로 정의)
#ifndef PI
#define PI 3.14159265358979323846
#endif

// --- 함수 선언 ---
// 핵심 로직 함수
void C100_readMPUData();
void C100_applyFiltering();
void C100_inferCarState();
void C100_updateEyeState();
void C100_drawCurrentEyeState(); // 현재 눈 상태에 맞는 눈을 그리는 함수

// LED 매트릭스 픽셀 인덱스 계산 함수 (중요! 실제 하드웨어 배선에 맞게 수정 필요!)
int C100_getLedIndex(uint8_t p_matrix_index, uint8_t p_x, uint8_t p_y);

// 기본 그리기 함수
void C100_clearDisplay(); // 모든 LED 끄기
void C100_drawPixel(uint8_t p_matrix_index, uint8_t p_x, uint8_t p_y, CRGB p_color); // 특정 픽셀 켜기

// 눈 모양 그리기 함수 (각 눈 상태에 맞춰 구현)
// 이 함수들은 C100_getLedIndex와 C100_drawPixel/FastLED 함수를 사용하여 눈 모양을 그림
void C100_drawEyeNeutral(uint8_t p_matrix_index); // 기본 눈
void C100_drawEyeBlink(uint8_t p_matrix_index, unsigned long p_animation_phase); // 깜빡임 (애니메이션 단계 전달)
void C100_drawEyeLook(uint8_t p_matrix_index, int8_t p_target_x, int8_t p_target_y); // 특정 방향 보기 (상대 좌표 전달)
void C100_drawEyeSquint(uint8_t p_matrix_index, bool p_tight); // 찡그림 (강도 전달)
void C100_drawEyeAngry(uint8_t p_matrix_index); // 화남
void C100_drawEyeAbsurd(uint8_t p_matrix_index); // 황당/놀람
void C100_drawEyeGlaring(uint8_t p_matrix_index); // 불쾌함/노려봄
void C100_drawEyeSleepy(uint8_t p_matrix_index); // 졸림
void C100_drawEyeConfused(uint8_t p_matrix_index); // 혼란스러움

// --- 설정 (Setup) 함수 ---
void setup() {
  Serial.begin(115200); // 시리얼 통신 시작 (디버깅용)
  Wire.begin(G_C100_MPU_I2C_SDA, G_C100_MPU_I2C_SCL); // MPU I2C 통신 시작

  // MPU6050 초기화
  if (!g_C100_mpu.begin()) {
    Serial.println("MPU6050 센서 초기화 실패! 배선 및 전원을 확인하세요.");
    while (1) delay(10); // 센서 초기화 실패 시 무한 대기
  }
  Serial.println("MPU6050 센서 초기화 완료.");

  // MPU6050 설정 (측정 범위 및 필터 대역폭 설정)
  // 설정에 따라 임계값 (Threshold) 튜닝 필수!
  g_C100_mpu.setAccelerometerRange(MPU6050_RANGE_8_G);     // 가속도 범위 설정 (예: +/- 8g)
  g_C100_mpu.setGyroRange(MPU6050_RANGE_500_DEG);        // 자이로 범위 설정 (예: +/- 500 deg/s)
  g_C100_mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);     // 디지털 저역 통과 필터 설정 (센서 자체 필터링)
  // MPU6050_BAND_260_HZ (필터링 약함) ~ MPU6050_BAND_5_HZ (필터링 강함)

  // FastLED 초기화
  FastLED.addLeds<G_C100_LED_TYPE, G_C100_LED_DATA_PIN, G_C100_COLOR_ORDER>(g_C100_leds, G_C100_TOTAL_NUM_LEDS).setCorrection(TypicalLEDStrip); // LED 설정
  FastLED.setBrightness(80); // 전체 밝기 설정 (0-255), 너무 밝으면 눈부심 및 전력 소모 증가

  // 초기 필터 값 설정 (첫 센서 값으로 초기화)
  C100_readMPUData(); // 초기 값 한번 읽기
  g_C100_filtered_ax = g_C100_a.acceleration.x;
  g_C100_filtered_ay = g_C100_a.acceleration.y;
  g_C100_filtered_az = g_C100_a.acceleration.z;
  g_C100_filtered_gx = g_C100_g.gyro.x;
  g_C100_filtered_gy = g_C100_g.gyro.y;
  g_C100_filtered_gz = g_C100_g.gyro.z;
  // g_C100_last_filter_time = millis(); // 상보 필터 사용 시 초기화

  g_C100_currentCarState = G_C100_STATE_UNKNOWN;      // 초기 차량 상태
  g_C100_previousCarState = G_C100_STATE_UNKNOWN;
  g_C100_currentEyeState = G_C100_E_NEUTRAL;          // 초기 눈 표현

  g_C100_state_start_time = millis();          // 상태 시작 시간 기록 시작
  g_C100_last_blink_time = millis();           // 깜빡임 타이머 시작
  randomSeed(analogRead(0));            // 랜덤 시드 초기화 (무작위 깜빡임/둘러보기 사용 시)
}

// --- 메인 루프 (Loop) 함수 ---
void loop() {
  unsigned long v_currentTime = millis(); // 현재 시간

  // 1. MPU 데이터 읽기
  C100_readMPUData();

  // 2. 노이즈 필터링 적용
  C100_applyFiltering();

  // 3. 차량 상태 추론
  C100_inferCarState();

  // 4. 눈 표현 상태 업데이트 (상태 머신)
  C100_updateEyeState();

  // 5. 디스플레이에 눈 그리기
  C100_clearDisplay(); // 매 프레임 LED 버퍼 초기화
  C100_drawCurrentEyeState(); // 현재 눈 상태에 맞는 눈을 그림
  FastLED.show(); // LED 업데이트 내용을 실제 LED에 표시

  // 센서 읽기 및 루프 실행 간격 제어 (너무 빠르면 필터링 효과 감소 및 부하 증가)
  // MPU6050의 샘플링 속도와 처리할 내용에 따라 적절히 조절
  delay(20); // 예시: 20ms 간격 (약 50 FPS)
}

//====================================================
// --- 함수 정의 ---
//====================================================

// MPU6050 센서 데이터 읽기 함수
void C100_readMPUData() {
  g_C100_mpu.getEvent(&g_C100_a, &g_C100_g, &g_C100_temp); // 가속도, 자이로, 온도 최신 값 읽기

  // 시리얼 모니터로 Raw 데이터 확인 (디버깅 시 유용)
  /*
  Serial.print("Raw Accel (m/s^2): "); Serial.print(g_C100_a.acceleration.x); Serial.print(", "); Serial.print(g_C100_a.acceleration.y); Serial.print(", "); Serial.println(g_C100_a.acceleration.z);
  Serial.print("Raw Gyro (deg/s): "); Serial.print(g_C100_g.gyro.x); Serial.print(", "); Serial.print(g_C100_g.gyro.y); Serial.print(", "); Serial.println(g_C100_g.gyro.z);
  */
}

// 노이즈 필터링 적용 함수 (LPF 및 정지 시 기울기 계산 포함)
void C100_applyFiltering() {
  // 1차 LPF 적용 (가속도 및 자이로 각 축)
  g_C100_filtered_ax = g_C100_alpha * g_C100_a.acceleration.x + (1.0 - g_C100_alpha) * g_C100_filtered_ax;
  g_C100_filtered_ay = g_C100_alpha * g_C100_a.acceleration.y + (1.0 - g_C100_alpha) * g_C100_filtered_ay;
  g_C100_filtered_az = g_C100_alpha * g_C100_a.acceleration.z + (1.0 - g_C100_alpha) * g_C100_filtered_az;
  g_C100_filtered_gx = g_C100_alpha * g_C100_g.gyro.x + (1.0 - g_C100_alpha) * g_C100_filtered_gx;
  g_C100_filtered_gy = g_C100_alpha * g_C100_g.gyro.y + (1.0 - g_C100_alpha) * g_C100_filtered_gy;
  g_C100_filtered_gz = g_C100_alpha * g_C100_g.gyro.z + (1.0 - g_C100_alpha) * g_C100_filtered_gz;

  // 시리얼 모니터로 필터링된 데이터 확인 (튜닝 시 필수)
  /*
  Serial.print("Filtered Accel (m/s^2): "); Serial.print(g_C100_filtered_ax); Serial.print(", "); Serial.print(g_C100_filtered_ay); Serial.print(", "); Serial.println(g_C100_filtered_az);
  Serial.print("Filtered Gyro (deg/s): "); Serial.print(g_C100_filtered_gx); Serial.print(", "); Serial.print(g_C100_filtered_gy); Serial.print(", "); Serial.println(g_C100_filtered_gz);
  */

  // 정지 상태 판단을 위한 가속도/자이로 벡터 크기 계산
  // 가속도 크기는 중력가속도(약 9.8) 근처여야 함
  float v_accel_magnitude = sqrt(g_C100_filtered_ax*g_C100_filtered_ax + g_C100_filtered_ay*g_C100_filtered_ay + g_C100_filtered_az*g_C100_filtered_az);
  float v_gyro_magnitude = sqrt(g_C100_filtered_gx*g_C100_filtered_gx + g_C100_filtered_gy*g_C100_filtered_gy + g_C100_filtered_gz*g_C100_filtered_gz);

  // 시리얼 모니터로 필터링된 크기 확인 (튜닝 시 필수)
  /*
  Serial.print("Accel Mag: "); Serial.print(v_accel_magnitude); Serial.print(", Gyro Mag: "); Serial.println(v_gyro_magnitude);
  */


  // --- 정지 상태일 때만 가속도 센서로 Roll/Pitch 각도 계산 ---
  // MPU6050이 완전히 수평일 때 ax=0, ay=0, az=~9.8 로 가정한 계산 (장착 방향에 따라 atan2의 인자 순서 및 부호 변경 필수!)
  // 상보 필터 사용 시 이 부분을 대체하거나 보완
  if (g_C100_currentCarState == G_C100_STATE_STOPPED || g_C100_previousCarState == G_C100_STATE_STOPPED || g_C100_currentCarState == G_C100_STATE_TILTED || g_C100_previousCarState == G_C100_STATE_TILTED) { // 정지/기울어짐 상태 근처에서 계산 유효
      g_C100_accel_roll = atan2(g_C100_filtered_ay, g_C100_filtered_az) * 180.0 / PI; // Y, Z 축으로 Roll 각도 계산 (예시)
      g_C100_accel_pitch = atan2(-g_C100_filtered_ax, sqrt(g_C100_filtered_ay*g_C100_filtered_ay + g_C100_filtered_az*g_C100_filtered_az)) * 180.0 / PI; // X, YZ 평면으로 Pitch 각도 계산 (예시)

       /*
       Serial.print("Accel Roll: "); Serial.print(g_C100_accel_roll); Serial.print(", Pitch: "); Serial.println(g_C100_accel_pitch);
       */
  }
}

// 차량 상태 추론 로직 함수
void C100_inferCarState() {
  unsigned long v_currentTime = millis();
  CarState v_nextCarState = g_C100_currentCarState; // 기본적으로 현재 상태 유지

  // 필터링된 가속도 및 자이로 벡터 크기 계산 (C100_applyFiltering에서 이미 계산)
  float v_accel_magnitude = sqrt(g_C100_filtered_ax*g_C100_filtered_ax + g_C100_filtered_ay*g_C100_filtered_ay + g_C100_filtered_az*g_C100_filtered_az);
  float v_gyro_magnitude = sqrt(g_C100_filtered_gx*g_C100_filtered_gx + g_C100_filtered_gy*g_C100_filtered_gy + g_C100_filtered_gz*g_C100_filtered_gz);

  // --- 상태 판단 우선순위 (순서 중요!) ---
  // 순간적인 상태 (충격, 급감속, 가속, 회전)를 먼저 판단하고, 아니면 정지 또는 일반 주행 판단

  // 1. 충격/요철 감지 (다른 움직임 상태가 아닐 때의 순간적인 큰 가속도 변화)
  // 간단한 구현: 필터링된 가속도 절대값 임계값 초과 및 자이로 변화 적을 때
  // 더 정확한 구현: 이전 가속도 값과의 변화량, 또는 특정 주파수 대역 분석 필요
  bool v_isMovingState = (g_C100_currentCarState != G_C100_STATE_STOPPED && g_C100_currentCarState != G_C100_STATE_TILTED && g_C100_currentCarState != G_C100_STATE_UNKNOWN);

  if (v_gyro_magnitude < G_C100_GYRO_TURN_THRESHOLD_DEGS && // 회전 중이 아님
      ! (g_C100_currentCarState == G_C100_STATE_ACCELERATING || g_C100_currentCarState == G_C100_STATE_BRAKING) && // 가감속 중이 아님
      (abs(g_C100_filtered_ax) > G_C100_ACCEL_BUMP_THRESHOLD_MS2 || abs(g_C100_filtered_ay) > G_C100_ACCEL_BUMP_THRESHOLD_MS2 || abs(g_C100_filtered_az - 9.8) > G_C100_ACCEL_BUMP_THRESHOLD_MS2) // 순간적인 큰 가속도 변화
     )
  {
      v_nextCarState = G_C100_STATE_BUMP;
  }
  // 충격 상태는 매우 짧게 유지 후 이전 상태로 복귀하는 로직 필요 (C100_updateEyeState 또는 별도 타이머)
  if (g_C100_currentCarState == G_C100_STATE_BUMP && (v_currentTime - g_C100_state_start_time) > 500) { // 예시: 충격 후 0.5초 지나면
       v_nextCarState = g_C100_previousCarState; // 충격 직전 상태로 복귀 (또는 G_C100_STATE_MOVING_NORMAL)
  }


  // 2. 정지 상태 감지 (충격 감지 후 확인)
  // 가속도 크기가 중력가속도 근처이고 변화가 적으며, 자이로 값도 매우 작을 때
  else if (abs(v_accel_magnitude - 9.8) < G_C100_ACCEL_IDLE_THRESHOLD_MS2 && v_gyro_magnitude < G_C100_GYRO_IDLE_THRESHOLD_DEGS) {
    if (g_C100_currentCarState != G_C100_STATE_STOPPED && g_C100_currentCarState != G_C100_STATE_TILTED) {
      // 정지 또는 기울어짐 상태로 방금 진입했으면 시간 기록 시작
      v_nextCarState = G_C100_STATE_STOPPED; // 일단 정지로 판단
      g_C100_state_start_time = v_currentTime;
      g_C100_stop_duration = 0; // 새로 정지했으니 시간 0으로 초기화
    } else {
      // 이미 정지 또는 기울어짐 상태면 지속 시간 업데이트
      g_C100_stop_duration = v_currentTime - g_C100_state_start_time;

       // 정지 상태일 때만 기울어짐 추가 판단
       // C100_applyFiltering에서 계산된 g_C100_accel_roll, g_C100_accel_pitch 사용
       if (abs(g_C100_accel_roll) > G_C100_TILT_ANGLE_THRESHOLD_DEG || abs(g_C100_accel_pitch) > G_C100_TILT_ANGLE_THRESHOLD_DEG) {
           v_nextCarState = G_C100_STATE_TILTED; // 정지 상태이면서 기울어졌음
       } else {
           v_nextCarState = G_C100_STATE_STOPPED; // 기울어지지 않은 정지 상태
       }
    }
  }
  // 정지 또는 기울어짐 상태가 아니라면 정지 시간 초기화
  else {
       g_C100_stop_duration = 0; // 정지 풀림
       if (g_C100_currentCarState == G_C100_STATE_STOPPED || g_C100_currentCarState == G_C100_STATE_TILTED) {
           // 정지/기울어짐 상태에서 방금 벗어난 경우 시간 기록 시작
           g_C100_state_start_time = v_currentTime;
       }
        // 계속 움직이는 중이면 g_C100_state_start_time 유지
  }


  // 3. 회전 감지 (정지/충격 감지 후 확인)
  // Z축 자이로 값이 특정 임계값 이상일 때 (부호로 방향 구분)
  if (v_nextCarState != G_C100_STATE_STOPPED && v_nextCarState != G_C100_STATE_TILTED && v_nextCarState != G_C100_STATE_BUMP) { // 정지, 기울어짐, 충격 상태가 아닐 때만 회전 판단
       if (g_C100_filtered_gz > G_C100_GYRO_TURN_THRESHOLD_DEGS) {
         v_nextCarState = G_C100_STATE_TURNING_RIGHT;
       } else if (g_C100_filtered_gz < -G_C100_GYRO_TURN_THRESHOLD_DEGS) {
         v_nextCarState = G_C100_STATE_TURNING_LEFT;
       }
        // 회전 상태로 진입했으면 시간 기록 시작 (이미 움직이는 중이었더라도 특정 상태 진입으로 간주)
       if (g_C100_currentCarState != v_nextCarState) g_C100_state_start_time = v_currentTime;
  }


  // 4. 가속/감속 감지 (정지, 충격, 회전 감지 후 확인)
  // Y축 가속도 값이 특정 임계값 이상일 때 (부호로 방향 구분)
   if (v_nextCarState != G_C100_STATE_STOPPED && v_nextCarState != G_C100_STATE_TILTED && v_nextCarState != G_C100_STATE_BUMP &&
       v_nextCarState != G_C100_STATE_TURNING_LEFT && v_nextCarState != G_C100_STATE_TURNING_RIGHT) // 다른 특정 상태가 아닐 때만 가감속 판단
   {
       if (g_C100_filtered_ay > G_C100_ACCEL_FORWARD_THRESHOLD_MS2) {
         v_nextCarState = G_C100_STATE_ACCELERATING;
       } else if (g_C100_filtered_ay < G_C100_ACCEL_BRAKE_THRESHOLD_MS2) {
         v_nextCarState = G_C100_STATE_BRAKING;
       }
       // 가감속 상태로 진입했으면 시간 기록 시작
       if (g_C100_currentCarState != v_nextCarState) g_C100_state_start_time = v_currentTime;
   }


  // 5. 위 모든 상태가 아니면 일반 주행 또는 불분명 상태
  // (이전 상태가 정지였다가 움직이기 시작한 경우 등)
   if (v_nextCarState == G_C100_STATE_UNKNOWN || // 초기 상태
       ((g_C100_previousCarState == G_C100_STATE_STOPPED || g_C100_previousCarState == G_C100_STATE_TILTED) && v_nextCarState != G_C100_STATE_STOPPED && v_nextCarState != G_C100_STATE_TILTED) || // 정지/기울어짐에서 방금 벗어남
       (v_isMovingState && v_nextCarState != G_C100_STATE_ACCELERATING && v_nextCarState != G_C100_STATE_BRAKING && v_nextCarState != G_C100_STATE_TURNING_LEFT && v_nextCarState != G_C100_STATE_TURNING_RIGHT && v_nextCarState != G_C100_STATE_BUMP) // 움직이는 상태였으나 특정 움직임 임계값 아래로 내려옴
       )
    {
        v_nextCarState = G_C100_STATE_MOVING_NORMAL;
        if (g_C100_currentCarState != v_nextCarState) g_C100_state_start_time = v_currentTime;
    }


  // 시리얼 모니터로 차량 상태 변화 확인 (디버깅 시 유용)
  if (v_nextCarState != g_C100_currentCarState) {
    Serial.print("Car state changed from ");
    // Serial.print(C100_currentStateToString(g_C100_currentCarState)); // 상태를 문자열로 출력하는 헬퍼 함수 필요 시 구현
    Serial.print(g_C100_currentCarState);
    Serial.print(" to ");
    // Serial.println(C100_currentStateToString(v_nextCarState));
    Serial.println(v_nextCarState); // enum 값 자체 출력
  }

  g_C100_previousCarState = g_C100_currentCarState; // 현재 상태를 이전 상태로 업데이트
  g_C100_currentCarState = v_nextCarState; // 최종 결정된 다음 상태를 현재 상태로 업데이트
}

// 차량 상태에 따라 눈 표현 상태 업데이트 함수
void C100_updateEyeState() {
  unsigned long v_currentTime = millis();
  EyeState v_nextEyeState = g_C100_currentEyeState; // 기본적으로 현재 눈 표현 유지

  // 차량 상태별 눈 표현 결정
  switch (g_C100_currentCarState) {
    case G_C100_STATE_STOPPED:
      // 정지 시간 경과에 따른 눈 표현 변화
      if (g_C100_stop_duration < G_C100_SHORT_STOP_DURATION) {
        v_nextEyeState = G_C100_E_NEUTRAL;
        g_C100_last_blink_time = v_currentTime; // 정지 후 짧은 시간 동안은 깜빡임/둘러보기 타이머 리셋
      } else if (g_C100_stop_duration < G_C100_LONG_STOP_DURATION) {
        // 긴 정지: 주기적 깜빡임 또는 둘러보기 애니메이션
        // 애니메이션 중에는 해당 애니메이션 상태 유지
        if (g_C100_currentEyeState == G_C100_E_BLINK) {
            // 깜빡임 애니메이션 지속 시간 체크
            if (v_currentTime - g_C100_last_blink_time > g_C100_blink_duration) {
                 v_nextEyeState = G_C100_E_NEUTRAL; // 애니메이션 끝나면 기본으로
                 g_C100_last_blink_time = v_currentTime; // 다음 타이머 시작
            }
        } else if (g_C100_currentEyeState >= G_C100_E_LOOK_LEFT && g_C100_currentEyeState <= G_C100_E_LOOK_DOWN) { // 둘러보기 상태 범위
             // 둘러보기 애니메이션 지속 시간 체크
             if (v_currentTime - g_C100_last_blink_time > g_C100_look_animation_duration) {
                 v_nextEyeState = G_C100_E_NEUTRAL; // 애니메이션 끝나면 기본으로
                 g_C100_last_blink_time = v_currentTime; // 다음 타이머 시작
             }
        } else { // 현재 G_C100_E_NEUTRAL 상태 등에서 새로운 애니메이션 시작 타이밍 체크
             if (v_currentTime - g_C100_last_blink_time > g_C100_blink_interval) {
                // 무작위로 깜빡임 또는 둘러보기 선택
                if (random(10) < 7) { // 70% 확률로 깜빡임
                    v_nextEyeState = G_C100_E_BLINK;
                    g_C100_blink_duration = random(100, 300); // 깜빡임 시간 랜덤
                } else { // 30% 확률로 둘러보기
                    int v_look_dir = random(4); // 0:LEFT, 1:RIGHT, 2:UP, 3:DOWN
                    if (v_look_dir == 0) v_nextEyeState = G_C100_E_LOOK_LEFT;
                    else if (v_look_dir == 1) v_nextEyeState = G_C100_E_LOOK_RIGHT;
                    else if (v_look_dir == 2) v_nextEyeState = G_C100_E_LOOK_UP;
                    else v_nextEyeState = G_C100_E_LOOK_DOWN;
                    g_C100_look_animation_duration = random(400, 800); // 둘러보기 시간 랜덤
                }
                g_C100_last_blink_time = v_currentTime; // 새 애니메이션 시작 시간 기록
                g_C100_blink_interval = random(3000, 7000); // 다음 애니메이션 간격 랜덤
             }
        }
      } else { // 매우 긴 정지
        v_nextEyeState = G_C100_E_SLEEPY;
        // 졸린 눈 상태에서도 아주 느린 깜빡임 등 추가 애니메이션 로직 필요 시 구현
      }
      break;

    case G_C100_STATE_MOVING_NORMAL:
      v_nextEyeState = G_C100_E_NEUTRAL; // 또는 G_C100_E_LOOK_UP 등 기본 움직임 표현
      // 주행 중에도 가끔 아주 느린 깜빡임 추가 가능
       if (g_C100_currentEyeState == G_C100_E_BLINK) { // 깜빡임 애니메이션 중이면 지속
             if (v_currentTime - g_C100_last_blink_time > g_C100_blink_duration) {
                 v_nextEyeState = G_C100_E_NEUTRAL; // 끝나면 기본으로
                 g_C100_last_blink_time = v_currentTime; // 다음 타이머 시작
                 g_C100_blink_interval = random(7000, 15000); // 주행 중 깜빡임 간격은 더 길게
             }
       } else { // G_C100_E_NEUTRAL 상태 등에서 깜빡임 시작 타이밍 체크
            if (v_currentTime - g_C100_last_blink_time > g_C100_blink_interval) {
                v_nextEyeState = G_C100_E_BLINK;
                g_C100_blink_duration = random(100, 300);
                g_C100_last_blink_time = v_currentTime;
                g_C100_blink_interval = random(7000, 15000);
            }
       }

      break;

    case G_C100_STATE_ACCELERATING:
      v_nextEyeState = G_C100_E_SQUINT; // 가속 집중/약간 긴장
      // 눈동자 위치는 C100_drawCurrentEyeState 함수에서 G_C100_STATE_ACCELERATING일 때 별도 처리
      break;

    case G_C100_STATE_BRAKING:
      // 급감속 시작 시 순간적으로 강한 표현 후 찡그림 유지
      if (g_C100_currentCarState != g_C100_previousCarState) { // 감속 상태로 방금 진입
         v_nextEyeState = G_C100_E_ABSURD; // 또는 G_C100_E_ANGRY
      } else { // 감속 상태 유지 중
         // ABSURD/ANGRY 상태를 짧게 유지 후 SQUINT_TIGHT로 전환
         if (g_C100_currentEyeState == G_C100_E_ABSURD || g_C100_currentEyeState == G_C100_E_ANGRY) {
             if (v_currentTime - g_C100_state_start_time > 500) { // 예시: 0.5초 유지
                 v_nextEyeState = G_C100_E_SQUINT_TIGHT;
             }
         } else {
             v_nextEyeState = G_C100_E_SQUINT_TIGHT; // 이미 전환되었거나 처음부터 찡그림
         }
      }
       // 눈동자 순간 이동 애니메이션은 C100_drawCurrentEyeState 함수에서 G_C100_STATE_BRAKING일 때 처리
      break;

    case G_C100_STATE_TURNING_LEFT:
      v_nextEyeState = G_C100_E_LOOK_LEFT; // 왼쪽 보기
      // 찡그림 조합 및 강성 비례 눈동자 쏠림은 C100_drawCurrentEyeState 함수에서 G_C100_STATE_TURNING_LEFT일 때 처리
      break;

    case G_C100_STATE_TURNING_RIGHT:
      v_nextEyeState = G_C100_E_LOOK_RIGHT; // 오른쪽 보기
       // 찡그림 조합 및 강성 비례 눈동자 쏠림은 C100_drawCurrentEyeState 함수에서 G_C100_STATE_TURNING_RIGHT일 때 처리
      break;

    case G_C100_STATE_BUMP:
      // 충격 시작 시 순간적으로 강한 표현
      if (g_C100_currentCarState != g_C100_previousCarState) { // 충격 상태로 방금 진입
         v_nextEyeState = G_C100_E_ABSURD; // 또는 G_C100_E_GLARING
      }
      // 충격 상태는 짧게 유지 후 이전 상태로 복귀하므로 C100_updateEyeState에서 눈 상태를 직접 바꾸는 로직은 간단.
      // 중요한 것은 C100_inferCarState에서 G_C100_STATE_BUMP를 짧게 유지하고 다른 상태로 빠르게 전환하는 것.
      // 눈동자 순간 이동 애니메이션은 C100_drawCurrentEyeState 함수에서 G_C100_STATE_BUMP일 때 처리
      break;

    case G_C100_STATE_TILTED:
       // 기울어짐 상태에서는 눈동자 위치만 기울기 방향으로 조정 (C100_drawCurrentEyeState 함수에서 처리)
       v_nextEyeState = G_C100_E_NEUTRAL; // 눈 형태는 기본으로 유지
       // 찡그림 조합은 C100_drawCurrentEyeState 함수에서 기울기 각도에 따라 처리 가능
      break;

    case G_C100_STATE_UNKNOWN:
      v_nextEyeState = G_C100_E_CONFUSED; // 초기 또는 알 수 없는 상태 시 혼란스러운 눈
      break;

    // case G_C100_STATE_REVERSING: // MPU만으로는 어려움
    //   break;
  }

  // 최종 결정된 눈 표현 상태 업데이트
  if (v_nextEyeState != g_C100_currentEyeState) {
    Serial.print("Eye state changed to ");
    // 시리얼 출력용 눈 상태 문자열 변환 함수 필요 시 구현
    // Serial.println(C100_eyeStateToString(v_nextEyeState));
    Serial.println(v_nextEyeState); // enum 값 자체 출력
    g_C100_currentEyeState = v_nextEyeState;
    // 필요하다면 상태 변화 시 애니메이션 시작 시간 등 초기화
    // (예: 깜빡임/둘러보기 타이머는 정지 상태에서만 유효하게 작동하도록 로직 보강)
    // (예2: 충격/감속 애니메이션은 상태 진입 시 한 번만 실행되도록 플래그 사용)
  }
}

// 현재 눈 표현 상태에 맞는 눈을 LED 매트릭스에 그리는 함수
// 이 함수는 FastLED 함수들과 C100_getLedIndex를 사용하여 픽셀을 그림
void C100_drawCurrentEyeState() {
  unsigned long v_currentTime = millis(); // 현재 시간 (애니메이션에 사용)

  // 각 눈(매트릭스)별로 그리기 함수 호출
  for (uint8_t v_eye = 0; v_eye < G_C100_NUM_MATRICES; v_eye++) {
    // 현재 눈 상태에 따라 해당 그리기 함수 호출
    switch (g_C100_currentEyeState) {
      case G_C100_E_NEUTRAL:
        C100_drawEyeNeutral(v_eye);
        break;
      case G_C100_E_BLINK:
        // 깜빡임 애니메이션 진행 단계에 따라 그리기
        C100_drawEyeBlink(v_eye, v_currentTime - g_C100_last_blink_time);
        break;
      case G_C100_E_LOOK_LEFT:
         C100_drawEyeLook(v_eye, -1, 0); // 예시: 왼쪽 방향 지시 (-1, 0)
        break;
      case G_C100_E_LOOK_RIGHT:
        C100_drawEyeLook(v_eye, 1, 0); // 예시: 오른쪽 방향 지시 (1, 0)
        break;
      case G_C100_E_LOOK_UP:
         C100_drawEyeLook(v_eye, 0, -1); // 예시: 위쪽 방향 지시 (0, -1)
        break;
      case G_C100_E_LOOK_DOWN:
         C100_drawEyeLook(v_eye, 0, 1); // 예시: 아래쪽 방향 지시 (0, 1)
        break;
      case G_C100_E_SQUINT:
        C100_drawEyeSquint(v_eye, false); // 약하게 찡그림
        break;
      case G_C100_E_SQUINT_TIGHT:
        C100_drawEyeSquint(v_eye, true); // 강하게 찡그림
        break;
      case G_C100_E_ANGRY:
        C100_drawEyeAngry(v_eye);
        break;
      case G_C100_E_ABSURD:
        C100_drawEyeAbsurd(v_eye);
        // 충격/감속 시 눈동자 순간 이동 애니메이션 로직 필요 시 C100_drawEyeAbsurd 내에서 시간(v_currentTime - g_C100_state_start_time) 활용하여 구현
        break;
      case G_C100_E_GLARING:
        C100_drawEyeGlaring(v_eye);
        break;
      case G_C100_E_SLEEPY:
        C100_drawEyeSleepy(v_eye);
        break;
      case G_C100_E_CONFUSED:
        C100_drawEyeConfused(v_eye);
        break;
      // ... 기타 눈 모양 상태 처리 ...
    }
  }

    // 특정 차량 상태일 때 눈 모양 외 추가적인 그리기 또는 오버레이
    // 이 부분은 위에 그린 눈 모양 위에 덧그리는 방식이 될 수 있습니다.
    if (g_C100_currentCarState == G_C100_STATE_ACCELERATING) {
         // 가속 시 찡그림(G_C100_E_SQUINT)과 함께 눈동자를 중앙 또는 약간 위로 모으는 효과
         for(uint8_t v_eye=0; v_eye<G_C100_NUM_MATRICES; v_eye++) {
             // 눈동자만 따로 그리는 함수를 만들거나, C100_drawEyeLook 로직을 여기에 가져와 눈동자 픽셀만 그리기
             // 여기서는 간단히 C100_drawEyeLook을 사용하여 기본 눈 위에 눈동자 위치를 재지정
             C100_drawEyeLook(v_eye, 0, -1); // 눈동자를 약간 위로 (기존 눈 위에 덮어 그림)
         }
    } else if (g_C100_currentCarState == G_C100_STATE_BRAKING) {
         // 급감속 시 눈동자 순간 이동 애니메이션 (C100_drawEyeAbsurd 등에서 시간 기반 구현 필요)
         // 이 부분은 STATE_BRAKING 상태 진입 시 drawEyeAbsurd 등의 애니메이션을 트리거하고,
         // drawEyeAbsurd 함수 내에서 시간 경과에 따라 눈동자 위치를 이동시키는 것이 더 자연스러움.
    } else if (g_C100_currentCarState == G_C100_STATE_TILTED) {
         // 정지 시 기울어짐 상태에서는 기본 눈 형태(G_C100_E_NEUTRAL)를 유지하고 눈동자 위치만 기울기 방향으로 조정
         for(uint8_t v_eye=0; v_eye<G_C100_NUM_MATRICES; v_eye++) {
              // g_C100_accel_roll, g_C100_accel_pitch 값을 이용하여 눈동자 상대 위치 계산 후 그리기
              // 맵핑 범위는 실제 테스트 후 튜닝 필요
              int8_t v_pupil_offset_x = map(g_C100_accel_roll, -20, 20, -2, 2); // 예시: 롤 각도 -20~20도를 눈동자 x 오프셋 -2~2으로 매핑
              int8_t v_pupil_offset_y = map(g_C100_accel_pitch, -20, 20, -2, 2); // 예시: 피치 각도 -20~20도를 눈동자 y 오프셋 -2~2으로 매핑
              v_pupil_offset_x = constrain(v_pupil_offset_x, -2, 2); // 오프셋 범위 제한 (8x8 매트릭스에 맞게)
              v_pupil_offset_y = constrain(v_pupil_offset_y, -2, 2);

             // drawEyeLook 함수를 사용하여 눈동자만 해당 위치에 다시 그리기 (기존 눈 모양 위에 덧그림)
             C100_drawEyeLook(v_eye, v_pupil_offset_x, v_pupil_offset_y);
         }
    }
    // ... 기타 차량 상태별 추가 표현 ...
}


// LED 매트릭스 픽셀 인덱스 계산 함수 (!!! 중요 !!!)
// 2개의 8x8 지그재그(Serpentine) 매트릭스가 직렬 연결된 것을 가정한 구현.
// p_matrix_index: 0 또는 1 (좌/우 눈)
// p_x, p_y: 해당 매트릭스 내에서의 픽셀 좌표 (0 ~ G_C100_MATRIX_WIDTH-1, 0 ~ G_C100_MATRIX_HEIGHT-1)
int C100_getLedIndex(uint8_t p_matrix_index, uint8_t p_x, uint8_t p_y) {
  if (p_matrix_index >= G_C100_NUM_MATRICES || p_x >= G_C100_MATRIX_WIDTH || p_y >= G_C100_MATRIX_HEIGHT) {
    // 유효 범위를 벗어나면 -1 등 오류 값 반환
    return -1;
  }

  int v_index = -1;
  int v_base_index = p_matrix_index * G_C100_NUM_LEDS_PER_MATRIX; // 해당 매트릭스의 시작 LED 인덱스

  // 매트릭스 내부의 지그재그(Serpentine) 배선 계산
  if (p_y % 2 == 0) { // 짝수 행 (0, 2, 4, 6) -> 왼쪽에서 오른쪽 (x 증가)
    v_index = v_base_index + (p_y * G_C100_MATRIX_WIDTH + p_x);
  } else { // 홀수 행 (1, 3, 5, 7) -> 오른쪽에서 왼쪽 (x 감소)
    v_index = v_base_index + (p_y * G_C100_MATRIX_WIDTH + (G_C100_MATRIX_WIDTH - 1 - p_x));
  }

  // 계산된 인덱스가 전체 LED 범위 내에 있는지 최종 확인 (사실 위 로직이면 범위 안 벗어남)
  if (v_index < 0 || v_index >= G_C100_TOTAL_NUM_LEDS) return -1;

  return v_index;
}

// 모든 LED를 끄는 함수
void C100_clearDisplay() {
  FastLED.clear(); // 모든 LED 색상을 CRGB::Black으로 설정
}

// 특정 픽셀에 색상을 설정하는 함수 (C100_getLedIndex 활용)
void C100_drawPixel(uint8_t p_matrix_index, uint8_t p_x, uint8_t p_y, CRGB p_color) {
  int v_index = C100_getLedIndex(p_matrix_index, p_x, p_y);
  if (v_index != -1) { // 유효한 인덱스일 경우
    g_C100_leds[v_index] = p_color;
  }
}

//====================================================
// --- 눈 모양 그리기 함수들 (구현) ---
// 여기에 각 눈 모양 디자인을 8x8 픽셀 아트로 구현합니다.
// C100_drawPixel 또는 leds[] 배열과 C100_getLedIndex를 사용합니다.
//====================================================

// 기본 눈 모양 그리기 (간단한 원형 눈 + 가운데 눈동자)
void C100_drawEyeNeutral(uint8_t p_matrix_index) {
  int v_center_x = G_C100_MATRIX_WIDTH / 2; // 4
  int v_center_y = G_C100_MATRIX_HEIGHT / 2; // 4
  int v_eye_radius_sq = 3*3; // 눈 테두리 반경 제곱 예시
  int v_pupil_radius_sq = 1*1; // 눈동자 반경 제곱 예시

  for(uint8_t v_y = 0; v_y < G_C100_MATRIX_HEIGHT; v_y++) {
    for(uint8_t v_x = 0; v_x < G_C100_MATRIX_WIDTH; v_x++) {
      int dx = v_x - v_center_x;
      int dy = v_y - v_center_y;
      int dist_sq = dx*dx + dy*dy;

      // 눈 테두리 그리기 (정확한 원은 아니지만 근사)
      if (dist_sq <= v_eye_radius_sq && dist_sq > (v_eye_radius_sq - 3)) { // 테두리 두께 예시
          C100_drawPixel(p_matrix_index, v_x, v_y, CRGB::White);
      }

      // 눈동자 그리기 (가운데)
      if (dist_sq <= v_pupil_radius_sq) {
           C100_drawPixel(p_matrix_index, v_x, v_y, CRGB::White);
      }
    }
  }
}

// 깜빡임 눈 모양 그리기 (애니메이션 단계 전달)
void C100_drawEyeBlink(uint8_t p_matrix_index, unsigned long p_animation_phase) {
  // p_animation_phase: 깜빡임 애니메이션이 시작된 후 경과된 시간 (ms)
  // g_C100_blink_duration: 총 깜빡임 애니메이션 지속 시간 (ms)

  // 시간 경과에 따라 눈꺼풀 모양을 조절하여 깜빡임 표현
  // 0% ~ 50%까지는 감는 애니, 50% ~ 100%까지는 뜨는 애니
  unsigned long v_half_duration = g_C100_blink_duration / 2;
  int v_center_y = G_C100_MATRIX_HEIGHT / 2; // 4

  if (p_animation_phase < v_half_duration) {
    // 눈 감는 중간 단계 (아래에서 위로 감거나, 위에서 아래로 감는 애니 연출)
    // 여기서는 간단히 눈꺼풀 라인을 그리는 방식으로 구현 (가운데 라인)
     for(uint8_t v_x = 0; v_x < G_C100_MATRIX_WIDTH; v_x++) {
          C100_drawPixel(p_matrix_index, v_x, v_center_y, CRGB::White); // 가운데 가로선
     }
     // animation_phase에 따라 라인 두께나 위치 조절 가능 (더 부드러운 애니메이션)

  } else {
    // 눈 뜨는 중간 단계 또는 완전히 뜬 상태
    // 여기서는 간단히 완전히 뜬 상태 (기본 눈 모양)으로 돌아옴
    C100_drawEyeNeutral(p_matrix_index);
  }
}

// 특정 방향 보기 눈 모양 그리기 (상대 좌표 전달)
void C100_drawEyeLook(uint8_t p_matrix_index, int8_t p_target_x, int8_t p_target_y) {
    // p_target_x, p_target_y: 눈동자가 이동할 방향 지시 (예: -1:왼쪽, 1:오른쪽, 0:중앙)
    // 이 값을 기반으로 눈동자의 중심 픽셀 좌표 오프셋을 계산
    int v_center_x = G_C100_MATRIX_WIDTH / 2; // 4
    int v_center_y = G_C100_MATRIX_HEIGHT / 2; // 4
    int v_pupil_radius_sq = 1*1; // 눈동자 반경 제곱 예시

    // p_target 값을 눈동자가 움직일 수 있는 최대 오프셋으로 매핑
    // 예: target_x가 -1일 때 눈동자는 왼쪽 최대치, 1일 때 오른쪽 최대치
    int v_max_offset_x = 2; // 눈동자가 좌우로 움직일 수 있는 최대 픽셀 수
    int v_max_offset_y = 1; // 눈동자가 상하로 움직일 수 있는 최대 픽셀 수

    int v_pupil_center_x = v_center_x + p_target_x * v_max_offset_x;
    int v_pupil_center_y = v_center_y + p_target_y * v_max_offset_y;

     // 눈 테두리 그리기 (기본 눈 모양 사용)
     C100_drawEyeNeutral(p_matrix_index); // 또는 눈 테두리만 그리는 별도 함수 호출

    // 눈동자 그리기 (계산된 위치에)
   for(int v_py = -1; v_py <= 1; v_py++) { // 3x3 픽셀 영역에서 눈동자 그리기
       for(int v_px = -1; v_px <= 1; v_px++) {
           // 원형 눈동자 (반경 1 픽셀)
           if (v_px*v_px + v_py*v_py <= v_pupil_radius_sq) { // 원 범위 내 픽셀
               C100_drawPixel(p_matrix_index, v_pupil_center_x + v_px, v_pupil_center_y + v_py, CRGB::White);
           }
       }
   }
   // 필요 시 눈동자 색상 변경 가능
}

// 찡그림 눈 모양 그리기 (강도 전달)
void C100_drawEyeSquint(uint8_t p_matrix_index, bool p_tight) {
    // p_tight: true면 강하게, false면 약하게 찡그림

    int v_center_y = G_C100_MATRIX_HEIGHT / 2; // 4
    int v_squint_level = p_tight ? 2 : 1; // 찡그림 강도

    // 눈꺼풀 라인 그리기 (가로선 - 찡그린 모양)
    for(uint8_t v_x = 0; v_x < G_C100_MATRIX_WIDTH; v_x++) {
         // 위/아래 찡그린 라인
         if (v_center_y - v_squint_level >= 0) C100_drawPixel(p_matrix_index, v_x, v_center_y - v_squint_level, CRGB::White);
         if (v_center_y + v_squint_level < G_C100_MATRIX_HEIGHT) C100_drawPixel(p_matrix_index, v_x, v_center_y + v_squint_level, CRGB::White);
    }

    // 눈동자 그리기 (찡그렸을 때는 작게 또는 약간 가려지게)
    int v_center_x = G_C100_MATRIX_WIDTH / 2; // 4
    int v_pupil_radius_sq = 0*0; // 눈동자 없애거나 아주 작게

    // 눈동자 위치 (약간 위로 또는 가운데)
    int v_pupil_center_x = v_center_x;
    int v_pupil_center_y = v_center_y - v_squint_level/2; // 찡그린 정도에 따라 약간 위로 이동

    for(int v_py = -0; v_py <= 0; v_py++) { // 1x1 픽셀 영역에서 눈동자 (거의 안 보임)
       for(int v_px = -0; v_px <= 0; v_px++) {
           if (v_px*v_px + v_py*v_py <= v_pupil_radius_sq) {
               C100_drawPixel(p_matrix_index, v_pupil_center_x + v_px, v_pupil_center_y + v_py, CRGB::White);
           }
       }
    }
}

// 화남 눈 모양 그리기 (찡그림 + 눈썹)
void C100_drawEyeAngry(uint8_t p_matrix_index) {
    C100_drawEyeSquint(p_matrix_index, true); // 강하게 찡그린 모양 기본

    // 눈썹 모양 추가 (안쪽 아래로 쏠리게)
    int v_center_x = G_C100_MATRIX_WIDTH / 2; // 4
    int v_eyebrow_y = G_C100_MATRIX_HEIGHT / 2 - 3; // 눈꺼풀 위쪽 예시

    if (p_matrix_index == 0) { // 왼쪽 눈썹 (오른쪽 아래로)
        C100_drawPixel(p_matrix_index, v_center_x - 1, v_eyebrow_y, CRGB::White);
        C100_drawPixel(p_matrix_index, v_center_x - 2, v_eyebrow_y + 1, CRGB::White);
    } else { // 오른쪽 눈썹 (왼쪽 아래로)
        C100_drawPixel(p_matrix_index, v_center_x, v_eyebrow_y, CRGB::White);
        C100_drawPixel(p_matrix_index, v_center_x + 1, v_eyebrow_y + 1, CRGB::White);
    }
}

// 황당/놀람 눈 모양 그리기 (눈 크게 + 작은 눈동자)
void C100_drawEyeAbsurd(uint8_t p_matrix_index) {
     int v_center_x = G_C100_MATRIX_WIDTH / 2; // 4
     int v_center_y = G_C100_MATRIX_HEIGHT / 2; // 4
     int v_eye_radius_sq = 4*4; // 눈 테두리 크게
     int v_pupil_radius_sq = 0*0; // 눈동자 아주 작게 또는 없애기

     for(uint8_t v_y = 0; v_y < G_C100_MATRIX_HEIGHT; v_y++) {
       for(uint8_t v_x = 0; v_x < G_C100_MATRIX_WIDTH; v_x++) {
         int dx = v_x - v_center_x;
         int dy = v_y - v_center_y;
         int dist_sq = dx*dx + dy*dy;

         // 눈 테두리 그리기 (큰 원)
         if (dist_sq <= v_eye_radius_sq && dist_sq > (v_eye_radius_sq - 3)) { // 테두리 두께 예시
             C100_drawPixel(p_matrix_index, v_x, v_y, CRGB::White);
         }

         // 눈동자 그리기 (아주 작게)
         if (dist_sq <= v_pupil_radius_sq) {
              C100_drawPixel(p_matrix_index, v_x, v_y, CRGB::White);
         }
       }
     }
     // 충격/감속 시 눈동자 순간 이동 애니메이션 로직 필요 시 여기에 시간(millis() - g_C100_state_start_time) 활용하여 구현
}

// 불쾌함/노려봄 눈 모양 그리기 (작은 눈 + 강조된 눈동자)
void C100_drawEyeGlaring(uint8_t p_matrix_index) {
    C100_drawEyeSquint(p_matrix_index, false); // 약하게 찡그린 모양 기본

    // 눈동자 강조 (색상 변경 또는 크기 약간 키우기)
    int v_center_x = G_C100_MATRIX_WIDTH / 2; // 4
    int v_center_y = G_C100_MATRIX_HEIGHT / 2; // 4
    int v_pupil_radius_sq = 1*1; // 눈동자 반경

    for(int v_py = -1; v_py <= 1; v_py++) {
       for(int v_px = -1; v_px <= 1; v_px++) {
           if (v_px*v_px + v_py*v_py <= v_pupil_radius_sq) {
               C100_drawPixel(p_matrix_index, v_center_x + v_px, v_center_y + v_py, CRGB::Red); // 빨간 눈동자 예시
           }
       }
    }
}

// 졸림 눈 모양 그리기 (눈꺼풀 처짐)
void C100_drawEyeSleepy(uint8_t p_matrix_index) {
    int v_center_x = G_C100_MATRIX_WIDTH / 2; // 4
    int v_center_y = G_C100_MATRIX_HEIGHT / 2; // 4
    int v_droop = 2; // 처지는 정도 예시

    // 눈꺼풀 라인 그리기 (위 라인은 아래로, 아래 라인은 위로)
    for(uint8_t v_x = 0; v_x < G_C100_MATRIX_WIDTH; v_x++) {
         // 위 눈꺼풀 처짐
         if (v_center_y - v_droop >= 0) C100_drawPixel(p_matrix_index, v_x, v_center_y - v_droop, CRGB::White);
         // 아래 눈꺼풀 올라옴
         if (v_center_y + v_droop < G_C100_MATRIX_HEIGHT) C100_drawPixel(p_matrix_index, v_x, v_center_y + v_droop, CRGB::White);
    }

    // 눈동자 그리기 (아주 작게 또는 없애기)
     int v_pupil_radius_sq = 0*0; // 눈동자 없애기 (또는 아주 작게 1x1 픽셀)
     int v_pupil_center_x = v_center_x;
     int v_pupil_center_y = v_center_y;

     // 눈동자 없애는 대신 가운데 1x1 픽셀만 그리는 것도 가능
     // C100_drawPixel(p_matrix_index, v_pupil_center_x, v_pupil_center_y, CRGB::White);
}

// 혼란스러움 눈 모양 그리기 (비대칭)
void C100_drawEyeConfused(uint8_t p_matrix_index) {
    C100_drawEyeNeutral(p_matrix_index); // 기본 눈 모양 사용

    // 눈썹 모양 비대칭으로 추가 (예시)
    int v_center_x = G_C100_MATRIX_WIDTH / 2; // 4
    int v_eyebrow_y = G_C100_MATRIX_HEIGHT / 2 - 3; // 눈꺼풀 위쪽 예시

    if (p_matrix_index == 0) { // 왼쪽 눈썹 (오른쪽 위로)
        C100_drawPixel(p_matrix_index, v_center_x - 1, v_eyebrow_y, CRGB::White);
        C100_drawPixel(p_matrix_index, v_center_x - 2, v_eyebrow_y - 1, CRGB::White);
    } else { // 오른쪽 눈썹 (왼쪽 아래로 - 화난 눈썹 비슷)
        C100_drawPixel(p_matrix_index, v_center_x, v_eyebrow_y, CRGB::White);
        C100_drawPixel(p_matrix_index, v_center_x + 1, v_eyebrow_y + 1, CRGB::White);
    }
     // 또는 눈동자 위치를 좌우 눈 다르게 설정하는 로직 추가 가능
}

// --- 기타 헬퍼 함수 (선택 사항) ---

// 차량 상태 enum 값을 문자열로 변환 (디버깅 출력용)
/*
String C100_currentStateToString(CarState p_state) {
  switch (p_state) {
    case G_C100_STATE_UNKNOWN: return "UNKNOWN";
    case G_C100_STATE_STOPPED: return "STOPPED";
    case G_C100_STATE_MOVING_NORMAL: return "MOVING_NORMAL";
    case G_C100_STATE_ACCELERATING: return "ACCELERATING";
    case G_C100_STATE_BRAKING: return "BRAKING";
    case G_C100_STATE_TURNING_LEFT: return "TURNING_LEFT";
    case G_C100_STATE_TURNING_RIGHT: return "TURNING_RIGHT";
    case G_C100_STATE_BUMP: return "BUMP";
    case G_C100_STATE_TILTED: return "TILTED";
    default: return "INVALID";
  }
}
*/

// 눈 상태 enum 값을 문자열로 변환 (디버깅 출력용)
/*
String C100_eyeStateToString(EyeState p_state) {
  switch (p_state) {
    case G_C100_E_NEUTRAL: return "NEUTRAL";
    case G_C100_E_BLINK: return "BLINK";
    case G_C100_E_LOOK_LEFT: return "LOOK_LEFT";
    case G_C100_E_LOOK_RIGHT: return "LOOK_RIGHT";
    case G_C100_E_LOOK_UP: return "LOOK_UP";
    case G_C100_E_LOOK_DOWN: return "LOOK_DOWN";
    case G_C100_E_SQUINT: return "SQUINT";
    case G_C100_E_SQUINT_TIGHT: return "SQUINT_TIGHT";
    case G_C100_E_ANGRY: return "ANGRY";
    case G_C100_E_ABSURD: return "ABSURD";
    case G_C100_E_GLARING: return "GLARING";
    case G_C100_E_SLEEPY: return "SLEEPY";
    case G_C100_E_CONFUSED: return "CONFUSED";
    default: return "INVALID";
  }
}
*/

// 상보 필터 구현 예시 (C100_applyFiltering 함수 내에서 호출하거나 대체)
/*
void C100_complementaryFilter(float p_ax, float p_ay, float p_az, float p_gx, float p_gy, float p_gz, float p_dt) {
    // 자이로 데이터로 각도 변화 계산
    float v_gyro_roll_change = p_gx * p_dt;
    float v_gyro_pitch_change = p_gy * p_dt;
    // float v_gyro_yaw_change = p_gz * p_dt; // Yaw는 중력 영향 없어 가속도 보정 불가

    // 가속도 데이터로 Roll/Pitch 계산 (정적인 상태에서 정확)
    // atan2(y, x) 사용, MPU 장착 방향에 따라 인자 및 부호 조절 필수!
    float v_accel_roll = atan2(p_ay, p_az) * 180.0 / PI;
    float v_accel_pitch = atan2(-p_ax, sqrt(p_ay*p_ay + p_az*p_az)) * 180.0 / PI;

    // 상보 필터 업데이트
    float v_filter_weight = 0.98; // 자이로 비중 (튜닝 필요, 1에 가까울수록 자이로 영향 큼)
    g_C100_comp_roll = v_filter_weight * (g_C100_comp_roll + v_gyro_roll_change) + (1.0 - v_filter_weight) * v_accel_roll;
    g_C100_comp_pitch = v_filter_weight * (g_C100_comp_pitch + v_gyro_pitch_change) + (1.0 - v_filter_weight) * v_accel_pitch;

    // g_C100_comp_yaw = g_C100_comp_yaw + v_gyro_yaw_change; // Yaw는 자이로만 누적 (드리프트 발생)
}
*/
