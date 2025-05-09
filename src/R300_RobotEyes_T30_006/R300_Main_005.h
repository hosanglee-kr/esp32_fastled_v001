#pragma once

// R300_variables_and_funcs_005.h - 로봇 눈 애니메이션 및 상태 관리의 변수 및 함수 구현 파일

// 기본 타입 및 설정 헤더 파일 포함
#include "R300_types_config_006.h"
// 정적 데이터 테이블 헤더 파일 포함
#include "R300_data_006.h"


// --- 글로벌 변수 정의 (g_R300_ 로 시작) ---
// FastLED CRGB 배열
CRGB g_R300_leds[G_R300_NEOPIXEL_NUM_LEDS];
// CRGB 배열 포인터
CRGB* g_R300_leds_ptr = nullptr; // setup()에서 실제 배열 주소 할당

// 현재 로봇 상태
T_R300_State g_R300_state = AWAKE;

// 애니메이션 실행 및 시간 관리 변수
// 현재 애니메이션 프레임 표시 시작 시간
uint32_t g_R300_timeStartPause = 0;
// 마지막 애니메이션/활동 시작 시간 (자동 깜빡임 타이머 기준)
uint32_t g_R300_timeLastAnimation = 0;
// 자동 깜빡임 최소 대기 시간 (밀리초)
uint16_t g_R300_timeBlinkMinimum = 5000; // 기본값 5초

// 애니메이션 상태 머신 현재 상태
T_R300_animState_t g_R300_animState = S_IDLE;

// 애니메이션 제어 및 플래그
// 자동 깜빡임 기능 활성화 여부
bool g_R300_autoBlink = true;
// 현재 실행 중인 애니메이션 시퀀스 정보
T_R300_animTable_t g_R300_animEntry;
// 현재 시퀀스 내 프레임 인덱스
int8_t g_R300_animIndex = 0;
// 애니메이션 시퀀스 역방향 재생 여부
bool g_R300_animReverse = false;
// 시퀀스 완료 후 자동 역방향 재생 여부
bool g_R300_autoReverse = false;
// 다음에 재생할 애니메이션 감정 종류
T_R300_emotion_t g_R300_nextEmotion = E_NONE;
// 현재 화면에 표시되는 애니메이션 감정 종류
T_R300_emotion_t g_R300_currentEmotion = E_NONE;

// 표시할 텍스트 문자열 고정 크기 버퍼
char g_R300_textBuffer[G_R300_MAX_TEXT_LENGTH + 1];
// 표시할 텍스트 문자열 포인터 (g_R300_textBuffer 시작 주소)
char* g_R300_pText = nullptr;

// 로봇 상태 관리를 위한 마지막 활동 시간 기록
unsigned long g_R300_lastCommandTime = 0;


// --- 글로벌 함수 정의 (R300_ 로 시작) ---

// (눈 인덱스, 행, 열) 좌표를 FastLED CRGB 배열의 선형 픽셀 인덱스로 변환
// @param p_eye_index 눈의 인덱스 (0: 오른쪽, 1: 왼쪽)
// @param p_row 매트릭스 내 행 (0-7)
// @param p_col 매트릭스 내 열 (0-7)
// @return 해당 픽셀의 FastLED CRGB 배열 내 선형 인덱스. 범위 벗어날 시 0 반환.
uint16_t R300_mapEyePixel(uint8_t p_eye_index, uint8_t p_row, uint8_t p_col) {
    uint16_t v_base_pixel = (p_eye_index == 0) ? G_R300_RIGHT_EYE_START_PIXEL : G_R300_LEFT_EYE_START_PIXEL;
    uint16_t v_pixel_index;

    // 행 우선 지그재그(serpentine) 방식 가정
    if (p_row % 2 == 0) {
        v_pixel_index = v_base_pixel + (p_row * 8) + p_col;
    } else {
        v_pixel_index = v_base_pixel + (p_row * 8) + (7 - p_col);
    }

    // 픽셀 인덱스 범위 확인
    if (v_pixel_index >= G_R300_NEOPIXEL_NUM_LEDS) {
        Serial.print("Error: Mapped pixel index out of bounds: ");
        Serial.println(v_pixel_index);
        return 0;
    }

    return v_pixel_index;
}

