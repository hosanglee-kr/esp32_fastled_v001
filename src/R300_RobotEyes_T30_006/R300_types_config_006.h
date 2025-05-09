#pragma once

// R300_types_config_005.h - 로봇 눈 애니메이션 및 상태 관리의 기본 타입 및 설정 파일

// 필요한 표준 라이브러리 헤더 포함
#include <stdint.h>
#include <FastLED.h>
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>


// --- WS2812b (FastLED) 하드웨어 설정 (전역 상수: G_R300_ 로 시작) ---
// WS2812b LED 데이터 핀 번호
#define G_R300_NEOPIXEL_PIN 13
// WS2812b LED 총 개수 (8x8 매트릭스 2개)
#define G_R300_NEOPIXEL_NUM_LEDS 128
// LED 칩셋 타입
#define G_R300_LED_TYPE WS2812B
// LED 색상 순서 (일반적으로 GRB)
#define G_R300_COLOR_ORDER GRB
// 눈 색상
#define G_R300_EYE_COLOR CRGB::White

// --- 디스플레이 레이아웃 설정 (8x8 매트릭스 2개 기준, 전역 상수: G_R300_ 로 시작) ---
// 전체 디스플레이 너비 (픽셀)
#define G_R300_DISPLAY_WIDTH 16
// 전체 디스플레이 높이 (픽셀)
#define G_R300_DISPLAY_HEIGHT 8
// 오른쪽 눈 시작 픽셀 인덱스 (하드웨어 배선에 맞게 수정 필요)
#define G_R300_RIGHT_EYE_START_PIXEL 0
// 왼쪽 눈 시작 픽셀 인덱스 (하드웨어 배선에 맞게 수정 필요)
#define G_R300_LEFT_EYE_START_PIXEL 64
// 눈 하나당 픽셀 개수 (8x8)
#define G_R300_EYE_MATRIX_SIZE 64

// --- 기타 정의 (전역 상수: G_R300_ 로 시작) ---
// 배열 요소 개수 계산 매크로
#define G_R300_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
// 눈 폰트 데이터 열 개수 (8x8 비트맵)
#define G_R300_EYE_COL_SIZE 8

// 애니메이션 프레임 기본 표시 시간 (밀리초)
#define G_R300_FRAME_TIME 100

// 표시할 텍스트 최대 길이 (고정 버퍼 크기)
#define G_R300_MAX_TEXT_LENGTH 31 // 최대 31자 + 널 종료 문자

// 로봇 상태 관리를 위한 비활성 시간 기준 선언
extern const unsigned long G_R300_TIME_TO_SLEEP_EXAMPLE;


// --- 열거형 및 구조체 정의 (사용자 정의 타입: T_R300_ 로 시작) ---
// 로봇의 현재 상태
typedef enum {
    AWAKE,   // 깨어있는 상태
    SLEEPING // 잠자는 상태
} T_R300_State;

// 로봇 눈 감정 애니메이션 종류
typedef enum {
    E_NONE,          // 애니메이션 없음
    E_NEUTRAL,       // 일반적인 눈 모양
    E_BLINK,         // 깜빡임
    E_WINK,          // 윙크
    E_LOOK_L,        // 왼쪽 보기
    E_LOOK_R,        // 오른쪽 보기
    E_LOOK_U,        // 위 보기
    E_LOOK_D,        // 아래 보기
    E_ANGRY,         // 화남
    E_SAD,           // 슬픔
    E_EVIL,          // 사악함
    E_EVIL2,         // 사악함 2
    E_SQUINT,        // 찡그림
    E_DEAD,          // 죽은 눈
    E_SCAN_UD,       // 상하 스캔
    E_SCAN_LR,       // 좌우 스캔
    E_SQUINT_BLINK,  // 찡그림 깜빡임
    E_IDLE,          // 대기 상태
    E_HEART,         // 하트
    E_SLEEP,         // 잠자는 눈
} T_R300_emotion_t;

// 애니메이션 시퀀스 단일 프레임
typedef struct {
    uint8_t eyeData[2];  // [0] = 오른쪽 눈 폰트 인덱스, [1] = 왼쪽 눈 폰트 인덱스
    uint16_t timeFrame;  // 프레임 표시 시간 (밀리초)
} T_R300_animFrame_t;

// 감정 애니메이션 시퀀스 조회 테이블 항목
typedef struct {
    T_R300_emotion_t e;          // 감정 종류
    const T_R300_animFrame_t* seq; // 시퀀스 데이터 PROGMEM 주소
    uint8_t size;         // 시퀀스 총 프레임 개수
} T_R300_animTable_t;

// 애니메이션 상태 머신(FSM) 상태
typedef enum {
    S_IDLE,      // 유휴 상태
    S_RESTART,   // 재시작 준비 상태
    S_ANIMATE,   // 애니메이션 실행 상태
    S_PAUSE,     // 프레임 대기 상태
    S_TEXT,      // 텍스트 표시 상태
} T_R300_animState_t;

// 눈 모양 비트맵 데이터
typedef struct {
    uint8_t width; // 문자 너비 (픽셀)
    uint8_t data[8]; // 문자 비트맵 데이터 (8x8)
} T_R300_SimpleFontChar;
