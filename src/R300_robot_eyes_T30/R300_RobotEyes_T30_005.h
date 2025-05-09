#include <Arduino.h>
#include <FastLED.h> // FastLED 라이브러리 사용
#include <string.h>  // 문자열 함수 사용 (strcmp, strncpy, strlen)
#include <stdlib.h>  // 동적 메모리 할당/해제 함수 사용 (free는 사용하지 않음)

// --- WS2812b (FastLED) 하드웨어 설정 ---
// WS2812b LED 스트립/매트릭스가 연결된 데이터 핀 번호
#define NEOPIXEL_PIN 13
// WS2812b LED의 총 개수 (8x8 매트릭스 2개 사용 시 64 * 2 = 128)
#define NEOPIXEL_NUM_LEDS 128
// 사용하는 LED 칩셋 타입 (WS2812B 또는 SK6812 등 FastLED에서 지원하는 타입)
#define LED_TYPE WS2812B
// LED 색상 순서 (WS2812b는 보통 GRB)
#define COLOR_ORDER GRB
// 눈 색상 (FastLED CRGB 형식, 예: CRGB::White, CRGB::Red, CRGB::Blue 등)
#define EYE_COLOR CRGB::White

// --- 디스플레이 레이아웃 설정 (8x8 매트릭스 2개 기준) ---
// 두 개의 8x8 매트릭스가 가로로 나란히 배치된 구성을 가정합니다: [왼쪽 눈] [오른쪽 눈] 또는 [오른쪽 눈] [왼쪽 눈]
// 전체 디스플레이의 너비 (픽셀)
#define DISPLAY_WIDTH 16
// 전체 디스플레이의 높이 (픽셀)
#define DISPLAY_HEIGHT 8
// 실제 LED 스트립의 연결 순서에 따라 오른쪽 눈과 왼쪽 눈에 해당하는 픽셀 인덱스 범위를 정의합니다.
// 여기서는 오른쪽 눈이 스트립의 앞부분(픽셀 0부터), 왼쪽 눈이 그 다음 부분(픽셀 64부터)에 연결되었다고 가정합니다.
// 하드웨어 배선에 맞게 이 값을 반드시 수정해야 합니다.
#define RIGHT_EYE_START_PIXEL 0   // 오른쪽 눈이 시작하는 픽셀 인덱스
#define LEFT_EYE_START_PIXEL 64  // 왼쪽 눈이 시작하는 픽셀 인덱스
// 눈 하나당 픽셀 개수 (8x8 = 64)
#define EYE_MATRIX_SIZE 64

// --- 기타 정의 ---
// 배열의 요소 개수를 계산하는 매크로
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
// 눈 폰트 데이터의 열 개수 (8x8 비트맵이므로 8)
#define EYE_COL_SIZE 8

// 애니메이션 프레임이 화면에 표시되는 기본 시간 (밀리초)
#define FRAME_TIME 100

// 로봇의 현재 상태를 나타내는 열거형
typedef enum {
    AWAKE,   // 깨어있는 상태
    SLEEPING // 잠자는 상태
} State;

// 로봇 눈이 표현할 수 있는 다양한 감정 애니메이션 종류를 정의하는 열거형.
typedef enum {
    E_NONE,          // 애니메이션 없음 (내부 사용, 사용자 선택 불가)
    E_NEUTRAL,       // 일반적인 눈 모양 (애니메이션 없음)
    E_BLINK,         // 양쪽 눈 깜빡임 애니메이션
    E_WINK,          // 한쪽 눈 윙크 애니메이션 (원본 데이터 기준 오른쪽 눈 윙크)
    E_LOOK_L,        // 양쪽 눈 왼쪽 보기 애니메이션
    E_LOOK_R,        // 양쪽 눈 오른쪽 보기 애니메이션
    E_LOOK_U,        // 양쪽 눈 위 보기 애니메이션
    E_LOOK_D,        // 양쪽 눈 아래 보기 애니메이션
    E_ANGRY,         // 화난 눈 애니메이션 (좌우 대칭)
    E_SAD,           // 슬픈 눈 애니메이션 (좌우 대칭)
    E_EVIL,          // 사악한 눈 애니메이션 (좌우 대칭)
    E_EVIL2,         // 사악한 눈 애니메이션 (좌우 비대칭)
    E_SQUINT,        // 찡그린 눈 애니메이션
    E_DEAD,          // 죽은 눈 애니메이션
    E_SCAN_UD,       // 상하 스캔 애니메이션
    E_SCAN_LR,       // 좌우 스캔 애니메이션
    E_SQUINT_BLINK,  // 찡그리고 깜빡이는 애니메이션
    E_IDLE,          // 대기 상태 눈 모양 (Peering 4 프레임으로 리셋)
    E_HEART,         // 하트 모양 애니메이션
    E_SLEEP,         // 잠자는 눈 애니메이션
} emotion_t;

// 애니메이션 시퀀스의 단일 프레임을 정의하는 구조체
struct animFrame_t {
    uint8_t eyeData[2];  // [0] = 오른쪽 눈에 사용할 폰트 문자 인덱스, [1] = 왼쪽 눈에 사용할 폰트 문자 인덱스
    uint16_t timeFrame;  // 이 프레임이 화면에 표시될 시간 (밀리초)
};

// 감정 종류와 해당 애니메이션 시퀀스 데이터를 연결하는 조회 테이블의 항목 구조체
struct animTable_t {
    emotion_t e;          // 해당 항목이 나타내는 감정 종류
    const animFrame_t* seq; // 이 감정의 애니메이션 시퀀스 데이터가 저장된 PROGMEM 주소
    uint8_t size;         // 해당 시퀀스의 총 프레임 개수
};

// 애니메이션 상태 머신(FSM) 상태 열거형
typedef enum {
    S_IDLE,      // 유휴 상태: 애니메이션 또는 텍스트 표시 중이 아니며, 새로운 명령 또는 자동 깜빡임 발생 대기
    S_RESTART,   // 새 애니메이션 시퀀스를 로드하고 시작할 준비를 하는 상태
    S_ANIMATE,   // 현재 애니메이션 프레임을 LED 버퍼에 그리고 다음 프레임 정보를 업데이트하는 상태
    S_PAUSE,     // 현재 애니메이션 프레임이 화면에 표시되는 동안 지정된 시간만큼 대기하는 상태
    S_TEXT,      // 텍스트를 화면에 표시하는 상태 (현재 구현에서는 정적 표시)
} animState_t;


// --- 글로벌 변수 ---
// FastLED CRGB 배열. FastLED가 관리하는 실제 LED 픽셀 색상 데이터를 저장하는 버퍼입니다.
CRGB leds[NEOPIXEL_NUM_LEDS];
// CRGB 배열 포인터. FastLED 초기화 후 leds 배열의 주소가 할당됩니다.
CRGB* _leds = nullptr;

// 현재 로봇 상태 (AWAKE 또는 SLEEPING)
State _state = AWAKE;

// 애니메이션 실행 및 시간 관리를 위한 변수
// 현재 애니메이션 프레임이 화면에 그려지고 S_PAUSE 상태로 진입한 시간 (millis() 값)
uint32_t _timeStartPause = 0;
// 마지막으로 애니메이션(깜빡임 포함)이 시작되거나 텍스트 표시가 끝난 시간. 자동 깜빡임 타이밍 계산에 사용됩니다.
uint32_t _timeLastAnimation = 0;
// 자동 깜빡임 기능이 활성화되었을 때 다음 깜빡임이 발생하기 전 최소 대기해야 하는 시간 (밀리초)
uint16_t _timeBlinkMinimum = 5000; // 기본값 5초

