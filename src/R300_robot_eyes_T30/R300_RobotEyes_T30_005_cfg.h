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