// 단일 눈 모양 비트맵 데이터(폰트 문자 인덱스)를 LED 버퍼에 그리기
// @param p_eye_index 눈의 인덱스 (0: 오른쪽, 1: 왼쪽)
// @param p_ch 폰트 문자 인덱스
void R300_drawEye(uint8_t p_eye_index, uint8_t p_ch) {
    // 폰트 문자 인덱스 유효 범위 확인
    if (p_ch >= G_R300_ARRAY_SIZE(g_R300_RobotEyes_Font)) {
        Serial.print("Error: Invalid character index: ");
        Serial.println(p_ch);
        return;
    }

    // PROGMEM에서 비트맵 데이터 읽어오기
    T_R300_SimpleFontChar v_charData;
    memcpy_P(&v_charData, &g_R300_RobotEyes_Font[p_ch], sizeof(T_R300_SimpleFontChar));

    // 비트맵 데이터 기반 픽셀 설정
    for (uint8_t v_col = 0; v_col < G_R300_EYE_COL_SIZE; v_col++) {
        uint8_t v_column_byte = pgm_read_byte(&v_charData.data[v_col]);

        for (uint8_t v_row = 0; v_row < G_R300_DISPLAY_HEIGHT / 2; v_row++) {
            if ((v_column_byte >> v_row) & 0x01) {
                uint16_t v_pixel_idx = R300_mapEyePixel(p_eye_index, v_row, v_col);
                g_R300_leds[v_pixel_idx] = G_R300_EYE_COLOR;
            }
        }
    }
}

// 양쪽 눈을 그리고 LED에 표시
// @param p_R 오른쪽 눈 폰트 인덱스
// @param p_L 왼쪽 눈 폰트 인덱스
void R300_drawEyes(uint8_t p_R, uint8_t p_L) {
    FastLED.clear(); // 버퍼 초기화

    R300_drawEye(0, p_R); // 오른쪽 눈 그리기
    R300_drawEye(1, p_L); // 왼쪽 눈 그리기

    FastLED.show(); // LED에 표시
}

// 지정된 감정에 해당하는 애니메이션 시퀀스 로드
// @param p_e 로드할 감정 종류
// @return 시퀀스 프레임 개수. 찾지 못하면 1 반환 (기본값 중립).
uint8_t R300_loadSequence(T_R300_emotion_t p_e) {
    bool v_found = false;
    for (uint8_t v_i = 0; v_i < G_R300_ARRAY_SIZE(g_R300_lookupTable); v_i++) {
        T_R300_animTable_t v_entry;
        memcpy_P(&v_entry, &g_R300_lookupTable[v_i], sizeof(T_R300_animTable_t));
        if (v_entry.e == p_e) {
            g_R300_animEntry = v_entry;
            v_found = true;
            break;
        }
    }

    // 시퀀스 찾지 못함
    if (!v_found) {
        Serial.print("Warning: Animation sequence not found for emotion: ");
        Serial.println(p_e);
        g_R300_animEntry = {E_NEUTRAL, g_R300_seqBlink, 1}; // 중립 시퀀스 기본값 설정
    }

    // 애니메이션 시작 인덱스 설정 (정방향 또는 역방향)
    if (g_R300_animReverse)
        g_R300_animIndex = g_R300_animEntry.size - 1;
    else
        g_R300_animIndex = 0;

    return (g_R300_animEntry.size);
}