// 애니메이션 상태 머신의 현재 상태
animState_t _animState = S_IDLE;

// 애니메이션 제어 및 플래그
// 자동 깜빡임 기능 활성화 여부 플래그
bool _autoBlink = true;
// 현재 실행 중인 애니메이션 시퀀스의 데이터를 저장하는 구조체 (시퀀스 포인터와 크기 포함)
animTable_t _animEntry;
// 현재 실행 중인 시퀀스 내의 프레임 인덱스
int8_t _animIndex = 0;
// 현재 애니메이션 시퀀스를 역방향으로 재생 중인지 여부 플래그
bool _animReverse = false;
// 현재 애니메이션 시퀀스 완료 후 자동으로 역방향 재생을 시작해야 하는지 여부 플래그
bool _autoReverse = false;
// 다음에 재생할 애니메이션 감정 종류를 저장하는 변수 (애니메이션 큐 역할)
emotion_t _nextEmotion = E_NONE;
// 현재 화면에 표시되고 있는 애니메이션의 감정 종류
emotion_t _currentEmotion = E_NONE;

// 표시할 텍스트의 최대 길이 정의 (예: 텍스트 스크롤링 기능 확장 시 필요)
// 현재 단순 구현은 첫 2자만 사용하므로 3자(2자 + 널 종료)면 충분하나, 향후 확장을 고려하여 좀 더 크게 잡을 수 있습니다.
#define MAX_TEXT_LENGTH 31 // 최대 31자 + 널 종료 문자

// 표시할 텍스트 문자열을 저장할 고정 크기 버퍼. 동적 할당을 대체합니다.
char _textBuffer[MAX_TEXT_LENGTH + 1];
// 표시할 텍스트 문자열의 시작을 가리키는 포인터. 항상 _textBuffer의 시작 주소를 가리킵니다.
char* _pText = nullptr; // begin()에서 _textBuffer의 주소를 할당받습니다.


// --- 정적 데이터 테이블 정의 (원본 RobotEyes_Data.h 내용 기반) ---
// 애니메이션 시퀀스 데이터 (PROGMEM에 저장하여 플래시 메모리 사용, RAM 절약)
const animFrame_t seqBlink[] PROGMEM = {
    {{0, 0}, FRAME_TIME / 2}, {{1, 1}, FRAME_TIME / 2}, {{2, 2}, FRAME_TIME / 2},
    {{3, 3}, FRAME_TIME / 2}, {{4, 4}, FRAME_TIME / 2}, {{5, 5}, FRAME_TIME},
};

const animFrame_t seqWink[] PROGMEM = {
    {{0, 0}, FRAME_TIME / 2}, {{1, 0}, FRAME_TIME / 2}, {{2, 0}, FRAME_TIME / 2},
    {{3, 0}, FRAME_TIME / 2}, {{4, 0}, FRAME_TIME / 2}, {{5, 0}, FRAME_TIME * 2},
};

const animFrame_t seqRight[] PROGMEM = {
    {{0, 0}, FRAME_TIME}, {{6, 6}, FRAME_TIME}, {{7, 7}, FRAME_TIME * 5},
};

const animFrame_t seqLeft[] PROGMEM = {
    {{0, 0}, FRAME_TIME}, {{8, 8}, FRAME_TIME}, {{9, 9}, FRAME_TIME * 5},
};

const animFrame_t seqUp[] PROGMEM = {
    {{0, 0}, FRAME_TIME}, {{11, 11}, FRAME_TIME}, {{12, 12}, FRAME_TIME},
    {{13, 13}, FRAME_TIME * 5},
};

const animFrame_t seqDown[] PROGMEM = {
    {{0, 0}, FRAME_TIME}, {{14, 14}, FRAME_TIME}, {{15, 15}, FRAME_TIME},
    {{16, 16}, FRAME_TIME * 5},
};

const animFrame_t seqAngry[] PROGMEM = {
    {{0, 0}, FRAME_TIME}, {{22, 17}, FRAME_TIME}, {{23, 18}, FRAME_TIME},
    {{24, 19}, FRAME_TIME}, {{25, 20}, 2000},
};

const animFrame_t seqSad[] PROGMEM = {
    {{0, 0}, FRAME_TIME}, {{32, 27}, FRAME_TIME}, {{33, 28}, FRAME_TIME},
    {{34, 29}, 2000},
};

const animFrame_t seqEvil[] PROGMEM = {
    {{0, 0}, FRAME_TIME}, {{39, 37}, FRAME_TIME}, {{40, 38}, 2000},
};

const animFrame_t seqEvil2[] PROGMEM = {
    {{0, 0}, FRAME_TIME}, {{54, 17}, FRAME_TIME}, {{55, 18}, FRAME_TIME},
    {{56, 19}, FRAME_TIME}, {{57, 20}, 2000},
};

const animFrame_t seqSquint[] PROGMEM = {
    {{0, 0}, FRAME_TIME}, {{54, 54}, FRAME_TIME}, {{55, 55}, FRAME_TIME},
    {{56, 56}, FRAME_TIME}, {{57, 57}, 2000},
};

const animFrame_t seqDead[] PROGMEM = {
    {{52, 52}, FRAME_TIME * 4}, {{53, 53}, FRAME_TIME * 4}, {{52, 52}, FRAME_TIME * 2},
};

const animFrame_t seqScanLeftRight[] PROGMEM = {
    {{41, 41}, FRAME_TIME * 2}, {{42, 42}, FRAME_TIME}, {{43, 43}, FRAME_TIME},
    {{44, 44}, FRAME_TIME},
};

const animFrame_t seqScanUpDown[] PROGMEM = {
    {{46, 46}, FRAME_TIME * 2}, {{47, 47}, FRAME_TIME}, {{48, 48}, FRAME_TIME},
    {{49, 49}, FRAME_TIME}, {{50, 50}, FRAME_TIME}, {{51, 51}, FRAME_TIME},
};

const animFrame_t seqSquintBlink[] PROGMEM = {
    {{57, 57}, FRAME_TIME}, {{5, 5}, FRAME_TIME},
};

const animFrame_t seqIdle[] PROGMEM = {
    {{57, 57}, FRAME_TIME},
};

const animFrame_t seqHeart[] PROGMEM = {
    {{0, 0}, FRAME_TIME}, {{59, 59}, FRAME_TIME}, {{60, 60}, 2000},
};

const animFrame_t seqSleep[] PROGMEM = {
    {{0, 0}, FRAME_TIME}, {{62, 62}, FRAME_TIME}, {{63, 63}, FRAME_TIME},
    {{64, 64}, FRAME_TIME}, {{65, 65}, FRAME_TIME}, {{66, 66}, FRAME_TIME},
    {{67, 67}, FRAME_TIME},
};

