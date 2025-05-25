#include <FastLED.h>

// -----------------------------------------------------------------------
// FastLED (WS2812B) 및 하드웨어 설정 (전역 상수)
// -----------------------------------------------------------------------
#define g_R100_DATA_PIN			   13		// <-- 실제 WS2812B 데이터 선이 연결된 핀 번호로 변경하세요 (예: 6번 핀)
#define g_R100_LED_TYPE			   WS2812B	// 사용하는 LED 타입 (WS2811, WS2812B 등)
#define g_R100_COLOR_ORDER		   GRB		// LED의 색상 순서 (대부분 GRB 또는 RGB)

#define g_R100_MATRIX_SIZE		   8												   // 매트릭스 크기 (8x8)
#define g_R100_NUM_MATRICES		   1												   // 사용하는 8x8 매트릭스 개수 (좌/우)
#define g_R100_NUM_LEDS_PER_MATRIX (g_R100_MATRIX_SIZE * g_R100_MATRIX_SIZE)		   // 매트릭스 하나당 LED 개수
#define g_R100_NUM_LEDS			   (g_R100_NUM_LEDS_PER_MATRIX * g_R100_NUM_MATRICES)  // 총 LED 개수

// FastLED LED 배열 (전역 변수)
CRGB g_R100_leds[g_R100_NUM_LEDS];

// -----------------------------------------------------------------------
// 2차원 (행, 열) 좌표를 1차원 LED 배열 인덱스로 변환하는 함수
// 사용하는 8x8 매트릭스의 물리적인 LED 배선 방식에 맞게 수정해야 합니다.
// 아래 함수는 '2개의 8x8 매트릭스가 직렬 연결'되어 있고,
// 각 매트릭스 내부 배선이 '지그재그(serpentine)' 방식이라고 가정한 것입니다.
// p_matrix_addr: 0 (왼쪽), 1 (오른쪽)
// p_row: 0-g_R100_MATRIX_SIZE-1
// p_col: 0-g_R100_MATRIX_SIZE-1
// -----------------------------------------------------------------------
int	 R100_getLedIndex(int p_matrix_addr, int p_row, int p_col) {
	 int v_base_index = p_matrix_addr * g_R100_NUM_LEDS_PER_MATRIX;	 // 해당 매트릭스의 시작 인덱스
	 int v_local_index;												 // 8x8 매트릭스 내에서의 상대 인덱스

	 // 지그재그(serpentine) 방식 가정
	 if (p_row % 2 == 0) {	// 짝수 행 (0, 2, 4, 6)은 왼쪽 -> 오른쪽으로 인덱스 증가
		 v_local_index = p_row * g_R100_MATRIX_SIZE + p_col;
	 } else {  // 홀수 행 (1, 3, 5, 7)은 오른쪽 -> 왼쪽으로 인덱스 증가
		 v_local_index = p_row * g_R100_MATRIX_SIZE + (g_R100_MATRIX_SIZE - 1 - p_col);
	 }

	 return v_base_index + v_local_index;  // 전체 LED 배열에서의 최종 인덱스
}

// -----------------------------------------------------------------------
// 얼굴 표정 패턴 데이터 구조체 및 정의
// 각 EmotionPattern은 왼쪽 눈과 오른쪽 눈의 8x8 픽셀 패턴을 저장합니다.
// 각 byte는 한 행을 나타내며, 비트가 1이면 픽셀이 켜집니다.
// -----------------------------------------------------------------------
struct EmotionPattern {
	byte leftEye[g_R100_MATRIX_SIZE];
	byte rightEye[g_R100_MATRIX_SIZE];
	// CRGB color; // 패턴별 색상을 다르게 하고 싶다면 이 필드를 추가
};

