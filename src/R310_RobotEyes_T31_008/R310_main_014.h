
#pragma once

// R310_Main_013.h - 로봇 눈 애니메이션 및 상태 관리의 변수 및 함수 구현 파일
// 이 파일은 필요한 데이터 및 타입 헤더 파일을 포함하고 main.cpp에서 사용됩니다.

//#define R310_DEB
#define R310_PROGRESS_1


// 기본 타입 및 설정 헤더 파일 포함
#include "R310_config_009.h"
// 정적 데이터 테이블 헤더 파일 포함
#include "R310_data2_014.h"

#define G_R310_BUFFEREDSERIAL_USE

#ifdef G_R310_BUFFEREDSERIAL_USE
    #include <StreamUtils.h>
    ReadBufferingStream g_R310_BufferedSerial{Serial, 64};
#endif

// --- 글로벌 변수 정의 (g_R310_ 로 시작) ---

CRGB			        g_R310_leds[G_R310_NEOPIXEL_NUM_LEDS];          // FastLED CRGB 배열

CRGB*			        g_R310_leds_ptr			    = nullptr;	        // CRGB 배열 포인터


T_R310_RobotState_t	    g_R310_robotState		    = R_STATE_AWAKE;    // 현재 로봇 상태

// 애니메이션 실행 및 시간 관리 변수
// 현재 애니메이션 프레임 표시 시작 시간
/// uint32_t g_R310_timeStartPause = 0;

uint32_t		        g_R310_timeLastAnimation    = 0;                // 마지막 애니메이션/활동 시작 시간 (자동 깜빡임 타이머 기준)

uint16_t		        g_R310_timeBlinkMinimum	    = 5000;	            // 자동 깜빡임 최소 대기 시간 (밀리초) 기본값 5초

T_R310_ani_ply_state_t  g_R310_ani_ply_state	    = ANI_PLY_STATE_IDLE; // 애니메이션 상태 머신 현재 상태
     
bool			        g_R310_ani_ply_autoBlink_on			= true;             // 자동 깜빡임 기능 활성화 여부

T_R310_ani_Table_t      g_R310_animEntry;                               // 현재 실행 중인 애니메이션 시퀀스 정보

int8_t			        g_R310_animIndex		    = 0;                // 현재 시퀀스 내 프레임 인덱스
  
EMTP_PlyDirect_t	    g_R310_animReverse	        = EMTP_PLY_DIR_FIRST;     // 애니메이션 시퀀스 역방향 재생 여부
EMTP_AutoReverse_t	    g_R310_autoReverse	        = EMTP_AUTO_REVERSE_OFF;  // 시퀀스 완료 후 자동 역방향 재생 여부

T_R310_emotion_t        g_R310_emotion_next	        = EMT_NONE;         // 다음에 재생할 애니메이션 감정 종류

T_R310_emotion_t        g_R310_emotion_current       = EMT_NONE;         // 현재 화면에 표시되는 애니메이션 감정 종류

char			        g_R310_textBuffer[G_R310_MAX_TEXT_LENGTH + 1];  // 표시할 텍스트 문자열 고정 크기 버퍼

char*			        g_R310_pText			    = nullptr;          // 표시할 텍스트 문자열 포인터 (g_R310_textBuffer 시작 주소)


unsigned long	        g_R310_lastCommandTime = 0;                     // 로봇 상태 관리를 위한 마지막 활동 시간 기록

// ====================================================================================================
// 함수 선언 (프로토타입)
// ====================================================================================================
 // R310_Main_013.h 내부 함수 호출 관계 트리 

// main.cpp 
// ├── R310_init()
// │   └── R310_setAnimation()
// │
// └── R310_run()
//     ├── R310_runAnimation()
//     │   ├── R310_clearText()
//     │   ├── R310_setAnimation()
//     │   ├── R310_loadSequence()
//     │   ├── R310_loadFrame()
//     │   ├── R310_drawEyes()
//     │       ├── R310_drawEye()
//     │           └── R310_mapEyePixel()
//     │
//     ├── R310_set_RobotState()
//     │   └── R310_setAnimation()
//     │
//     ├── R310_processCommand()
//         ├── R310_clearText()
//         ├── R310_setAnimation()
//         ├── R310_set_RobotState()



/*
 * 독립적으로 호출될 수 있는 함수:
 */
