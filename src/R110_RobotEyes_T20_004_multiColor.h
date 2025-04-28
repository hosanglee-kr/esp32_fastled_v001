// R110_RobotEyes_T20_003.h

#pragma once // 헤더 파일 중복 포함 방지

// ======================================================================
// FastLED + WS2812B 로봇 눈 애니메이션 라이브러리 (ESP32 Arduino Framework용)
// MD_MAX72XX RobotEyes 예제 기반, FastLED 이식, 명명 규칙 접두사 R110 적용
//
// 기능:
// - 8x8 WS2812B 매트릭스 1개 또는 2개를 로봇 눈으로 제어
// - 다양한 눈 표정 패턴 정의 및 표시
// - 눈 깜빡임, 둘러보기 등 기본 애니메이션 제공
// - 일부 표정(중립, 보기, 생각, 혼돈)은 큰 외곽선을 유지하며 눈동자만 움직이도록 구현
// - 그 외 표정(깜빡임, 찡그림, 감정 표현 등)은 기존 정의된 패턴 그대로 표시
// - 애니메이션 함수에 대각선 눈동자 위치를 활용한 중간 단계 추가 및 타이밍 개선
// - 눈동자 이동 범위 최대한 넓게 조정
// - **각 감정 및 애니메이션 단계에 적절한 컬러 및 컬러 변경 효과 추가**
//
// 하드웨어 설정:
// - WS2812B LED 데이터 핀 (g_R110_DATA_PIN) 연결
// - 사용하는 8x8 매트릭스 개수 설정 (g_R110_NUM_MATRICES: 1 또는 2)
// - 매트릭스 내부 및 매트릭스 간의 LED 배선 방식에 따라 R110_getLedIndex 함수 수정 필요
//
// 사용법:
// 1. 이 파일을 ESP32 Arduino 프로젝트의 src 폴더 등에 포함합니다.
// 2. 메인 .ino 또는 .cpp 파일에서 #include "R110_RobotEyes_T20_003.h" 를 추가합니다.
// 3. setup() 함수에서 R110_init() 함수를 호출하여 라이브러리를 초기화합니다.
// 4. loop() 함수에서 R110_run() 함수를 호출하여 내장 애니메이션 시퀀스를 실행하거나,
//    R110_showEmotion(), R110_animateBlink() 등 개별 함수들을 호출하여 원하는 눈 동작을 구현합니다.
//    R110_showEmotion(emotion_index, eye_color, pupil_color) 형태로 색상 지정 가능합니다.
// ======================================================================

#include <Arduino.h>  // Arduino 핵심 라이브러리 (ESP32 지원)
#include <FastLED.h>  // FastLED 라이브러리 (WS2812B 제어)

// -----------------------------------------------------------------------
// 하드웨어 설정 상수
// -----------------------------------------------------------------------
#define g_R110_DATA_PIN			   13		// WS2812B 데이터 핀 번호
#define g_R110_LED_TYPE			   WS2812B	// 사용하는 LED 칩 타입
#define g_R110_COLOR_ORDER		   GRB		// LED 색상 순서 (모듈에 맞게 변경)

#define g_R110_MATRIX_SIZE		   8												// 8x8 매트릭스 크기
#define g_R110_NUM_MATRICES		   2												// 사용하는 8x8 매트릭스 개수 (1 또는 2)
#define g_R110_NUM_LEDS_PER_MATRIX (g_R110_MATRIX_SIZE * g_R110_MATRIX_SIZE)		// 매트릭스 하나당 LED 개수
#define g_R110_NUM_LEDS			   (g_R110_NUM_LEDS_PER_MATRIX * g_R110_NUM_MATRICES)	// WS2812B 총 LED 개수

// FastLED LED 배열 - 모든 WS2812B LED의 색상 값을 저장
CRGB g_R110_leds[g_R110_NUM_LEDS];

// -----------------------------------------------------------------------
// 색상 정의 (CRGB 상수)
// -----------------------------------------------------------------------
const CRGB R110_COLOR_WHITE = CRGB::White;
const CRGB R110_COLOR_BLACK = CRGB::Black;
const CRGB R110_COLOR_RED = CRGB::Red;
const CRGB R110_COLOR_ORANGE = CRGB::Orange;
const CRGB R110_COLOR_YELLOW = CRGB::Yellow;
const CRGB R110_COLOR_GREEN = CRGB::Green;
const CRGB R110_COLOR_BLUE = CRGB::Blue;
const CRGB R110_COLOR_PURPLE = CRGB::Purple;
const CRGB R110_COLOR_CYAN = CRGB::Cyan;
const CRGB R110_COLOR_MAGENTA = CRGB::Magenta;
const CRGB R110_COLOR_GRAY = CRGB::Gray;
const CRGB R110_COLOR_DIM_WHITE = CRGB(40, 40, 40); // 어두운 흰색
const CRGB R110_COLOR_DIM_YELLOW = CRGB(40, 40, 0); // 어두운 노란색


// -----------------------------------------------------------------------
// 2차원 (행, 열) 좌표를 1차원 LED 배열 인덱스로 변환
// p_matrix_addr: 매트릭스 주소 (0부터 g_R110_NUM_MATRICES-1 까지)
// p_row: 행 번호 (0부터 g_R110_MATRIX_SIZE-1 까지)
// p_col: 열 번호 (0부터 g_R110_MATRIX_SIZE-1 까지)
// 반환값: 전체 g_R110_leds 배열에서의 해당 픽셀 인덱스, 범위 벗어날 시 -1
// -----------------------------------------------------------------------
int	 R110_getLedIndex(int p_matrix_addr, int p_row, int p_col) {
	 if (p_matrix_addr < 0 || p_matrix_addr >= g_R110_NUM_MATRICES || p_row < 0 || p_row >= g_R110_MATRIX_SIZE || p_col < 0 || p_col >= g_R110_MATRIX_SIZE) {
		 return -1; // 범위를 벗어나는 좌표
	 }

	 int v_base_index = p_matrix_addr * g_R110_NUM_LEDS_PER_MATRIX;	 // 해당 매트릭스의 시작 인덱스
	 int v_local_index;												 // 8x8 매트릭스 내에서의 상대 인덱스

	 // 지그재그(serpentine) 방식 가정 - 실제 배선에 따라 아래 로직을 수정하세요!
	 if (p_row % 2 == 0) {	// 짝수 행 (0, 2, 4, 6)은 왼쪽 -> 오른쪽으로 인덱스 증가
		 v_local_index = p_row * g_R110_MATRIX_SIZE + p_col;
	 } else {  // 홀수 행 (1, 3, 5, 7)은 오른쪽 -> 왼쪽으로 인덱스 증가
		 v_local_index = p_row * g_R110_MATRIX_SIZE + (g_R110_MATRIX_SIZE - 1 - p_col);
	 }

	 return v_base_index + v_local_index;  // 전체 LED 배열에서의 최종 인덱스
}


