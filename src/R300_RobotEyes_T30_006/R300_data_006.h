#pragma once

// R300_data_005.h - 로봇 눈 애니메이션 및 폰트 데이터 정의 파일

// 기본 타입 및 설정 헤더 파일 포함
#include "R300_types_config_006.h"

// 로봇 상태 관리를 위한 비활성 시간 기준 정의
const unsigned long G_R300_TIME_TO_SLEEP_EXAMPLE = 10000; // 예제: 10초 동안 명령이 없으면 SLEEPING 상태로 전환


// --- 정적 데이터 테이블 정의 (PROGMEM에 저장, g_R300_ 로 시작) ---
// 애니메이션 시퀀스 데이터
const T_R300_animFrame_t g_R300_seqBlink[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME / 2}, {{1, 1}, G_R300_FRAME_TIME / 2}, {{2, 2}, G_R300_FRAME_TIME / 2},
    {{3, 3}, G_R300_FRAME_TIME / 2}, {{4, 4}, G_R300_FRAME_TIME / 2}, {{5, 5}, G_R300_FRAME_TIME},
};

const T_R300_animFrame_t g_R300_seqWink[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME / 2}, {{1, 0}, G_R300_FRAME_TIME / 2}, {{2, 0}, G_R300_FRAME_TIME / 2},
    {{3, 0}, G_R300_FRAME_TIME / 2}, {{4, 0}, G_R300_FRAME_TIME / 2}, {{5, 0}, G_R300_FRAME_TIME * 2},
};

const T_R300_animFrame_t g_R300_seqRight[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME}, {{6, 6}, G_R300_FRAME_TIME}, {{7, 7}, G_R300_FRAME_TIME * 5},
};

const T_R300_animFrame_t g_R300_seqLeft[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME}, {{8, 8}, G_R300_FRAME_TIME}, {{9, 9}, G_R300_FRAME_TIME * 5},
};

const T_R300_animFrame_t g_R300_seqUp[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME}, {{11, 11}, G_R300_FRAME_TIME}, {{12, 12}, G_R300_FRAME_TIME},
    {{13, 13}, G_R300_FRAME_TIME * 5},
};

const T_R300_animFrame_t g_R300_seqDown[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME}, {{14, 14}, G_R300_FRAME_TIME}, {{15, 15}, G_R300_FRAME_TIME},
    {{16, 16}, G_R300_FRAME_TIME * 5},
};

const T_R300_animFrame_t g_R300_seqAngry[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME}, {{22, 17}, G_R300_FRAME_TIME}, {{23, 18}, G_R300_FRAME_TIME},
    {{24, 19}, G_R300_FRAME_TIME}, {{25, 20}, 2000},
};

const T_R300_animFrame_t g_R300_seqSad[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME}, {{32, 27}, G_R300_FRAME_TIME}, {{33, 28}, G_R300_FRAME_TIME},
    {{34, 29}, 2000},
};

const T_R300_animFrame_t g_R300_seqEvil[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME}, {{39, 37}, G_R300_FRAME_TIME}, {{40, 38}, 2000},
};

const T_R300_animFrame_t g_R300_seqEvil2[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME}, {{54, 17}, G_R300_FRAME_TIME}, {{55, 18}, G_R300_FRAME_TIME},
    {{56, 19}, G_R300_FRAME_TIME}, {{57, 20}, 2000},
};

const T_R300_animFrame_t g_R300_seqSquint[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME}, {{54, 54}, G_R300_FRAME_TIME}, {{55, 55}, G_R300_FRAME_TIME},
    {{56, 56}, G_R300_FRAME_TIME}, {{57, 57}, 2000},
};

const T_R300_animFrame_t g_R300_seqDead[] PROGMEM = {
    {{52, 52}, G_R300_FRAME_TIME * 4}, {{53, 53}, G_R300_FRAME_TIME * 4}, {{52, 52}, G_R300_FRAME_TIME * 2},
};

const T_R300_animFrame_t g_R300_seqScanLeftRight[] PROGMEM = {
    {{41, 41}, G_R300_FRAME_TIME * 2}, {{42, 42}, G_R300_FRAME_TIME}, {{43, 43}, G_R300_FRAME_TIME},
    {{44, 44}, G_R300_FRAME_TIME},
};