// 애니메이션 시퀀스 검색을 위한 조회 테이블 (PROGMEM에 저장)
// 각 항목은 감정 종류, 해당 시퀀스 PROGMEM 주소, 시퀀스 크기를 가집니다.
const animTable_t lookupTable[] PROGMEM = {
    {E_NEUTRAL, seqBlink, 1},  // 중립: Blink 시퀀스의 첫 프레임만 사용
    {E_BLINK, seqBlink, ARRAY_SIZE(seqBlink)},
    {E_WINK, seqWink, ARRAY_SIZE(seqWink)},
    {E_LOOK_L, seqLeft, ARRAY_SIZE(seqLeft)},
    {E_LOOK_R, seqRight, ARRAY_SIZE(seqRight)},
    {E_LOOK_U, seqUp, ARRAY_SIZE(seqUp)},
    {E_LOOK_D, seqDown, ARRAY_SIZE(seqDown)},
    {E_ANGRY, seqAngry, ARRAY_SIZE(seqAngry)},
    {E_SAD, seqSad, ARRAY_SIZE(seqSad)},
    {E_EVIL, seqEvil, ARRAY_SIZE(seqEvil)},
    {E_EVIL2, seqEvil2, ARRAY_SIZE(seqEvil2)},
    {E_DEAD, seqDead, ARRAY_SIZE(seqDead)},
    {E_SCAN_LR, seqScanLeftRight, ARRAY_SIZE(seqScanLeftRight)},
    {E_SCAN_UD, seqScanUpDown, ARRAY_SIZE(seqScanUpDown)},
    // Idle animations
    {E_IDLE, seqIdle, ARRAY_SIZE(seqIdle)},
    {E_SQUINT, seqSquint, ARRAY_SIZE(seqSquint)},
    {E_SQUINT_BLINK, seqSquintBlink, ARRAY_SIZE(seqSquintBlink)},
    // Emojis
    {E_HEART, seqHeart, ARRAY_SIZE(seqHeart)},
    // Sleep
    {E_SLEEP, seqSleep, ARRAY_SIZE(seqSleep)},
};

// 눈 모양 비트맵 데이터 (폰트 역할, PROGMEM에 저장)
// 각 항목은 너비(8)와 8바이트의 데이터(각 열의 8행 비트맵)를 가집니다.
// 이 데이터는 MD_MAX72xx 폰트 형식이지만, FastLED와 함께 사용하기 위해 SimpleFontChar 구조체로 매핑됩니다.
struct SimpleFontChar {
    uint8_t width; // 문자 너비 (픽셀), 눈 모양은 8
    uint8_t data[8]; // 문자 비트맵 데이터 (각 바이트는 한 열의 8개 픽셀의 비트맵)
};

const SimpleFontChar _RobotEyes_Font[] PROGMEM = {
    {8, {0, 126, 129, 177, 177, 129, 126, 0}},      // 0   - 'Rest Position'
    {8, {0, 124, 130, 178, 178, 130, 124, 0}},      // 1   - 'Blink 1'
    {8, {0, 120, 132, 180, 180, 132, 120, 0}},      // 2   - 'Blink 2'
    {8, {0, 48, 72, 120, 120, 72, 48, 0}},          // 3   - 'Blink 3'
    {8, {0, 32, 80, 112, 112, 80, 32, 0}},          // 4   - 'Blink 4'
    {8, {0, 32, 96, 96, 96, 96, 32, 0}},            // 5   - 'Blink 5'
    {8, {0, 126, 129, 129, 177, 177, 126, 0}},      // 6   - 'Right 1'
    {8, {0, 0, 126, 129, 129, 177, 177, 126}},      // 7   - 'Right 2'
    {8, {0, 126, 177, 177, 129, 129, 126, 0}},      // 8   - 'Left 1'
    {8, {126, 177, 177, 129, 129, 126, 0, 0}},      // 9   - 'Left 2'
    {0, {0, 0, 0, 0, 0, 0, 0, 0}},                  // 10 (unused)
    {8, {0, 126, 129, 153, 153, 129, 126, 0}},      // 11   - 'Up 1'
    {8, {0, 126, 129, 141, 141, 129, 126, 0}},      // 12   - 'Up 2'
    {8, {0, 126, 129, 135, 135, 129, 126, 0}},      // 13   - 'Up 3'
    {8, {0, 126, 129, 225, 225, 129, 126, 0}},      // 14   - 'Down 1'
    {8, {0, 126, 129, 193, 193, 129, 126, 0}},      // 15   - 'Down 2'
    {8, {0, 124, 130, 194, 194, 130, 124, 0}},      // 16   - 'Down 3'
    {8, {0, 124, 130, 177, 177, 129, 126, 0}},      // 17   - 'Angry L 1'
    {8, {0, 120, 132, 178, 177, 129, 126, 0}},      // 18   - 'Angry L 2'
    {8, {0, 112, 136, 164, 178, 129, 126, 0}},      // 19   - 'Angry L 3'
    {8, {0, 96, 144, 168, 180, 130, 127, 0}},       // 20   - 'Angry L 4'
    {0, {0, 0, 0, 0, 0, 0, 0, 0}},                  // 21 (unused)
    {8, {0, 126, 129, 177, 177, 130, 124, 0}},      // 22   - 'Angry R 1'
    {8, {0, 126, 129, 177, 178, 132, 120, 0}},      // 23   - 'Angry R 2'
    {8, {0, 126, 129, 178, 164, 136, 112, 0}},      // 24   - 'Angry R 3'
    {8, {0, 127, 130, 180, 168, 144, 96, 0}},       // 25   - 'Angry R 4'
    {0, {0, 0, 0, 0, 0, 0, 0, 0}},                  // 26 (unused)
    {8, {0, 62, 65, 153, 153, 130, 124, 0}},        // 27   - 'Sad L 1'
    {8, {0, 30, 33, 89, 154, 132, 120, 0}},         // 28   - 'Sad L 2'
    {8, {0, 14, 17, 41, 90, 132, 120, 0}},          // 29   - 'Sad L 3'
    {0, {0, 0, 0, 0, 0, 0, 0, 0}},                  // 30 (unused)
    {0, {0, 0, 0, 0, 0, 0, 0, 0}},                  // 31 (unused)
    {8, {0, 124, 130, 153, 153, 65, 62, 0}},        // 32   - 'Sad R 1'
    {8, {0, 120, 132, 154, 89, 33, 30, 0}},         // 33   - 'Sad R 2'
    {8, {0, 120, 132, 90, 41, 17, 14, 0}},          // 34   - 'Sad R 3'
    {0, {0, 0, 0, 0, 0, 0, 0, 0}},                  // 35 (unused)
    {0, {0, 0, 0, 0, 0, 0, 0, 0}},                  // 36 (unused)
    {8, {0, 124, 194, 177, 177, 193, 126, 0}},      // 37   - 'Evil L 1'
    {8, {0, 56, 68, 178, 177, 66, 60, 0}},          // 38   - 'Evil L 2'
    {8, {0, 126, 193, 177, 177, 194, 124, 0}},      // 39   - 'Evil R 1'
    {8, {0, 60, 66, 177, 178, 68, 56, 0}},          // 40   - 'Evil R 2'
    {8, {0, 126, 129, 129, 129, 189, 126, 0}},      // 41   - 'Scan H 1'
    {8, {0, 126, 129, 129, 189, 129, 126, 0}},      // 42   - 'Scan H 2'
    {8, {0, 126, 129, 189, 129, 129, 126, 0}},      // 43   - 'Scan H 3'
    {8, {0, 126, 189, 129, 129, 129, 126, 0}},      // 44   - 'Scan H 4'
    {0, {0, 0, 0, 0, 0, 0, 0, 0}},                  // 45 (unused)
    {8, {0, 126, 129, 131, 131, 129, 126, 0}},      // 46   - 'Scan V 1'
    {8, {0, 126, 129, 133, 133, 129, 126, 0}},      // 47   - 'Scan V 2'
    {8, {0, 126, 129, 137, 137, 129, 126, 0}},      // 48   - 'Scan V 3'
    {8, {0, 126, 129, 145, 145, 129, 126, 0}},      // 49   - 'Scan V 4'
    {8, {0, 126, 129, 161, 161, 129, 126, 0}},      // 50   - 'Scan V 5'
    {8, {0, 126, 129, 193, 193, 129, 126, 0}},      // 51   - 'Scan V 6'
    {8, {0, 126, 137, 157, 137, 129, 126, 0}},      // 52   - 'RIP 1'
    {8, {0, 126, 129, 145, 185, 145, 126, 0}},      // 53   - 'RIP 2'
    {8, {0, 60, 66, 114, 114, 66, 60, 0}},          // 54   - 'Peering 1'
    {8, {0, 56, 68, 116, 116, 68, 56, 0}},          // 55   - 'Peering 2'
    {8, {0, 48, 72, 120, 120, 72, 48, 0}},          // 56   - 'Peering 3'
    {8, {0, 32, 80, 112, 112, 80, 32, 0}},          // 57   - 'Peering 4'  Idle position reset
    {0, {0, 0, 0, 0, 0, 0, 0, 0}},                  // 58 (unused)
    {8, {0, 28, 34, 114, 114, 34, 28, 0}},          // 59   - Core 1
    {8, {0, 28, 34, 100, 100, 34, 28, 0}},          // 60   - Core 2
    {0, {0, 0, 0, 0, 0, 0, 0, 0}},                  // 61 (unused)
    {8, {129, 193, 161, 145, 137, 133, 131, 129}},  // 62   - 'Sleep 1'
    {8, {144, 208, 176, 176, 9, 13, 11, 9}},        // 63   - 'Sleep 2'
    {8, {160, 224, 160, 20, 28, 20, 0, 0}},         // 64   - 'Sleep 3'
    {8, {144, 208, 176, 144, 0, 0, 0, 0}},          // 65   - 'Sleep 4'
    {8, {160, 224, 160, 0, 0, 0, 0, 0}},            // 66   - 'Sleep 5'
    {8, {128, 0, 0, 0, 0, 0, 0, 0}},                // 67   - 'Sleep 6'
    // 68-255는 사용되지 않으며 배열에서 0으로 표현됩니다.
};


