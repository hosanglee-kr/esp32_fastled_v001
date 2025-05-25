// https://github.com/MajicDesigns/MD_MAX72XX/tree/main/examples/MD_MAX72xx_RobotEyes

// ======================================================================
// FastLED + WS2812B 코드 (ESP32 Arduino Framework용)
// 원본 MD_MAX72XX 예제 통합 코드 기반, 명명 규칙 접두사 R200 적용
// 기능 설명 주석 추가
// ======================================================================

#include <Arduino.h>    // Arduino 핵심 라이브러리 포함 (ESP32에서도 사용)
#include <FastLED.h>    // FastLED 라이브러리 포함 (ESP32 지원 버전 설치 필요)
// ESP32에서는 <avr/pgmspace.h>가 필요 없습니다. const 데이터는 직접 접근합니다.

// -----------------------------------------------------------------------
// FastLED (WS2812B) 및 하드웨어 설정 (전역 상수)
// -----------------------------------------------------------------------
// WS2812B 데이터 핀 번호 - 실제 ESP32 GPIO 핀 번호로 변경하세요!
// 예: 13, 14, 27 등 (스트래핑 핀은 피하는 것이 좋습니다)
#define g_R200_DATA_PIN    13

// 사용하는 LED 타입 및 색상 순서 - WS2812B 모듈에 맞게 변경하세요!
#define g_R200_LED_TYPE    WS2812B // 예: WS2811, WS2812B 등
#define g_R200_COLOR_ORDER GRB   // 예: RGB, GRB, BRG 등

// 매트릭스 설정
#define g_R200_MATRIX_SIZE 8              // 8x8 매트릭스 크기
#define g_R200_NUM_MATRICES 2             // 사용하는 8x8 매트릭스 개수 (좌/우)
#define g_R200_NUM_LEDS_PER_MATRIX (g_R200_MATRIX_SIZE * g_R200_MATRIX_SIZE) // 매트릭스 하나당 LED 개수
#define g_R200_NUM_LEDS    (g_R200_NUM_LEDS_PER_MATRIX * g_R200_NUM_MATRICES) // WS2812B 총 LED 개수

// FastLED LED 배열 (전역 변수) - 모든 WS2812B LED의 색상 값을 저장
CRGB g_R200_leds[g_R200_NUM_LEDS];

// -----------------------------------------------------------------------
// 2차원 (행, 열) 좌표를 1차원 LED 배열 인덱스로 변환하는 함수
// 기능: 8x8 매트릭스 내의 특정 (행, 열) 위치를 전체 LED 배열에서의 선형 인덱스로 변환합니다.
// 이 함수는 하드웨어 배선 방식(지그재그 등)에 따라 구현이 달라져야 합니다.
// p_matrix_addr: 매트릭스 주소 (0: 왼쪽, 1: 오른쪽)
// p_row: 행 번호 (0부터 g_R200_MATRIX_SIZE-1 까지)
// p_col: 열 번호 (0부터 g_R200_MATRIX_SIZE-1 까지)
// 반환값: 전체 g_R200_leds 배열에서의 해당 픽셀 인덱스
// -----------------------------------------------------------------------
int R200_getLedIndex(int p_matrix_addr, int p_row, int p_col) {
  int v_base_index = p_matrix_addr * g_R200_NUM_LEDS_PER_MATRIX; // 해당 매트릭스의 시작 인덱스
  int v_local_index; // 8x8 매트릭스 내에서의 상대 인덱스

  // 지그재그(serpentine) 방식 가정 - 실제 배선에 따라 아래 로직을 수정하세요!
  if (p_row % 2 == 0) { // 짝수 행 (0, 2, 4, 6)은 왼쪽 -> 오른쪽으로 인덱스 증가
    v_local_index = p_row * g_R200_MATRIX_SIZE + p_col;
  } else { // 홀수 행 (1, 3, 5, 7)은 오른쪽 -> 왼쪽으로 인덱스 증가
    v_local_index = p_row * g_R200_MATRIX_SIZE + (g_R200_MATRIX_SIZE - 1 - p_col);
  }

  return v_base_index + v_local_index; // 전체 LED 배열에서의 최종 인덱스
}

