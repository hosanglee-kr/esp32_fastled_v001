// R210_RobotEyes_T20_003.h

#pragma once // 헤더 파일 중복 포함 방지

// ======================================================================
// FastLED + WS2812B 로봇 눈 애니메이션 라이브러리 (ESP32 Arduino Framework용)
// MD_MAX72XX RobotEyes 예제 기반, FastLED 이식, 명명 규칙 접두사 R210 적용
//
// 기능:
// - 8x8 WS2812B 매트릭스 1개 또는 2개를 로봇 눈으로 제어
// - 다양한 눈 표정 패턴 정의 및 표시
// - 눈 깜빡임, 둘러보기 등 기본 애니메이션 제공
// - 일부 표정(중립, 보기, 생각, 혼돈)은 큰 외곽선을 유지하며 눈동자만 움직이도록 구현
// - 그 외 표정(깜빡임, 찡그림, 감정 표현 등)은 기존 정의된 패턴 그대로 표시
// - 애니메이션 함수에 대각선 눈동자 위치를 활용한 중간 단계 추가 및 타이밍 개선
// - 눈동자 이동 범위 최대한 넓게 조정
//
// 하드웨어 설정:
// - WS2812B LED 데이터 핀 (g_R210_DATA_PIN) 연결
// - 사용하는 8x8 매트릭스 개수 설정 (g_R210_NUM_MATRICES: 1 또는 2)
// - 매트릭스 내부 및 매트릭스 간의 LED 배선 방식에 따라 R210_getLedIndex 함수 수정 필요
//
// 사용법:
// 1. 이 파일을 ESP32 Arduino 프로젝트의 src 폴더 등에 포함합니다.
// 2. 메인 .ino 또는 .cpp 파일에서 #include "R210_RobotEyes_T20_003.h" 를 추가합니다.
// 3. setup() 함수에서 R210_init() 함수를 호출하여 라이브러리를 초기화합니다.
// 4. loop() 함수에서 R210_run() 함수를 호출하여 내장 애니메이션 시퀀스를 실행하거나,
//    R210_showEmotion(), R210_animateBlink() 등 개별 함수들을 호출하여 원하는 눈 동작을 구현합니다.
// ======================================================================

#include <Arduino.h>  // Arduino 핵심 라이브러리 (ESP32 지원)
#include <FastLED.h>  // FastLED 라이브러리 (WS2812B 제어)

// -----------------------------------------------------------------------
// 하드웨어 설정 상수
// -----------------------------------------------------------------------

#if defined(CONFIG_IDF_TARGET_ESP32C3)
    // ESP32-C3 보드의 경우 FastLED RMT에 적합한 핀 (예: GPIO2, 8, 10)
    // 일반적으로 GPIO2가 흔히 사용되며, RMT 채널 사용에 제약이 적습니다.
    #define g_R210_DATA_PIN             10  //2      // ESP32-C3 데이터 핀 번호 (GPIO2)
    #warning "Compiling for ESP32-C3, using DATA_PIN 2, 8, 10 for FastLED RMT" // 컴파일 시 확인 메시지
#elif defined(CONFIG_IDF_TARGET_ESP32)
    // 일반 ESP32 보드 (ESP32-WROOM, ESP32-DevKitC 등)의 경우
    // FastLED RMT에 적합한 핀 (예: GPIO13, 14, 27 등)
    // GPIO13은 흔히 사용되며, RMT 채널 사용에 제약이 적은 편입니다.
    #define g_R210_DATA_PIN             13     // 일반 ESP32 데이터 핀 번호 (GPIO13)
    #warning "Compiling for standard ESP32, using DATA_PIN 13 for FastLED RMT" // 컴파일 시 확인 메시지
#else
    // 다른 ESP32 칩 (ESP32-S2, ESP32-S3, ESP32-H2 등) 의 경우
    // 필요 시 여기에 #elif ... 를 추가하여 칩별 설정을 합니다.
    // 현재는 기본값으로 설정하며, 해당 핀이 FastLED RMT와 호환되는지 확인 필요
    #define g_R210_DATA_PIN             13     // 기본 데이터 핀 번호 (확인 필요)
    #warning "Unknown ESP32 target, using default DATA_PIN 13. Please check if this pin is RMT capable and supported by FastLED for your specific chip."
#endif

// #ifdef 
//     #define g_R210_DATA_PIN			   GPIO10		// WS2812B 데이터 핀 번호
// #else    
//     #define g_R210_DATA_PIN			   13		// WS2812B 데이터 핀 번호
// #endif 

#define g_R210_LED_TYPE			   WS2812B	// 사용하는 LED 칩 타입
#define g_R210_COLOR_ORDER		   GRB		// LED 색상 순서 (모듈에 맞게 변경)

#define g_R210_MATRIX_SIZE			8												// 8x8 매트릭스 크기
#define g_R210_NUM_MATRICES			2												// 사용하는 8x8 매트릭스 개수 (1 또는 2)
#define g_R210_NUM_LEDS_PER_MATRIX (g_R210_MATRIX_SIZE * g_R210_MATRIX_SIZE)		// 매트릭스 하나당 LED 개수
#define g_R210_NUM_LEDS				(g_R210_NUM_LEDS_PER_MATRIX * g_R210_NUM_MATRICES)	// WS2812B 총 LED 개수