// -----------------------------------------------------------------------
// 눈 모양 픽셀 데이터 (전체 패턴 오버라이드용)
// 외곽선+눈동자 방식이 아닌, 패턴 그대로 그려질 때 사용됩니다.
// 각 패턴 옆 주석은 8x8 매트릭스 모양 ( #=on, .=off ) 입니다.
// -----------------------------------------------------------------------
const byte Neutral_eye[g_R110_MATRIX_SIZE] = { // 윙크에서 사용되는 작은 중립 눈
	B00000000, // ........
	B00111100, // ..####..
	B01000010, // .#....#.
	B01011010, // .#.##.#.
	B01011010, // .#.##.#.
	B01000010, // .#....#.
	B00111100, // ..####..
	B00000000  // ........
};
const byte BlinkOpen_eye[g_R110_MATRIX_SIZE] = { // 깜빡임 (중간)
	B00000000, // ........
	B00000000, // ........
	B00111100, // ..####..
	B01000010, // .#....#.
	B01000010, // .#....#.
	B00111100, // ..####..
	B00000000, // ........
	B00000000  // ........
};
const byte BlinkMid_eye[g_R110_MATRIX_SIZE] = { // 깜빡임 (더 감김)
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00111100, // ..####..
	B00111100, // ..####..
	B00000000, // ........
	B00000000, // ........
	B00000000  // ........
};
const byte BlinkClose_eye[g_R110_MATRIX_SIZE] = { // 깜빡임 (완전히 감김)
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00000000  // ........
};
const byte Squint_eye[g_R110_MATRIX_SIZE] = { // 찡그림
	B00000000, // ........
	B00000000, // ........
	B01111110, // .######.
	B01111110, // .######.
	B01111110, // .######.
	B01111110, // .######.
	B00000000, // ........
	B00000000  // ........
};
const byte SquintTight_eye[g_R110_MATRIX_SIZE] = { // 더 찡그림
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B01111110, // .######.
	B01111110, // .######.
	B00000000, // ........
	B00000000, // ........
	B00000000  // ........
};
const byte Sleepy_eye[g_R110_MATRIX_SIZE] = { // 졸린 눈
	B00000000, // ........
	B00000000, // ........
	B00111100, // ..####..
	B01000010, // .#....#.
	B01000010, // .#....#.
	B00111100, // ..####..
	B00000000, // ........
	B00000000  // ........
};
const byte Angry_eye[g_R110_MATRIX_SIZE] = { // 화난 눈
	B00000000, // ........
	B00000000, // ........
	B01111110, // .######.
	B01011010, // .#.##.#.
	B01011010, // .#.##.#.
	B01111110, // .######.
	B00000000, // ........
	B00000000  // ........
};
const byte Absurd_eye[g_R110_MATRIX_SIZE] = { // 황당한 눈 (큰 눈동자)
	B00000000, // ........
	B01111110, // .######.
	B10000001, // #......#
	B10111101, // #.####.#
	B10111101, // #.####.#
	B10000001, // #......#
	B01111110, // .######.
	B00000000  // ........
};
const byte Glaring_eye[g_R110_MATRIX_SIZE] = { // 째려보는 눈 (다른 외곽선)
	B00000000, // ........
	B00111100, // ..####..
	B01000010, // .#....#.
	B01000010, // .#....#.
	B01000010, // .#....#.
	B10000010, // #....#.# (수정된 바이트)
	B00111100, // ..####..
	B00000000  // ........
};

// -----------------------------------------------------------------------
// 기본 눈 외곽선 패턴 (최대한 크게)
// Absurd_eye에서 동공 부분을 제거한 형태를 사용합니다.
// 이 외곽선은 눈동자 움직임시 항상 표시됩니다.
// -----------------------------------------------------------------------
const byte BaseEyeOutline[g_R110_MATRIX_SIZE] = {
	B00000000, // ........
	B01111110, // .######.
	B10000001, // #......#
	B10000001, // #......#
	B10000001, // #......#
	B10000001, // #......#
	B01111110, // .######.
	B00000000  // ........
};

// -----------------------------------------------------------------------
// 눈동자 위치 정의 (외곽선 + 눈동자 방식 표정용)
// 2x2 크기의 눈동자의 좌상단 픽셀 좌표 (행, 열)
// -----------------------------------------------------------------------
struct PupilPos {
	int r, c;
};

// 외곽선 + 눈동자 방식을 사용하는 표정들의 눈동자 위치 매핑 헬퍼
// p_emotionIndex에 해당하는 눈동자 블록의 좌상단 위치를 반환
PupilPos R110_getPupilPosForEmotion(int p_emotionIndex) {
    switch(p_emotionIndex) {
        case 0: /* E_NEUTRAL */     return {3, 3}; // Center
        case 4: /* E_LOOK_LEFT */   return {3, 1}; // Extreme Left
        case 5: /* E_LOOK_RIGHT */  return {3, 6}; // Extreme Right
        case 6: /* E_LOOK_UP */     return {2, 3}; // Extreme Up (limited by outline)
        case 7: /* E_LOOK_DOWN */   return {5, 3}; // Extreme Down
        case 16: /* E_THINKING */    return {2, 6}; // Extreme Up-Right (used in Thinking sequence)
        case 18: /* E_LOOK_UP_LEFT */    return {2, 1}; // Extreme Up-Left
        case 19: /* E_LOOK_UP_RIGHT */   return {2, 6}; // Extreme Up-Right (same as Thinking target)
        case 20: /* E_LOOK_DOWN_LEFT */  return {5, 1}; // Extreme Down-Left
        case 21: /* E_LOOK_DOWN_RIGHT */ return {5, 6}; // Extreme Down-Right
        default: return {-1, -1}; // 해당하지 않는 표정
    }
}

// 혼돈스런 눈 (E_CONFUSED) 전용 눈동자 위치 (왼쪽 눈, 오른쪽 눈 순서)
const PupilPos g_R110_confusedPupilPos[2] = {
	{3, 6}, // 왼쪽 눈의 눈동자 위치 (오른쪽으로 치우침)
	{3, 1}  // 오른쪽 눈의 눈동자 위치 (왼쪽으로 치우침)
};

// -----------------------------------------------------------------------
// 감정 패턴 데이터 (전체 패턴 오버라이드용 데이터 포인터 배열)
// EmotionIndex와 직접 매핑되지 않으며, R110_displayEmotionState에서 특정 EmotionIndex일 때 사용됩니다.
// {왼쪽 눈 패턴 데이터 포인터, 오른쪽 눈 패턴 데이터 포인터} 쌍의 배열
// -----------------------------------------------------------------------
const byte *const g_R110_fullOverridePatternsData[]				   = {
	BlinkOpen_eye, BlinkOpen_eye,	   // For E_BLINK_OPEN (1)
	BlinkMid_eye, BlinkMid_eye,		   // For E_BLINK_MID (2)
	BlinkClose_eye, BlinkClose_eye,	   // For E_BLINK_CLOSE (3)
	Squint_eye, Squint_eye,			   // For E_SQUINT (8)
	SquintTight_eye, SquintTight_eye,  // For E_SQUINT_TIGHT (9)
	BlinkClose_eye, Neutral_eye,	   // For E_LEFT_WINK (10) - 왼감, 오뜸 (오른쪽 눈에 기존 Neutral 패턴 사용)
	Neutral_eye, BlinkClose_eye,	   // For E_RIGHT_WINK (11) - 왼뜸, 오감 (왼쪽 눈에 기존 Neutral 패턴 사용)
	Sleepy_eye, Sleepy_eye,			   // For E_SLEEPY (12)
	Angry_eye, Angry_eye,			   // For E_ANGRY (13)
	Absurd_eye, Absurd_eye,			   // For E_ABSURD (14)
	Glaring_eye, Glaring_eye,		   // For E_GLARING (15)
};

// -----------------------------------------------------------------------
// 감정 패턴 인덱스 정의
// -----------------------------------------------------------------------
enum R110_EmotionIndex {
	 E_NEUTRAL		 = 0,
	 E_BLINK_OPEN	 = 1,
	 E_BLINK_MID	 = 2,
	 E_BLINK_CLOSE	 = 3,
	 E_LOOK_LEFT	 = 4,
	 E_LOOK_RIGHT	 = 5,
	 E_LOOK_UP		 = 6,
	 E_LOOK_DOWN	 = 7,
	 E_SQUINT		 = 8,
	 E_SQUINT_TIGHT = 9,
	 E_LEFT_WINK	 = 10,
	 E_RIGHT_WINK	 = 11,
	 E_SLEEPY		 = 12,
	 E_ANGRY		 = 13,
	 E_ABSURD		 = 14,
	 E_GLARING		 = 15,
	 E_THINKING	 = 16, // Thinking uses {2,6} in its animation sequence
	 E_CONFUSED	 = 17,
     // 새로운 대각선 보기 인덱스
     E_LOOK_UP_LEFT = 18,
     E_LOOK_UP_RIGHT = 19,
     E_LOOK_DOWN_LEFT = 20,
     E_LOOK_DOWN_RIGHT = 21
};
// 전체 '표정' 개수
const int g_R110_NUM_EMOTION_PATTERNS = 22; // 마지막 인덱스 + 1