const T_R300_animFrame_t g_R300_seqScanUpDown[] PROGMEM = {
    {{46, 46}, G_R300_FRAME_TIME * 2}, {{47, 47}, G_R300_FRAME_TIME}, {{48, 48}, G_R300_FRAME_TIME},
    {{49, 49}, G_R300_FRAME_TIME}, {{50, 50}, G_R300_FRAME_TIME}, {{51, 51}, G_R300_FRAME_TIME},
};

const T_R300_animFrame_t g_R300_seqSquintBlink[] PROGMEM = {
    {{57, 57}, G_R300_FRAME_TIME}, {{5, 5}, G_R300_FRAME_TIME},
};

const T_R300_animFrame_t g_R300_seqIdle[] PROGMEM = {
    {{57, 57}, G_R300_FRAME_TIME},
};

const T_R300_animFrame_t g_R300_seqHeart[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME}, {{59, 59}, G_R300_FRAME_TIME}, {{60, 60}, 2000},
};

const T_R300_animFrame_t g_R300_seqSleep[] PROGMEM = {
    {{0, 0}, G_R300_FRAME_TIME}, {{62, 62}, G_R300_FRAME_TIME}, {{63, 63}, G_R300_FRAME_TIME},
    {{64, 64}, G_R300_FRAME_TIME}, {{65, 65}, G_R300_FRAME_TIME}, {{66, 66}, G_R300_FRAME_TIME},
    {{67, 67}, G_R300_FRAME_TIME},
};

// 애니메이션 시퀀스 검색을 위한 조회 테이블 정의
const T_R300_animTable_t g_R300_lookupTable[] PROGMEM = {
    {E_NEUTRAL, g_R300_seqBlink, 1},  // 중립: Blink 시퀀스의 첫 프레임만 사용
    {E_BLINK, g_R300_seqBlink, G_R300_ARRAY_SIZE(g_R300_seqBlink)},
    {E_WINK, g_R300_seqWink, G_R300_ARRAY_SIZE(g_R300_seqWink)},
    {E_LOOK_L, g_R300_seqLeft, G_R300_ARRAY_SIZE(g_R300_seqLeft)},
    {E_LOOK_R, g_R300_seqRight, G_R300_ARRAY_SIZE(g_R300_seqRight)},
    {E_LOOK_U, g_R300_seqUp, G_R300_ARRAY_SIZE(g_R300_seqUp)},
    {E_LOOK_D, g_R300_seqDown, G_R300_ARRAY_SIZE(g_R300_seqDown)},
    {E_ANGRY, g_R300_seqAngry, G_R300_ARRAY_SIZE(g_R300_seqAngry)},
    {E_SAD, g_R300_seqSad, G_R300_ARRAY_SIZE(g_R300_seqSad)},
    {E_EVIL, g_R300_seqEvil, G_R300_ARRAY_SIZE(g_R300_seqEvil)},
    {E_EVIL2, g_R300_seqEvil2, G_R300_ARRAY_SIZE(g_R300_seqEvil2)},
    {E_DEAD, g_R300_seqDead, G_R300_ARRAY_SIZE(g_R300_seqDead)},
    {E_SCAN_LR, g_R300_seqScanLeftRight, G_R300_ARRAY_SIZE(g_R300_seqScanLeftRight)},
    {E_SCAN_UD, g_R300_seqScanUpDown, G_R300_ARRAY_SIZE(g_R300_seqScanUpDown)},
    // Idle animations
    {E_IDLE, g_R300_seqIdle, G_R300_ARRAY_SIZE(g_R300_seqIdle)},
    {E_SQUINT, g_R300_seqSquint, G_R300_ARRAY_SIZE(g_R300_seqSquint)},
    {E_SQUINT_BLINK, g_R300_seqSquintBlink, G_R300_ARRAY_SIZE(g_R300_seqSquintBlink)},
    // Emojis
    {E_HEART, g_R300_seqHeart, G_R300_ARRAY_SIZE(g_R300_seqHeart)},
    // Sleep
    {E_SLEEP, g_R300_seqSleep, G_R300_ARRAY_SIZE(g_R300_seqSleep)},
};

// 눈 모양 비트맵 데이터 정의 (폰트 역할, PROGMEM에 저장)
const T_R300_SimpleFontChar g_R300_RobotEyes_Font[] PROGMEM = {
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
    // 68-255는 사용되지 않음
};
