#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <FastLED.h> // WS2812B 등 LED 제어 라이브러리
#include <Adafruit_MPU6050.h> // MPU6050 센서 라이브러리
#include <Wire.h> // I2C 통신 라이브러리
#include <math.h> // sqrt, atan2 등을 위한 수학 함수
#include <Arduino.h> // 기본 Arduino 함수 (millis, random, constrain, map 등)

// --- IMU 하드웨어 설정 (전역 상수: C110) ---
#define G_C110_MPU_I2C_SDA 21 // MPU6050 I2C SDA 핀 (ESP32 기본값 또는 사용자 지정)
#define G_C110_MPU_I2C_SCL 22 // MPU6050 I2C SCL 핀 (ESP32 기본값 또는 사용자 지정)

// --- LED Matrix 하드웨어 설정 (전역 상수: C120) ---
#define G_C120_LED_DATA_PIN 13 // FastLED 데이터 핀 (사용자 지정 ESP32 GPIO 핀)

#define G_C120_COLOR_ORDER GRB // LED 색상 순서 (WS2812B는 GRB가 많음, 확인 필요)
#define G_C120_LED_TYPE WS2812B // 사용 LED 칩 타입

#define G_C120_MATRIX_WIDTH 8 // 매트릭스 가로 픽셀 수
#define G_C120_MATRIX_HEIGHT 8 // 매트릭스 세로 픽셀 수
#define G_C120_NUM_MATRICES 2 // 사용 매트릭스 개수 (좌/우 눈)
#define G_C120_NUM_LEDS_PER_MATRIX (G_C120_MATRIX_WIDTH * G_C120_MATRIX_HEIGHT)
#define G_C120_TOTAL_NUM_LEDS (G_C120_NUM_LEDS_PER_MATRIX * G_C120_NUM_MATRICES) // 전체 LED 개수

// --- IMU 데이터 처리 임계값 (전역 상수: C110) ---
// 가속도 및 자이로 값 단위는 MPU6050 라이브러리 및 설정에 따라 다름 (Adafruit 라이브러리는 가속도 m/s^2, 자이로 deg/s 기본)
// 여기서는 MPU6050_RANGE_8_G (가속도 약 8*9.8 m/s^2), MPU6050_RANGE_500_DEG (자이로 500 deg/s) 기준 임계값 예시
// 실제 MPU6050.setAccelerometerRange, setGyroRange 설정에 맞춰 임계값 조정 필요!
// 또한 소프트웨어 필터링 강도(g_C110_alpha)에 따라서도 임계값 조정 필요!

const float G_C110_ACCEL_IDLE_THRESHOLD_G = 0.2; // 정지 판별 가속도 변화 임계값 (g 단위 환산값 근처)
// Adafruit 라이브러리는 m/s^2 이므로 약 9.8m/s^2이 중력가속도.
// 정지 시 가속도 크기는 약 9.8 m/s^2 근처.
// abs(accel_magnitude - 9.8) < G_C110_ACCEL_IDLE_THRESHOLD_MS2 로 사용
const float G_C110_ACCEL_IDLE_THRESHOLD_MS2 = 9.8 * G_C110_ACCEL_IDLE_THRESHOLD_G; // 예시: 0.2g -> 약 1.96 m/s^2

const float G_C110_GYRO_IDLE_THRESHOLD_DEGS = 3.0; // 정지 판별 자이로 변화 임계값 (deg/s)

const float G_C110_ACCEL_FORWARD_THRESHOLD_MS2 = 2.5; // 전진 가속 판별 Y축 임계값 (m/s^2)
const float G_C110_ACCEL_BRAKE_THRESHOLD_MS2 = -3.0; // 급감속 판별 Y축 임계값 (m/s^2)

const float G_C110_GYRO_TURN_THRESHOLD_DEGS = 15.0; // 회전 판별 Z축 임계값 (deg/s)

// 충격/요철 판별 가속도 변화 임계값 (필터링 후 사용, 튜닝 매우 중요)
// 이전 필터링 값과의 차이 또는 순간적인 절대값 크기 등으로 판단 가능
// 여기서는 순간적인 필터링된 가속도 절대값 크기 임계값으로 예시 (다른 상태와 겹치지 않게 로직 필요)
const float G_C110_ACCEL_BUMP_THRESHOLD_MS2 = 5.0; // 예시: 5 m/s^2 이상의 순간 가속도

const float G_C110_TILT_ANGLE_THRESHOLD_DEG = 5.0; // 정지 시 기울어짐 판별 각도 임계값 (도)

const float G_C110_GYRO_TURN_MAX_SQUINT_DEGS = 50.0; // 찡그림 강도 100%가 되는 최대 회전 각속도 (deg/s)


// --- 공통 상수 (C100) ---
const unsigned long G_C100_SHORT_STOP_DURATION = 4000; // 짧은 정지 시간 기준 (ms)
const unsigned long G_C100_LONG_STOP_DURATION = 20000; // 긴 정지 시간 기준 (ms)

// PI 값 정의 (math.h에 있을 수 있으나 명시적으로 정의)
#ifndef PI
	#define PI 3.14159265358979323846
#endif

// --- 차량 상태 정의 (Enum) ---
enum CarState { // Enum 타입 이름은 규칙에서 제외
	G_C100_STATE_UNKNOWN, // 초기 또는 알 수 없는 상태
	G_C100_STATE_STOPPED, // 정지
	G_C100_STATE_MOVING_NORMAL, // 일반 주행
	G_C100_STATE_ACCELERATING, // 가속 중
	G_C100_STATE_BRAKING, // 감속 중
	G_C100_STATE_TURNING_LEFT, // 좌회전 중
	G_C100_STATE_TURNING_RIGHT, // 우회전 중
	G_C100_STATE_BUMP, // 충격 또는 요철 통과
	G_C100_STATE_TILTED // 정지 상태에서 기울어짐
};

// --- 로봇 눈 표현 상태 정의 (Enum) ---
enum EyeState { // Enum 타입 이름은 규칙에서 제외
	G_C100_E_NEUTRAL, // 기본 눈
	G_C100_E_BLINK, // 깜빡임 (애니메이션)
	G_C100_E_LOOK_LEFT, // 왼쪽 보기
	G_C100_E_LOOK_RIGHT, // 오른쪽 보기
	G_C100_E_LOOK_UP, // 위 보기
	G_C100_E_LOOK_DOWN, // 아래 보기
	G_C100_E_SQUINT, // 약하게 찡그림
	G_C100_E_SQUINT_TIGHT, // 강하게 찡그림 (악!)
	G_C100_E_ANGRY, // 화남 (감속/충격 시)
	G_C100_E_ABSURD, // 황당/놀람 (감속/충격 시)
	G_C100_E_GLARING, // 불쾌함/노려봄 (충격 시)
	G_C100_E_SLEEPY, // 졸림 (매우 긴 정지 시)
	G_C100_E_CONFUSED, // 혼란스러움 (예비 상태)
};


#endif // _CONFIG_H_