// 미리 정의된 얼굴 표정 패턴들 (전역 상수)
const EmotionPattern g_R100_emotionPatterns[] = {
	{// Neutral Face (중립 표정)
	 {B00000000, B00111100, B01000010, B01011010, B01011010, B01000010, B00111100, B00000000},
	 {B00000000, B00111100, B01000010, B01011010, B01011010, B01000010, B00111100, B00000000}},
	{
		// Look Left (왼쪽 보는 눈)
		{0b00000000, 0b00111100, 0b01001110, 0b01001110, 0b01111110, 0b01111110, 0b00111100, 0b00000000},
		{0b00000000, 0b00111100, 0b01001110, 0b01001110, 0b01111110, 0b01111110, 0b00111100, 0b00000000}  // 오른쪽 눈도 같은 패턴 사용 예시
	},
	{
		// Look Right (오른쪽 보는 눈)
		{0b00000000, 0b00111100, 0b01111110, 0b01111110, 0b01001110, 0b01001110, 0b00111100, 0b00000000},
		{0b00000000, 0b00111100, 0b01111110, 0b01111110, 0b01001110, 0b01001110, 0b00111100, 0b00000000}  // 오른쪽 눈도 같은 패턴 사용 예시
	},
	{
		// Squint/Angry (찡그린 표정) - he
		{0b01000000, 0b10011000, 0b10100100, 0b10111100, 0b10111100, 0b10100100, 0b10011000, 0b01000000},
		{0b01000000, 0b10011000, 0b10100100, 0b10111100, 0b10111100, 0b10100100, 0b10011000, 0b01000000}  // 오른쪽 눈도 같은 패턴 사용 예시
	},
	{
		// Squint/Angry Compact (더 찡그린 표정) - hec
		{0b01000000, 0b10011000, 0b10111100, 0b10111100, 0b10111100, 0b10111100, 0b10011000, 0b01000000},
		{0b01000000, 0b10011000, 0b10111100, 0b10111100, 0b10111100, 0b10111100, 0b10011000, 0b01000000}  // 오른쪽 눈도 같은 패턴 사용 예시
	}};

const int g_R100_NUM_EMOTIONS = sizeof(g_R100_emotionPatterns) / sizeof(g_R100_emotionPatterns[0]);	 // 전체 표정 개수 (전역 상수)

// -----------------------------------------------------------------------
// FastLED를 사용하여 정의된 8x8 패턴을 매트릭스에 표시하는 함수
// p_left_pattern, p_right_pattern 배열의 각 바이트는 한 행의 픽셀 데이터를 비트로 가집니다.
// 비트가 1이면 p_onColor로 픽셀을 켜고, 0이면 끕니다 (검은색).
// -----------------------------------------------------------------------
void	  R100_displayMatrixPattern(const byte p_left_pattern[g_R100_MATRIX_SIZE], const byte p_right_pattern[g_R100_MATRIX_SIZE], CRGB p_onColor) {
	 fill_solid(g_R100_leds, g_R100_NUM_LEDS, CRGB::Black);	 // 모든 LED를 일단 끕니다 (검은색으로 설정)

	 // 왼쪽 매트릭스 패턴 표시 (matrix_addr = 0)
	 for (int v_row = 0; v_row < g_R100_MATRIX_SIZE; v_row++) {
		 for (int v_col = 0; v_col < g_R100_MATRIX_SIZE; v_col++) {
			 // p_left_pattern[v_row]의 (7-v_col) 번째 비트가 1인지 확인
			 if ((p_left_pattern[v_row] >> (g_R100_MATRIX_SIZE - 1 - v_col)) & 1) {
				 int v_index = R100_getLedIndex(0, v_row, v_col);  // 왼쪽 매트릭스의 해당 (v_row, v_col) 인덱스 계산
				 if (v_index < g_R100_NUM_LEDS) {				   // 안전 체크
					 g_R100_leds[v_index] = p_onColor;			   // 해당 LED 색상 설정
				 }
			 }
		 }
	 }

	 // 오른쪽 매트릭스 패턴 표시 (matrix_addr = 1)
	 for (int v_row = 0; v_row < g_R100_MATRIX_SIZE; v_row++) {
		 for (int v_col = 0; v_col < g_R100_MATRIX_SIZE; v_col++) {
			 // p_right_pattern[v_row]의 (7-v_col) 번째 비트가 1인지 확인
			 if ((p_right_pattern[v_row] >> (g_R100_MATRIX_SIZE - 1 - v_col)) & 1) {
				 int v_index = R100_getLedIndex(1, v_row, v_col);  // 오른쪽 매트릭스의 해당 (v_row, v_col) 인덱스 계산
				 if (v_index < g_R100_NUM_LEDS) {				   // 안전 체크
					 g_R100_leds[v_index] = p_onColor;			   // 해당 LED 색상 설정
				 }
			 }
			 // 주의: 위 if 문 조건에서 p_left_pattern 대신 p_right_pattern을 사용해야 합니다.
			 // if ((p_right_pattern[v_row] >> (g_R100_MATRIX_SIZE - 1 - v_col)) & 1) { ... }
		 }
		 // 수정: 오른쪽 매트릭스 패턴 확인 로직 오류 수정
		 for (int v_col = 0; v_col < g_R100_MATRIX_SIZE; v_col++) {
			 if ((p_right_pattern[v_row] >> (g_R100_MATRIX_SIZE - 1 - v_col)) & 1) {
				 int v_index = R100_getLedIndex(1, v_row, v_col);  // 오른쪽 매트릭스의 해당 (v_row, v_col) 인덱스 계산
				 if (v_index < g_R100_NUM_LEDS) {				   // 안전 체크
					 g_R100_leds[v_index] = p_onColor;			   // 해당 LED 색상 설정
				 }
			 }
		 }
	 }

	 FastLED.show();  // 설정된 색상 데이터를 실제 LED로 전송하여 표시
}