// FastLED LED 배열 - 모든 WS2812B LED의 색상 값을 저장
CRGB g_R210_leds[g_R210_NUM_LEDS];

// -----------------------------------------------------------------------
// 2차원 (행, 열) 좌표를 1차원 LED 배열 인덱스로 변환
// p_matrix_addr: 매트릭스 주소 (0부터 g_R210_NUM_MATRICES-1 까지)
// p_row: 행 번호 (0부터 g_R210_MATRIX_SIZE-1 까지)
// p_col: 열 번호 (0부터 g_R210_MATRIX_SIZE-1 까지)
// 반환값: 전체 g_R210_leds 배열에서의 해당 픽셀 인덱스, 범위 벗어날 시 -1
// -----------------------------------------------------------------------
int	 R210_getLedIndex(int p_matrix_addr, int p_row, int p_col) {
	 if (p_matrix_addr < 0 || p_matrix_addr >= g_R210_NUM_MATRICES || p_row < 0 || p_row >= g_R210_MATRIX_SIZE || p_col < 0 || p_col >= g_R210_MATRIX_SIZE) {
		 return -1; // 범위를 벗어나는 좌표
	 }

	 int v_base_index = p_matrix_addr * g_R210_NUM_LEDS_PER_MATRIX;	 // 해당 매트릭스의 시작 인덱스
	 int v_local_index;												 // 8x8 매트릭스 내에서의 상대 인덱스

	 // 지그재그(serpentine) 방식 가정 - 실제 배선에 따라 아래 로직을 수정하세요!
	 if (p_row % 2 == 0) {	// 짝수 행 (0, 2, 4, 6)은 왼쪽 -> 오른쪽으로 인덱스 증가
		 v_local_index = p_row * g_R210_MATRIX_SIZE + p_col;
	 } else {  // 홀수 행 (1, 3, 5, 7)은 오른쪽 -> 왼쪽으로 인덱스 증가
		 v_local_index = p_row * g_R210_MATRIX_SIZE + (g_R210_MATRIX_SIZE - 1 - p_col);
	 }

	 return v_base_index + v_local_index;  // 전체 LED 배열에서의 최종 인덱스
}