// R310_showText()
// ├── R310_clearText()
// ├── R310_drawEye()
// └── (외부 함수 호출: FastLED.clear(), FastLED.show())

uint16_t R310_mapEyePixel(T_R310_EyeSide_Idx_t p_eyeSide_idx, uint8_t p_row, uint8_t p_col);
void     R310_drawEye(T_R310_EyeSide_Idx_t p_eyeSide_idx, uint8_t p_eye_font_idx) ;
void     R310_drawEyes(uint8_t p_eye_font_idx_Right, uint8_t p_eye_font_idx_Left);
uint8_t  R310_loadSequence(T_R310_emotion_t p_eyeEmotion_idx);
void     R310_loadFrame(T_R310_animFrame_t* p_pBuf) ;

void     R310_clearText();
void     R310_showText(bool p_bInit);

void     R310_setAnimation(T_R310_emotion_t p_e, EMTP_AutoReverse_t p_r, EMTP_PlyDirect_t p_b, EMTP_ForcePly_t p_force);
void     R310_set_RobotState(T_R310_RobotState_t p_robotState);
void     R310_processCommand(const char* p_command);

bool     R310_runAnimation(void);
void     R310_init() ;
void     R310_run() ;

// ====================================================================================================
// 함수 정의 (R310_ 로 시작)
// ====================================================================================================

// (눈 인덱스, 행, 열) 좌표를 FastLED CRGB 배열의 선형 픽셀 인덱스로 변환
// @param p_eyeSide_idx 눈의 인덱스 (0: 오른쪽, 1: 왼쪽)
// @param p_row 매트릭스 내 행 (0-7)
// @param p_col 매트릭스 내 열 (0-7)
// @return 해당 픽셀의 FastLED CRGB 배열 내 선형 인덱스. 범위 벗어날 시 0 반환.
//T_R310_Eye_Idx
uint16_t R310_mapEyePixel(T_R310_EyeSide_Idx_t p_eyeSide_idx, uint8_t p_row, uint8_t p_col) {
    uint16_t v_base_pixel = (p_eyeSide_idx == EYE_RIGHT) ? G_R310_RIGHT_EYE_START_PIXEL : G_R310_LEFT_EYE_START_PIXEL;
    uint16_t v_pixel_index;

    // 행 우선 지그재그(serpentine) 방식 가정
    if (p_row % 2 == 0) {
        v_pixel_index = v_base_pixel + (p_row * 8) + p_col;
    } else {
        v_pixel_index = v_base_pixel + (p_row * 8) + (7 - p_col);
    }

    // 픽셀 인덱스 범위 확인
    if (v_pixel_index >= G_R310_NEOPIXEL_NUM_LEDS) {
        Serial.print("Error: Mapped pixel index out of bounds: ");
        Serial.println(v_pixel_index);
        return 0;
    }

    return v_pixel_index;
}