// 예제: 로봇 상태(깨어있음/잠자는 중) 관리를 위한 마지막 활동 시간 기록 변수
const unsigned long TIME_TO_SLEEP_EXAMPLE = 10000; // 예제: 10초 동안 명령이 없으면 SLEEPING 상태로 전환
unsigned long lastCommandTime = 0; // 마지막으로 명령을 받거나 상태가 변경된 시간 (millis())

// --- 글로벌 함수 ---

// (눈 인덱스, 행, 열) 좌표를 FastLED CRGB 배열의 선형 픽셀 인덱스로 변환합니다.
// 이 함수는 8x8 매트릭스 2개와 WS2812b 스트립의 실제 물리적 연결 순서 및 배선 방식에 맞게 정확하게 구현되어야 합니다.
// 현재 구현은 각 8x8 눈 매트릭스가 행 우선 지그재그(serpentine) 방식으로 배선되고,
// 전체 스트립에서 오른쪽 눈 영역(RIGHT_EYE_START_PIXEL 부터)이 먼저, 왼쪽 눈 영역(LEFT_EYE_START_PIXEL 부터)이 그 다음에 연결되었다고 가정합니다.
uint16_t mapEyePixel(uint8_t eye_index, uint8_t row, uint8_t col) {
    // 눈 인덱스(0: 오른쪽, 1: 왼쪽)에 따라 해당 눈 영역이 시작하는 픽셀 인덱스를 결정합니다.
    uint16_t base_pixel = (eye_index == 0) ? RIGHT_EYE_START_PIXEL : LEFT_EYE_START_PIXEL;
    uint16_t pixel_index; // 계산될 최종 픽셀 인덱스

    // 8x8 매트릭스 내에서의 (row, col) 좌표를 기반으로 선형 인덱스를 계산합니다.
    // 이 부분은 실제 매트릭스의 LED 배선 방식(예: 행 우선, 열 우선, 지그재그 등)에 따라 달라져야 합니다.
    // 현재는 행 우선 지그재그(serpentine) 방식을 가정합니다.
    if (row % 2 == 0) {
        // 짝수 행 (0, 2, 4, 6): 왼쪽(col 0)에서 오른쪽(col 7)으로 픽셀 인덱스가 증가합니다.
        pixel_index = base_pixel + (row * 8) + col;
    } else {
        // 홀수 행 (1, 3, 5, 7): 오른쪽(col 7)에서 왼쪽(col 0)으로 픽셀 인덱스가 감소하는 지그재그 방식입니다.
        pixel_index = base_pixel + (row * 8) + (7 - col);
    }

    // 계산된 픽셀 인덱스가 전체 LED 개수 범위를 벗어나는지 확인하는 디버깅 코드입니다.
    if (pixel_index >= NEOPIXEL_NUM_LEDS) {
        Serial.print("Error: Mapped pixel index out of bounds: ");
        Serial.println(pixel_index);
        // 실제 코드에서는 범위를 벗어나는 인덱스 사용 시 예상치 못한 동작이나 크래시가 발생할 수 있으므로 주의해야 합니다.
        return 0; // 안전한 기본값(첫 번째 픽셀)을 반환하거나 다른 오류 처리 로직을 추가할 수 있습니다.
    }

    return pixel_index; // 계산된 선형 픽셀 인덱스 반환
}