// -----------------------------------------------------------------------
// 애니메이션 속도 조절 상수
// -----------------------------------------------------------------------
const int g_R110_BLINK_SPEED_MS = 50;
const int g_R110_LOOK_SPEED_MS = 80; // 일반적인 룩 애니메이션 프레임 간 지연 (약간 빠르게 조정)
const int g_R110_LOOK_HOLD_MS = 1000; // 보는 방향 유지 시간
const int g_R110_WINK_SPEED_MS = 80;
const int g_R110_WINK_HOLD_MS = 1500;
const int g_R110_CONFUSED_SPEED_MS = 150; // 혼돈 상태 유지 시간
const int g_R110_CONFUSED_DART_SPEED_MS = 40; // 혼돈 애니메이션 중 눈동자 움직임 속도 (더 빠르게 조정)

// -----------------------------------------------------------------------
// 헬퍼: 전체 8x8 패턴을 FastLED 배열에 그리기
// p_pattern_data: 그릴 패턴 데이터 포인터
// p_onColor: 켜짐 픽셀 색상
// -----------------------------------------------------------------------
void	  R110_drawFullPattern(int p_matrix_addr, const byte *p_pattern_data, CRGB p_onColor) {
	if (!p_pattern_data) return; // 유효성 검사

	 for (int v_row = 0; v_row < g_R110_MATRIX_SIZE; v_row++) {
		 byte v_rowData = p_pattern_data[v_row];

		 for (int v_col = 0; v_col < g_R110_MATRIX_SIZE; v_col++) {
			 if ((v_rowData >> (g_R110_MATRIX_SIZE - 1 - v_col)) & 1) {
				 int v_index = R110_getLedIndex(p_matrix_addr, v_row, v_col);
				 if (v_index != -1) {
					 g_R110_leds[v_index] = p_onColor;
				 }
			 }
		 }
	 }
}

// -----------------------------------------------------------------------
// 헬퍼: 눈 외곽선 패턴 그리기
// p_outline_data: 그릴 외곽선 패턴 데이터 포인터
// p_onColor: 외곽선 색상
// -----------------------------------------------------------------------
void R110_drawOutline(int p_matrix_addr, const byte *p_outline_data, CRGB p_onColor) {
	if (!p_outline_data) return; // 유효성 검사

    for (int r = 0; r < g_R110_MATRIX_SIZE; r++) {
        byte rowData = p_outline_data[r];
        for (int c = 0; c < g_R110_MATRIX_SIZE; c++) {
            if ((rowData >> (g_R110_MATRIX_SIZE - 1 - c)) & 1) {
                int index = R110_getLedIndex(p_matrix_addr, r, c);
                if (index != -1) {
                    g_R110_leds[index] = p_onColor; // Draw outline pixel
                }
            }
        }
    }
}

// -----------------------------------------------------------------------
// 헬퍼: 2x2 눈동자 블록 그리기 (기존 픽셀 위에 덮어 씀)
// p_pupil_row, p_pupil_col: 눈동자 블록 좌상단 픽셀 좌표 (0부터 시작)
// p_pupil_color: 눈동자 색상
// -----------------------------------------------------------------------
void R110_drawPupil(int p_matrix_addr, int p_pupil_row, int p_pupil_col, CRGB p_pupil_color) {
    // 눈동자 블록 범위 체크 (8x8 매트릭스 내에 2x2 블록이 완전히 들어와야 함)
    if (p_pupil_row < 0 || p_pupil_col < 0 || p_pupil_row > g_R110_MATRIX_SIZE - 2 || p_pupil_col > g_R110_MATRIX_SIZE - 2) {
        return; // 눈동자 위치가 매트릭스 범위를 벗어남
    }
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 2; c++) {
            int index = R110_getLedIndex(p_matrix_addr, p_pupil_row + r, p_pupil_col + c);
             if (index != -1) {
                 g_R110_leds[index] = p_pupil_color; // Draw pupil pixel (outline 픽셀 위에 덮어 씀)
             }
        }
    }
}


// -----------------------------------------------------------------------
// FastLED 배열에 특정 감정 패턴 그리기 (메인 그리기 로직)
// 기능: 감정 인덱스를 받아 외곽선+눈동자 방식 또는 전체 패턴 방식으로 FastLED 배열에 그립니다.
// p_emotionIndex: 감정 패턴의 논리적인 인덱스
// p_eyeColor: 눈 외곽선 색상
// p_pupilColor: 눈동자 색상
// -----------------------------------------------------------------------
void R110_displayEmotionState(int p_emotionIndex, CRGB p_eyeColor, CRGB p_pupilColor) {
	fill_solid(g_R110_leds, g_R110_NUM_LEDS, CRGB::Black); // 프레임 버퍼 클리어

	bool is_pupil_based = false;
    PupilPos left_pupil = {-1, -1};
    PupilPos right_pupil = {-1, -1};
    const byte* left_override_pattern = nullptr;
    const byte* right_override_pattern = nullptr;

	// 1. 감정 인덱스 유효성 및 그리기 방식/데이터 판단
	if (p_emotionIndex >= 0 && p_emotionIndex < g_R110_NUM_EMOTION_PATTERNS) {

        // 외곽선 + 눈동자 방식을 사용하는 표정들 판단 및 눈동자 위치 설정
        // E_NEUTRAL, E_LOOK_*, E_THINKING, E_CONFUSED, E_LOOK_DIAGONAL_*
        if (p_emotionIndex == E_NEUTRAL ||
            (p_emotionIndex >= E_LOOK_LEFT && p_emotionIndex <= E_LOOK_DOWN) ||
            p_emotionIndex == E_THINKING ||
            (p_emotionIndex >= E_LOOK_UP_LEFT && p_emotionIndex <= E_LOOK_DOWN_RIGHT)
            )
        {
             is_pupil_based = true;
             if (p_emotionIndex == E_CONFUSED) {
                 // 혼돈은 외곽선 + 눈동자 방식이지만 비대칭
                 left_pupil = g_R110_confusedPupilPos[0];
                 right_pupil = g_R110_confusedPupilPos[1];
             } else {
                 // 그 외 외곽선 + 눈동자 방식 표정 (좌우 눈동자 위치 동일)
                 left_pupil = R110_getPupilPosForEmotion(p_emotionIndex); // 헬퍼 함수로 위치 가져옴
                 right_pupil = left_pupil;
             }
        }
        // 나머지 표정은 전체 패턴 오버라이드 방식

        // 전체 패턴 오버라이드 방식을 사용하는 표정들 패턴 데이터 설정
        if (!is_pupil_based) {
             // g_R110_fullOverridePatternsData 배열에서 해당 EmotionIndex에 맞는 패턴을 찾음
             // EmotionIndex와 g_R110_fullOverridePatternsData 배열의 인덱스 매핑 필요
             int override_array_index = -1;
             switch(p_emotionIndex) {
                 case E_BLINK_OPEN: override_array_index = 0; break;
                 case E_BLINK_MID: override_array_index = 1; break;
                 case E_BLINK_CLOSE: override_array_index = 2; break;
                 case E_SQUINT: override_array_index = 3; break;
                 case E_SQUINT_TIGHT: override_array_index = 4; break;
                 case E_LEFT_WINK: override_array_index = 5; break;
                 case E_RIGHT_WINK: override_array_index = 6; break;
                 case E_SLEEPY: override_array_index = 7; break;
                 case E_ANGRY: override_array_index = 8; break;
                 case E_ABSURD: override_array_index = 9; break;
                 case E_GLARING: override_array_index = 10; break;
                 default: override_array_index = -1; break; // 해당하지 않는 표정
             }

             if (override_array_index != -1) {
                  int pattern_data_index = override_array_index * 2;
                  // g_R110_fullOverridePatternsData 배열 경계 체크
                  if (pattern_data_index >= 0 && pattern_data_index < sizeof(g_R110_fullOverridePatternsData)/sizeof(g_R110_fullOverridePatternsData[0])) {
                      left_override_pattern = g_R110_fullOverridePatternsData[pattern_data_index];
                      right_override_pattern = g_R110_fullOverridePatternsData[pattern_data_index + 1];
                  }
             }
        }

	} else {
        // 유효하지 않은 감정 인덱스 (모두 꺼진 상태 유지)
    }


    // 2. 그리기 실행
    // 왼쪽 눈 (Matrix 0)
    if (is_pupil_based) {
        R110_drawOutline(0, BaseEyeOutline, p_eyeColor); // 외곽선 그림
        if (left_pupil.r != -1) { // 눈동자 위치가 유효하면 그림
            R110_drawPupil(0, left_pupil.r, left_pupil.c, p_pupilColor); // 눈동자 그림
        }
    } else if (left_override_pattern) { // 전체 패턴 오버라이드 방식
         R110_drawFullPattern(0, left_override_pattern, p_eyeColor); // 전체 패턴 그림
    }
    // else: 유효하지 않은 인덱스이거나 패턴이 정의되지 않은 경우, 해당 매트릭스는 검은색 유지

    // 오른쪽 눈 (Matrix 1) - 매트릭스가 2개 이상일 때만 해당
	if (g_R110_NUM_MATRICES > 1) {
		if (is_pupil_based) { // 오른쪽 눈도 외곽선+눈동자 방식인 경우
			R110_drawOutline(1, BaseEyeOutline, p_eyeColor); // 외곽선 그림
			if (right_pupil.r != -1) { // 눈동자 위치가 유효하면 그림
				R110_drawPupil(1, right_pupil.r, right_pupil.c, p_pupilColor); // 눈동자 그림
			}
		} else if (right_override_pattern) { // 오른쪽 눈이 전체 패턴 오버라이드인 경우
			 R110_drawFullPattern(1, right_override_pattern, p_eyeColor); // 전체 패턴 그림
		}
		// else: 유효하지 않은 인덱스이거나 패턴이 정의되지 않은 경우, 해당 매트릭스는 검은색 유지
	}

	FastLED.show();	 // 설정된 색상 데이터를 실제 LED로 전송하여 표시
}