// 현재 시퀀스에서 특정 인덱스의 프레임 데이터 로드
// @param p_pBuf 로드된 프레임 데이터를 저장할 구조체 포인터
void R300_loadFrame(T_R300_animFrame_t* p_pBuf) {
    // 애니메이션 인덱스 유효 범위 확인
    if (g_R300_animIndex >= 0 && g_R300_animIndex < g_R300_animEntry.size) {
        // PROGMEM에서 프레임 데이터 읽어오기
        memcpy_P(p_pBuf, &g_R300_animEntry.seq[g_R300_animIndex], sizeof(T_R300_animFrame_t));
    } else {
         // 유효하지 않은 인덱스 접근 시 오류 처리 및 기본 프레임 설정
         Serial.print("Error: Invalid animation index: ");
         Serial.println(g_R300_animIndex);
         p_pBuf->eyeData[0] = 0;
         p_pBuf->eyeData[1] = 0;
         p_pBuf->timeFrame = G_R300_FRAME_TIME;
    }
}

// 표시할 텍스트 문자열 초기화
void R300_clearText() {
    // 고정 크기 버퍼 초기화
    g_R300_textBuffer[0] = '\0';
}

// CRGB 버퍼에 텍스트 정적 표시 (첫 두 문자)
// @param p_bInit 초기화 플래그 (현재 사용되지 않음)
void R300_showText(bool p_bInit) {
    // 표시할 텍스트 없으면 종료
    if (g_R300_pText == nullptr || g_R300_textBuffer[0] == '\0') {
         R300_clearText();
         g_R300_animState = S_IDLE; // 유휴 상태 복귀
         g_R300_timeLastAnimation = millis(); // 타이머 리셋
         return;
    }

    FastLED.clear(); // 버퍼 초기화

    // 첫 번째 문자를 오른쪽 눈에 표시 (ASCII 값을 폰트 인덱스로 사용)
    R300_drawEye(0, (uint8_t)g_R300_pText[0]);

    // 두 번째 문자가 있으면 왼쪽 눈에 표시 (ASCII 값을 폰트 인덱스로 사용)
    if (g_R300_pText[1] != '\0') {
        R300_drawEye(1, (uint8_t)g_R300_pText[1]);
    }

    FastLED.show(); // LED에 표시
}

// 다음에 표시할 애니메이션 설정
// @param p_e 감정 애니메이션 종류
// @param p_r 시퀀스 완료 후 자동 역재생 여부
// @param p_b 애니메이션 시작 방향 (false: 정방향, true: 역방향)
// @param p_force 현재 상태에 관계없이 즉시 시작 여부
void R300_setAnimation(T_R300_emotion_t p_e, bool p_r, bool p_b, bool p_force) {
    // 텍스트 표시 중이고 강제 시작 아니면 무시
    if (g_R300_pText != nullptr && g_R300_textBuffer[0] != '\0' && !p_force) return;

    // 현재 감정과 다르거나 강제 시작 시 처리
    if (p_e != g_R300_currentEmotion || p_force) {
        g_R300_nextEmotion = p_e;     // 다음에 재생할 감정 설정
        g_R300_autoReverse = p_r;     // 자동 역재생 여부 설정
        g_R300_animReverse = p_b;     // 시작 방향 설정
        // 강제 시작 또는 현재 유휴 상태이면 즉시 재시작 준비
        if (p_force || g_R300_animState == S_IDLE) {
             g_R300_animState = S_RESTART;
        }
    }
}

// 로봇 상태 (AWAKE/SLEEPING) 설정
// 상태 변경 시 해당 애니메이션 자동 트리거
// @param p_s 설정할 새로운 상태
void R300_setState(T_R300_State p_s) {
    // 현재 상태와 다를 경우에만 처리
    if (p_s != g_R300_state) {
        if (p_s == SLEEPING && g_R300_state == AWAKE) {
            // AWAKE -> SLEEPING: 잠자는 애니메이션 시작
            R300_setAnimation(E_SLEEP, false, false, true);
        } else if (p_s == AWAKE && g_R300_state == SLEEPING) {
            // SLEEPING -> AWAKE: 잠 깨는 애니메이션 시작 (역방향)
            R300_setAnimation(E_SLEEP, false, true, true);
        }
        g_R300_state = p_s; // 상태 업데이트
        // Serial.print("State changed to: "); // 상태 변경 시리얼 출력 (옵션)
        // Serial.println(p_s == AWAKE ? "AWAKE" : "SLEEPING");
    }
}