// 단일 눈 모양 비트맵 데이터(폰트 문자 인덱스)를 FastLED CRGB 배열 버퍼의 해당 눈 영역에 그립니다.
// 이 함수 호출 전에 FastLED.clear() 등으로 전체 버퍼를 초기화해야 합니다.
void drawEye(uint8_t eye_index, uint8_t ch) {
    // 입력된 폰트 문자 인덱스(ch)가 _RobotEyes_Font 배열의 유효 범위 내인지 확인합니다.
    if (ch >= ARRAY_SIZE(_RobotEyes_Font)) {
        Serial.print("Error: Invalid character index: ");
        Serial.println(ch);
        return; // 유효하지 않은 인덱스이면 그리지 않고 함수를 종료합니다.
    }

    // PROGMEM 메모리에 저장된 _RobotEyes_Font 배열에서 지정된 문자 인덱스(ch)에 해당하는 비트맵 데이터를 읽어옵니다.
    SimpleFontChar charData; // 읽어온 데이터를 저장할 임시 구조체
    memcpy_P(&charData, &_RobotEyes_Font[ch], sizeof(SimpleFontChar));

    // 읽어온 비트맵 데이터(charData)를 기반으로 해당 눈 영역의 픽셀들을 설정합니다.
    for (uint8_t col = 0; col < EYE_COL_SIZE; col++) { // 각 열(0-7)을 반복합니다.
        // 현재 열에 해당하는 8행의 비트맵 데이터(1바이트)를 읽어옵니다.
        uint8_t column_byte = pgm_read_byte(&charData.data[col]);

        for (uint8_t row = 0; row < DISPLAY_HEIGHT / 2; row++) { // 각 행(0-7, 8x8 매트릭스이므로 높이는 8)을 반복합니다.
            // 현재 열의 비트맵 바이트(column_byte)에서 해당 행에 해당하는 비트가 1인지 확인합니다. (Bitwise AND 연산 및 시프트)
            if ((column_byte >> row) & 0x01) {
                // 만약 비트가 1이면 (픽셀을 켜야 하면), 해당 픽셀의 (눈 인덱스, 행, 열) 좌표를 선형 픽셀 인덱스로 변환합니다.
                uint16_t pixel_idx = mapEyePixel(eye_index, row, col);
                // 계산된 픽셀 인덱스(_leds 배열의 위치)에 미리 정의된 눈 색상(EYE_COLOR)을 설정합니다.
                _leds[pixel_idx] = EYE_COLOR; // 정의된 눈 색상으로 설정
            }
            // 만약 비트가 0이면 (픽셀을 꺼야 하면), 해당 픽셀은 그대로 유지됩니다. (FastLED.clear()로 미리 검은색으로 설정됨)
        }
    }
}

// 오른쪽 눈(R)과 왼쪽 눈(L)에 사용할 폰트 문자 인덱스를 받아, 해당 눈 모양을 CRGB 버퍼에 그리고 FastLED.show()로 표시합니다.
void drawEyes(uint8_t R, uint8_t L) {
    FastLED.clear(); // 전체 LED 픽셀 버퍼를 검은색으로 초기화합니다. (이전 프레임 지우기)

    drawEye(0, R); // 오른쪽 눈 영역(eye_index 0)에 문자 인덱스 R에 해당하는 눈 모양을 그립니다.
    drawEye(1, L); // 왼쪽 눈 영역(eye_index 1)에 문자 인덱스 L에 해당하는 눈 모양을 그립니다.

    FastLED.show(); // CRGB 버퍼에 그려진 현재 프레임 데이터를 실제 WS2812b LED 스트립에 전송하여 표시합니다.
}

// 지정된 감정(e)에 해당하는 애니메이션 시퀀스 데이터를 조회 테이블에서 찾아 로드합니다.
// 로드된 시퀀스의 프레임 개수를 반환합니다.
uint8_t loadSequence(emotion_t e) {
    bool found = false; // 감정에 해당하는 시퀀스를 찾았는지 여부를 나타내는 플래그
    // 조회 테이블(lookupTable)을 반복하여 지정된 감정(e)에 해당하는 항목을 찾습니다.
    for (uint8_t i = 0; i < ARRAY_SIZE(lookupTable); i++) {
        animTable_t entry; // PROGMEM에서 읽어올 임시 구조체
        memcpy_P(&entry, &lookupTable[i], sizeof(animTable_t)); // PROGMEM에서 항목 읽어오기
        if (entry.e == e) {
            _animEntry = entry; // 찾은 항목의 데이터를 글로벌 변수 _animEntry에 할당합니다.
            found = true; // 찾았음을 표시
            break; // 찾았으면 반복 중단
        }
    }

    // 감정에 해당하는 시퀀스를 찾지 못한 경우 처리
    if (!found) {
        Serial.print("Warning: Animation sequence not found for emotion: ");
        Serial.println(e);
        // 해당하는 시퀀스가 없을 경우, 기본값으로 중립 애니메이션 시퀀스(seqBlink의 첫 프레임)를 설정합니다.
        _animEntry = {E_NEUTRAL, seqBlink, 1};
    }

    // 애니메이션 방향(_animReverse)에 따라 시작 프레임 인덱스 설정
    if (_animReverse)
        _animIndex = _animEntry.size - 1; // 역방향 시작은 시퀀스 마지막 인덱스부터
    else
        _animIndex = 0; // 정방향 시작은 시퀀스 첫 인덱스부터

    return (_animEntry.size); // 로드된 시퀀스의 크기 반환
}

// 현재 애니메이션 시퀀스에서 지정된 인덱스의 프레임 데이터 로드
void loadFrame(animFrame_t* pBuf) {
    // 애니메이션 인덱스(_animIndex)가 현재 로드된 시퀀스(_animEntry)의 유효 범위 내인지 확인합니다.
    if (_animIndex >= 0 && _animIndex < _animEntry.size) {
        // PROGMEM에 저장된 현재 프레임(_animEntry.seq[_animIndex])의 데이터(눈 모양 인덱스, 표시 시간)를 읽어 pBuf에 저장합니다.
        memcpy_P(pBuf, &_animEntry.seq[_animIndex], sizeof(animFrame_t));
    } else {
         // 유효하지 않은 인덱스 접근 시 오류 메시지를 출력하고 기본 프레임 데이터로 설정합니다.
         Serial.print("Error: Invalid animation index: ");
         Serial.println(_animIndex);
         pBuf->eyeData[0] = 0; // 오른쪽 눈 모양 인덱스를 기본값(0번 비트맵)으로 설정
         pBuf->eyeData[1] = 0; // 왼쪽 눈 모양 인덱스를 기본값(0번 비트맵)으로 설정
         pBuf->timeFrame = FRAME_TIME; // 기본 프레임 시간으로 설정
    }
}

// 표시할 텍스트 문자열을 지우고 할당된 메모리 해제 (고정 버퍼 사용 시 버퍼 초기화)
void clearText() {
    // 고정 크기 버퍼(_textBuffer)를 사용하므로 메모리 해제(free)는 필요 없습니다.
    // 버퍼의 첫 번째 문자를 널 종료 문자('\0')로 설정하여 문자열을 비웁니다.
    _textBuffer[0] = '\0';
    // _pText는 항상 _textBuffer를 가리키므로 nullptr로 설정할 필요 없습니다.
}

