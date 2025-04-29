#ifndef _DATA_H_
#define _DATA_H_

#include "config_001.h" // 상수 및 Enum 정의 포함
#include <Adafruit_MPU6050.h> // MPU 객체 및 이벤트 구조체 정의 포함
#include <FastLED.h> // CRGB 정의 포함

// --- LED Matrix 데이터 (전역 변수: C120) ---
CRGB g_C120_leds[G_C120_TOTAL_NUM_LEDS]; // 전체 LED를 담을 배열

// --- IMU 데이터 및 필터링 결과 (전역 변수: C110) ---
Adafruit_MPU6050 g_C110_mpu; // MPU6050 객체
sensors_event_t g_C110_a, g_C110_g, g_C110_temp; // 가속도, 자이로, 온도 이벤트 데이터 구조체

// 간단한 LPF 계수 (0.0 ~ 1.0, 작을수록 필터링 강함, 튜닝 필요)
// MPU6050 내부 필터와 함께 사용 시 alpha 값을 1.0에 가깝게 설정하거나 소프트웨어 필터링 사용 중단 고려
float g_C110_alpha = 0.1;
// 필터링된 가속도 및 자이로 값 저장 변수
float g_C110_filtered_ax, g_C110_filtered_ay, g_C110_filtered_az;
float g_C110_filtered_gx, g_C110_filtered_gy, g_C110_filtered_gz;

// 기울기 계산을 위한 변수 (Roll, Pitch) - 정지 시 기울어짐 판단에 사용
// 가속도 센서만으로 계산 (정지 시에만 유효)
float g_C110_accel_roll = 0.0, g_C110_accel_pitch = 0.0;
// 상보 필터 사용 시 변수 (더 안정적인 기울기 측정 가능하나 구현 복잡성 증가)
// 이 변수들을 사용하려면 imu.h 의 C110_complementaryFilter 함수 관련 주석 해제 필요
// float g_C110_comp_roll = 0.0, g_C110_comp_pitch = 0.0;
// unsigned long g_C110_last_filter_time = 0; // 상보 필터 시간 간격 계산용


// --- 상태 및 타이머 (전역 변수: C100) ---
CarState g_C100_currentCarState = G_C100_STATE_UNKNOWN;
CarState g_C100_previousCarState = G_C100_STATE_UNKNOWN; // 상태 변화 감지용

EyeState g_C100_currentEyeState = G_C100_E_NEUTRAL; // 현재 로봇 눈 표현 상태

unsigned long g_C100_state_start_time = 0; // 현재 차량 상태가 시작된 시간 (millis())
unsigned long g_C100_stop_duration = 0; // 정지 상태 지속 시간 (ms)

unsigned long g_C100_last_blink_time = 0; // 마지막 깜빡임/둘러보기 시작 시간
unsigned long g_C100_blink_interval = 5000; // 다음 깜빡임/둘러보기까지 간격 (ms, 랜덤 가능)
unsigned long g_C100_blink_duration = 200; // 깜빡임 애니메이션 지속 시간 (ms)
unsigned long g_C100_look_animation_duration = 500; // 둘러보기 애니메이션 지속 시간 (ms)


#endif // _DATA_H_
