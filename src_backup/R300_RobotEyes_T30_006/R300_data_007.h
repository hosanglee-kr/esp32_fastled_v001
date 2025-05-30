#pragma once

// R300_data_005.h - 로봇 눈 애니메이션 및 폰트 데이터 정의 파일
// 이 파일은 로봇 눈 애니메이션에 사용되는 모든 정적 데이터 테이블을 정의합니다.
// 데이터는 프로그램 메모리(PROGMEM)에 저장되어 RAM 사용량을 절약합니다.

// 기본 타입 및 설정 헤더 파일 포함
// 이 헤더 파일은 T_R300_로 시작하는 구조체 및 열거형과 G_R300_로 시작하는 상수를 정의합니다.
#include "R300_types_config_006.h"

// --- 전역 상수 정의 (G_R300_ 로 시작) ---
// 로봇 상태 관리를 위한 비활성 시간 기준 정의
// 예제: 로봇이 일정 시간(10초) 동안 명령을 받지 않으면 SLEEPING 상태로 전환됩니다.
const unsigned long G_R300_TIME_TO_SLEEP_EXAMPLE = 10000; // 밀리초 단위 (10초)


// --- 정적 데이터 테이블 정의 (PROGMEM에 저장, g_R300_ 로 시작) ---
// 각 배열은 특정 감정 애니메이션의 프레임 시퀀스를 나타냅니다.
// 각 요소는 T_R300_animFrame_t 구조체이며,
// { {오른쪽 눈 폰트 인덱스, 왼쪽 눈 폰트 인덱스}, 해당 프레임 표시 시간(ms) } 형식입니다.

// 깜빡임 애니메이션 시퀀스 데이터
const T_R300_animFrame_t g_R300_seqBlink[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME / 2},                                // 프레임 0: 기본 눈 모양
    {{1, 1}, G_R300_FRAME_TIME / 2},                                // 프레임 1: 살짝 감김
    {{2, 2}, G_R300_FRAME_TIME / 2},                                // 프레임 2: 더 감김
    {{3, 3}, G_R300_FRAME_TIME / 2},                                // 프레임 3: 거의 감김
    {{4, 4}, G_R300_FRAME_TIME / 2},                                // 프레임 4: 거의 완전히 감김
    {{5, 5}, G_R300_FRAME_TIME},                                    // 프레임 5: 완전히 감김 (최대 시간 유지)
};

// 윙크 애니메이션 시퀀스 데이터 (원본 데이터 기준 오른쪽 눈 윙크)
const T_R300_animFrame_t g_R300_seqWink[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME / 2},                                // 프레임 0: 기본 눈 모양
    {{1, 0}, G_R300_FRAME_TIME / 2},                                // 프레임 1: 오른쪽 눈 살짝 감김
    {{2, 0}, G_R300_FRAME_TIME / 2},                                // 프레임 2: 오른쪽 눈 더 감김
    {{3, 0}, G_R300_FRAME_TIME / 2},                                // 프레임 3: 오른쪽 눈 거의 감김
    {{4, 0}, G_R300_FRAME_TIME / 2},                                // 프레임 4: 오른쪽 눈 거의 완전히 감김
    {{5, 0}, G_R300_FRAME_TIME * 2},                                // 프레임 5: 오른쪽 눈 완전히 감김 (최대 시간 유지)
};

// 오른쪽 보기 애니메이션 시퀀스 데이터
const T_R300_animFrame_t g_R300_seqRight[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME},                                    // 프레임 0: 기본 눈 모양
    {{6, 6}, G_R300_FRAME_TIME},                                    // 프레임 1: 눈동자 오른쪽 이동 시작
    {{7, 7}, G_R300_FRAME_TIME * 5},                                // 프레임 2: 눈동자 오른쪽 끝 (최대 시간 유지)
};