// -----------------------------------------------------------------------
// 지정된 감정 패턴을 표시하는 함수
// 기능: 감정 패턴 인덱스를 받아 display 함수 호출
// p_emotionIndex: 감정 패턴의 논리적인 인덱스
// p_eyeColor: 눈 외곽선 색상 (기본값 CRGB::White)
// p_pupilColor: 눈동자 색상 (기본값 CRGB::Black)
// -----------------------------------------------------------------------
void R110_showEmotion(int p_emotionIndex, CRGB p_eyeColor = R110_COLOR_WHITE, CRGB p_pupilColor = R110_COLOR_BLACK) {
    R110_displayEmotionState(p_emotionIndex, p_eyeColor, p_pupilColor);
}

// -----------------------------------------------------------------------
// 애니메이션 함수: 눈 깜빡임
// 기능: Blink 패턴을 순차적으로 표시하여 깜빡임 효과 생성
// p_times: 깜빡이는 횟수
// -----------------------------------------------------------------------
void R110_animateBlink(int p_times = 1) {
	for (int i = 0; i < p_times; i++) {
		R110_showEmotion(E_NEUTRAL); // 외곽선+눈동자 방식 (기본 색상)
		delay(random(500, 2000)); // 중립 상태에서 무작위 대기 후 깜빡임 시작

		R110_showEmotion(E_BLINK_OPEN); // 전체 패턴 오버라이드 사용 (기본 색상)
		delay(g_R110_BLINK_SPEED_MS);

		R110_showEmotion(E_BLINK_MID); // 전체 패턴 오버라이드 사용 (기본 색상)
		delay(g_R110_BLINK_SPEED_MS);

		R110_showEmotion(E_BLINK_CLOSE); // 전체 패턴 오버라이드 사용 (기본 색상)
		delay(g_R110_BLINK_SPEED_MS * 2);

		R110_showEmotion(E_BLINK_MID); // 전체 패턴 오버라이드 사용 (기본 색상)
		delay(g_R110_BLINK_SPEED_MS);

		R110_showEmotion(E_BLINK_OPEN); // 전체 패턴 오버라이드 사용 (기본 색상)
		delay(g_R110_BLINK_SPEED_MS);

		R110_showEmotion(E_NEUTRAL); // 외곽선+눈동자 방식 (기본 색상)
	}
}