// -----------------------------------------------------------------------
// 눈 모양 픽셀 데이터 (const로 플래시 메모리에 저장)
// MD_MAX72XX 예제 eyes_data.h 의 내용을 가져왔습니다.
// 각 byte 배열은 8x8 매트릭스의 픽셀 패턴 (1: 켜짐, 0: 꺼짐).
// ESP32에서는 const 전역 배열을 통해 플래시 메모리에 저장됩니다.
// -----------------------------------------------------------------------
const byte Neutral_eye[g_R200_MATRIX_SIZE] = {
  B00000000, B00111100, B01000010, B01011010, B01011010, B01000010, B00111100, B00000000
};
const byte BlinkOpen_eye[g_R200_MATRIX_SIZE] = {
  B00000000, B00000000, B00111100, B01000010, B01000010, B00111100, B00000000, B00000000
};
const byte BlinkMid_eye[g_R200_MATRIX_SIZE] = {
  B00000000, B00000000, B00000000, B00111100, B00111100, B00000000, B00000000, B00000000
};
const byte BlinkClose_eye[g_R200_MATRIX_SIZE] = {
  B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000
};
const byte LookLeft_eye[g_R200_MATRIX_SIZE] = {
  B00000000, B00111100, B01100000, B01101100, B01101100, B01100000, B00111100, B00000000
};
const byte LookRight_eye[g_R200_MATRIX_SIZE] = {
  B00000000, B00111100, B00000110, B00110110, B00110110, B00000110, B00111100, B00000000
};
const byte LookUp_eye[g_R200_MATRIX_SIZE] = {
  B00000000, B00100100, B01100110, B01000010, B01011010, B01000010, B00111100, B00000000
};
const byte LookDown_eye[g_R200_MATRIX_SIZE] = {
  B00000000, B00111100, B01000010, B01011010, B01000010, B01100110, B00100100, B00000000
};
const byte Squint_eye[g_R200_MATRIX_SIZE] = {
  B00000000, B00000000, B01111110, B01111110, B01111110, B01111110, B00000000, B00000000
};
const byte SquintTight_eye[g_R200_MATRIX_SIZE] = {
  B00000000, B00000000, B00000000, B01111110, B01111110, B00000000, B00000000, B00000000
};

// -----------------------------------------------------------------------
// 감정 패턴 데이터 (const로 플래시 메모리에 저장)
// showEmotion 함수에서 사용할 눈 패턴 데이터 포인터 배열
// -----------------------------------------------------------------------
// ESP32에서는 const 포인터 배열도 플래시에 저장됩니다.
const byte* const g_R200_emotionPatternsData[] = {
  Neutral_eye,     // 0: Neutral
  BlinkOpen_eye,   // 1: Blink Open
  BlinkMid_eye,    // 2: Blink Mid
  BlinkClose_eye,  // 3: Blink Close
  LookLeft_eye,    // 4: Look Left
  LookRight_eye,   // 5: Look Right
  LookUp_eye,      // 6: Look Up
  LookDown_eye,    // 7: Look Down
  Squint_eye,      // 8: Squint
  SquintTight_eye  // 9: Squint Tight
};

const int g_R200_NUM_EMOTION_PATTERNS = sizeof(g_R200_emotionPatternsData) / sizeof(g_R200_emotionPatternsData[0]); // 전체 패턴 개수

// -----------------------------------------------------------------------
// 헬퍼 함수: 단일 8x8 매트릭스에 const 패턴을 그리는 함수
// 기능: 주어진 8x8 비트 패턴 데이터를 읽어 특정 매트릭스 영역의 FastLED 배열에 색상을 설정합니다.
// p_matrix_addr: 그릴 매트릭스 주소 (0: 왼쪽, 1: 오른쪽)
// p_pattern_data: const 패턴 데이터 포인터 (플래시 메모리)
// p_onColor: 패턴의 '켜짐' 픽셀에 사용할 색상
// -----------------------------------------------------------------------
void R200_drawSingleMatrixPattern(int p_matrix_addr, const byte *p_pattern_data, CRGB p_onColor) {
  for (int v_row = 0; v_row < g_R200_MATRIX_SIZE; v_row++) {
    // ESP32에서는 const 포인터를 통해 플래시 메모리 데이터를 직접 읽습니다.
    byte v_rowData = p_pattern_data[v_row];

    for (int v_col = 0; v_col < g_R200_MATRIX_SIZE; v_col++) {
      // v_rowData의 (7-v_col) 번째 비트가 1인지 확인 (MSB가 왼쪽 픽셀)
      if ((v_rowData >> (g_R200_MATRIX_SIZE - 1 - v_col)) & 1) {
        int v_index = R200_getLedIndex(p_matrix_addr, v_row, v_col); // 해당 매트릭스의 (v_row, v_col) 인덱스 계산
        if (v_index >= 0 && v_index < g_R200_NUM_LEDS) { // 안전 체크
          g_R200_leds[v_index] = p_onColor; // 해당 LED 색상 설정
        }
      }
    }
  }
}

// -----------------------------------------------------------------------
// FastLED를 사용하여 정의된 8x8 패턴을 두 매트릭스에 표시하는 함수
// 기능: 왼쪽/오른쪽 눈에 해당하는 두 개의 패턴 데이터를 받아,
// R200_drawSingleMatrixPattern 헬퍼 함수를 사용하여 FastLED 배열에 그린 후 실제 LED에 표시합니다.
// p_left_eye_data: 왼쪽 눈 const 패턴 데이터 포인터
// p_right_eye_data: 오른쪽 눈 const 패턴 데이터 포인터
// p_onColor: 패턴의 '켜짐' 픽셀에 사용할 색상
// -----------------------------------------------------------------------
void R200_displayEmotionPattern(const byte *p_left_eye_data, const byte *p_right_eye_data, CRGB p_onColor) {
  // 모든 LED를 일단 끕니다 (FastLED 배열을 검은색으로 설정)
  fill_solid(g_R200_leds, g_R200_NUM_LEDS, CRGB::Black);

  // 왼쪽 매트릭스 (주소 0)에 패턴 적용
  R200_drawSingleMatrixPattern(0, p_left_eye_data, p_onColor);

  // 오른쪽 매트릭스 (주소 1)에 패턴 적용
  R200_drawSingleMatrixPattern(1, p_right_eye_data, p_onColor); // 좌우 패턴이 다를 경우 p_right_eye_data 사용

  FastLED.show(); // 설정된 색상 데이터를 실제 LED로 전송하여 표시
}