// 단일 눈 모양 비트맵 데이터(폰트 문자 인덱스)를 LED 버퍼에 그리기
// 데이터는 행 우선(Row Major) 형식으로 저장됨을 가정하고 읽습니다.
// @param p_eyeSide_idx 눈의 인덱스 (0: 오른쪽, 1: 왼쪽)
// @param p_eye_font_idx 폰트 문자 인덱스
void R310_drawEye(T_R310_EyeSide_Idx_t p_eyeSide_idx, uint8_t p_eye_font_idx) {
    // 폰트 문자 인덱스 유효 범위 확인
    if (p_eye_font_idx >= G_R310_ARRAY_SIZE(g_R310_RobotEyes_Font)) {
        Serial.print("Error: Invalid Eye Font index: ");
        Serial.println(p_eye_font_idx);
        return;
    }

    // PROGMEM에서 비트맵 데이터 읽어오기
    T_R310_FontChar v_charData;
    memcpy_P(&v_charData, &g_R310_RobotEyes_Font[p_eye_font_idx], sizeof(T_R310_FontChar));

    // 읽어온 행 우선(Row Major) 비트맵 데이터 기반 픽셀 설정
    // v_charData.data[row]는 해당 행의 8개 픽셀 비트맵을 나타냅니다.
    // 각 바이트의 비트는 MSB(왼쪽)부터 LSB(오른쪽) 순서로 열(Col 0 ~ Col 7)을 나타냅니다.
    for (uint8_t v_row = 0; v_row < G_R310_DISPLAY_HEIGHT; v_row++) { // 각 행(0-7)을 반복합니다.
    /////for (uint8_t v_row = 0; v_row < G_R310_DISPLAY_HEIGHT / 2; v_row++) { // 각 행(0-7)을 반복합니다.
        // 현재 행에 해당하는 8열의 비트맵 데이터(1바이트)를 읽어옵니다.
        uint8_t v_row_byte = pgm_read_byte(&v_charData.data[v_row]); // 이제 data[row]가 행 데이터입니다.

        for (uint8_t v_col = 0; v_col < G_R310_EYE_COL_SIZE; v_col++) { // 각 열(0-7)을 반복합니다.
            // 현재 행의 비트맵 바이트(v_row_byte)에서 해당 열에 해당하는 비트가 1인지 확인합니다.
            // 비트 순서는 MSB(Col 0) -> LSB(Col 7) 이므로, v_col에 해당하는 비트는 (7 - v_col) 위치에 있습니다.
            if ((v_row_byte >> (7 - v_col)) & 0x01) {
                // 만약 비트가 1이면 (픽셀을 켜야 하면), 해당 픽셀의 (눈 인덱스, 행, 열) 좌표를 선형 픽셀 인덱스로 변환합니다.
                uint16_t v_pixel_idx = R310_mapEyePixel(p_eyeSide_idx, v_row, v_col);
                // 계산된 픽셀 인덱스(g_R310_leds 배열의 위치)에 미리 정의된 눈 색상(G_R310_EYE_COLOR)을 설정합니다.
                g_R310_leds[v_pixel_idx] = G_R310_EYE_COLOR; // 정의된 눈 색상으로 설정
            }
            // 만약 비트가 0이면 (픽셀을 꺼야 하면), 해당 픽셀은 그대로 유지됩니다. (FastLED.clear()로 미리 검은색으로 설정됨)
        }
    }
}


// 오른쪽 눈(R)과 왼쪽 눈(L)에 사용할 폰트 문자 인덱스를 받아, 해당 눈 모양을 CRGB 버퍼에 그리고 FastLED.show()로 표시합니다.
// @param p_eye_font_idx_Right 오른쪽 눈에 사용할 폰트 문자 인덱스.
// @param p_eye_font_idx_Left 왼쪽 눈에 사용할 폰트 문자 인덱스.
void R310_drawEyes(uint8_t p_eye_font_idx_Right, uint8_t p_eye_font_idx_Left) {
    FastLED.clear(); // 전체 LED 픽셀 버퍼를 검은색으로 초기화합니다.

    R310_drawEye(EYE_RIGHT, p_eye_font_idx_Right); // 오른쪽 눈 그리기
    R310_drawEye(EYE_LEFT, p_eye_font_idx_Left);  // 왼쪽 눈 그리기

    FastLED.show(); // LED에 표시
}

// 지정된 감정에 해당하는 애니메이션 시퀀스 로드
// @param p_eyeEmotion_idx 로드할 감정 종류
// @return 시퀀스 프레임 개수. 찾지 못하면 1 반환 (기본값 중립).
uint8_t R310_loadSequence(T_R310_emotion_t p_eyeEmotion_idx) {
    bool v_found = false;

    for (uint8_t v_i = 0; v_i < G_R310_ARRAY_SIZE(g_R310_ani_Table); v_i++) {
        T_R310_ani_Table_t v_entry;
        memcpy_P(&v_entry, &g_R310_ani_Table[v_i], sizeof(T_R310_ani_Table_t));
        if (v_entry.emotion_idx == p_eyeEmotion_idx) {
            g_R310_animEntry = v_entry;
            v_found = true;
            break;
        }
    }

    // 시퀀스 찾지 못함
    if (!v_found) {
        Serial.print("Warning: Animation sequence not found for emotion: ");
        Serial.println(p_eyeEmotion_idx);
        g_R310_animEntry = {EMT_NEUTRAL, g_R310_seqBlink, 1}; // 중립 시퀀스 기본값 설정
    }

    // 애니메이션 시작 인덱스 설정 (정방향 또는 역방향)
    if (g_R310_animReverse == EMTP_PLY_DIR_LAST)
        g_R310_animIndex = g_R310_animEntry.seq_size - 1;
    else
        g_R310_animIndex = 0;

    return (g_R310_animEntry.seq_size);
}