// -----------------------------------------------------------------------
// 애니메이션 함수: 지정된 방향 보기
// 기능: 중립에서 지정된 방향 패턴으로 이동, 유지 후 중립 복귀 (대각선 중간 포함)
// p_direction_emotion_index: E_LOOK_LEFT, E_LOOK_RIGHT, E_LOOK_UP, E_LOOK_DOWN, E_THINKING
// -----------------------------------------------------------------------
void R110_animateLook(int p_direction_emotion_index) {
	// 유효한 타겟 보기 인덱스인지 확인 (Neutral은 이 애니메이션의 시작/끝점으로만 사용)
	bool is_valid_target_look = ( (p_direction_emotion_index >= E_LOOK_LEFT && p_direction_emotion_index <= E_LOOK_DOWN) || p_direction_emotion_index == E_THINKING );


	if (is_valid_target_look) {
		// 기본 보기 색상 (생각하기 제외)
		CRGB eyeColor = R110_COLOR_WHITE;
		CRGB pupilColor = R110_COLOR_BLACK;

		if (p_direction_emotion_index == E_THINKING) {
			eyeColor = R110_COLOR_BLUE; // 생각하기 눈 색상
			pupilColor = R110_COLOR_YELLOW; // 생각하기 눈동자 색상
		}

		R110_showEmotion(E_NEUTRAL); // Start from Neutral (기본 색상)
		delay(random(100, 300)); // Short random delay before starting the move

        // 각 타겟 방향별 애니메이션 시퀀스
        switch(p_direction_emotion_index) {
            case E_LOOK_LEFT: // 왼쪽 보기 (위-왼쪽, 아래-왼쪽 경유)
                R110_showEmotion(E_LOOK_UP_LEFT, eyeColor, pupilColor);    delay(g_R110_LOOK_SPEED_MS);
                R110_showEmotion(E_LOOK_LEFT, eyeColor, pupilColor);       delay(g_R110_LOOK_HOLD_MS);      // Hold Left
                R110_showEmotion(E_LOOK_DOWN_LEFT, eyeColor, pupilColor);  delay(g_R110_LOOK_SPEED_MS);
                break;
            case E_LOOK_RIGHT: // 오른쪽 보기 (위-오른쪽, 아래-오른쪽 경유)
                R110_showEmotion(E_LOOK_UP_RIGHT, eyeColor, pupilColor);   delay(g_R110_LOOK_SPEED_MS);
                R110_showEmotion(E_LOOK_RIGHT, eyeColor, pupilColor);      delay(g_R110_LOOK_HOLD_MS);      // Hold Right
                R110_showEmotion(E_LOOK_DOWN_RIGHT, eyeColor, pupilColor); delay(g_R110_LOOK_SPEED_MS);
                break;
            case E_LOOK_UP: // 위 보기 (위-왼쪽, 위-오른쪽 경유)
                R110_showEmotion(E_LOOK_UP_LEFT, eyeColor, pupilColor);    delay(g_R110_LOOK_SPEED_MS);
                R110_showEmotion(E_LOOK_UP, eyeColor, pupilColor);         delay(g_R110_LOOK_HOLD_MS);      // Hold Up
                R110_showEmotion(E_LOOK_UP_RIGHT, eyeColor, pupilColor);   delay(g_R110_LOOK_SPEED_MS);
                break;
            case E_LOOK_DOWN: // 아래 보기 (아래-왼쪽, 아래-오른쪽 경유)
                R110_showEmotion(E_LOOK_DOWN_LEFT, eyeColor, pupilColor);  delay(g_R110_LOOK_SPEED_MS);
                R110_showEmotion(E_LOOK_DOWN, eyeColor, pupilColor);       delay(g_R110_LOOK_HOLD_MS);      // Hold Down
                R110_showEmotion(E_LOOK_DOWN_RIGHT, eyeColor, pupilColor); delay(g_R110_LOOK_SPEED_MS);
                break;
            case E_THINKING: // 생각하기 (빠르게 여러 방향을 훑어본 후 특정 위치로 - 지정된 색상 사용)
                 R110_showEmotion(E_LOOK_UP_LEFT, eyeColor, pupilColor);    delay(g_R110_LOOK_SPEED_MS / 2);
                 R110_showEmotion(E_LOOK_DOWN_RIGHT, eyeColor, pupilColor); delay(g_R110_LOOK_SPEED_MS / 2);
                 R110_showEmotion(E_LOOK_UP_RIGHT, eyeColor, pupilColor);   delay(g_R110_LOOK_SPEED_MS / 2);
                 R110_showEmotion(E_LOOK_DOWN_LEFT, eyeColor, pupilColor);  delay(g_R110_LOOK_SPEED_MS / 2);
                 R110_showEmotion(E_LOOK_UP, eyeColor, pupilColor);         delay(g_R110_LOOK_SPEED_MS / 2);
                 R110_showEmotion(E_LOOK_RIGHT, eyeColor, pupilColor);      delay(g_R110_LOOK_SPEED_MS / 2); // Dart Right (extreme)
                 R110_showEmotion(E_THINKING, eyeColor, pupilColor);       delay(g_R110_LOOK_HOLD_MS * 1.5);      // Hold Thinking (longer)
                 R110_showEmotion(E_LOOK_RIGHT, eyeColor, pupilColor);      delay(g_R110_LOOK_SPEED_MS / 2); // Dart back Right
                 R110_showEmotion(E_LOOK_UP, eyeColor, pupilColor);         delay(g_R110_LOOK_SPEED_MS / 2); // Dart back Up
                 break;
            default:
                // Should not happen if is_valid_target_look is true
                break;
        }

       R110_showEmotion(E_NEUTRAL); // Return to Neutral (기본 색상)
       delay(random(100, 300)); // Short random delay after returning
	} else {
        // Serial.printf("Warning: R110_animateLook called with invalid target index: %d\n", p_direction_emotion_index); // Debug
    }
}

// -----------------------------------------------------------------------
// 애니메이션 함수: 윙크
// 기능: 지정된 방향으로 윙크 패턴을 표시하고 잠시 유지한 후 중립으로 돌아옵니다.
// p_side: 0 for Left Wink, 1 for Right Wink
// -----------------------------------------------------------------------
void R110_animateWink(int p_side) {
	int wink_emotion_index = (p_side == 0) ? E_LEFT_WINK : E_RIGHT_WINK;

	R110_showEmotion(E_NEUTRAL); // Start from Neutral (기본 색상)
	delay(random(100, 300)); // Short random delay before starting

	R110_showEmotion(wink_emotion_index); // Show Wink pattern (전체 패턴, 기본 색상 사용)
	delay(g_R110_WINK_HOLD_MS);			  // Hold Wink

	R110_showEmotion(E_NEUTRAL); // Return to Neutral (기본 색상)
	delay(random(100, 300)); // Short random delay after returning
}

// -----------------------------------------------------------------------
// 애니메이션 함수: 혼돈스런 눈
// 기능: 중립에서 혼돈스러운 눈 패턴으로 이동하고 잠시 유지한 후 중립으로 돌아옵니다.
// -----------------------------------------------------------------------
void R110_animateConfused() {
	R110_showEmotion(E_NEUTRAL); // Start from Neutral (기본 색상)
	delay(random(100, 300)); // Short random delay before starting

    // Add darting animation using extreme diagonal and cardinal positions with rapid color changes
    CRGB dartColor1 = R110_COLOR_YELLOW;
    CRGB dartColor2 = R110_COLOR_BLUE;
    CRGB currentPupilColor = dartColor1; // 눈동자 색상만 빠르게 변경

    R110_showEmotion(E_LOOK_UP_LEFT, R110_COLOR_WHITE, currentPupilColor);    delay(g_R110_CONFUSED_DART_SPEED_MS); currentPupilColor = (currentPupilColor == dartColor1) ? dartColor2 : dartColor1;
    R110_showEmotion(E_LOOK_DOWN_RIGHT, R110_COLOR_WHITE, currentPupilColor); delay(g_R110_CONFUSED_DART_SPEED_MS); currentPupilColor = (currentPupilColor == dartColor1) ? dartColor2 : dartColor1;
    R110_showEmotion(E_LOOK_UP_RIGHT, R110_COLOR_WHITE, currentPupilColor);   delay(g_R110_CONFUSED_DART_SPEED_MS); currentPupilColor = (currentPupilColor == dartColor1) ? dartColor2 : dartColor1;
    R110_showEmotion(E_LOOK_DOWN_LEFT, R110_COLOR_WHITE, currentPupilColor);  delay(g_R110_CONFUSED_DART_SPEED_MS); currentPupilColor = (currentPupilColor == dartColor1) ? dartColor2 : dartColor1;
    R110_showEmotion(E_LOOK_UP, R110_COLOR_WHITE, currentPupilColor);         delay(g_R110_CONFUSED_DART_SPEED_MS); currentPupilColor = (currentPupilColor == dartColor1) ? dartColor2 : dartColor1;
    R110_showEmotion(E_LOOK_DOWN, R110_COLOR_WHITE, currentPupilColor);       delay(g_R110_CONFUSED_DART_SPEED_MS); currentPupilColor = (currentPupilColor == dartColor1) ? dartColor2 : dartColor1;
    R110_showEmotion(E_LOOK_LEFT, R110_COLOR_WHITE, currentPupilColor);       delay(g_R110_CONFUSED_DART_SPEED_MS); currentPupilColor = (currentPupilColor == dartColor1) ? dartColor2 : dartColor1;
    R110_showEmotion(E_LOOK_RIGHT, R110_COLOR_WHITE, currentPupilColor);      delay(g_R110_CONFUSED_DART_SPEED_MS); // 마지막은 토글 없이

    R110_showEmotion(E_NEUTRAL); // Briefly back to neutral (기본 색상)
    delay(g_R110_LOOK_SPEED_MS); // Pause briefly

	R110_showEmotion(E_CONFUSED, R110_COLOR_ORANGE, R110_COLOR_BLUE); // Show Confused pattern with specific colors (주황색 외곽선, 파란 눈동자)
	delay(g_R110_CONFUSED_SPEED_MS);  // Hold Confused

	R110_showEmotion(E_NEUTRAL); // Return to Neutral (기본 색상)
	delay(random(100, 300)); // Short random delay after returning
}