// -----------------------------------------------------------------------
// 초기화 함수 (FastLED ESP32에 맞게 수정, 명명 규칙 적용)
// 기능: FastLED 라이브러리를 초기화하고, LED 밝기 설정, 전체 LED 클리어 및 테스트 패턴 표시를 수행합니다.
// -----------------------------------------------------------------------
void R200_initEyes()
{
  // FastLED 라이브러리 초기화 (ESP32용)
  // 사용하는 LED 타입, 데이터 핀, 색상 순서에 맞게 addLeds 함수 호출
  // ESP32에서는 RMT 채널을 지정하는 것이 권장됩니다 (예: &FastLED.addLeds<...>(..., RMT_CHANNEL_0);).
  // 자세한 내용은 FastLED ESP32 예제를 참고하세요.
  FastLED.addLeds<g_R200_LED_TYPE, g_R200_DATA_PIN, g_R200_COLOR_ORDER>(g_R200_leds, g_R200_NUM_LEDS);

  // 전체 밝기 설정 (0-255, 선택 사항)
  FastLED.setBrightness(100); // 예시: 최대 밝기의 약 40%

  // 디스플레이 클리어 (모든 LED 끄기)
  fill_solid(g_R200_leds, g_R200_NUM_LEDS, CRGB::Black); // 모든 LED 색상을 검은색으로 설정
  FastLED.show(); // 설정 내용을 LED에 반영
  delay(100); // 잠깐 대기

  // 테스트: 모든 LED 켜기
  fill_solid(g_R200_leds, g_R200_NUM_LEDS, CRGB::White); // 모든 LED 색상을 흰색으로 설정
  FastLED.show(); // 표시
  delay(1000); // 1초간 대기

  // 테스트 끝: 다시 모든 LED 끄기
  fill_solid(g_R200_leds, g_R200_NUM_LEDS, CRGB::Black);
  FastLED.show();
  delay(300);
}

// -----------------------------------------------------------------------
// 지정된 감정 패턴을 표시하는 함수 (FastLED ESP32에 맞게 수정, 명명 규칙 적용)
// 기능: 감정 패턴 인덱스를 받아 해당 패턴 데이터를 찾아 R200_displayEmotionPattern 함수를 호출합니다.
// p_emotionIndex: g_R200_emotionPatternsData 배열의 인덱스 (0부터 g_R200_NUM_EMOTION_PATTERNS-1 까지)
// -----------------------------------------------------------------------
void R200_showEmotion(int p_emotionIndex)
{
  // 인덱스 범위 유효성 검사
  if (p_emotionIndex >= 0 && p_emotionIndex < g_R200_NUM_EMOTION_PATTERNS) {
    // const 포인터 배열에서 해당 인덱스의 패턴 데이터 포인터를 직접 읽습니다.
    const byte *v_leftData = g_R200_emotionPatternsData[p_emotionIndex];
    // 이 예제는 좌우 눈 모양이 같은 경우만 고려하므로 오른쪽 눈도 같은 패턴을 사용합니다.
    const byte *v_rightData = g_R200_emotionPatternsData[p_emotionIndex];

    // FastLED 표시 함수 호출 (표시 색상은 흰색으로 고정 예시)
    // 필요에 따라 v_rightData를 다른 패턴으로 바꾸거나, 패턴별 색상을 EmotionPattern 구조체에 추가할 수 있습니다.
    R200_displayEmotionPattern(v_leftData, v_rightData, CRGB::White);
  }
}



// -----------------------------------------------------------------------
// Arduino 표준 loop 함수 - 감정 패턴 순환 표시 (명명 규칙 미적용)
// 기능: setup() 실행 후 무한 반복됩니다. 정의된 표정 패턴들을 순차적으로 표시합니다.
// -----------------------------------------------------------------------
void R200_run() {
  // 미리 정의된 표정 패턴들을 순차적으로 표시
  for (int v_i = 0; v_i < g_R200_NUM_EMOTION_PATTERNS; v_i++) {
     R200_showEmotion(v_i); // v_i 번째 감정 패턴 표시
     delay(1000);   // 1초 대기 (필요에 따라 조절)
  }

  // 특정 감정 패턴 조합 예시 (선택 사항)
  /*
  R200_showEmotion(0); // Neutral
  delay(2000);
  R200_showEmotion(4); // Look Left
  delay(2000);
  R200_showEmotion(5); // Look Right
  delay(2000);
  R200_showEmotion(8); // Squint
  delay(2000);
  R200_showEmotion(0); // Neutral
  delay(2000);
  */
}