// 왼쪽 보기 애니메이션 시퀀스 데이터
const T_R300_animFrame_t g_R300_seqLeft[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME},                                    // 프레임 0: 기본 눈 모양
    {{8, 8}, G_R300_FRAME_TIME},                                    // 프레임 1: 눈동자 왼쪽 이동 시작
    {{9, 9}, G_R300_FRAME_TIME * 5},                                // 프레임 2: 눈동자 왼쪽 끝 (최대 시간 유지)
};

// 위 보기 애니메이션 시퀀스 데이터
const T_R300_animFrame_t g_R300_seqUp[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME},                                    // 프레임 0: 기본 눈 모양
    {{11, 11}, G_R300_FRAME_TIME},                                  // 프레임 1: 눈동자 위 이동 시작
    {{12, 12}, G_R300_FRAME_TIME},                                  // 프레임 2: 눈동자 더 위로
    {{13, 13}, G_R300_FRAME_TIME * 5},                              // 프레임 3: 눈동자 위 끝 (최대 시간 유지)
};

// 아래 보기 애니메이션 시퀀스 데이터
const T_R300_animFrame_t g_R300_seqDown[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME},                                    // 프레임 0: 기본 눈 모양
    {{14, 14}, G_R300_FRAME_TIME},                                  // 프레임 1: 눈동자 아래 이동 시작
    {{15, 15}, G_R300_FRAME_TIME},                                  // 프레임 2: 눈동자 더 아래로
    {{16, 16}, G_R300_FRAME_TIME * 5},                              // 프레임 3: 눈동자 아래 끝 (최대 시간 유지)
};

// 화남 애니메이션 시퀀스 데이터 (좌우 눈 모양 인덱스 다름)
const T_R300_animFrame_t g_R300_seqAngry[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME},                                    // 프레임 0: 기본 눈 모양
    {{22, 17}, G_R300_FRAME_TIME},                                  // 프레임 1: 눈썹 찡그림 시작 (오른쪽 눈 22, 왼쪽 눈 17)
    {{23, 18}, G_R300_FRAME_TIME},                                  // 프레임 2: 눈썹 더 찡그림 (오른쪽 눈 23, 왼쪽 눈 18)
    {{24, 19}, G_R300_FRAME_TIME},                                  // 프레임 3: 눈썹 많이 찡그림 (오른쪽 눈 24, 왼쪽 눈 19)
    {{25, 20}, 2000},                                               // 프레임 4: 가장 화난 눈 모양 (오른쪽 눈 25, 왼쪽 눈 20, 2초 유지)
};

// 슬픔 애니메이션 시퀀스 데이터 (좌우 눈 모양 인덱스 다름)
const T_R300_animFrame_t g_R300_seqSad[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME},                                    // 프레임 0: 기본 눈 모양
    {{32, 27}, G_R300_FRAME_TIME},                                  // 프레임 1: 눈꼬리 처짐 시작 (오른쪽 눈 32, 왼쪽 눈 27)
    {{33, 28}, G_R300_FRAME_TIME},                                  // 프레임 2: 눈꼬리 더 처짐 (오른쪽 눈 33, 왼쪽 눈 28)
    {{34, 29}, 2000},                                               // 프레임 3: 가장 슬픈 눈 모양 (오른쪽 눈 34, 왼쪽 눈 29, 2초 유지)
};

// 사악함 애니메이션 시퀀스 데이터 (좌우 눈 모양 인덱스 다름)
const T_R300_animFrame_t g_R300_seqEvil[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME},                                    // 프레임 0: 기본 눈 모양
    {{39, 37}, G_R300_FRAME_TIME},                                  // 프레임 1: 눈썹 치켜올림 시작 (오른쪽 눈 39, 왼쪽 눈 37)
    {{40, 38}, 2000},                                               // 프레임 2: 가장 사악한 눈 모양 (오른쪽 눈 40, 왼쪽 눈 38, 2초 유지)
};