// -----------------------------------------------------------------------
// 초기화 함수
// 기능: FastLED 초기화, 밝기 설정, 디스플레이 클리어 및 테스트.
// 초기화 완료 후 기본 상태인 중립 눈을 표시합니다.
// 이 함수는 main sketch의 setup() 함수에서 호출되어야 합니다.
// -----------------------------------------------------------------------
void R110_init() {
	// FastLED 라이브러리 초기화 (ESP32용)
	FastLED.addLeds<g_R110_LED_TYPE, g_R110_DATA_PIN, g_R110_COLOR_ORDER>(g_R110_leds, g_R110_NUM_LEDS);

	// 전체 밝기 설정 (0-255, 선택 사항)
	FastLED.setBrightness(70);

	// 시리얼 통신 초기화 (디버깅용)
	Serial.println("R110 RobotEyes Library Init");

	// 디스플레이 클리어 (모든 LED 끄기)
	fill_solid(g_R110_leds, g_R110_NUM_LEDS, CRGB::Black);
	FastLED.show();
	delay(500);

	// 초기화 테스트: 모든 LED 켜기
	Serial.println("Testing all LEDs ON...");
	fill_solid(g_R110_leds, g_R110_NUM_LEDS, CRGB::Red);
	FastLED.show();
	delay(1000);

	// 테스트 끝: 다시 모든 LED 끄기
	Serial.println("Test complete. Clearing display.");
	fill_solid(g_R110_leds, g_R110_NUM_LEDS, CRGB::Black);
	FastLED.show();
	delay(500);

	// 초기화 완료 후 기본 상태인 중립 눈을 표시 (기본 색상)
	Serial.println("Setting initial state to Neutral (large outline).");
	R110_showEmotion(E_NEUTRAL); // 기본 색상 (White/Black)으로 표시
}

// -----------------------------------------------------------------------
// 기본 애니메이션 루프
// 기능: 다양한 애니메이션 패턴을 순차적으로 보여줌
// 이 함수는 main sketch의 loop() 함수에서 주기적으로 호출될 수 있습니다.
// 또는 loop() 함수에서 원하는 애니메이션 함수들을 직접 호출해도 됩니다.
// -----------------------------------------------------------------------
void R110_run() {
	Serial.println("Starting RobotEyes Animation Sequence...");

	// R110_init()에서 이미 Neutral 표시됨. 바로 애니메이션 시작

	Serial.println("Animating Blink...");
	R110_animateBlink(1); // 깜빡임 (기본 색상 사용)

	Serial.println("Animating Look Left...");
	R110_animateLook(E_LOOK_LEFT); // 보기 애니메이션 (내부에서 색상 지정 - 기본 색상)

	Serial.println("Animating Look Right...");
	R110_animateLook(E_LOOK_RIGHT); // 보기 애니메이션 (내부에서 색상 지정 - 기본 색상)

	Serial.println("Animating Look Up...");
	R110_animateLook(E_LOOK_UP); // 보기 애니메이션 (내부에서 색상 지정 - 기본 색상)

	Serial.println("Animating Look Down...");
	R110_animateLook(E_LOOK_DOWN); // 보기 애니메이션 (내부에서 색상 지정 - 기본 색상)

	Serial.println("Animating Wink Left...");
	R110_animateWink(0); // 윙크 (기본 색상 사용)

	Serial.println("Animating Wink Right...");
	R110_animateWink(1); // 윙크 (기본 색상 사용)

	Serial.println("Showing Squint...");
	R110_showEmotion(E_SQUINT, R110_COLOR_DIM_WHITE); // 찡그림 (어두운 흰색)
	delay(1500);
	Serial.println("Showing Squint Tight...");
	R110_showEmotion(E_SQUINT_TIGHT, R110_COLOR_GRAY); // 더 찡그림 (회색)
	delay(1500);
	R110_showEmotion(E_NEUTRAL); // 기본 색상으로 복귀
	delay(1000);

	Serial.println("Showing Sleepy...");
	R110_showEmotion(E_SLEEPY, R110_COLOR_DIM_YELLOW); // 졸린 눈 (어두운 노란색)
	delay(2000);
	R110_showEmotion(E_NEUTRAL); // 기본 색상으로 복귀
	delay(1000);

	Serial.println("Showing Angry...");
	R110_showEmotion(E_ANGRY, R110_COLOR_RED); // 화난 눈 (빨간색)
	delay(2000);
	R110_showEmotion(E_NEUTRAL); // 기본 색상으로 복귀
	delay(1000);

	Serial.println("Showing Absurd...");
	R110_showEmotion(E_ABSURD, R110_COLOR_YELLOW); // 황당한 눈 (노란색)
	delay(2000);
	R110_showEmotion(E_NEUTRAL); // 기본 색상으로 복귀
	delay(1000);

	Serial.println("Showing Glaring...");
	R110_showEmotion(E_GLARING, R110_COLOR_PURPLE); // 째려보는 눈 (보라색)
	delay(2000);
	R110_showEmotion(E_NEUTRAL); // 기본 색상으로 복귀
	delay(1000);

	Serial.println("Showing Thinking...");
	R110_animateLook(E_THINKING); // 생각하기 애니메이션 (내부에서 파란 눈, 노란 눈동자 지정)

	Serial.println("Animating Confused...");
	R110_animateConfused(); // 혼돈 애니메이션 (내부에서 눈동자 색상 빠르게 변경, 주황 눈, 파란 눈동자 지정)

	Serial.println("Animation sequence finished.");
}