// CRGB 배열 버퍼에 텍스트를 표시합니다. (WS2812b FastLED 버전에서는 텍스트 스크롤링 없이 첫 두 문자만 정적으로 표시합니다.)
// _pText가 null이 아닐 때 S_TEXT 상태에서 이 함수가 호출될 수 있습니다.
void showText(bool bInit) {
    // 표시할 텍스트가 없으면 (_pText가 nullptr이거나 _pText[0]가 널 종료 문자이면) 텍스트 표시 종료 처리
    if (_pText == nullptr || _pText[0] == '\0') {
         clearText(); // 텍스트 버퍼 초기화 및 관련 상태 정리
         _animState = S_IDLE; // 애니메이션 상태를 유휴 상태(S_IDLE)로 복귀
         _timeLastAnimation = millis(); // 자동 깜빡임 타이머 리셋
         return; // 함수 종료
    }

    FastLED.clear(); // 전체 LED 픽셀 버퍼를 검은색으로 초기화합니다.

    // 텍스트의 첫 번째 문자를 오른쪽 눈 (eye_index 0) 영역에 표시
    // 주의: _RobotEyes_Font의 인덱스와 ASCII 문자가 직접 매핑되지 않습니다.
    // 여기서는 텍스트 첫 문자의 ASCII 값을 폰트 인덱스로 사용합니다. 이는 의도된 문자 모양이 아닐 수 있습니다.
    // 실제 ASCII 텍스트를 표시하려면 적절한 폰트 데이터와 매핑 로직이 필요합니다.
    drawEye(0, (uint8_t)_pText[0]); // 텍스트 첫 문자의 ASCII 값을 폰트 인덱스로 사용

    // 텍스트에 두 번째 문자가 있으면 왼쪽 눈 (eye_index 1) 영역에 표시
    if (_pText[1] != '\0') {
        drawEye(1, (uint8_t)_pText[1]); // 텍스트 두 번째 문자의 ASCII 값을 폰트 인덱스로 사용
    }

    FastLED.show(); // CRGB 버퍼에 그려진 데이터를 실제 WS2812b LED 스트립에 전송하여 표시합니다.

    // 이 단순화된 정적 텍스트 표시에서는 시간 경과에 따른 상태 전환(예: 스크롤 완료)은 없습니다.
    // 텍스트 표시는 새 애니메이션 명령이 들어와 clearText()가 호출될 때까지 유지됩니다.
    // runAnimation의 S_TEXT 상태는 _pText가 null이 될 때까지 유지됩니다.
}

// 다음에 표시할 애니메이션 종류 및 매개변수를 설정합니다. (클래스 외부 함수 버전)
// 현재 애니메이션 재생 중이면 완료 후 설정된 애니메이션이 시작되거나, force=true 시 현재 애니메이션을 중단하고 즉시 시작합니다.
// @param e 표시할 감정 애니메이션 종류 (emotion_t 열거형 값).
// @param r true이면 애니메이션 시퀀스 완료 후 자동으로 역재생을 시작합니다.
// @param b true이면 애니메이션 시퀀스를 처음부터(false) 대신 끝부터(true) 시작하여 역방향으로 재생합니다.
// @param force true이면 현재 상태에 관계없이 즉시 설정된 애니메이션을 시작합니다.
void setAnimation(emotion_t e, bool r, bool b, bool force) {
    // 현재 텍스트 표시 중이고 force 플래그가 false이면 애니메이션 변경 요청을 무시합니다. (텍스트 표시 우선)
    if (_pText != nullptr && _pText[0] != '\0' && !force) return; // _pText가 유효한 텍스트를 가리키는지 추가 확인

    // 요청된 감정(e)이 현재 표시 중인 감정(_currentEmotion)과 다르거나 force 플래그가 true인 경우에만 처리합니다.
    if (e != _currentEmotion || force) {
        _nextEmotion = e;     // 다음에 재생할 애니메이션 감정을 큐에 설정합니다.
        _autoReverse = r;     // 시퀀스 완료 후 자동 역재생 여부를 설정합니다.
        _animReverse = b;     // 애니메이션 재생 시작 방향을 설정합니다.
        // force 플래그가 true이거나 현재 상태가 S_IDLE(유휴 상태)이면 즉시 S_RESTART 상태로 전환하여 새 애니메이션 준비를 시작합니다.
        if (force || _animState == S_IDLE) {
             _animState = S_RESTART;
        }
        // 만약 이미 다른 애니메이션이 재생 중이고 force=false이면, 현재 애니메이션이 완료된 후에 _nextEmotion에 설정된 애니메이션이 S_IDLE 상태를 거쳐 처리됩니다.
    }
}

// 로봇의 상태(AWAKE 또는 SLEEPING)를 설정합니다.
// 상태가 변경될 때, 해당 상태 전환에 맞는 애니메이션(예: 잠들기, 깨어나기)이 자동으로 트리거됩니다.
// @param s 설정할 새로운 로봇 상태 (State 열거형 값).
void setState(State s) {
    // 현재 상태(_state)와 다를 경우에만 처리
    if (s != _state) {
        if (s == SLEEPING && _state == AWAKE) {
            // AWAKE에서 SLEEPING으로 전환: 잠자는 애니메이션 시작 (처음부터 정방향, 강제 시작)
            setAnimation(E_SLEEP, false, false, true);
        } else if (s == AWAKE && _state == SLEEPING) {
            // SLEEPING에서 AWAKE로 전환: 잠자는 애니메이션 역방향 재생 (끝부터 역방향, 강제 시작)
            setAnimation(E_SLEEP, false, true, true); // true for reverse
        }
        _state = s; // 상태 업데이트
        // Serial.print("State changed to: ");
        // Serial.println(s == AWAKE ? "AWAKE" : "SLEEPING"); // 시리얼 출력 (옵션)
    }
}

// 외부 명령 문자열 처리 (애니메이션 설정 또는 텍스트 표시)
// 수신된 명령 문자열을 파싱하여 로봇 눈의 애니메이션 또는 표시 텍스트를 변경합니다.
// @param command 처리할 명령 문자열 (예: "blink", "sad", "awake", "sleeping", 또는 표시할 텍스트 내용).
void processCommand(const char* command) {
    // 새 명령 처리 전에 현재 표시 중인 텍스트 메모리 해제 (고정 버퍼 초기화)
    clearText();

    // 명령 문자열에 따라 해당 애니메이션 설정
    if (strcmp(command, "neutral") == 0) {
        setAnimation(E_NEUTRAL, true, false, true); // 중립
    } else if (strcmp(command, "blink") == 0) {
        setAnimation(E_BLINK, true, false, true); // 깜빡임
    } else if (strcmp(command, "angry") == 0) {
        setAnimation(E_ANGRY, true, false, true); // 화남
    } else if (strcmp(command, "sad") == 0) {
        setAnimation(E_SAD, true, false, true); // 슬픔
    } else if (strcmp(command, "evil") == 0) {
        setAnimation(E_EVIL, true, false, true); // 사악함
    } else if (strcmp(command, "evil2") == 0) {
        setAnimation(E_EVIL2, true, false, true); // 사악함 2
    } else if (strcmp(command, "squint") == 0) {
        setAnimation(E_SQUINT, true, false, true); // 찡그림
    } else if (strcmp(command, "dead") == 0) {
        setAnimation(E_DEAD, true, false, true); // 죽음
    } else if (strcmp(command, "core") == 0) { // "core" 명령은 E_HEART에 해당한다고 가정
        setAnimation(E_HEART, true, false, true); // 하트
    } else if (strcmp(command, "sleep_anim") == 0) { // 잠자는 애니메이션 명시적 실행 명령
        setAnimation(E_SLEEP, false, false, true); // 잠자는 애니메이션
    }
    // 로봇 상태 직접 변경 명령 (선택 사항)
    else if (strcmp(command, "awake") == 0) {
        setState(AWAKE); // 깨어있는 상태로 변경
    } else if (strcmp(command, "sleeping") == 0) {
        setState(SLEEPING); // 잠자는 상태로 변경
    }
    // 위 명령들에 해당하지 않으면 텍스트 표시 명령으로 간주
    else {
        setText(command); // 수신된 문자열을 텍스트로 설정 (showText 상태로 전환 포함)
    }
}