// 사악함 2 애니메이션 시퀀스 데이터 (좌우 눈 모양 인덱스 다름)
const T_R300_animFrame_t g_R300_seqEvil2[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME},                                    // 프레임 0: 기본 눈 모양
    {{54, 17}, G_R300_FRAME_TIME},                                  // 프레임 1: 좌우 비대칭 눈 모양 시작 (오른쪽 54, 왼쪽 17)
    {{55, 18}, G_R300_FRAME_TIME},                                  // 프레임 2: 좌우 비대칭 진행 (오른쪽 55, 왼쪽 18)
    {{56, 19}, G_R300_FRAME_TIME},                                  // 프레임 3: 좌우 비대칭 진행 (오른쪽 56, 왼쪽 19)
    {{57, 20}, 2000},                                               // 프레임 4: 좌우 비대칭 최종 상태 (오른쪽 57, 왼쪽 20, 2초 유지)
};

// 찡그림 애니메이션 시퀀스 데이터 (좌우 눈 동일 모양)
const T_R300_animFrame_t g_R300_seqSquint[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME},                                    // 프레임 0: 기본 눈 모양
    {{54, 54}, G_R300_FRAME_TIME},                                  // 프레임 1: 눈 살짝 찡그림 시작
    {{55, 55}, G_R300_FRAME_TIME},                                  // 프레임 2: 눈 더 찡그림
    {{56, 56}, G_R300_FRAME_TIME},                                  // 프레임 3: 눈 많이 찡그림
    {{57, 57}, 2000},                                               // 프레임 4: 최대로 찡그린 모양 (2초 유지)
};

// 죽은 눈 애니메이션 시퀀스 데이터 (좌우 눈 동일 모양)
const T_R300_animFrame_t g_R300_seqDead[] PROGMEM = {
    {{52, 52}, G_R300_FRAME_TIME * 4},                              // 프레임 0: X자 눈 모양 1 (400ms 유지)
    {{53, 53}, G_R300_FRAME_TIME * 4},                              // 프레임 1: X자 눈 모양 2 (400ms 유지)
    {{52, 52}, G_R300_FRAME_TIME * 2},                              // 프레임 2: X자 눈 모양 1 (200ms 유지) - 짧게 반복되는 효과
};

// 좌우 스캔 애니메이션 시퀀스 데이터 (좌우 눈 동일 모양)
const T_R300_animFrame_t g_R300_seqScanLeftRight[] PROGMEM = {
    {{41, 41}, G_R300_FRAME_TIME * 2},                              // 프레임 0: 눈동자 왼쪽 끝 (200ms 유지)
    {{42, 42}, G_R300_FRAME_TIME},                                  // 프레임 1: 눈동자 이동
    {{43, 43}, G_R300_FRAME_TIME},                                  // 프레임 2: 눈동자 이동
    {{44, 44}, G_R300_FRAME_TIME},                                  // 프레임 3: 눈동자 오른쪽 끝
};

// 상하 스캔 애니메이션 시퀀스 데이터 (좌우 눈 동일 모양)
const T_R300_animFrame_t g_R300_seqScanUpDown[] PROGMEM = {
    {{46, 46}, G_R300_FRAME_TIME * 2},                              // 프레임 0: 눈동자 위쪽 중간 (200ms 유지)
    {{47, 47}, G_R300_FRAME_TIME},                                  // 프레임 1: 눈동자 이동
    {{48, 48}, G_R300_FRAME_TIME},                                  // 프레임 2: 눈동자 이동
    {{49, 49}, G_R300_FRAME_TIME},                                  // 프레임 3: 눈동자 이동
    {{50, 50}, G_R300_FRAME_TIME},                                  // 프레임 4: 눈동자 이동
    {{51, 51}, G_R300_FRAME_TIME},                                  // 프레임 5: 눈동자 아래 끝
};

// 찡그림 깜빡임 애니메이션 시퀀스 데이터 (좌우 눈 동일 모양)
const T_R300_animFrame_t g_R300_seqSquintBlink[] PROGMEM = {
    {{57, 57}, G_R300_FRAME_TIME},                                  // 프레임 0: 최대로 찡그린 모양
    {{5, 5}, G_R300_FRAME_TIME},                                    // 프레임 1: 완전히 감긴 모양
};