/*
 * R110_RobotEyes_T20_003.h - Expected Matrix Output for R110_run() Animation Sequence with Colors
 *
 * WARNING: This comment block contains a detailed trace of the animation sequence
 * and is very large. It is provided for reference of the visual output.
 * For better code readability, you may prefer to refer to the separate text output.
 *
 * Color Key: # = ON pixel (eye or pupil color), . = OFF pixel (Black)
 * Note: Color names (e.g., [White]) indicate the color applied to the ON pixels in the grid.
 *
 * -- Animation Sequence Trace (based on R110_run() function) --
 *
 * Note: All "Outline + Pupil {r,c}" patterns use the BaseEyeOutline below.
 * BaseEyeOutline (always drawn with eyeColor):
 * ........
 * .######.
 * #......#
 * #......#
 * #......#
 * #......#
 * .######.
 * ........
 *
 * Initial State (after R110_init() completes):
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid:
 * ........
 * .######.
 * #......#
 * #..##..#
 * #..##..#
 * #......#
 * .######.
 * ........
 * (Ready state before R110_run starts)
 *
 * -- R110_run() Sequence Starts --
 *
 * 1. R110_animateBlink(1) Sequence:
 * (Short random delay before starting)
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (random(500, 2000)ms delay)
 *
 * -- Step 2 --
 * Emotion: BlinkOpen (Full Pattern Override)
 * Colors: Eye [White]
 * Grid:
 * ........
 * ........
 * ..####..
 * .#....#.
 * .#....#.
 * ..####..
 * ........
 * ........
 * (g_R110_BLINK_SPEED_MS ms delay)
 *
 * -- Step 3 --
 * Emotion: BlinkMid (Full Pattern Override)
 * Colors: Eye [White]
 * Grid:
 * ........
 * ........
 * ........
 * ..####..
 * ..####..
 * ........
 * ........
 * ........
 * (g_R110_BLINK_SPEED_MS ms delay)
 *
 * -- Step 4 --
 * Emotion: BlinkClose (Full Pattern Override)
 * Colors: Eye [White]
 * Grid:
 * ........
 * ........
 * ........
 * ........
 * ........
 * ........
 * ........
 * ........
 * (g_R110_BLINK_SPEED_MS * 2 ms delay)
 *
 * -- Step 5 --
 * Emotion: BlinkMid (Full Pattern Override)
 * Colors: Eye [White]
 * Grid: (Same as Step 3)
 * (g_R110_BLINK_SPEED_MS ms delay)
 *
 * -- Step 6 --
 * Emotion: BlinkOpen (Full Pattern Override)
 * Colors: Eye [White]
 * Grid: (Same as Step 2)
 * (g_R110_BLINK_SPEED_MS ms delay)
 *
 * -- Step 7 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (Short random delay after completing)
 *
 * 2. R110_animateLook(E_LOOK_LEFT) Sequence:
 * (Short random delay before starting)
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (random(100, 300)ms delay)
 *
 * -- Step 2 --
 * Emotion: LookUpLeft (Outline + Pupil {2,1})
 * Colors: Eye [White], Pupil [Black]
 * Grid:
 * ........
 * .######.
 * #.##....
 * #.##....
 * #......#
 * #......#
 * .######.
 * ........
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 3 --
 * Emotion: LookLeft (Outline + Pupil {3,1}) - Final Position
 * Colors: Eye [White], Pupil [Black]
 * Grid:
 * ........
 * .######.
 * #......#
 * #.##....
 * #.##....
 * #......#
 * .######.
 * ........
 * (g_R110_LOOK_HOLD_MS ms delay - Hold)
 *
 * -- Step 4 --
 * Emotion: LookDownLeft (Outline + Pupil {5,1})
 * Colors: Eye [White], Pupil [Black]
 * Grid:
 * ........
 * .######.
 * #......#
 * #......#
 * #.##....
 * #.##....
 * .######.
 * ........
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 5 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (Short random delay after completing)
 *
 * 3. R110_animateLook(E_LOOK_RIGHT) Sequence:
 * (Short random delay before starting)
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (random(100, 300)ms delay)
 *
 * -- Step 2 --
 * Emotion: LookUpRight (Outline + Pupil {2,6})
 * Colors: Eye [White], Pupil [Black]
 * Grid:
 * ........
 * .######.
 * ......##
 * ......##
 * #......#
 * #......#
 * .######.
 * ........
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 3 --
 * Emotion: LookRight (Outline + Pupil {3,6}) - Final Position
 * Colors: Eye [White], Pupil [Black]
 * Grid:
 * ........
 * .######.
 * #......#
 * ......##
 * ......##
 * #......#
 * .######.
 * ........
 * (g_R110_LOOK_HOLD_MS ms delay - Hold)
 *
 * -- Step 4 --
 * Emotion: LookDownRight (Outline + Pupil {5,6})
 * Colors: Eye [White], Pupil [Black]
 * Grid:
 * ........
 * .######.
 * #......#
 * #......#
 * ......##
 * ......##
 * .#######
 * ........
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 5 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (Short random delay after completing)
 *
 * 4. R110_animateLook(E_LOOK_UP) Sequence:
 * (Short random delay before starting)
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (random(100, 300)ms delay)
 *
 * -- Step 2 --
 * Emotion: LookUpLeft (Outline + Pupil {2,1})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as grid in LookLeft sequence)
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 3 --
 * Emotion: LookUp (Outline + Pupil {2,3}) - Final Position
 * Colors: Eye [White], Pupil [Black]
 * Grid:
 * ........
 * .######.
 * #..##..#
 * #..##..#
 * #......#
 * #......#
 * .######.
 * ........
 * (g_R110_LOOK_HOLD_MS ms delay - Hold)
 *
 * -- Step 4 --
 * Emotion: LookUpRight (Outline + Pupil {2,6})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as grid in LookRight sequence)
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 5 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (Short random delay after completing)
 *
 * 5. R110_animateLook(E_LOOK_DOWN) Sequence:
 * (Short random delay before starting)
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (random(100, 300)ms delay)
 *
 * -- Step 2 --
 * Emotion: LookDownLeft (Outline + Pupil {5,1})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as grid in LookLeft sequence)
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 3 --
 * Emotion: LookDown (Outline + Pupil {5,3}) - Final Position
 * Colors: Eye [White], Pupil [Black]
 * Grid:
 * ........
 * .######.
 * #......#
 * #......#
 * #..##..#
 * #..##..#
 * .######.
 * ........
 * (g_R110_LOOK_HOLD_MS ms delay - Hold)
 *
 * -- Step 4 --
 * Emotion: LookDownRight (Outline + Pupil {5,6})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as grid in LookRight sequence)
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 5 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (Short random delay after completing)
 *
 * 6. R110_animateWink(0) [Left Wink] Sequence:
 * (Short random delay before starting)
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (random(100, 300)ms delay)
 *
 * -- Step 2 --
 * Emotion: Left Wink (Full Pattern Override - Asymmetric)
 * Colors: Eye [White] (Pupil is off/part of pattern, uses eye color if ON)
 * Left Eye Grid: BlinkClose (all off)
 * ........
 * ........
 * ........
 * ........
 * ........
 * ........
 * ........
 * ........
 * Right Eye Grid: Original Neutral_eye pattern
 * ........
 * ..####..
 * .#....#.
 * .#.##.#.
 * .#.##.#.
 * .#....#.
 * ..####..
 * ........
 * (g_R110_WINK_HOLD_MS ms delay - Hold)
 *
 * -- Step 3 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (Short random delay after completing)
 *
 * 7. R110_animateWink(1) [Right Wink] Sequence:
 * (Short random delay before starting)
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (random(100, 300)ms delay)
 *
 * -- Step 2 --
 * Emotion: Right Wink (Full Pattern Override - Asymmetric)
 * Colors: Eye [White]
 * Left Eye Grid: Original Neutral_eye pattern
 * ........
 * ..####..
 * .#....#.
 * .#.##.#.
 * .#.##.#.
 * .#....#.
 * ..####..
 * ........
 * Right Eye Grid: BlinkClose (all off)
 * ........
 * ........
 * ........
 * ........
 * ........
 * ........
 * ........
 * ........
 * (g_R110_WINK_HOLD_MS ms delay - Hold)
 *
 * -- Step 3 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (Short random delay after completing)
 *
 * 8. R110_showEmotion(E_SQUINT) Sequence:
 * -- Step 1 --
 * Emotion: Squint (Full Pattern Override)
 * Colors: Eye [Dim White]
 * Grid:
 * ........
 * ........
 * .######.
 * .######.
 * .######.
 * .######.
 * ........
 * ........
 * (1500ms delay from R110_run)
 *
 * 9. R110_showEmotion(E_SQUINT_TIGHT) Sequence:
 * -- Step 1 --
 * Emotion: SquintTight (Full Pattern Override)
 * Colors: Eye [Gray]
 * Grid:
 * ........
 * ........
 * ........
 * .######.
 * .######.
 * ........
 * ........
 * ........
 * (1500ms delay from R110_run)
 *
 * 10. R110_showEmotion(E_NEUTRAL) Sequence:
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (1000ms delay from R110_run)
 *
 * 11. R110_showEmotion(E_SLEEPY) Sequence:
 * -- Step 1 --
 * Emotion: Sleepy (Full Pattern Override)
 * Colors: Eye [Dim Yellow]
 * Grid:
 * ........
 * ........
 * ..####..
 * .#....#.
 * .#....#.
 * ..####..
 * ........
 * ........
 * (2000ms delay from R110_run)
 *
 * 12. R110_showEmotion(E_NEUTRAL) Sequence:
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (1000ms delay from R110_run)
 *
 * 13. R110_showEmotion(E_ANGRY) Sequence:
 * -- Step 1 --
 * Emotion: Angry (Full Pattern Override)
 * Colors: Eye [Red]
 * Grid:
 * ........
 * ........
 * .######.
 * .#.##.#.
 * .#.##.#.
 * .######.
 * ........
 * ........
 * (2000ms delay from R110_run)
 *
 * 14. R110_showEmotion(E_NEUTRAL) Sequence:
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (1000ms delay from R110_run)
 *
 * 15. R110_showEmotion(E_ABSURD) Sequence:
 * -- Step 1 --
 * Emotion: Absurd (Full Pattern Override)
 * Colors: Eye [Yellow]
 * Grid:
 * ........
 * .######.
 * #......#
 * #.####.#
 * #.####.#
 * #......#
 * .######.
 * ........
 * (2000ms delay from R110_run)
 *
 * 16. R110_showEmotion(E_NEUTRAL) Sequence:
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (1000ms delay from R110_run)
 *
 * 17. R110_showEmotion(E_GLARING) Sequence:
 * -- Step 1 --
 * Emotion: Glaring (Full Pattern Override)
 * Colors: Eye [Purple]
 * Grid:
 * ........
 * ..####..
 * .#....#.
 * .#....#.
 * .#....#.
 * #....#.#
 * ..####..
 * ........
 * (2000ms delay from R110_run)
 *
 * 18. R110_showEmotion(E_NEUTRAL) Sequence:
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (1000ms delay from R110_run)
 *
 * 19. R110_animateLook(E_THINKING) Sequence:
 * (Short random delay before starting)
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (random(100, 300)ms delay)
 *
 * -- Step 2 --
 * Emotion: LookUpLeft (Outline + Pupil {2,1})
 * Colors: Eye [Blue], Pupil [Yellow]
 * Grid:
 * ........
 * .######.
 * #.##....
 * #.##....
 * #......#
 * #......#
 * .######.
 * ........
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 3 --
 * Emotion: LookDownRight (Outline + Pupil {5,6})
 * Colors: Eye [Blue], Pupil [Yellow]
 * Grid:
 * ........
 * .######.
 * #......#
 * #......#
 * ......##
 * ......##
 * .#######
 * ........
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 4 --
 * Emotion: LookUpRight (Outline + Pupil {2,6})
 * Colors: Eye [Blue], Pupil [Yellow]
 * Grid:
 * ........
 * .######.
 * ......##
 * ......##
 * #......#
 * #......#
 * .######.
 * ........
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 5 --
 * Emotion: LookDownLeft (Outline + Pupil {5,1})
 * Colors: Eye [Blue], Pupil [Yellow]
 * Grid:
 * ........
 * .######.
 * #......#
 * #......#
 * #.##....
 * #.##....
 * .######.
 * ........
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 6 --
 * Emotion: LookUp (Outline + Pupil {2,3})
 * Colors: Eye [Blue], Pupil [Yellow]
 * Grid:
 * ........
 * .######.
 * #..##..#
 * #..##..#
 * #......#
 * #......#
 * .######.
 * ........
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 7 --
 * Emotion: LookRight (Outline + Pupil {3,6})
 * Colors: Eye [Blue], Pupil [Yellow]
 * Grid:
 * ........
 * .######.
 * #......#
 * ......##
 * ......##
 * #......#
 * .######.
 * ........
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 8 --
 * Emotion: Thinking (Outline + Pupil {2,6}) - Final Position
 * Colors: Eye [Blue], Pupil [Yellow]
 * Grid: (Same as grid in Step 4)
 * (g_R110_LOOK_HOLD_MS * 1.5 ms delay - Hold)
 *
 * -- Step 9 --
 * Emotion: LookRight (Outline + Pupil {3,6})
 * Colors: Eye [Blue], Pupil [Yellow]
 * Grid: (Same as Step 7)
 * (g_R110_LOOK_SPEED_MS / 2 ms delay)
 *
 * -- Step 10 --
 * Emotion: LookUp (Outline + Pupil {2,3})
 * Colors: Eye [Blue], Pupil [Yellow]
 * Grid: (Same as Step 6)
 * (g_R110_LOOK_SPEED_MS / 2 ms delay)
 *
 * -- Step 11 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (Short random delay after completing)
 *
 * 20. R110_animateConfused() Sequence:
 * (Short random delay before starting)
 * -- Step 1 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (random(100, 300)ms delay)
 *
 * -- Step 2 -- (Darting Sequence with rapidly changing Pupil Color)
 * Emotion: LookUpLeft (Outline + Pupil {2,1})
 * Colors: Eye [White], Pupil [Yellow]
 * Grid: (Same as grid in LookLeft sequence)
 * (g_R110_CONFUSED_DART_SPEED_MS ms delay)
 *
 * -- Step 3 --
 * Emotion: LookDownRight (Outline + Pupil {5,6})
 * Colors: Eye [White], Pupil [Blue]
 * Grid: (Same as grid in LookRight sequence)
 * (g_R110_CONFUSED_DART_SPEED_MS ms delay)
 *
 * -- Step 4 --
 * Emotion: LookUpRight (Outline + Pupil {2,6})
 * Colors: Eye [White], Pupil [Yellow]
 * Grid: (Same as grid in LookRight sequence)
 * (g_R110_CONFUSED_DART_SPEED_MS ms delay)
 *
 * -- Step 5 --
 * Emotion: LookDownLeft (Outline + Pupil {5,1})
 * Colors: Eye [White], Pupil [Blue]
 * Grid: (Same as grid in LookLeft sequence)
 * (g_R110_CONFUSED_DART_SPEED_MS ms delay)
 *
 * -- Step 6 --
 * Emotion: LookUp (Outline + Pupil {2,3})
 * Colors: Eye [White], Pupil [Yellow]
 * Grid: (Same as grid in LookUp sequence)
 * (g_R110_CONFUSED_DART_SPEED_MS ms delay)
 *
 * -- Step 7 --
 * Emotion: LookDown (Outline + Pupil {5,3})
 * Colors: Eye [White], Pupil [Blue]
 * Grid: (Same as grid in LookDown sequence)
 * (g_R110_CONFUSED_DART_SPEED_MS ms delay)
 *
 * -- Step 8 --
 * Emotion: LookLeft (Outline + Pupil {3,1})
 * Colors: Eye [White], Pupil [Yellow]
 * Grid: (Same as grid in LookLeft sequence)
 * (g_R110_CONFUSED_DART_SPEED_MS ms delay)
 *
 * -- Step 9 --
 * Emotion: LookRight (Outline + Pupil {3,6})
 * Colors: Eye [White], Pupil [Blue]
 * Grid: (Same as grid in LookRight sequence)
 * (g_R110_CONFUSED_DART_SPEED_MS ms delay)
 *
 * -- Step 10 -- (Brief pause)
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (g_R110_LOOK_SPEED_MS ms delay)
 *
 * -- Step 11 -- (Confused State)
 * Emotion: Confused (Outline + Asymmetric Pupils)
 * Colors: Eye [Orange], Pupil [Blue]
 * Left Eye Grid (Outline + Pupil {3,6}):
 * ........
 * .######.
 * #......#
 * ......##
 * ......##
 * #......#
 * .######.
 * ........
 * Right Eye Grid (Outline + Pupil {3,1}):
 * ........
 * .######.
 * #......#
 * #.##....
 * #.##....
 * #......#
 * .######.
 * ........
 * (g_R110_CONFUSED_SPEED_MS ms delay - Hold)
 *
 * -- Step 12 --
 * Emotion: Neutral (Outline + Pupil {3,3})
 * Colors: Eye [White], Pupil [Black]
 * Grid: (Same as Initial State)
 * (Short random delay after completing)
 *
 * -- End of R110_run() Sequence --
 *
 * Note: The actual timing on the hardware may vary slightly based on FastLED.show() duration
 * and processing time.
 */

// Note: setup() and loop() functions are expected in the main .ino or .cpp file
// as shown in the example usage.