// 애니메이션 상태 머신 실행
// 이 함수는 loop()에서 반복 호출되어야 애니메이션이 부드럽게 표시됩니다.
// 현재 상태에 따라 다음 동작을 결정하고 필요한 함수를 호출합니다.
// 로봇이 IDLE 상태이면 true를 반환합니다.
bool runAnimation(void) {
    static animFrame_t thisFrame; // 현재 표시 중인 프레임 데이터
    static uint32_t timeOfLastFrame = 0; // 현재 프레임이 화면에 표시되기 시작한 시간 (millis() 값)

    switch (_animState) {
        case S_IDLE:
            // S_IDLE 상태: 애니메이션 또는 텍스트 표시 중 아님. 새 명령 또는 자동 깜빡임 발생 대기.

            // 큐에 텍스트(_pText가 유효하고 비어있지 않으면)가 대기 중이면 텍스트 표시 상태로 전환
            if (_pText != nullptr && _pText[0] != '\0') {
                _animState = S_TEXT;
                // showText(true); // 텍스트 표시 초기화 (필요 시 S_TEXT 진입 시 호출 가능)
                break; // switch 문 종료, 다음 loop() 호출 시 S_TEXT 상태 처리
            }

            // 큐에 새 애니메이션(_nextEmotion이 E_NONE이 아니면)이 대기 중이면 RESTART 상태로 전환
            if (_nextEmotion != E_NONE) {
                 _animState = S_RESTART;
                 break; // switch 문 종료, 다음 loop() 호출 시 S_RESTART 상태 처리
            }

            // 큐에 아무것도 없고 자동 깜빡임이 활성화되어 있으면 깜빡임 발생 조건 확인
            // 마지막 애니메이션/활동 시간(_timeLastAnimation)으로부터 미리 설정된 최소 대기 시간(_timeBlinkMinimum)이 경과했는지 확인
            if (_autoBlink && (millis() - _timeLastAnimation) >= _timeBlinkMinimum) {
                 // 최소 대기 시간 경과 후 무작위 확률(예: 1000분의 300 확률)로 깜빡임 트리거
                 if (random(1000) > 700) {
                    if (_state == SLEEPING) {
                        // 로봇이 잠자는 상태이면 찡그림 깜빡임 애니메이션 트리거
                        setAnimation(E_SQUINT_BLINK, true); // 자동 역재생 포함
                    } else if (_state == AWAKE) {
                        // 로봇이 깨어있는 상태이면 일반 깜빡임 애니메이션 트리거
                        setAnimation(E_BLINK, true); // 자동 역재생 포함
                    }
                    // setAnimation 호출은 _nextEmotion 멤버 변수를 설정하고, 이는 다음 loop() 호출 시 S_RESTART 상태로의 전환을 트리거합니다.
                    _timeLastAnimation = millis(); // 깜빡임 애니메이션이 트리거된 시간을 기록하여 다음 자동 깜빡임 타이밍 계산의 기준점으로 삼습니다.
                 } else {
                    // 이번 루프에서는 무작위 확률에 걸리지 않아 깜빡임이 발생하지 않았지만, 다음 깜빡임 계산을 위해 마지막 활동 시간을 현재 시간으로 업데이트합니다.
                    _timeLastAnimation = millis();
                 }
            }
            // 위에 해당하지 않으면 S_IDLE 상태를 유지합니다.
            break;

        case S_RESTART:
            // S_RESTART 상태: 새 애니메이션 시퀀스 로드 및 시작 준비.

            // 큐에 다음에 재생할 애니메이션(_nextEmotion)이 있는지 확인합니다.
            if (_nextEmotion != E_NONE) {
                loadSequence(_nextEmotion); // _nextEmotion에 해당하는 애니메이션 시퀀스 데이터(_animEntry, _animIndex)를 PROGMEM에서 로드합니다.
                _currentEmotion = _nextEmotion; // 현재 재생 중인 애니메이션 감정을 업데이트합니다.
                _nextEmotion = E_NONE; // 다음에 재생할 애니메이션 큐를 비웁니다.
                _animState = S_ANIMATE; // 다음 상태는 애니메이션 실행(S_ANIMATE)입니다.
            } else {
                 // _nextEmotion이 NONE인 상태에서 RESTART에 도달하면 논리적 오류일 수 있습니다. 안전하게 S_IDLE 상태로 복귀합니다.
                 _animState = S_IDLE;
            }
            break;

        case S_ANIMATE:
            // S_ANIMATE 상태: 현재 프레임을 화면에 그리고 다음 프레임 준비.

            loadFrame(&thisFrame); // 현재 _animIndex에 해당하는 프레임 데이터(눈 모양 인덱스, 표시 시간)를 thisFrame에 로드합니다.
            drawEyes(thisFrame.eyeData[0], thisFrame.eyeData[1]); // thisFrame에 로드된 눈 모양 인덱스를 사용하여 FastLED 버퍼에 눈을 그리고 show()로 표시합니다.
            timeOfLastFrame = millis(); // 현재 프레임이 화면에 그려지고 표시되기 시작한 시간을 기록합니다.

            // 애니메이션 재생 방향(_animReverse)에 따라 다음 프레임의 인덱스를 계산합니다.
            if (_animReverse) {
                _animIndex--; // 역방향 재생 시 인덱스 감소
            } else {
                _animIndex++; // 정방향 재생 시 인덱스 증가
            }

            _animState = S_PAUSE; // 현재 프레임의 표시 시간만큼 대기하기 위해 S_PAUSE 상태로 이동합니다.
            break;

        case S_PAUSE:
            // S_PAUSE 상태: 현재 프레임이 화면에 표시된 상태로 지정된 시간(thisFrame.timeFrame)만큼 대기합니다.

            // 현재 시간(millis())이 프레임 표시 시작 시간(timeOfLastFrame) + 프레임 지속 시간(thisFrame.timeFrame) 보다 작은지 확인합니다.
            // 시간이 아직 충분히 경과하지 않았으면 대기를 계속합니다.
            if ((millis() - timeOfLastFrame) < thisFrame.timeFrame) {
                break; // 대기 시간이 경과하지 않았으므로 S_PAUSE 상태를 유지합니다.
            }

            // 대기 시간이 경과했으므로 다음 상태를 결정합니다.
            // 현재 애니메이션 시퀀스가 끝났는지 확인합니다.
            // 정방향(_animReverse가 false)일 때 인덱스(_animIndex)가 시퀀스 크기(_animEntry.size) 이상이면 완료.
            // 역방향(_animReverse가 true)일 때 인덱스(_animIndex)가 0 미만이면 완료.
            if ((!_animReverse && _animIndex >= _animEntry.size) || (_animReverse && _animIndex < 0)) {
                // 애니메이션 시퀀스 완료
                if (_autoReverse) {
                    // 자동 역재생이 설정된 경우: 동일한 시퀀스를 역방향으로 다시 시작할 준비를 합니다.
                    // 현재 재생 중인 감정(_animEntry.e)을 다시 설정하되, 자동 역재생은 이번에는 false로, 시작 방향은 현재 방향(_animReverse)의 반대로 설정합니다.
                    setAnimation(_animEntry.e, false, !_animReverse);
                     // setAnimation 호출은 _nextEmotion을 설정하고, 이는 다음 loop() 호출 시 S_RESTART 상태로의 전환을 트리거합니다.
                } else {
                    // 자동 역재생이 아니거나 역재생까지 완료된 경우: 애니메이션 재생이 완전히 종료되었습니다.
                    _animState = S_IDLE; // 애니메이션 상태를 유휴 상태(S_IDLE)로 복귀합니다.
                    _currentEmotion = E_NONE; // 현재 재생 중인 감정 상태를 지웁니다.
                    _timeLastAnimation = millis(); // 자동 깜빡임 타이머의 기준 시간을 현재 시간으로 리셋합니다.
                }
            } else {
                // 애니메이션 시퀀스가 끝나지 않았으면 다음 프레임을 실행할 준비를 합니다.
                _animState = S_ANIMATE; // 다음 상태는 S_ANIMATE입니다.
            }
            break;

        case S_TEXT:
            // S_TEXT 상태: 텍스트 표시 중.
            // 현재 FastLED 단순 구현에서는 텍스트 스크롤링이 없으므로, showText() 함수는 정적인 표시만 수행합니다.
            // 이 상태는 _pText 멤버 변수가 유효한 텍스트를 가리키는 동안 유지됩니다.
            // 외부(예: processCommand)에서 clearText()가 호출되어 텍스트 버퍼가 비워지면 (_pText[0] == '\0'), 이 상태에서 벗어납니다.

            // 텍스트 버퍼가 비워졌는지 확인합니다. 비워졌으면 텍스트 표시가 종료되었음을 의미합니다.
            if (_pText[0] == '\0') {
                 // 텍스트 표시가 외부 명령 등으로 인해 종료되었으므로 S_IDLE 상태로 복귀합니다.
                 _animState = S_IDLE;
                 _timeLastAnimation = millis(); // 자동 깜빡임 타이머 리셋
            }
            // 텍스트 버퍼가 비워지지 않았으면 텍스트 표시 상태(S_TEXT)를 유지합니다.
            break;

        default: // 예상치 못한 애니메이션 상태에 도달했을 경우.
            // 오류 방지 및 상태 초기화를 수행합니다.
            _animState = S_IDLE; // 상태를 S_IDLE로 리셋합니다.
            _currentEmotion = E_NONE; // 현재 감정 상태를 초기화합니다.
            clearText(); // 텍스트 버퍼 초기화 및 관련 상태 정리
            _timeLastAnimation = millis(); // 자동 깜빡임 타이머를 리셋합니다.
            break;
    }

    // 현재 상태가 S_IDLE이면 true를 반환하여 로봇이 현재 활동 중이 아님을 나타냅니다.
    return (_animState == S_IDLE);
}