// 대기 상태 눈 모양 시퀀스 데이터 (정적 표시)
const T_R300_animFrame_t g_R300_seqIdle[] PROGMEM = {
    {{57, 57}, G_R300_FRAME_TIME},                                  // 프레임 0: 최대로 찡그린 모양 유지
};

// 하트 애니메이션 시퀀스 데이터 (좌우 눈 동일 모양)
const T_R300_animFrame_t g_R300_seqHeart[] PROGMEM = {
    {{ 0,  0}, G_R300_FRAME_TIME},                                  // 프레임 0: 기본 눈 모양
    {{59, 59}, G_R300_FRAME_TIME},                                  // 프레임 1: 하트 모양 나타남
    {{60, 60}, 2000},                                               // 프레임 2: 완성된 하트 모양 (2초 유지)
};

// 잠자는 눈 애니메이션 시퀀스 데이터 (좌우 눈 동일 모양)
const T_R300_animFrame_t g_R300_seqSleep[] PROGMEM = {
    {{ 0,  0}, G_R300_FRAME_TIME},                                  // 프레임 0: 기본 눈 모양
    {{62, 62}, G_R300_FRAME_TIME},                                  // 프레임 1: 감기기 시작
    {{63, 63}, G_R300_FRAME_TIME},                                  // 프레임 2: 더 감김
    {{64, 64}, G_R300_FRAME_TIME},                                  // 프레임 3: 더 감김
    {{65, 65}, G_R300_FRAME_TIME},                                  // 프레임 4: 더 감김
    {{66, 66}, G_R300_FRAME_TIME},                                  // 프레임 5: 더 감김
    {{67, 67}, G_R300_FRAME_TIME},                                  // 프레임 6: 완전히 감긴 모양 또는 패턴 등
};

// --- 애니메이션 시퀀스 검색을 위한 조회 테이블 정의 ---
// T_R300_emotion_t 열거형 값과 실제 애니메이션 시퀀스 데이터를 연결합니다.
// 새로운 애니메이션 시퀀스를 추가하려면 이 테이블에도 항목을 추가해야 합니다.
const T_R300_animTable_t g_R300_lookupTable[] PROGMEM = {
    {E_NEUTRAL      , g_R300_seqBlink           , 1                                         },      // 중립: Blink 시퀀스의 첫 프레임만 사용 (정적)
    {E_BLINK        , g_R300_seqBlink           , G_R300_ARRAY_SIZE(g_R300_seqBlink)        },      // 깜빡임
    {E_WINK         , g_R300_seqWink            , G_R300_ARRAY_SIZE(g_R300_seqWink)         },      // 윙크
    {E_LOOK_L       , g_R300_seqLeft            , G_R300_ARRAY_SIZE(g_R300_seqLeft)         },      // 왼쪽 보기
    {E_LOOK_R       , g_R300_seqRight           , G_R300_ARRAY_SIZE(g_R300_seqRight)        },      // 오른쪽 보기
    {E_LOOK_U       , g_R300_seqUp              , G_R300_ARRAY_SIZE(g_R300_seqUp)           },      // 위 보기
    {E_LOOK_D       , g_R300_seqDown            , G_R300_ARRAY_SIZE(g_R300_seqDown)         },      // 아래 보기
    {E_ANGRY        , g_R300_seqAngry           , G_R300_ARRAY_SIZE(g_R300_seqAngry)        },      // 화남
    {E_SAD          , g_R300_seqSad             , G_R300_ARRAY_SIZE(g_R300_seqSad)          },      // 슬픔
    {E_EVIL         , g_R300_seqEvil            , G_R300_ARRAY_SIZE(g_R300_seqEvil)         },      // 사악함
    {E_EVIL2        , g_R300_seqEvil2           , G_R300_ARRAY_SIZE(g_R300_seqEvil2)        },      // 사악함 2
    {E_DEAD         , g_R300_seqDead            , G_R300_ARRAY_SIZE(g_R300_seqDead)         },      // 죽은 눈
    {E_SCAN_LR      , g_R300_seqScanLeftRight   , G_R300_ARRAY_SIZE(g_R300_seqScanLeftRight)},      // 좌우 스캔
    {E_SCAN_UD      , g_R300_seqScanUpDown      , G_R300_ARRAY_SIZE(g_R300_seqScanUpDown)   },      // 상하 스캔
    // Idle animations
    {E_IDLE         , g_R300_seqIdle            , G_R300_ARRAY_SIZE(g_R300_seqIdle)         },      // 대기 상태
    {E_SQUINT       , g_R300_seqSquint          , G_R300_ARRAY_SIZE(g_R300_seqSquint)       },      // 찡그림
    {E_SQUINT_BLINK , g_R300_seqSquintBlink     , G_R300_ARRAY_SIZE(g_R300_seqSquintBlink)  },      // 찡그림 깜빡임
    // Emojis
    {E_HEART        , g_R300_seqHeart           , G_R300_ARRAY_SIZE(g_R300_seqHeart)        },      // 하트
    // Sleep
    {E_SLEEP        , g_R300_seqSleep           , G_R300_ARRAY_SIZE(g_R300_seqSleep)        },      // 잠자는 눈
};