// 현재 시퀀스에서 특정 인덱스의 프레임 데이터 로드
// @param p_pBuf 로드된 프레임 데이터를 저장할 구조체 포인터
void R310_loadFrame(T_R310_animFrame_t* p_pBuf) {
    // 애니메이션 인덱스 유효 범위 확인
    if (g_R310_animIndex >= 0 && g_R310_animIndex < g_R310_animEntry.seq_size) {
        // PROGMEM에서 프레임 데이터 읽어오기
        memcpy_P(p_pBuf, &g_R310_animEntry.seq[g_R310_animIndex], sizeof(T_R310_animFrame_t));
    } else {
         // 유효하지 않은 인덱스 접근 시 오류 처리 및 기본 프레임 설정
         Serial.print("Error: Invalid animation index: ");
         Serial.println(g_R310_animIndex);
         p_pBuf->eyeData[0] = 0;
         p_pBuf->eyeData[1] = 0;
         p_pBuf->timeFrame = G_R310_FRAME_TIME;
    }
}

// 표시할 텍스트 문자열 초기화
void R310_clearText() {
    // 고정 크기 버퍼 초기화
    g_R310_textBuffer[0] = '\0';
}

// CRGB 버퍼에 텍스트 정적 표시 (첫 두 문자)
// @param p_bInit 초기화 플래그 (현재 사용되지 않음)
void R310_showText(bool p_bInit) {
    // 표시할 텍스트 없으면 종료
    if (g_R310_pText == nullptr || g_R310_textBuffer[0] == '\0') {
         R310_clearText();
         g_R310_ani_ply_state = ANI_PLY_STATE_IDLE; // 유휴 상태 복귀
         g_R310_timeLastAnimation = millis(); // 타이머 리셋
         return;
    }

    FastLED.clear(); // 버퍼 초기화

    // 첫 번째 문자를 오른쪽 눈에 표시 (ASCII 값을 폰트 인덱스로 사용)
    R310_drawEye(EYE_RIGHT, (uint8_t)g_R310_pText[0]);

    // 두 번째 문자가 있으면 왼쪽 눈에 표시 (ASCII 값을 폰트 인덱스로 사용)
    if (g_R310_pText[1] != '\0') {
        R310_drawEye(EYE_LEFT, (uint8_t)g_R310_pText[1]);
    }

    FastLED.show(); // LED에 표시
}

// 다음에 표시할 애니메이션 설정
// @param p_e 감정 애니메이션 종류
// @param p_r 시퀀스 완료 후 자동 역재생 여부
// @param p_b 애니메이션 시작 방향 (false: 정방향, true: 역방향)
// @param p_force 현재 상태에 관계없이 즉시 시작 여부
void R310_setAnimation(T_R310_emotion_t p_e, EMTP_AutoReverse_t p_r, EMTP_PlyDirect_t p_b, EMTP_ForcePly_t p_force) {
    // 텍스트 표시 중이고 강제 시작 아니면 무시
    if (g_R310_pText != nullptr && g_R310_textBuffer[0] != '\0' && !p_force) return;

    // 현재 감정과 다르거나 강제 시작 시 처리
    if (p_e != g_R310_emotion_current || p_force) {
        g_R310_emotion_next = p_e;     // 다음에 재생할 감정 설정
        g_R310_autoReverse = p_r;     // 자동 역재생 여부 설정
        g_R310_animReverse = p_b;     // 시작 방향 설정

        // 강제 시작 또는 현재 유휴 상태이면 즉시 재시작 준비
        if (p_force == EMTP_FORCE_PLY_ON || g_R310_ani_ply_state == ANI_PLY_STATE_IDLE) {
             g_R310_ani_ply_state = ANI_PLY_STATE_RESTART;
        }
    }
}