// --- Arduino Setup 및 Loop 함수 ---

// Arduino 보드 시작 시 한 번 실행되는 설정 함수
void setup() {
    Serial.begin(115200); // 시리얼 통신 속도 설정 및 시작
    Serial.println("FastLED Persona Example"); // 시리얼 모니터에 시작 메시지 출력

    // FastLED 라이브러리 초기화 및 LED 스트립/매트릭스 설정
    // 지정된 LED 타입, 데이터 핀, 색상 순서, LED 개수로 FastLED를 설정하고 LED 버퍼(leds 배열)를 연결합니다.
    FastLED.addLeds<LED_TYPE, NEOPIXEL_PIN, COLOR_ORDER>(leds, NEOPIXEL_NUM_LEDS).setCorrection(TypicalLEDStrip);
    _leds = leds; // 전역 leds 배열의 주소를 _leds 포인터에 할당하여 글로벌 함수에서 접근 가능하게 합니다.

    FastLED.clear(); // 모든 LED 픽셀 버퍼의 색상 데이터를 0 (검은색)으로 초기화합니다.
    FastLED.show();  // 버퍼의 현재 상태를 실제 LED에 반영하여 초기에는 모든 LED가 꺼지도록 합니다.

    // 로봇 상태 관련 글로벌 변수 초기화
    _state = AWAKE; // 초기 로봇 상태는 깨어있음
    _animState = S_IDLE; // 초기 애니메이션 상태는 유휴 상태
    _autoBlink = true; // 자동 깜빡임 기능 기본 활성화
    _timeBlinkMinimum = 5000; // 자동 깜빡임 최소 대기 시간 기본값 설정 (5초)
    _timeLastAnimation = millis(); // 자동 깜빡임 타이머의 기준 시간을 현재 시간으로 초기화합니다.

    // 텍스트 버퍼 초기화 및 포인터 연결
    _textBuffer[0] = '\0'; // 텍스트 버퍼를 빈 문자열로 초기화
    _pText = _textBuffer; // _pText가 _textBuffer의 시작 주소를 가리키도록 설정

    // 로봇 상태 관리를 위한 마지막 활동 시간 기록 변수를 현재 시간으로 초기화합니다.
    lastCommandTime = millis();

    // 시작 시 중립 애니메이션 설정 (runAnimation에서 처리될 수 있도록 _nextEmotion 및 상태 설정)
    setAnimation(E_NEUTRAL, false);
}

// 설정 함수 실행 후 계속 반복 실행되는 메인 루프 함수
void loop() {
    // Persona 애니메이션/표시 로직을 실행합니다. 이 함수 내부에서 상태 머신이 작동하며 LED 업데이트를 처리합니다.
    runAnimation();

    // 비활성 시간 기준 로봇 상태 변경 로직 예제
    // 현재 로봇 상태가 SLEEPING이 아니고 (깨어있거나 다른 애니메이션 중),
    // 마지막 활동 시간(lastCommandTime)으로부터 미리 정의된 TIME_TO_SLEEP_EXAMPLE 시간(예: 10초)이 경과했으면
    if (_state != SLEEPING && millis() - lastCommandTime >= TIME_TO_SLEEP_EXAMPLE) {
        setState(SLEEPING); // 로봇 상태를 SLEEPING으로 변경합니다. (setState 내부에서 잠자는 애니메이션 시작)
    }
    // 만약 로봇 상태가 SLEEPING인데, 마지막 활동 시간으로부터 TIME_TO_SLEEP_EXAMPLE 시간보다 적게 경과했다면 (활동이 감지되었다면)
    else if (_state == SLEEPING && millis() - lastCommandTime < TIME_TO_SLEEP_EXAMPLE) {
         setState(AWAKE); // 로봇 상태를 AWAKE로 변경 시도합니다. (setState 내부에서 잠 깨는 애니메이션 시작)
    }

    // 시리얼 입력 처리 (선택 사항)
    // 시리얼 모니터에서 "blink", "sad", "awake", "sleeping" 또는 표시할 텍스트 등을 입력하여 테스트할 수 있습니다.
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n'); // 시리얼 버퍼에서 줄바꿈 문자가 올 때까지 문자열 읽기
        command.trim(); // 읽어온 문자열의 앞뒤 공백 및 줄바꿈 문자를 제거
        Serial.print("Received command: "); // 수신된 명령을 시리얼 모니터에 출력
        Serial.println(command);

        processCommand(command.c_str()); // 수신된 명령 문자열을 processCommand 함수에 전달하여 처리
        lastCommandTime = millis(); // 명령을 수신했으므로 마지막 활동 시간을 현재 시간으로 업데이트합니다.
    }

    // 루프가 너무 빠르게 반복되지 않도록 필요에 따라 작은 지연을 추가할 수 있습니다.
    // FastLED.show() 함수 자체에 어느 정도의 시간이 소요되므로, 대부분의 경우 추가 delay는 필요하지 않습니다.
    // delay(1); // 예시
}