// 외부 명령 문자열 처리 (애니메이션 또는 텍스트)
// 수신된 명령 파싱하여 눈 모양 변경
// @param p_command 처리할 명령 문자열
void R300_processCommand(const char* p_command) {
    R300_clearText(); // 새 명령 처리 전 텍스트 버퍼 초기화

    // 명령 문자열에 따라 애니메이션 설정
    if (strcmp(p_command, "neutral") == 0) {
        R300_setAnimation(E_NEUTRAL, true, false, true);
    } else if (strcmp(p_command, "blink") == 0) {
        R300_setAnimation(E_BLINK, true, false, true);
    } else if (strcmp(p_command, "angry") == 0) {
        R300_setAnimation(E_ANGRY, true, false, true);
    } else if (strcmp(p_command, "sad") == 0) {
        R300_setAnimation(E_SAD, true, false, true);
    } else if (strcmp(p_command, "evil") == 0) {
        R300_setAnimation(E_EVIL, true, false, true);
    } else if (strcmp(p_command, "evil2") == 0) {
        R300_setAnimation(E_EVIL2, true, false, true);
    } else if (strcmp(p_command, "squint") == 0) {
        R300_setAnimation(E_SQUINT, true, false, true);
    } else if (strcmp(p_command, "dead") == 0) {
        R300_setAnimation(E_DEAD, true, false, true);
    } else if (strcmp(p_command, "core") == 0) { // "core" 명령은 E_HEART
        R300_setAnimation(E_HEART, true, false, true);
    } else if (strcmp(p_command, "sleep_anim") == 0) { // 잠자는 애니메이션 명시적 실행
        R300_setAnimation(E_SLEEP, false, false, true);
    }
    // 로봇 상태 직접 변경 명령
    else if (strcmp(p_command, "awake") == 0) {
        R300_setState(AWAKE);
    } else if (strcmp(p_command, "sleeping") == 0) {
        R300_setState(SLEEPING);
    }
    // 위 명령들에 해당하지 않으면 텍스트 표시 명령으로 간주
    else {
        // 명령 문자열을 고정 버퍼로 복사 (최대 길이 제한 및 널 종료 보장)
        strncpy(g_R300_textBuffer, p_command, G_R300_MAX_TEXT_LENGTH);
        g_R300_textBuffer[G_R300_MAX_TEXT_LENGTH] = '\0';
        g_R300_animState = S_TEXT; // 텍스트 표시 상태로 전환
    }
}