// -----------------------------------------------------------------------
// 눈 모양 픽셀 데이터 (전체 패턴 오버라이드용)
// 외곽선+눈동자 방식이 아닌, 패턴 그대로 그려질 때 사용됩니다.
// 각 패턴 옆 주석은 8x8 매트릭스 모양 ( #=on, .=off ) 입니다.
// -----------------------------------------------------------------------
const byte Neutral_eye[g_R210_MATRIX_SIZE] = { // 윙크에서 사용되는 작은 중립 눈
	B00000000, // ........
	B00111100, // ..####..
	B01000010, // .#....#.
	B01011010, // .#.##.#.
	B01011010, // .#.##.#.
	B01000010, // .#....#.
	B00111100, // ..####..
	B00000000  // ........
};
const byte BlinkOpen_eye[g_R210_MATRIX_SIZE] = { // 깜빡임 (중간)
	B00000000, // ........
	B00000000, // ........
	B00111100, // ..####..
	B01000010, // .#....#.
	B01000010, // .#....#.
	B00111100, // ..####..
	B00000000, // ........
	B00000000  // ........
};
const byte BlinkMid_eye[g_R210_MATRIX_SIZE] = { // 깜빡임 (더 감김)
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00111100, // ..####..
	B00111100, // ..####..
	B00000000, // ........
	B00000000, // ........
	B00000000  // ........
};
const byte BlinkClose_eye[g_R210_MATRIX_SIZE] = { // 깜빡임 (완전히 감김)
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B00000000  // ........
};
const byte Squint_eye[g_R210_MATRIX_SIZE] = { // 찡그림
	B00000000, // ........
	B00000000, // ........
	B01111110, // .######.
	B01111110, // .######.
	B01111110, // .######.
	B01111110, // .######.
	B00000000, // ........
	B00000000  // ........
};
const byte SquintTight_eye[g_R210_MATRIX_SIZE] = { // 더 찡그림
	B00000000, // ........
	B00000000, // ........
	B00000000, // ........
	B01111110, // .######.
	B01111110, // .######.
	B00000000, // ........
	B00000000, // ........
	B00000000  // ........
};
const byte Sleepy_eye[g_R210_MATRIX_SIZE] = { // 졸린 눈
	B00000000, // ........
	B00000000, // ........
	B00111100, // ..####..
	B01000010, // .#....#.
	B01000010, // .#....#.
	B00111100, // ..####..
	B00000000, // ........
	B00000000  // ........
};
const byte Angry_eye[g_R210_MATRIX_SIZE] = { // 화난 눈
	B00000000, // ........
	B00000000, // ........
	B01111110, // .######.
	B01011010, // .#.##.#.
	B01011010, // .#.##.#.
	B01111110, // .######.
	B00000000, // ........
	B00000000  // ........
};
const byte Absurd_eye[g_R210_MATRIX_SIZE] = { // 황당한 눈 (큰 눈동자)
	B00000000, // ........
	B01111110, // .######.
	B10000001, // #......#
	B10111101, // #.####.#
	B10111101, // #.####.#
	B10000001, // #......#
	B01111110, // .######.
	B00000000  // ........
};
const byte Glaring_eye[g_R210_MATRIX_SIZE] = { // 째려보는 눈 (다른 외곽선)
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
const byte BaseEyeOutline[g_R210_MATRIX_SIZE] = {
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
PupilPos R210_getPupilPosForEmotion(int p_emotionIndex) {
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
const PupilPos g_R210_confusedPupilPos[2] = {
	{3, 6}, // 왼쪽 눈의 눈동자 위치 (오른쪽으로 치우침)
	{3, 1}  // 오른쪽 눈의 눈동자 위치 (왼쪽으로 치우침)
};

// -----------------------------------------------------------------------
// 감정 패턴 데이터 (전체 패턴 오버라이드용 데이터 포인터 배열)
// EmotionIndex와 직접 매핑되지 않으며, R210_displayEmotionState에서 특정 EmotionIndex일 때 사용됩니다.
// {왼쪽 눈 패턴 데이터 포인터, 오른쪽 눈 패턴 데이터 포인터} 쌍의 배열
// -----------------------------------------------------------------------
const byte *const g_R210_fullOverridePatternsData[]				   = {
	BlinkOpen_eye, BlinkOpen_eye,	   // For E_BLINK_OPEN (1)
	BlinkMid_eye, BlinkMid_eye,		   // For E_BLINK_MID (2)
	BlinkClose_eye, BlinkClose_eye,	   // For E_BLINK_CLOSE (3)
	Squint_eye, Squint_eye,			   // For E_SQUINT (8)
	SquintTight_eye, SquintTight_eye,  // For E_SQUINT_TIGHT (9)
	BlinkClose_eye, Neutral_eye,	   // For E_LEFT_WINK (10) - 왼감, 오뜸 (오른쪽 눈에 기존 Neutral 패턴 사용)
	Neutral_eye, BlinkClose_eye,	   // For E_RIGHT_WINK (11) - 왼뜸, 오감 (왼쪽 눈에 기존 Neutral 패턴 사용)
	Sleepy_eye, Sleepy_eye,			   // For E_SLEEPY (12)
	Angry_eye, Angry_eye,			   // For E_ANGRY (13)
	Absurd_eye, Absurd_eye,			   // For E_ABSURD (14)
	Glaring_eye, Glaring_eye,		   // For E_GLARING (15)
};

// -----------------------------------------------------------------------
// 감정 패턴 인덱스 정의
// -----------------------------------------------------------------------
enum R210_EmotionIndex {
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
const int g_R210_NUM_EMOTION_PATTERNS = 22; // 마지막 인덱스 + 1

// -----------------------------------------------------------------------
// 애니메이션 속도 조절 상수
// -----------------------------------------------------------------------
const int g_R210_BLINK_SPEED_MS = 400; //50;
const int g_R210_LOOK_SPEED_MS = 400; 	//80; // 일반적인 룩 애니메이션 프레임 간 지연 (약간 빠르게 조정)
const int g_R210_LOOK_HOLD_MS = 1000; 	// 보는 방향 유지 시간
const int g_R210_WINK_SPEED_MS = 400; //80;
const int g_R210_WINK_HOLD_MS = 1500;
const int g_R210_CONFUSED_SPEED_MS = 150; // 혼돈 상태 유지 시간
const int g_R210_CONFUSED_DART_SPEED_MS = 400; //40; // 혼돈 애니메이션 중 눈동자 움직임 속도 (더 빠르게 조정)

// -----------------------------------------------------------------------
// 헬퍼: 전체 8x8 패턴을 FastLED 배열에 그리기
// p_pattern_data: 그릴 패턴 데이터 포인터
// p_onColor: 켜짐 픽셀 색상
// -----------------------------------------------------------------------
void	  R210_drawFullPattern(int p_matrix_addr, const byte *p_pattern_data, CRGB p_onColor) {
	if (!p_pattern_data) return; // 유효성 검사

	 for (int v_row = 0; v_row < g_R210_MATRIX_SIZE; v_row++) {
		 byte v_rowData = p_pattern_data[v_row];

		 for (int v_col = 0; v_col < g_R210_MATRIX_SIZE; v_col++) {
			 if ((v_rowData >> (g_R210_MATRIX_SIZE - 1 - v_col)) & 1) {
				 int v_index = R210_getLedIndex(p_matrix_addr, v_row, v_col);
				 if (v_index != -1) {
					 g_R210_leds[v_index] = p_onColor;
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
void R210_drawOutline(int p_matrix_addr, const byte *p_outline_data, CRGB p_onColor) {
	if (!p_outline_data) return; // 유효성 검사

    for (int r = 0; r < g_R210_MATRIX_SIZE; r++) {
        byte rowData = p_outline_data[r];
        for (int c = 0; c < g_R210_MATRIX_SIZE; c++) {
            if ((rowData >> (g_R210_MATRIX_SIZE - 1 - c)) & 1) {
                int index = R210_getLedIndex(p_matrix_addr, r, c);
                if (index != -1) {
                    g_R210_leds[index] = p_onColor; // Draw outline pixel
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
void R210_drawPupil(int p_matrix_addr, int p_pupil_row, int p_pupil_col, CRGB p_pupil_color) {
    // 눈동자 블록 범위 체크 (8x8 매트릭스 내에 2x2 블록이 완전히 들어와야 함)
    if (p_pupil_row < 0 || p_pupil_col < 0 || p_pupil_row > g_R210_MATRIX_SIZE - 2 || p_pupil_col > g_R210_MATRIX_SIZE - 2) {
        return; // 눈동자 위치가 매트릭스 범위를 벗어남
    }
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 2; c++) {
            int index = R210_getLedIndex(p_matrix_addr, p_pupil_row + r, p_pupil_col + c);
             if (index != -1) {
                 g_R210_leds[index] = p_pupil_color; // Draw pupil pixel (outline 픽셀 위에 덮어 씀)
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
void R210_displayEmotionState(int p_emotionIndex, CRGB p_eyeColor, CRGB p_pupilColor) {
	fill_solid(g_R210_leds, g_R210_NUM_LEDS, CRGB::Black); // 프레임 버퍼 클리어

	bool is_pupil_based = false;
    PupilPos left_pupil = {-1, -1};
    PupilPos right_pupil = {-1, -1};
    const byte* left_override_pattern = nullptr;
    const byte* right_override_pattern = nullptr;

	// 1. 감정 인덱스 유효성 및 그리기 방식/데이터 판단
	if (p_emotionIndex >= 0 && p_emotionIndex < g_R210_NUM_EMOTION_PATTERNS) {

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
                 left_pupil = g_R210_confusedPupilPos[0];
                 right_pupil = g_R210_confusedPupilPos[1];
             } else {
                 // 그 외 외곽선 + 눈동자 방식 표정 (좌우 눈동자 위치 동일)
                 left_pupil = R210_getPupilPosForEmotion(p_emotionIndex); // 헬퍼 함수로 위치 가져옴
                 right_pupil = left_pupil;
             }
        }
        // 나머지 표정은 전체 패턴 오버라이드 방식

        // 전체 패턴 오버라이드 방식을 사용하는 표정들 패턴 데이터 설정
        if (!is_pupil_based) {
             // g_R210_fullOverridePatternsData 배열에서 해당 EmotionIndex에 맞는 패턴을 찾음
             // EmotionIndex와 g_R210_fullOverridePatternsData 배열의 인덱스 매핑 필요
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
                  // g_R210_fullOverridePatternsData 배열 경계 체크
                  if (pattern_data_index >= 0 && pattern_data_index < sizeof(g_R210_fullOverridePatternsData)/sizeof(g_R210_fullOverridePatternsData[0])) {
                      left_override_pattern = g_R210_fullOverridePatternsData[pattern_data_index];
                      right_override_pattern = g_R210_fullOverridePatternsData[pattern_data_index + 1];
                  }
             }
        }

	} else {
        // 유효하지 않은 감정 인덱스 (모두 꺼진 상태 유지)
    }


    // 2. 그리기 실행
    // 왼쪽 눈 (Matrix 0)
    if (is_pupil_based) {
        R210_drawOutline(0, BaseEyeOutline, p_eyeColor); // 외곽선 그림
        if (left_pupil.r != -1) { // 눈동자 위치가 유효하면 그림
            R210_drawPupil(0, left_pupil.r, left_pupil.c, p_pupilColor); // 눈동자 그림
        }
    } else if (left_override_pattern) { // 전체 패턴 오버라이드 방식
         R210_drawFullPattern(0, left_override_pattern, p_eyeColor); // 전체 패턴 그림
    }
    // else: 유효하지 않은 인덱스이거나 패턴이 정의되지 않은 경우, 해당 매트릭스는 검은색 유지

    // 오른쪽 눈 (Matrix 1) - 매트릭스가 2개 이상일 때만 해당
	if (g_R210_NUM_MATRICES > 1) {
		if (is_pupil_based) { // 오른쪽 눈도 외곽선+눈동자 방식인 경우
			R210_drawOutline(1, BaseEyeOutline, p_eyeColor); // 외곽선 그림
			if (right_pupil.r != -1) { // 눈동자 위치가 유효하면 그림
				R210_drawPupil(1, right_pupil.r, right_pupil.c, p_pupilColor); // 눈동자 그림
			}
		} else if (right_override_pattern) { // 오른쪽 눈이 전체 패턴 오버라이드인 경우
			 R210_drawFullPattern(1, right_override_pattern, p_eyeColor); // 전체 패턴 그림
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
void R210_showEmotion(int p_emotionIndex, CRGB p_eyeColor = CRGB::White, CRGB p_pupilColor = CRGB::Black) {
    R210_displayEmotionState(p_emotionIndex, p_eyeColor, p_pupilColor);
}

// -----------------------------------------------------------------------
// 애니메이션 함수: 눈 깜빡임
// 기능: Blink 패턴을 순차적으로 표시하여 깜빡임 효과 생성
// p_times: 깜빡이는 횟수
// -----------------------------------------------------------------------
void R210_animateBlink(int p_times = 1) {
	for (int i = 0; i < p_times; i++) {
		R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 방식
		delay(random(500, 2000)); // 중립 상태에서 무작위 대기 후 깜빡임 시작

		R210_showEmotion(E_BLINK_OPEN); // 전체 패턴 오버라이드 사용
		delay(g_R210_BLINK_SPEED_MS);

		R210_showEmotion(E_BLINK_MID); // 전체 패턴 오버라이드 사용
		delay(g_R210_BLINK_SPEED_MS);

		R210_showEmotion(E_BLINK_CLOSE); // 전체 패턴 오버라이드 사용
		delay(g_R210_BLINK_SPEED_MS * 2);

		R210_showEmotion(E_BLINK_MID); // 전체 패턴 오버라이드 사용
		delay(g_R210_BLINK_SPEED_MS);

		R210_showEmotion(E_BLINK_OPEN); // 전체 패턴 오버라이드 사용
		delay(g_R210_BLINK_SPEED_MS);

		R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 방식
	}
}

// -----------------------------------------------------------------------
// 애니메이션 함수: 지정된 방향 보기
// 기능: 중립에서 지정된 방향 패턴으로 이동, 유지 후 중립 복귀 (대각선 중간 포함)
// p_direction_emotion_index: E_LOOK_LEFT, E_LOOK_RIGHT, E_LOOK_UP, E_LOOK_DOWN, E_THINKING
// -----------------------------------------------------------------------
void R210_animateLook(int p_direction_emotion_index) {
	// 유효한 타겟 보기 인덱스인지 확인 (Neutral은 이 애니메이션의 시작/끝점으로만 사용)
	bool is_valid_target_look = ( (p_direction_emotion_index >= E_LOOK_LEFT && p_direction_emotion_index <= E_LOOK_DOWN) || p_direction_emotion_index == E_THINKING );


	if (is_valid_target_look) {
		R210_showEmotion(E_NEUTRAL); // Start from Neutral
		delay(random(100, 300)); // Short random delay before starting the move

        // 각 타겟 방향별 애니메이션 시퀀스
        switch(p_direction_emotion_index) {
            case E_LOOK_LEFT: // 왼쪽 보기 (위-왼쪽, 아래-왼쪽 경유)
                R210_showEmotion(E_LOOK_UP_LEFT);    delay(g_R210_LOOK_SPEED_MS);
                R210_showEmotion(E_LOOK_LEFT);       delay(g_R210_LOOK_HOLD_MS);      // Hold Left
                R210_showEmotion(E_LOOK_DOWN_LEFT);  delay(g_R210_LOOK_SPEED_MS);
                break;
            case E_LOOK_RIGHT: // 오른쪽 보기 (위-오른쪽, 아래-오른쪽 경유)
                R210_showEmotion(E_LOOK_UP_RIGHT);   delay(g_R210_LOOK_SPEED_MS);
                R210_showEmotion(E_LOOK_RIGHT);      delay(g_R210_LOOK_HOLD_MS);      // Hold Right
                R210_showEmotion(E_LOOK_DOWN_RIGHT); delay(g_R210_LOOK_SPEED_MS);
                break;
            case E_LOOK_UP: // 위 보기 (위-왼쪽, 위-오른쪽 경유)
                R210_showEmotion(E_LOOK_UP_LEFT);    delay(g_R210_LOOK_SPEED_MS);
                R210_showEmotion(E_LOOK_UP);         delay(g_R210_LOOK_HOLD_MS);      // Hold Up
                R210_showEmotion(E_LOOK_UP_RIGHT);   delay(g_R210_LOOK_SPEED_MS);
                break;
            case E_LOOK_DOWN: // 아래 보기 (아래-왼쪽, 아래-오른쪽 경유)
                R210_showEmotion(E_LOOK_DOWN_LEFT);  delay(g_R210_LOOK_SPEED_MS);
                R210_showEmotion(E_LOOK_DOWN);       delay(g_R210_LOOK_HOLD_MS);      // Hold Down
                R210_showEmotion(E_LOOK_DOWN_RIGHT); delay(g_R210_LOOK_SPEED_MS);
                break;
            case E_THINKING: // 생각하기 (빠르게 여러 방향을 훑어본 후 특정 위치로)
                 R210_showEmotion(E_LOOK_UP_LEFT);    delay(g_R210_LOOK_SPEED_MS / 2);
                 R210_showEmotion(E_LOOK_DOWN_RIGHT); delay(g_R210_LOOK_SPEED_MS / 2);
                 R210_showEmotion(E_LOOK_UP_RIGHT);   delay(g_R210_LOOK_SPEED_MS / 2);
                 R210_showEmotion(E_LOOK_DOWN_LEFT);  delay(g_R210_LOOK_SPEED_MS / 2);
                 R210_showEmotion(E_LOOK_UP);         delay(g_R210_LOOK_SPEED_MS / 2);
                 R210_showEmotion(E_LOOK_RIGHT);      delay(g_R210_LOOK_SPEED_MS / 2); // Dart Right (extreme)
                 R210_showEmotion(E_THINKING);       delay(g_R210_LOOK_HOLD_MS * 1.5);      // Hold Thinking (longer)
                 R210_showEmotion(E_LOOK_RIGHT);      delay(g_R210_LOOK_SPEED_MS / 2); // Dart back Right
                 R210_showEmotion(E_LOOK_UP);         delay(g_R210_LOOK_SPEED_MS / 2); // Dart back Up
                 break;
            default:
                // Should not happen if is_valid_target_look is true
                break;
        }

       R210_showEmotion(E_NEUTRAL); // Return to Neutral
       delay(random(100, 300)); // Short random delay after returning
	} else {
        // Serial.printf("Warning: R210_animateLook called with invalid target index: %d\n", p_direction_emotion_index); // Debug
    }
}

// -----------------------------------------------------------------------
// 애니메이션 함수: 윙크
// 기능: 지정된 방향으로 윙크 패턴을 표시하고 잠시 유지한 후 중립으로 돌아옵니다.
// p_side: 0 for Left Wink, 1 for Right Wink
// -----------------------------------------------------------------------
void R210_animateWink(int p_side) {
	int wink_emotion_index = (p_side == 0) ? E_LEFT_WINK : E_RIGHT_WINK;

	R210_showEmotion(E_NEUTRAL); // Start from Neutral
	delay(random(100, 300)); // Short random delay before starting

	R210_showEmotion(wink_emotion_index); // Show Wink pattern (full override)
	delay(g_R210_WINK_HOLD_MS);			  // Hold Wink

	R210_showEmotion(E_NEUTRAL); // Return to Neutral
	delay(random(100, 300)); // Short random delay after returning
}

// -----------------------------------------------------------------------
// 애니메이션 함수: 혼돈스런 눈
// 기능: 중립에서 혼돈스러운 눈 패턴으로 이동하고 잠시 유지한 후 중립으로 돌아옵니다.
// -----------------------------------------------------------------------
void R210_animateConfused() {
	R210_showEmotion(E_NEUTRAL); // Start from Neutral
	delay(random(100, 300)); // Short random delay before starting

    // Add darting animation using extreme diagonal and cardinal positions
    R210_showEmotion(E_LOOK_UP_LEFT);    delay(g_R210_CONFUSED_DART_SPEED_MS);
    R210_showEmotion(E_LOOK_DOWN_RIGHT); delay(g_R210_CONFUSED_DART_SPEED_MS);
    R210_showEmotion(E_LOOK_UP_RIGHT);   delay(g_R210_CONFUSED_DART_SPEED_MS);
    R210_showEmotion(E_LOOK_DOWN_LEFT);  delay(g_R210_CONFUSED_DART_SPEED_MS);
    R210_showEmotion(E_LOOK_UP);         delay(g_R210_CONFUSED_DART_SPEED_MS);
    R210_showEmotion(E_LOOK_DOWN);       delay(g_R210_CONFUSED_DART_SPEED_MS);
    R210_showEmotion(E_LOOK_LEFT);       delay(g_R210_CONFUSED_DART_SPEED_MS);
    R210_showEmotion(E_LOOK_RIGHT);      delay(g_R210_CONFUSED_DART_SPEED_MS);

    R210_showEmotion(E_NEUTRAL); // Briefly back to neutral
    delay(g_R210_LOOK_SPEED_MS); // Pause briefly

	R210_showEmotion(E_CONFUSED); // Show Confused pattern (outline+pupil)
	delay(g_R210_CONFUSED_SPEED_MS);  // Hold Confused

	R210_showEmotion(E_NEUTRAL); // Return to Neutral
	delay(random(100, 300)); // Short random delay after returning
}


// -----------------------------------------------------------------------
// 초기화 함수
// 기능: FastLED 초기화, 밝기 설정, 디스플레이 클리어 및 테스트.
// 초기화 완료 후 기본 상태인 중립 눈을 표시합니다.
// 이 함수는 main sketch의 setup() 함수에서 호출되어야 합니다.
// -----------------------------------------------------------------------
void R210_init() {
	// FastLED 라이브러리 초기화 (ESP32용)
    
	FastLED.addLeds<g_R210_LED_TYPE, g_R210_DATA_PIN, g_R210_COLOR_ORDER>(g_R210_leds, g_R210_NUM_LEDS);

	// 전체 밝기 설정 (0-255, 선택 사항)
	FastLED.setBrightness(40);

	// 시리얼 통신 초기화 (디버깅용)
	Serial.println("R210 RobotEyes Library Init");

	// 디스플레이 클리어 (모든 LED 끄기)
	fill_solid(g_R210_leds, g_R210_NUM_LEDS, CRGB::Black);
	FastLED.show();
	delay(500);

	// 초기화 테스트: 모든 LED 켜기
	Serial.println("Testing all LEDs ON...");
	fill_solid(g_R210_leds, g_R210_NUM_LEDS, CRGB::Red);
	FastLED.show();
	delay(1000);

	// 테스트 끝: 다시 모든 LED 끄기
	Serial.println("Test complete. Clearing display.");
	fill_solid(g_R210_leds, g_R210_NUM_LEDS, CRGB::Black);
	FastLED.show();
	delay(500);

	// // 초기화 완료 후 기본 상태인 중립 눈을 표시 (외곽선 + 눈동자 방식)
	// Serial.println("Setting initial state to Neutral (large outline).");
	// R210_showEmotion(E_NEUTRAL);
}

// -----------------------------------------------------------------------
// 기본 애니메이션 루프
// 기능: 다양한 애니메이션 패턴을 순차적으로 보여줌
// 이 함수는 main sketch의 loop() 함수에서 주기적으로 호출될 수 있습니다.
// 또는 loop() 함수에서 원하는 애니메이션 함수들을 직접 호출해도 됩니다.
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// 기본 애니메이션 루프
// 기능: 다양한 애니메이션 패턴을 순차적으로 보여줌
// 이 함수는 main sketch의 loop() 함수에서 주기적으로 호출될 수 있습니다.
// 또는 loop() 함수에서 원하는 애니메이션 함수들을 직접 호출해도 됩니다.
// -----------------------------------------------------------------------
void R210_run() {
    Serial.println("========== R210 로봇 눈 애니메이션 시작 ==========");

    // 초기화 완료 후 기본 상태인 중립 눈을 표시 (외곽선 + 눈동자 방식)
    Serial.println("-> 표정: E_NEUTRAL (중립 눈)");
    R210_showEmotion(E_NEUTRAL);
    delay(2000); // 잠시 중립 상태 유지


	Serial.println("-> 표정: E_SQUINT (찡그림)");
    R210_showEmotion(E_SQUINT); // 전체 패턴 오버라이드
    delay(1000);
    Serial.println("-> 표정: E_SQUINT_TIGHT (더 찡그림)");
    R210_showEmotion(E_SQUINT_TIGHT); // 전체 패턴 오버라이드
    delay(1000);
    Serial.println("-> 표정: E_NEUTRAL (중립 눈으로 복귀)");
    R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 사용
    delay(2000);


    Serial.println("-> 표정: E_SLEEPY (졸린 눈)");
    R210_showEmotion(E_SLEEPY); // 전체 패턴 오버라이드
    delay(1000);
    Serial.println("-> 표정: E_NEUTRAL (중립 눈으로 복귀)");
	R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 사용
    Serial.println("-> 표정: E_SLEEPY (졸린 눈)");
    R210_showEmotion(E_SLEEPY); // 전체 패턴 오버라이드
    delay(1000);
    Serial.println("-> 표정: E_NEUTRAL (중립 눈으로 복귀)");
    R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 사용
    delay(2000);

    Serial.println("-> 표정: E_ANGRY (화난 눈)");
    R210_showEmotion(E_ANGRY); // 전체 패턴 오버라이드
    delay(1000);
    Serial.println("-> 표정: E_NEUTRAL (중립 눈으로 복귀)");
    R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 사용
    delay(2000);

    Serial.println("-> 표정: E_ABSURD (황당한 눈)");
    R210_showEmotion(E_ABSURD); // 전체 패턴 오버라이드
    delay(1000);
    Serial.println("-> 표정: E_NEUTRAL (중립 눈으로 복귀)");
    R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 사용
    delay(2000);

    Serial.println("-> 표정: E_GLARING (째려보는 눈)");
    R210_showEmotion(E_GLARING); // 전체 패턴 오버라이드
    delay(1000);
    Serial.println("-> 표정: E_NEUTRAL (중립 눈으로 복귀)");
    R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 사용
    delay(2000);



    Serial.println("-> 애니메이션: R210_animateBlink (깜빡임)");
    R210_animateBlink(1); // 1번 깜빡이기
    delay(2000); // 애니메이션 후 대기

    Serial.println("-> 애니메이션: R210_animateLook(E_LOOK_LEFT) (왼쪽 보기)");
    R210_animateLook(E_LOOK_LEFT);
    delay(2000);

    Serial.println("-> 애니메이션: R210_animateLook(E_LOOK_RIGHT) (오른쪽 보기)");
    R210_animateLook(E_LOOK_RIGHT);
    delay(2000);

    Serial.println("-> 애니메이션: R210_animateLook(E_LOOK_UP) (위 보기)");
    R210_animateLook(E_LOOK_UP);
    delay(2000);

    Serial.println("-> 애니메이션: R210_animateLook(E_LOOK_DOWN) (아래 보기)");
    R210_animateLook(E_LOOK_DOWN);
    delay(2000);

    Serial.println("-> 애니메이션: R210_animateLook(E_LOOK_UP_LEFT) (왼쪽 위 보기)");
    R210_animateLook(E_LOOK_UP_LEFT); // 추가된 대각선 보기
    delay(2000);

    Serial.println("-> 애니메이션: R210_animateLook(E_LOOK_UP_RIGHT) (오른쪽 위 보기)");
    R210_animateLook(E_LOOK_UP_RIGHT); // 추가된 대각선 보기
    delay(2000);

    Serial.println("-> 애니메이션: R210_animateLook(E_LOOK_DOWN_LEFT) (왼쪽 아래 보기)");
    R210_animateLook(E_LOOK_DOWN_LEFT); // 추가된 대각선 보기
    delay(2000);

    Serial.println("-> 애니메이션: R210_animateLook(E_LOOK_DOWN_RIGHT) (오른쪽 아래 보기)");
    R210_animateLook(E_LOOK_DOWN_RIGHT); // 추가된 대각선 보기
    delay(2000);


    Serial.println("-> 애니메이션: R210_animateWink(0) (왼쪽 윙크)");
    R210_animateWink(0); // 왼쪽 윙크
    delay(2000);

    Serial.println("-> 애니메이션: R210_animateWink(1) (오른쪽 윙크)");
    R210_animateWink(1); // 오른쪽 윙크
    delay(2000);

    
    Serial.println("-> 애니메이션: R210_animateLook(E_THINKING) (생각하는 눈)");
    R210_animateLook(E_THINKING); // Thinking은 R210_animateLook을 통해 애니메이션으로 보여줌
    delay(2000); // 애니메이션 후 대기

    Serial.println("-> 애니메이션: R210_animateConfused (혼돈스러운 눈)");
    R210_animateConfused(); // 혼돈은 애니메이션으로 보여줌
    delay(2000); // 애니메이션 후 대기


    Serial.println("========== R210 로봇 눈 애니메이션 종료 ==========");
    Serial.println(); // 줄바꿈 추가
}

// void R210_run() {
// 	Serial.println("Starting RobotEyes Animation Sequence...");

// 	// 초기화 완료 후 기본 상태인 중립 눈을 표시 (외곽선 + 눈동자 방식)
// 	Serial.println("Setting initial state to Neutral (large outline).");

// 	for (int i=0; i<=g_R210_NUM_EMOTION_PATTERNS; i++){
// 		R210_showEmotion(i);
// 		//R210_showEmotion(E_NEUTRAL);
// 		delay(1000);
// 	}
	
// 	Serial.println("Animating Blink...");
// 	R210_animateBlink(1); // 1번 깜빡이기
// 	// animateBlink/Look/Wink/Confused 내부에 종료 후 짧은 랜덤 딜레이 포함됨

// 	Serial.println("Animating Look Left...");
// 	R210_animateLook(E_LOOK_LEFT);

// 	Serial.println("Animating Look Right...");
// 	R210_animateLook(E_LOOK_RIGHT);

// 	Serial.println("Animating Look Up...");
// 	R210_animateLook(E_LOOK_UP);

// 	Serial.println("Animating Look Down...");
// 	R210_animateLook(E_LOOK_DOWN);

// 	Serial.println("Animating Wink Left...");
// 	R210_animateWink(0); // 왼쪽 윙크

// 	Serial.println("Animating Wink Right...");
// 	R210_animateWink(1); // 오른쪽 윙크

// 	Serial.println("Showing Squint...");
// 	R210_showEmotion(E_SQUINT); // 전체 패턴 오버라이드
// 	delay(1500);
// 	Serial.println("Showing Squint Tight...");
// 	R210_showEmotion(E_SQUINT_TIGHT); // 전체 패턴 오버라이드
// 	delay(1500);
// 	R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 사용
// 	delay(1000);

// 	Serial.println("Showing Sleepy...");
// 	R210_showEmotion(E_SLEEPY); // 전체 패턴 오버라이드
// 	delay(2000);
// 	R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 사용
// 	delay(1000);

// 	Serial.println("Showing Angry...");
// 	R210_showEmotion(E_ANGRY); // 전체 패턴 오버라이드
// 	delay(2000);
// 	R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 사용
// 	delay(1000);

// 	Serial.println("Showing Absurd...");
// 	R210_showEmotion(E_ABSURD); // 전체 패턴 오버라이드
// 	delay(2000);
// 	R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 사용
// 	delay(1000);

// 	Serial.println("Showing Glaring...");
// 	R210_showEmotion(E_GLARING); // 전체 패턴 오버라이드
// 	delay(2000);
// 	R210_showEmotion(E_NEUTRAL); // 외곽선+눈동자 사용
// 	delay(1000);

// 	Serial.println("Showing Thinking...");
// 	R210_animateLook(E_THINKING); // Thinking은 R210_animateLook을 통해 애니메이션으로 보여줌

// 	Serial.println("Animating Confused...");
// 	R210_animateConfused(); // 혼돈은 애니메이션으로 보여줌

// 	Serial.println("Animation sequence finished.");
// }