//https://xantorohara.github.io/led-matrix-editor/#
// https://www.piskelapp.com/p/create/sprite/

// --- 눈 모양 비트맵 데이터 정의 (폰트 역할, PROGMEM에 저장) ---
// 각 항목은 8x8 픽셀의 눈 모양 비트맵을 나타냅니다.
// T_R300_SimpleFontChar 구조체 형식: { 너비(항상 8), {8바이트의 비트맵 데이터} }
// 비트맵 데이터: 각 바이트는 세로 열을 나타내며, 바이트 내 비트는 LSB(Bit 0)부터 MSB(Bit 7)까지
// 각각 0번째 행부터 7번째 행을 나타냅니다 (즉, LSB가 맨 아래 행, MSB가 맨 위 행).
const T_R300_SimpleFontChar g_R300_RobotEyes_Font[] PROGMEM = {
    {8, {  0, 126, 129, 177, 177, 129, 126,   0}},      //  0   - 'Rest Position' (기본 눈 모양)
    {8, {  0, 124, 130, 178, 178, 130, 124,   0}},      //  1   - 'Blink 1' (깜빡임 1단계)
    {8, {  0, 120, 132, 180, 180, 132, 120,   0}},      //  2   - 'Blink 2' (깜빡임 2단계)
    {8, {  0,  48,  72, 120, 120,  72,  48,   0}},      //  3   - 'Blink 3' (깜빡임 3단계)
    {8, {  0,  32,  80, 112, 112,  80,  32,   0}},      //  4   - 'Blink 4' (깜빡임 4단계)
    {8, {  0,  32,  96,  96,  96,  96,  32,   0}},      //  5   - 'Blink 5' (완전히 감김)
    {8, {  0, 126, 129, 129, 177, 177, 126,   0}},      //  6   - 'Right 1' (오른쪽 보기 1단계)
    {8, {  0,   0, 126, 129, 129, 177, 177, 126}},      //  7   - 'Right 2' (오른쪽 보기 2단계)
    {8, {  0, 126, 177, 177, 129, 129, 126,   0}},      //  8   - 'Left 1' (왼쪽 보기 1단계)
    {8, {126, 177, 177, 129, 129, 126,   0,   0}},      //  9   - 'Left 2' (왼쪽 보기 2단계)
    {0, {  0,   0,   0,   0,   0,   0,   0,   0}},      // 10      (사용되지 않음)
    {8, {  0, 126, 129, 153, 153, 129, 126,   0}},      // 11   - 'Up 1' (위 보기 1단계)
    {8, {  0, 126, 129, 141, 141, 129, 126,   0}},      // 12   - 'Up 2' (위 보기 2단계)
    {8, {  0, 126, 129, 135, 135, 129, 126,   0}},      // 13   - 'Up 3' (위 보기 3단계)
    {8, {  0, 126, 129, 225, 225, 129, 126,   0}},      // 14   - 'Down 1' (아래 보기 1단계)
    {8, {  0, 126, 129, 193, 193, 129, 126,   0}},      // 15   - 'Down 2' (아래 보기 2단계)
    {8, {  0, 124, 130, 194, 194, 130, 124,   0}},      // 16   - 'Down 3' (아래 보기 3단계)
    {8, {  0, 124, 130, 177, 177, 129, 126,   0}},      // 17   - 'Angry L 1' (왼쪽 눈 화남 1단계)
    {8, {  0, 120, 132, 178, 177, 129, 126,   0}},      // 18   - 'Angry L 2' (왼쪽 눈 화남 2단계)
    {8, {  0, 112, 136, 164, 178, 129, 126,   0}},      // 19   - 'Angry L 3' (왼쪽 눈 화남 3단계)
    {8, {  0,  96, 144, 168, 180, 130, 127,   0}},      // 20   - 'Angry L 4' (왼쪽 눈 화남 4단계)
    {0, {  0,   0,   0,   0,   0,   0,   0,   0}},      // 21     (사용되지 않음)
    {8, {  0, 126, 129, 177, 177, 130, 124,   0}},      // 22   - 'Angry R 1' (오른쪽 눈 화남 1단계)
    {8, {  0, 126, 129, 177, 178, 132, 120,   0}},      // 23   - 'Angry R 2' (오른쪽 눈 화남 2단계)
    {8, {  0, 126, 129, 178, 164, 136, 112,   0}},      // 24   - 'Angry R 3' (오른쪽 눈 화남 3단계)
    {8, {  0, 127, 130, 180, 168, 144,  96,   0}},      // 25   - 'Angry R 4' (오른쪽 눈 화남 4단계)
    {0, {  0,   0,   0,   0,   0,   0,   0,   0}},      // 26     (사용되지 않음)
    {8, {  0,  62,  65, 153, 153, 130, 124,   0}},      // 27   - 'Sad L 1' (왼쪽 눈 슬픔 1단계)
    {8, {  0,  30,  33,  89, 154, 132, 120,   0}},      // 28   - 'Sad L 2' (왼쪽 눈 슬픔 2단계)
    {8, {  0,  14,  17,  41,  90, 132, 120,   0}},      // 29   - 'Sad L 3' (왼쪽 눈 슬픔 3단계)
    {0, {  0,   0,   0,   0,   0,   0,   0,   0}},      // 30     (사용되지 않음)
    {0, {  0,   0,   0,   0,   0,   0,   0,   0}},      // 31     (사용되지 않음)
    {8, {  0, 124, 130, 153, 153,  65,  62,   0}},      // 32   - 'Sad R 1' (오른쪽 눈 슬픔 1단계)
    {8, {  0, 120, 132, 154,  89,  33,  30,   0}},      // 33   - 'Sad R 2' (오른쪽 눈 슬픔 2단계)
    {8, {  0, 120, 132,  90,  41,  17,  14,   0}},      // 34   - 'Sad R 3' (오른쪽 눈 슬픔 3단계)
    {0, {  0,   0,   0,   0,   0,   0,   0,   0}},      // 35     (사용되지 않음)
    {0, {  0,   0,   0,   0,   0,   0,   0,   0}},      // 36     (사용되지 않음)
    {8, {  0, 124, 194, 177, 177, 193, 126,   0}},      // 37   - 'Evil L 1' (왼쪽 눈 사악함 1단계)
    {8, {  0,  56,  68, 178, 177,  66,  60,   0}},      // 38   - 'Evil L 2' (왼쪽 눈 사악함 2단계)
    {8, {  0, 126, 193, 177, 177, 194, 124,   0}},      // 39   - 'Evil R 1' (오른쪽 눈 사악함 1단계)
    {8, {  0,  60,  66, 177, 178,  68,  56,   0}},      // 40   - 'Evil R 2' (오른쪽 눈 사악함 2단계)
    {8, {  0, 126, 129, 129, 129, 189, 126,   0}},      // 41   - 'Scan H 1' (좌우 스캔 1단계)
    {8, {  0, 126, 129, 129, 189, 129, 126,   0}},      // 42   - 'Scan H 2' (좌우 스캔 2단계)
    {8, {  0, 126, 129, 189, 129, 129, 126,   0}},      // 43   - 'Scan H 3' (좌우 스캔 3단계)
    {8, {  0, 126, 189, 129, 129, 129, 126,   0}},      // 44   - 'Scan H 4' (좌우 스캔 4단계)
    {0, {  0,   0,   0,   0,   0,   0,   0,   0}},      // 45     (사용되지 않음)
    {8, {  0, 126, 129, 131, 131, 129, 126,   0}},      // 46   - 'Scan V 1' (상하 스캔 1단계)
    {8, {  0, 126, 129, 133, 133, 129, 126,   0}},      // 47   - 'Scan V 2' (상하 스캔 2단계)
    {8, {  0, 126, 129, 137, 137, 129, 126,   0}},      // 48   - 'Scan V 3' (상하 스캔 3단계)
    {8, {  0, 126, 129, 145, 145, 129, 126,   0}},      // 49   - 'Scan V 4' (상하 스캔 4단계)
    {8, {  0, 126, 129, 161, 161, 129, 126,   0}},      // 50   - 'Scan V 5' (상하 스캔 5단계)
    {8, {  0, 126, 129, 193, 193, 129, 126,   0}},      // 51   - 'Scan V 6' (상하 스캔 6단계)
    {8, {  0, 126, 137, 157, 137, 129, 126,   0}},      // 52   - 'RIP 1' (죽은 눈 1)
    {8, {  0, 126, 129, 145, 185, 145, 126,   0}},      // 53   - 'RIP 2' (죽은 눈 2)
    {8, {  0,  60,  66, 114, 114,  66,  60,   0}},      // 54   - 'Peering 1' (찡그림/훔쳐보기 1단계)
    {8, {  0,  56,  68, 116, 116,  68,  56,   0}},      // 55   - 'Peering 2' (찡그림/훔쳐보기 2단계)
    {8, {  0,  48,  72, 120, 120,  72,  48,   0}},      // 56   - 'Peering 3' (찡그림/훔쳐보기 3단계)
    {8, {  0,  32,  80, 112, 112,  80,  32,   0}},      // 57   - 'Peering 4' (찡그림/훔쳐보기 4단계, 대기 상태 기본값)
    {0, {  0,   0,   0,   0,   0,   0,   0,   0}},      // 58     (사용되지 않음)
    {8, {  0,  28,  34, 114, 114,  34,  28,   0}},      // 59   - Core 1 (하트 1단계)
    {8, {  0,  28,  34, 100, 100,  34,  28,   0}},      // 60   - Core 2 (하트 2단계)
    {0, {  0,   0,   0,   0,   0,   0,   0,   0}},      // 61     (사용되지 않음)
    {8, {129, 193, 161, 145, 137, 133, 131, 129}},      // 62   - 'Sleep 1' (잠자는 눈 1단계)
    {8, {144, 208, 176, 176,   9,  13,  11,   9}},      // 63   - 'Sleep 2' (잠자는 눈 2단계)
    {8, {160, 224, 160,  20,  28,  20,   0,   0}},      // 64   - 'Sleep 3' (잠자는 눈 3단계)
    {8, {144, 208, 176, 144,   0,   0,   0,   0}},      // 65   - 'Sleep 4' (잠자는 눈 4단계)
    {8, {160, 224, 160,   0,   0,   0,   0,   0}},      // 66   - 'Sleep 5' (잠자는 눈 5단계)
    {8, {128,   0,   0,   0,   0,   0,   0,   0}},      // 67   - 'Sleep 6' (잠자는 눈 6단계)
    // 68부터는 사용되지 않으며, 필요시 이곳에 새로운 눈 모양 비트맵 데이터를 추가할 수 있습니다.
};