// 애니메이션 상태 머신 실행 (main 루프에서 반복 호출)
// 현재 상태에 따라 동작 결정 및 함수 호출
// 로봇이 IDLE 상태이면 true 반환
// @return 현재 로봇 눈 상태가 S_IDLE이면 true, 아니면 false
bool R300_runAnimation(void) {
    static T_R300_animFrame_t v_thisFrame; // 현재 프레임 데이터
    static uint32_t v_timeOfLastFrame = 0; // 현재 프레임 표시 시작 시간

    // 현재 애니메이션 상태에 따라 동작 수행
    switch (g_R300_animState) {
        case S_IDLE:
            // 유휴 상태: 새로운 명령 또는 자동 깜빡임 대기

            // 텍스트 표시 대기 중인지 확인
            if (g_R300_pText != nullptr && g_R300_textBuffer[0] != '\0') {
                g_R300_animState = S_TEXT; // 텍스트 표시 상태로 전환
                break;
            }

            // 새 애니메이션 대기 중인지 확인
            if (g_R300_nextEmotion != E_NONE) {
                 g_R300_animState = S_RESTART; // RESTART 상태로 전환
                 break;
            }

            // 자동 깜빡임 활성화 및 최소 대기 시간 경과 확인
            if (g_R300_autoBlink && (millis() - g_R300_timeLastAnimation) >= g_R300_timeBlinkMinimum) {
                 // 무작위 확률로 깜빡임 트리거 (예: 30% 확률)
                 if (random(1000) > 700) {
                    if (g_R300_state == SLEEPING) {
                        R300_setAnimation(E_SQUINT_BLINK, true, false, true); // 잠자는 상태: 찡그림 깜빡임
                    } else if (g_R300_state == AWAKE) {
                        R300_setAnimation(E_BLINK, true, false, true); // 깨어있는 상태: 일반 깜빡임
                    }
                    g_R300_timeLastAnimation = millis(); // 타이머 리셋
                 } else {
                    g_R300_timeLastAnimation = millis(); // 깜빡임 발생 안 해도 타이머 업데이트
                 }
            }
            // 대기 상태 유지
            break;

        case S_RESTART:
            // 새 애니메이션 시퀀스 로드 및 준비

            // 다음에 재생할 애니메이션 확인
            if (g_R300_nextEmotion != E_NONE) {
                R300_loadSequence(g_R300_nextEmotion); // 시퀀스 로드
                g_R300_currentEmotion = g_R300_nextEmotion; // 현재 감정 업데이트
                g_R300_nextEmotion = E_NONE; // 큐 비우기
                g_R300_animState = S_ANIMATE; // 애니메이션 실행 상태로 전환
            } else {
                 g_R300_animState = S_IDLE; // 오류 또는 논리적 문제 시 유휴 상태 복귀
            }
            break;

        case S_ANIMATE:
            // 현재 프레임 표시 및 다음 프레임 준비

            R300_loadFrame(&v_thisFrame); // 현재 프레임 데이터 로드
            R300_drawEyes(v_thisFrame.eyeData[0], v_thisFrame.eyeData[1]); // 눈 그리기 및 표시
            v_timeOfLastFrame = millis(); // 프레임 표시 시작 시간 기록

            // 애니메이션 재생 방향에 따라 다음 프레임 인덱스 계산
            if (g_R300_animReverse) {
                g_R300_animIndex--; // 역방향
            } else {
                g_R300_animIndex++; // 정방향
            }

            g_R300_animState = S_PAUSE; // 프레임 대기 상태로 이동
            break;

        case S_PAUSE:
            // 현재 프레임 표시 시간만큼 대기

            // 대기 시간 경과 확인
            if ((millis() - v_timeOfLastFrame) < v_thisFrame.timeFrame) {
                break; // 대기 시간 경과 전: 대기 상태 유지
            }

            // 대기 시간 경과: 다음 상태 결정
            // 애니메이션 시퀀스 완료 확인
            if ((!g_R300_animReverse && g_R300_animIndex >= g_R300_animEntry.size) || (g_R300_animReverse && g_R300_animIndex < 0)) {
                // 시퀀스 완료
                if (g_R300_autoReverse) {
                    // 자동 역재생: 동일 시퀀스를 반대 방향으로 다시 시작 준비
                    R300_setAnimation(g_R300_animEntry.e, false, !g_R300_animReverse, true); // 자동 역재생 아님, 반대 방향, 강제 실행
                } else {
                    // 완료: 유휴 상태 복귀
                    g_R300_animState = S_IDLE;
                    g_R300_currentEmotion = E_NONE; // 현재 감정 상태 지움
                    g_R300_timeLastAnimation = millis(); // 타이머 리셋
                }
            } else {
                // 시퀀스 미완료: 다음 프레임 실행 준비
                g_R300_animState = S_ANIMATE; // 애니메이션 실행 상태로 전환
            }
            break;

        case S_TEXT:
            // 텍스트 표시 상태 (정적 표시)

            // 텍스트 버퍼 비워졌는지 확인 (외부 명령 등에 의해)
            if (g_R300_textBuffer[0] == '\0') {
                 g_R300_animState = S_IDLE; // 유휴 상태 복귀
                 g_R300_timeLastAnimation = millis(); // 타이머 리셋
            }
            // 텍스트 버퍼 비워지지 않음: 텍스트 표시 상태 유지
            break;

        default: // 예상치 못한 상태: 초기화 및 유휴 상태 복귀
            g_R300_animState = S_IDLE;
            g_R300_currentEmotion = E_NONE;
            R300_clearText();
            g_R300_timeLastAnimation = millis();
            break;
    }

    // 현재 상태가 S_IDLE이면 true 반환
    return (g_R300_animState == S_IDLE);
}