// -----------------------------------------------------------------------
// 초기화 함수 (FastLED에 맞게 수정, 명명 규칙 적용)
// -----------------------------------------------------------------------
void R100_init() {
	// FastLED 라이브러리 초기화
	// 사용하는 LED 타입, 데이터 핀, 색상 순서에 맞게 addLeds 함수 호출
	FastLED.addLeds<g_R100_LED_TYPE, g_R100_DATA_PIN, g_R100_COLOR_ORDER>(g_R100_leds, g_R100_NUM_LEDS);

	// 전체 밝기 설정 (0-255, 선택 사항)
	FastLED.setBrightness(70);	 // 예시: 최대 밝기의 약 40%

	// 디스플레이 클리어 (모든 LED 끄기)
	fill_solid(g_R100_leds, g_R100_NUM_LEDS, CRGB::Black);	// 모든 LED 색상을 검은색으로 설정
	FastLED.show();											// 설정 내용을 LED에 반영
	delay(100);												// 잠깐 대기

	// 테스트: 모든 LED 켜기
	fill_solid(g_R100_leds, g_R100_NUM_LEDS, CRGB::White);	// 모든 LED 색상을 흰색으로 설정
	FastLED.show();											// 표시
	delay(1000);											// 1초간 대기

	// 테스트 끝: 다시 모든 LED 끄기
	fill_solid(g_R100_leds, g_R100_NUM_LEDS, CRGB::Black);
	FastLED.show();
	delay(300);
}

// -----------------------------------------------------------------------
// 실행 함수 (정의된 표정 패턴들을 순차적으로 표시, 명명 규칙 적용)
// -----------------------------------------------------------------------
void R100_run() {
	// 미리 정의된 표정 패턴들을 순차적으로 표시
	for (int v_i = 0; v_i < g_R100_NUM_EMOTIONS; v_i++) {
		// g_R100_emotionPatterns[v_i]에 저장된 왼쪽 눈 패턴과 오른쪽 눈 패턴을 사용
		// 여기서는 모든 표정을 흰색(CRGB::White)으로 표시하지만,
		// 필요에 따라 다른 색상을 지정하거나 EmotionPattern 구조체에 색상을 추가할 수 있습니다.
		R100_displayMatrixPattern(g_R100_emotionPatterns[v_i].leftEye, g_R100_emotionPatterns[v_i].rightEye, CRGB::White);
		delay(4000);  // 각 표정을 4초간 보여줌
	}
}