// 로봇 상태 (R_STATE_AWAKE/R_STATE_SLEEPING) 설정
// 상태 변경 시 해당 애니메이션 자동 트리거
// @param p_s 설정할 새로운 상태
void R310_set_RobotState(T_R310_RobotState_t p_robotState) {
    // 현재 상태와 다를 경우에만 처리
    if (p_robotState != g_R310_robotState) {
        if (p_robotState == R_STATE_SLEEPING && g_R310_robotState == R_STATE_AWAKE) {
            // R_STATE_AWAKE -> R_STATE_SLEEPING: 잠자는 애니메이션 시작
            R310_setAnimation(EMT_SLEEP_BLINK, EMTP_AUTO_REVERSE_OFF, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
        } else if (p_robotState == R_STATE_AWAKE && g_R310_robotState == R_STATE_SLEEPING) {
            // R_STATE_SLEEPING -> R_STATE_AWAKE: 잠 깨는 애니메이션 시작 (역방향)
            R310_setAnimation(EMT_SLEEP, EMTP_AUTO_REVERSE_OFF, EMTP_PLY_DIR_LAST, EMTP_FORCE_PLY_ON);
        }
        g_R310_robotState = p_robotState; // 상태 업데이트
        // Serial.print("State changed to: "); // 상태 변경 시리얼 출력 (옵션)
        // Serial.println(p_robotState == R_STATE_AWAKE ? "R_STATE_AWAKE" : "R_STATE_SLEEPING");
    }
}

// 외부 명령 문자열 처리 (애니메이션 또는 텍스트)
// 수신된 명령 파싱하여 눈 모양 변경
// @param p_command 처리할 명령 문자열
void R310_processCommand(const char* p_command) {
    R310_clearText(); // 새 명령 처리 전 텍스트 버퍼 초기화

    // 명령 문자열에 따라 애니메이션 설정
    if (strcmp(p_command, "neutral") == 0) {
        R310_setAnimation(EMT_NEUTRAL, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    } else if (strcmp(p_command, "blink") == 0) {
        R310_setAnimation(EMT_BLINK, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    
    } else if (strcmp(p_command, "wink") == 0) {
        R310_setAnimation(EMT_WINK, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);

    } else if (strcmp(p_command, "left") == 0) {
        R310_setAnimation(EMT_LOOK_L, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    } else if (strcmp(p_command, "right") == 0) {
        R310_setAnimation(EMT_LOOK_R, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    } else if (strcmp(p_command, "up") == 0) {
        R310_setAnimation(EMT_LOOK_U, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    } else if (strcmp(p_command, "down") == 0) {
        R310_setAnimation(EMT_LOOK_D, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    
    } else if (strcmp(p_command, "updown") == 0) {
        R310_setAnimation(EMT_SCAN_UD, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    } else if (strcmp(p_command, "leftright") == 0) {
        R310_setAnimation(EMT_SCAN_LR, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
	} else if (strcmp(p_command, "angry") == 0) {
        R310_setAnimation(EMT_ANGRY2, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
	} else if (strcmp(p_command, "smile") == 0) {
        R310_setAnimation(EMT_SMILE, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
	} else if (strcmp(p_command, "sleep") == 0) { // 잠자는 애니메이션 명시적 실행
        R310_setAnimation(EMT_SLEEP, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
	} else if (strcmp(p_command, "sleepblink") == 0) { // 잠자는 애니메이션 명시적 실행
        R310_setAnimation(EMT_SLEEP_BLINK, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    }
	
    // 로봇 상태 직접 변경 명령
    else if (strcmp(p_command, "awake") == 0) {
        R310_set_RobotState(R_STATE_AWAKE);
    } else if (strcmp(p_command, "sleeping") == 0) {
        R310_set_RobotState(R_STATE_SLEEPING);
    }
/*
    } else if (strcmp(p_command, "angry") == 0) {
        R310_setAnimation(EMT_ANGRY, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    } else if (strcmp(p_command, "sad") == 0) {
        R310_setAnimation(EMT_SAD, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    } else if (strcmp(p_command, "evil") == 0) {
        R310_setAnimation(EMT_EVIL, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    } else if (strcmp(p_command, "evil2") == 0) {
        R310_setAnimation(EMT_EVIL2, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    } else if (strcmp(p_command, "squint") == 0) {
        R310_setAnimation(EMT_SQUINT, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    } else if (strcmp(p_command, "dead") == 0) {
        R310_setAnimation(EMT_DEAD, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    } else if (strcmp(p_command, "core") == 0) { // "core" 명령은 E_HEART
        R310_setAnimation(EMT_HEART, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
    
*/

	
    // 위 명령들에 해당하지 않으면 텍스트 표시 명령으로 간주
    else {
        // 명령 문자열을 고정 버퍼로 복사 (최대 길이 제한 및 널 종료 보장)
        strncpy(g_R310_textBuffer, p_command, G_R310_MAX_TEXT_LENGTH);
        g_R310_textBuffer[G_R310_MAX_TEXT_LENGTH] = '\0';
        g_R310_ani_ply_state = ANI_PLY_STATE_TEXT; // 텍스트 표시 상태로 전환
    }
}

// 애니메이션 상태 머신 실행 (main 루프에서 반복 호출)
// 현재 상태에 따라 동작 결정 및 함수 호출
// 로봇이 IDLE 상태이면 true 반환
// @return 현재 로봇 눈 상태가 S_IDLE이면 true, 아니면 false
bool R310_runAnimation(void) {
    static T_R310_animFrame_t   v_thisFrame;            // 현재 프레임 데이터
    static uint32_t             v_timeOfLastFrame = 0;  // 현재 프레임 표시 시작 시간

    // 현재 애니메이션 상태에 따라 동작 수행
    switch (g_R310_ani_ply_state) {
        case ANI_PLY_STATE_IDLE:
            // 유휴 상태: 새로운 명령 또는 자동 깜빡임 대기

            // 텍스트 표시 대기 중인지 확인
            if (g_R310_pText != nullptr && g_R310_textBuffer[0] != '\0') {
                g_R310_ani_ply_state = ANI_PLY_STATE_TEXT; // 텍스트 표시 상태로 전환
                break;
            }

            // 새 애니메이션 대기 중인지 확인
            if (g_R310_emotion_next != EMT_NONE) {
                 g_R310_ani_ply_state = ANI_PLY_STATE_RESTART; // RESTART 상태로 전환
                 break;
            }

            // 자동 깜빡임 활성화 및 최소 대기 시간 경과 확인
            if (g_R310_ani_ply_autoBlink_on && (millis() - g_R310_timeLastAnimation) >= g_R310_timeBlinkMinimum) {
                 // 최소 대기 시간 경과 후 무작위 확률(예: 30% 확률)로 깜빡임을 트리거합니다.
                 if (random(1000) > 700) {
                    if (g_R310_robotState == R_STATE_SLEEPING) {					
						R310_setAnimation(EMT_SLEEP_BLINK, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_LAST, EMTP_FORCE_PLY_ON); // 잠자는 상태: 찡그림 깜빡임
						//R310_setAnimation(EMT_SQUINT_BLINK, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON); // 잠자는 상태: 찡그림 깜빡임
                        //R310_setAnimation(EMT_BLINK, true, false, true); 
                    } else if (g_R310_robotState == R_STATE_AWAKE) {
                        R310_setAnimation(EMT_BLINK, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON); // 깨어있는 상태: 일반 깜빡임
                    }
                    g_R310_timeLastAnimation = millis(); // 타이머 리셋
                 } else {
                    g_R310_timeLastAnimation = millis(); // 깜빡임 발생 안 해도 타이머 업데이트
                 }
            }
            // 대기 상태 유지
            break;

        case ANI_PLY_STATE_RESTART:
            // 새 애니메이션 시퀀스 로드 및 준비

            // 다음에 재생할 애니메이션 확인
            if (g_R310_emotion_next != EMT_NONE) {

                R310_loadSequence(g_R310_emotion_next); // 시퀀스 로드

                g_R310_emotion_current = g_R310_emotion_next; // 현재 감정 업데이트
                g_R310_emotion_next = EMT_NONE; // 큐 비우기

                g_R310_ani_ply_state = ANI_PLY_STATE_ANIMATE; // 애니메이션 실행 상태로 전환

            } else {
                 g_R310_ani_ply_state = ANI_PLY_STATE_IDLE; // 오류 또는 논리적 문제 시 유휴 상태 복귀
            }
            break;

        case ANI_PLY_STATE_ANIMATE:
            // 현재 프레임 표시 및 다음 프레임 준비

            R310_loadFrame(&v_thisFrame); // 현재 프레임 데이터 로드
            R310_drawEyes(v_thisFrame.eyeData[0], v_thisFrame.eyeData[1]); // 눈 그리기 및 표시
            v_timeOfLastFrame = millis(); // 프레임 표시 시작 시간 기록

            // 애니메이션 재생 방향에 따라 다음 프레임 인덱스 계산
            if (g_R310_animReverse) {
                g_R310_animIndex--; // 역방향
            } else {
                g_R310_animIndex++; // 정방향
            }

            g_R310_ani_ply_state = ANI_PLY_STATE_PAUSE; // 프레임 대기 상태로 이동
            break;

        case ANI_PLY_STATE_PAUSE:
            // 현재 프레임 표시 시간만큼 대기

            // 대기 시간 경과 확인
            if ((millis() - v_timeOfLastFrame) < v_thisFrame.timeFrame) {
                break; // 대기 시간 경과 전: 대기 상태 유지
            }

            // 대기 시간 경과: 다음 상태 결정
            // 애니메이션 시퀀스 완료 확인
            if ((!g_R310_animReverse && g_R310_animIndex >= g_R310_animEntry.seq_size) || (g_R310_animReverse && g_R310_animIndex < 0)) {
                // 시퀀스 완료
                if (g_R310_autoReverse) {
                    // 자동 역재생: 동일 시퀀스를 반대 방향으로 다시 시작 준비
					EMTP_PlyDirect_t   v_emtp_ply_dir;
					if( g_R310_animReverse == EMTP_PLY_DIR_LAST){
                       v_emtp_ply_dir  = EMTP_PLY_DIR_FIRST;
					} else {
						v_emtp_ply_dir  = EMTP_PLY_DIR_LAST;
					}
                    R310_setAnimation(g_R310_animEntry.emotion_idx, EMTP_AUTO_REVERSE_OFF, v_emtp_ply_dir, EMTP_FORCE_PLY_ON); // 자동 역재생 아님, 반대 방향, 강제 실행
                    //R310_setAnimation(g_R310_animEntry.emotion_idx, EMT_AUTO_REVERSE_OFF, !g_R310_animReverse, EMT_FORCE_PLY_ON); // 자동 역재생 아님, 반대 방향, 강제 실행
                } else {
                    // 완료: 유휴 상태 복귀
                    g_R310_ani_ply_state = ANI_PLY_STATE_IDLE;
                    g_R310_emotion_current = EMT_NONE; // 현재 감정 상태 지움
                    g_R310_timeLastAnimation = millis(); // 타이머 리셋
                }
            } else {
                // 시퀀스 미완료: 다음 프레임 실행 준비
                g_R310_ani_ply_state = ANI_PLY_STATE_ANIMATE; // 애니메이션 실행 상태로 전환
            }
            break;

        case ANI_PLY_STATE_TEXT:
            // 텍스트 표시 상태 (정적 표시)

            // 텍스트 버퍼 비워졌는지 확인 (외부 명령 등에 의해)
            if (g_R310_textBuffer[0] == '\0') {
                 g_R310_ani_ply_state = ANI_PLY_STATE_IDLE; // 유휴 상태 복귀
                 g_R310_timeLastAnimation = millis(); // 타이머 리셋
            }
            // 텍스트 버퍼 비워지지 않음: 텍스트 표시 상태 유지
            break;

        default: // 예상치 못한 상태: 초기화 및 유휴 상태 복귀
            g_R310_ani_ply_state = ANI_PLY_STATE_IDLE;
            g_R310_emotion_current = EMT_NONE;
            R310_clearText();
            g_R310_timeLastAnimation = millis();
            break;
    }

    // 현재 상태가 S_IDLE이면 true 반환
    return (g_R310_ani_ply_state == ANI_PLY_STATE_IDLE);
}


// 초기화 함수 (Arduino setup 대체)
void R310_init() {
    #ifdef R310_DEB
        Serial.println("R310_init - 100");
    #endif

    FastLED.setBrightness(20);

	   // limit my draw to 1A at 5v of power draw
    FastLED.setMaxPowerInVoltsAndMilliamps(5,1000); 
   
	// limit my draw to 5W로 전력제한
    // FastLED.setMaxPowerInMilliWatts(5000);

    // FastLED 초기화 및 LED 설정
    FastLED.addLeds<G_R310_LED_TYPE, G_R310_NEOPIXEL_PIN, G_R310_COLOR_ORDER>(g_R310_leds, G_R310_NEOPIXEL_NUM_LEDS).setCorrection(TypicalLEDStrip);
    g_R310_leds_ptr = g_R310_leds; // CRGB 배열 주소 포인터에 할당

    FastLED.clear(); // LED 버퍼 초기화 (검은색)
    FastLED.show();  // LED 표시 (초기에는 모든 LED 꺼짐)

    #ifdef R310_DEB
        Serial.println("R310_init - 110");
    #endif


    // 로봇 상태 관련 변수 초기화
    g_R310_robotState           = R_STATE_AWAKE;    // 초기 상태: 깨어있음
    g_R310_ani_ply_state            = ANI_PLY_STATE_IDLE;   // 초기 애니메이션 상태: 유휴
    // g_R310_ani_ply_autoBlink_on            = true;     // 자동 깜빡임 활성화
    g_R310_timeBlinkMinimum     = 5000;     // 자동 깜빡임 최소 대기 시간 (5초)
    g_R310_timeLastAnimation    = millis(); // 타이머 기준 시간 초기화



    // 텍스트 버퍼 초기화 및 포인터 연결
    g_R310_textBuffer[0]        = '\0';              // 버퍼 비우기
    g_R310_pText                = g_R310_textBuffer; // 포인터가 버퍼 시작 주소 가리키도록 설정

    // 로봇 상태 관리를 위한 마지막 활동 시간 기록 변수를 현재 시간으로 초기화합니다.
    g_R310_lastCommandTime      = millis();

    #ifdef R310_DEB
        Serial.println("R310_init - 120");
    #endif
    

    // 시작 시 중립 애니메이션 설정 (runAnimation에서 처리되도록)
    R310_setAnimation(EMT_NEUTRAL, EMTP_AUTO_REVERSE_OFF, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON); // 중립 애니메이션 설정 (강제 시작)

    #ifdef R310_DEB
        Serial.println("R310_init - 140");
    #endif

    
}

// 메인 실행 루프
void R310_run() {

    #ifdef R310_DEB
        Serial.println("R310_run - 100");
    #endif

    R310_runAnimation(); // 애니메이션/표시 로직 실행

    #ifdef R310_DEB
        Serial.println("R310_run - 110");
    #endif

    // 비활성 시간 기준 로봇 상태 변경 로직 예제
    // R_STATE_SLEEPING 상태가 아니고 비활성 시간 경과 시
    if (g_R310_robotState != R_STATE_SLEEPING && millis() - g_R310_lastCommandTime >= G_R310_TIME_TO_SLEEP) {
        R310_set_RobotState(R_STATE_SLEEPING); // R_STATE_SLEEPING 상태로 변경 (잠자는 애니메이션 트리거)
    }
    // R_STATE_SLEEPING 상태인데 활동 감지 시
    else if (g_R310_robotState == R_STATE_SLEEPING && millis() - g_R310_lastCommandTime < G_R310_TIME_TO_SLEEP) {
         R310_set_RobotState(R_STATE_AWAKE); // R_STATE_AWAKE 상태로 변경 (잠 깨는 애니메이션 트리거)
    }
	
    #ifdef G_R310_BUFFEREDSERIAL_USE	
        // 시리얼 입력 처리 (옵션: 명령 수신 테스트)
    	if (g_R310_BufferedSerial.available()) {
            String v_command_string = g_R310_BufferedSerial.readStringUntil('\n'); // 줄바꿈까지 문자열 읽기
            v_command_string.trim(); // 앞뒤 공백 제거
            Serial.print("Received command: "); // 수신 명령 출력
            Serial.println(v_command_string);

            R310_processCommand(v_command_string.c_str()); // 명령 처리
            g_R310_lastCommandTime = millis(); // 활동 시간 업데이트
         }
	#elif
        if (Serial.available()) {
            String v_command_string = Serial.readStringUntil('\n'); // 줄바꿈까지 문자열 읽기
            v_command_string.trim(); // 앞뒤 공백 제거
            Serial.print("Received command: "); // 수신 명령 출력
            Serial.println(v_command_string);

            R310_processCommand(v_command_string.c_str()); // 명령 처리
            g_R310_lastCommandTime = millis(); // 활동 시간 업데이트
        }
	#endif

    #ifdef R310_DEB
        Serial.println("R310_run - 120");
    #endif 
        // 루프 속도 조절 (필요시)
    //delay(1);

    delay(1);
}