// 초기화 함수 (Arduino setup 대체)
void R300_init() {
    
    // FastLED 초기화 및 LED 설정
    FastLED.addLeds<G_R300_LED_TYPE, G_R300_NEOPIXEL_PIN, G_R300_COLOR_ORDER>(g_R300_leds, G_R300_NEOPIXEL_NUM_LEDS).setCorrection(TypicalLEDStrip);
    g_R300_leds_ptr = g_R300_leds; // CRGB 배열 주소 포인터에 할당

    FastLED.clear(); // LED 버퍼 초기화 (검은색)
    FastLED.show();  // LED 표시 (초기에는 모든 LED 꺼짐)

    // 로봇 상태 관련 변수 초기화
    g_R300_state = AWAKE; // 초기 상태: 깨어있음
    g_R300_animState = S_IDLE; // 초기 애니메이션 상태: 유휴
    g_R300_autoBlink = true; // 자동 깜빡임 활성화
    g_R300_timeBlinkMinimum = 5000; // 자동 깜빡임 최소 대기 시간 (5초)
    g_R300_timeLastAnimation = millis(); // 타이머 기준 시간 초기화

    // 텍스트 버퍼 초기화 및 포인터 연결
    g_R300_textBuffer[0] = '\0'; // 버퍼 비우기
    g_R300_pText = g_R300_textBuffer; // 포인터가 버퍼 시작 주소 가리키도록 설정

    // 마지막 활동 시간 초기화
    g_R300_lastCommandTime = millis();

    // 시작 시 중립 애니메이션 설정 (runAnimation에서 처리되도록)
    R300_setAnimation(E_NEUTRAL, false, false, true); // 중립 애니메이션 설정 (강제 시작)
}

// 메인 실행 루프 (Arduino loop 대체)
void R300_run() {
    R300_runAnimation(); // 애니메이션/표시 로직 실행

    // 비활성 시간 기준 로봇 상태 변경 로직 예제
    // SLEEPING 상태가 아니고 비활성 시간 경과 시
    if (g_R300_state != SLEEPING && millis() - g_R300_lastCommandTime >= G_R300_TIME_TO_SLEEP_EXAMPLE) {
        R300_setState(SLEEPING); // SLEEPING 상태로 변경 (잠자는 애니메이션 트리거)
    }
    // SLEEPING 상태인데 활동 감지 시
    else if (g_R300_state == SLEEPING && millis() - g_R300_lastCommandTime < G_R300_TIME_TO_SLEEP_EXAMPLE) {
         R300_setState(AWAKE); // AWAKE 상태로 변경 (잠 깨는 애니메이션 트리거)
    }

    // 시리얼 입력 처리 (옵션: 명령 수신 테스트)
    if (Serial.available()) {
        String v_command_string = Serial.readStringUntil('\n'); // 줄바꿈까지 문자열 읽기
        v_command_string.trim(); // 앞뒤 공백 제거
        Serial.print("Received command: "); // 수신 명령 출력
        Serial.println(v_command_string);

        R300_processCommand(v_command_string.c_str()); // 명령 처리
        g_R300_lastCommandTime = millis(); // 활동 시간 업데이트
    }

    // 루프 속도 조절 (필요시)
    // delay(1);
}
