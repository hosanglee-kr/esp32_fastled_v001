// R310_Main_015.h

#pragma once

// R310_Main_015.h - 로봇 눈 애니메이션 및 상태 관리의 변수 및 함수 구현 파일
// 이 파일은 필요한 데이터 및 타입 헤더 파일을 포함하고 main.cpp에서 사용됩니다.

//#define R310_DEB
#define R310_PROGRESS_1


// 기본 타입 및 설정 헤더 파일 포함
#include "R310_config_009.h"
// 정적 데이터 테이블 헤더 파일 포함
#include "R310_data2_014.h"

// StreamUtils.h 포함 (필요시)
#ifdef G_R310_BUFFEREDSERIAL_USE
    #include <StreamUtils.h>
    ReadBufferingStream g_R310_BufferedSerial{Serial, 64};
#endif


// --- 새로운 구조체 정의 (R310_config_009.h 또는 이 파일 상단에 위치) ---
// 애니메이션 제어 구조체
typedef struct {
    T_R310_ani_ply_state_t  playState;          // 애니메이션 상태 머신 현재 상태
    bool                    autoBlinkOn;        // 자동 깜빡임 기능 활성화 여부
    T_R310_ani_Table_t      currentAniEntry;   // 현재 실행 중인 애니메이션 시퀀스 정보
    int8_t                  aniFrameIndex;     // 현재 시퀀스 내 프레임 인덱스
    EMTP_Ply_Direct_t       playDirection;      // 애니메이션 시퀀스 재생 방향
    EMTP_Ply_AutoReverse_t  autoReverse;        // 시퀀스 완료 후 자동 역방향 재생 여부
    T_R310_emotion_idx_t     nextEmotion;        // 다음에 재생할 애니메이션 감정 종류
    T_R310_emotion_idx_t     currentEmotion;     // 현재 화면에 표시되는 애니메이션 감정 종류
} T_R310_AnimationControl_t;

// 로봇 상태 및 타이밍 구조체
typedef struct {
    T_R310_RobotState_t robotState;         // 현재 로봇 상태
    uint32_t            lastAnimationTime;  // 마지막 애니메이션/활동 시작 시간 (자동 깜빡임 타이머 기준)
    uint16_t            blinkMinimumTime;   // 자동 깜빡임 최소 대기 시간 (밀리초)
    unsigned long       lastActivityTime;   // 로봇 상태 관리를 위한 마지막 활동 시간 기록
} T_R310_StatusAndTiming_t;

// 텍스트 표시 구조체
typedef struct {
    char    buffer[G_R310_MAX_TEXT_LENGTH + 1]; // 표시할 텍스트 문자열 고정 크기 버퍼
    char* pointer;                            // 표시할 텍스트 문자열 포인터 (buffer 시작 주소)
} T_R310_TextDisplay_t;


// --- 글로벌 변수 정의 (g_R310_ 로 시작) ---

CRGB			        g_R310_leds[G_R310_NEOPIXEL_NUM_LEDS];          // FastLED CRGB 배열
CRGB*			        g_R310_ledsPtr			    = nullptr;	        // CRGB 배열 포인터

T_R310_AnimationControl_t g_R310_aniControl;   // 애니메이션 제어 관련 변수
T_R310_StatusAndTiming_t  g_R310_robotStatus;        // 로봇 상태 및 타이밍 관련 변수
T_R310_TextDisplay_t      g_R310_textDisplay;   // 텍스트 표시 관련 변수

// ====================================================================================================
// 함수 선언 (프로토타입) - 파라미터 타입명도 변경된 열거형/구조체 명칭에 맞게 수정
// ====================================================================================================

uint16_t R310_mapEyePixel(T_R310_EyeSide_Idx_t p_eyeSideIdx, uint8_t p_row, uint8_t p_col);
void     R310_drawEye(T_R310_EyeSide_Idx_t p_eyeSideIdx, uint8_t p_eyeFontIdx) ;
void     R310_drawEyes(uint8_t p_eyeFontIdxRight, uint8_t p_eyeFontIdxLeft);
uint8_t  R310_loadSequence(T_R310_emotion_idx_t p_eyeEmotionIdx);
void     R310_loadFrame(T_R310_animFrame_t* p_animFrame) ;

void     R310_clearText();
void     R310_showText(bool p_bInit);

void     R310_setAnimation(T_R310_emotion_idx_t p_emotionIdx, EMTP_Ply_AutoReverse_t p_autoReverse, EMTP_Ply_Direct_t p_playDirection, EMTP_Ply_Force_t p_forcePlay);
void     R310_setRobotState(T_R310_RobotState_t p_robotState);
void     R310_processCommand(const char* p_command);

bool     R310_runAnimation(void);
void     R310_init() ;
void     R310_run() ;

// ====================================================================================================
// 함수 정의 (R310_ 로 시작) - 내부 로직에서 전역 변수 및 파라미터 사용 시 변경
// ====================================================================================================

// R310_mapEyePixel 함수는 기존과 동일하게 유지

// R310_drawEye 함수
void R310_drawEye(T_R310_EyeSide_Idx_t p_eyeSideIdx, uint8_t p_eyeFontIdx) {
    if (p_eyeFontIdx >= G_R310_ARRAY_SIZE(g_R310_RobotEyes_Font_arr)) { // 변경된 배열명
        Serial.print("Error: Invalid Eye Font index: ");
        Serial.println(p_eyeFontIdx);
        return;
    }
    T_R310_FontChar_t v_fontChar; // 변경된 구조체명
    memcpy_P(&v_fontChar, &g_R310_RobotEyes_Font_arr[p_eyeFontIdx], sizeof(T_R310_FontChar_t)); // 변경된 배열명, 구조체명

    for (uint8_t v_row = 0; v_row < G_R310_DISPLAY_HEIGHT; v_row++) {
        uint8_t v_rowByte = pgm_read_byte(&v_fontChar.data[v_row]);
        for (uint8_t v_col = 0; v_col < G_R310_EYE_COL_SIZE; v_col++) {
            if ((v_rowByte >> (7 - v_col)) & 0x01) {
                uint16_t v_pixelIdx = R310_mapEyePixel(p_eyeSideIdx, v_row, v_col);
                g_R310_leds[v_pixelIdx] = G_R310_EYE_COLOR;
            }
        }
    }
}

// R310_loadSequence 함수
uint8_t R310_loadSequence(T_R310_emotion_idx_t p_eyeEmotionIdx) { // 변경된 열거형명
    bool v_found = false;

    for (uint8_t v_i = 0; v_i < G_R310_ARRAY_SIZE(g_R310_ani_Tables_arr); v_i++) { // 변경된 배열명
        T_R310_ani_Table_t v_animTable; // 변경된 구조체명  
        memcpy_P(&v_animTable, &g_R310_ani_Tables_arr[v_i], sizeof(T_R310_ani_Table_t)); // 변경된 배열명, 구조체명
        if (v_animTable.emotionIdx == p_eyeEmotionIdx) { // 변경된 멤버명
            g_R310_aniControl.currentAniEntry = v_animTable; // 구조체 멤버 사용
            v_found = true;
            break;
        }
    }
    if (!v_found) {
        Serial.print("Warning: Animation sequence not found for emotion: ");
        Serial.println(p_eyeEmotionIdx);
        g_R310_aniControl.currentAniEntry = {EMT_NEUTRAL, g_R310_seqBlink, 1}; // 구조체 멤버 사용
    }

    if (g_R310_aniControl.playDirection == EMTP_PLY_DIR_LAST) // 구조체 멤버 사용
        g_R310_aniControl.aniFrameIndex = g_R310_aniControl.currentAniEntry.seqSize - 1; // 구조체 멤버 사용
    else
        g_R310_aniControl.aniFrameIndex = 0; // 구조체 멤버 사용

    return (g_R310_aniControl.currentAniEntry.seqSize); // 구조체 멤버 사용
}

// R310_loadFrame 함수
void R310_loadFrame(T_R310_animFrame_t* p_animFrame) { // 변경된 구조체명
    if (g_R310_aniControl.aniFrameIndex >= 0 && g_R310_aniControl.aniFrameIndex < g_R310_aniControl.currentAniEntry.seqSize) { // 구조체 멤버 사용
        memcpy_P(p_animFrame, &g_R310_aniControl.currentAniEntry.seq[g_R310_aniControl.aniFrameIndex], sizeof(T_R310_animFrame_t)); // 구조체 멤버 사용, 구조체명 변경
    } else {
        Serial.print("Error: Invalid animation index: ");
        Serial.println(g_R310_aniControl.aniFrameIndex); // 구조체 멤버 사용
        p_animFrame->eyeData[0] = 0;
        p_animFrame->eyeData[1] = 0;
        p_animFrame->timeFrame = G_R310_FRAME_TIME;
    }
}

// R310_setAnimation 함수
void R310_setAnimation(T_R310_emotion_idx_t p_emotionIdx, EMTP_Ply_AutoReverse_t p_autoReverse, EMTP_Ply_Direct_t p_playDirection, EMTP_Ply_Force_t p_forcePlay) { // 변경된 열거형명
    if (g_R310_textDisplay.pointer != nullptr && g_R310_textDisplay.buffer[0] != '\0' && p_forcePlay == EMTP_FORCE_PLY_OFF) return; // 구조체 멤버 사용

    if (p_emotionIdx != g_R310_aniControl.currentEmotion || p_forcePlay == EMTP_FORCE_PLY_ON) { // 구조체 멤버 사용
        g_R310_aniControl.nextEmotion          = p_emotionIdx;     // 구조체 멤버 사용
        g_R310_aniControl.autoReverse          = p_autoReverse;    // 구조체 멤버 사용
        g_R310_aniControl.playDirection        = p_playDirection;  // 구조체 멤버 사용

        if (p_forcePlay == EMTP_FORCE_PLY_ON || g_R310_aniControl.playState == ANI_PLY_STATE_IDLE) { // 구조체 멤버 사용
            g_R310_aniControl.playState = ANI_PLY_STATE_RESTART; // 구조체 멤버 사용
        }
    }
}

// R310_setRobotState 함수
void R310_setRobotState(T_R310_RobotState_t p_robotState) { // 변경된 열거형명
    if (p_robotState != g_R310_robotStatus.robotState) { // 구조체 멤버 사용
        if (p_robotState == R_STATE_SLEEPING && g_R310_robotStatus.robotState == R_STATE_AWAKE) { // 구조체 멤버 사용
            R310_setAnimation(EMT_SLEEP_BLINK, EMTP_AUTO_REVERSE_OFF, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
        } else if (p_robotState == R_STATE_AWAKE && g_R310_robotStatus.robotState == R_STATE_SLEEPING) { // 구조체 멤버 사용
            R310_setAnimation(EMT_SLEEP, EMTP_AUTO_REVERSE_OFF, EMTP_PLY_DIR_LAST, EMTP_FORCE_PLY_ON);
        }
        g_R310_robotStatus.robotState = p_robotState; // 구조체 멤버 사용
    }
}

// R310_runAnimation 함수
bool R310_runAnimation(void) {
    static T_R310_animFrame_t   v_thisFrame;
    static uint32_t             v_timeOfLastFrame = 0;

    switch (g_R310_aniControl.playState) { // 구조체 멤버 사용
        case ANI_PLY_STATE_IDLE:
            if (g_R310_textDisplay.pointer != nullptr && g_R310_textDisplay.buffer[0] != '\0') { // 구조체 멤버 사용
                g_R310_aniControl.playState = ANI_PLY_STATE_TEXT; // 구조체 멤버 사용
                break;
            }
            if (g_R310_aniControl.nextEmotion != EMT_NONE) { // 구조체 멤버 사용
                g_R310_aniControl.playState = ANI_PLY_STATE_RESTART; // 구조체 멤버 사용
                break;
            }
            if (g_R310_aniControl.autoBlinkOn && (millis() - g_R310_robotStatus.lastAnimationTime) >= g_R310_robotStatus.blinkMinimumTime) { // 구조체 멤버 사용
                if (random(1000) > 700) {
                    if (g_R310_robotStatus.robotState == R_STATE_SLEEPING) { // 구조체 멤버 사용
                        R310_setAnimation(EMT_SLEEP_BLINK, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_LAST, EMTP_FORCE_PLY_ON);
                    } else if (g_R310_robotStatus.robotState == R_STATE_AWAKE) { // 구조체 멤버 사용
                        R310_setAnimation(EMT_BLINK, EMTP_AUTO_REVERSE_ON, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
                    }
                    g_R310_robotStatus.lastAnimationTime = millis(); // 구조체 멤버 사용
                } else {
                    g_R310_robotStatus.lastAnimationTime = millis(); // 구조체 멤버 사용
                }
            }
            break;

        case ANI_PLY_STATE_RESTART:
            if (g_R310_aniControl.nextEmotion != EMT_NONE) { // 구조체 멤버 사용
                R310_loadSequence(g_R310_aniControl.nextEmotion); // 구조체 멤버 사용
                g_R310_aniControl.currentEmotion = g_R310_aniControl.nextEmotion; // 구조체 멤버 사용
                g_R310_aniControl.nextEmotion = EMT_NONE; // 구조체 멤버 사용
                g_R310_aniControl.playState = ANI_PLY_STATE_ANIMATE; // 구조체 멤버 사용
            } else {
                g_R310_aniControl.playState = ANI_PLY_STATE_IDLE; // 구조체 멤버 사용
            }
            break;

        case ANI_PLY_STATE_ANIMATE:
            R310_loadFrame(&v_thisFrame);
            R310_drawEyes(v_thisFrame.eyeData[0], v_thisFrame.eyeData[1]);
            v_timeOfLastFrame = millis();

            if (g_R310_aniControl.playDirection == EMTP_PLY_DIR_LAST) { // 구조체 멤버 사용
                g_R310_aniControl.aniFrameIndex--; // 구조체 멤버 사용
            } else {
                g_R310_aniControl.aniFrameIndex++; // 구조체 멤버 사용
            }
            g_R310_aniControl.playState = ANI_PLY_STATE_PAUSE; // 구조체 멤버 사용
            break;

        case ANI_PLY_STATE_PAUSE:
            if ((millis() - v_timeOfLastFrame) < v_thisFrame.timeFrame) {
                break;
            }
            if ((g_R310_aniControl.playDirection == EMTP_PLY_DIR_FIRST && g_R310_aniControl.aniFrameIndex >= g_R310_aniControl.currentAniEntry.seqSize) ||
                (g_R310_aniControl.playDirection == EMTP_PLY_DIR_LAST && g_R310_aniControl.aniFrameIndex < 0)) { // 구조체 멤버 사용
                if (g_R310_aniControl.autoReverse == EMTP_AUTO_REVERSE_ON) { // 구조체 멤버 사용
                    EMTP_Ply_Direct_t v_emtp_ply_dir;
                    if( g_R310_aniControl.playDirection == EMTP_PLY_DIR_FIRST){ // 구조체 멤버 사용
                        v_emtp_ply_dir  = EMTP_PLY_DIR_LAST; // 역방향 시작으로 변경 (자동 역재생)
                    } else {
                        v_emtp_ply_dir  = EMTP_PLY_DIR_FIRST; // 정방향 시작으로 변경 (자동 역재생)
                    }
                    R310_setAnimation(g_R310_aniControl.currentAniEntry.emotionIdx, EMTP_AUTO_REVERSE_OFF, v_emtp_ply_dir, EMTP_FORCE_PLY_ON); // 구조체 멤버 사용
                } else {
                    g_R310_aniControl.playState        = ANI_PLY_STATE_IDLE; // 구조체 멤버 사용
                    g_R310_aniControl.currentEmotion   = EMT_NONE; // 구조체 멤버 사용
                    g_R310_robotStatus.lastAnimationTime     = millis(); // 구조체 멤버 사용
                }
            } else {
                g_R310_aniControl.playState = ANI_PLY_STATE_ANIMATE; // 구조체 멤버 사용
            }
            break;

        case ANI_PLY_STATE_TEXT:
            if (g_R310_textDisplay.buffer[0] == '\0') { // 구조체 멤버 사용
                g_R310_aniControl.playState = ANI_PLY_STATE_IDLE; // 구조체 멤버 사용
                g_R310_robotStatus.lastAnimationTime = millis(); // 구조체 멤버 사용
            }
            break;

        default:
            g_R310_aniControl.playState = ANI_PLY_STATE_IDLE; // 구조체 멤버 사용
            g_R310_aniControl.currentEmotion = EMT_NONE; // 구조체 멤버 사용
            R310_clearText();
            g_R310_robotStatus.lastAnimationTime = millis(); // 구조체 멤버 사용
            break;
    }
    return (g_R310_aniControl.playState == ANI_PLY_STATE_IDLE); // 구조체 멤버 사용
}

// R310_init 함수
void R310_init() {
    // ... (기존 FastLED 초기화 부분 동일) ...
    g_R310_ledsPtr = g_R310_leds; // 변경된 변수명

    // 로봇 상태 관련 변수 초기화 (구조체 멤버 사용)
    g_R310_robotStatus.robotState            = R_STATE_AWAKE;
    g_R310_aniControl.playState        = ANI_PLY_STATE_IDLE;
    g_R310_aniControl.autoBlinkOn      = true;     // 이제 명시적으로 설정
    g_R310_robotStatus.blinkMinimumTime      = 5000;
    g_R310_robotStatus.lastAnimationTime     = millis();

    // 텍스트 버퍼 초기화 및 포인터 연결 (구조체 멤버 사용)
    g_R310_textDisplay.buffer[0]        = '\0';
    g_R310_textDisplay.pointer          = g_R310_textDisplay.buffer;

    g_R310_robotStatus.lastActivityTime      = millis(); // 구조체 멤버 사용

    R310_setAnimation(EMT_NEUTRAL, EMTP_AUTO_REVERSE_OFF, EMTP_PLY_DIR_FIRST, EMTP_FORCE_PLY_ON);
}

// R310_run 함수
void R310_run() {
    R310_runAnimation();

    if (g_R310_robotStatus.robotState != R_STATE_SLEEPING && millis() - g_R310_robotStatus.lastActivityTime >= G_R310_TIME_TO_SLEEP) { // 구조체 멤버 사용
        R310_setRobotState(R_STATE_SLEEPING); // 변경된 함수명
    } else if (g_R310_robotStatus.robotState == R_STATE_SLEEPING && millis() - g_R310_robotStatus.lastActivityTime < G_R310_TIME_TO_SLEEP) { // 구조체 멤버 사용
        R310_setRobotState(R_STATE_AWAKE); // 변경된 함수명
    }
	
    #ifdef G_R310_BUFFEREDSERIAL_USE	
    	if (g_R310_BufferedSerial.available()) {
            String v_commandString = g_R310_BufferedSerial.readStringUntil('\n'); // 변경된 지역변수명
            v_commandString.trim();
            Serial.print("Received command: ");
            Serial.println(v_commandString);
            R310_processCommand(v_commandString.c_str());
            g_R310_robotStatus.lastActivityTime = millis(); // 구조체 멤버 사용
         }
	#else
        if (Serial.available()) {
            String v_commandString = Serial.readStringUntil('\n'); // 변경된 지역변수명
            v_commandString.trim();
            Serial.print("Received command: ");
            Serial.println(v_commandString);
            R310_processCommand(v_commandString.c_str());
            g_R310_robotStatus.lastActivityTime = millis(); // 구조체 멤버 사용
        }
	#endif
    delay(1);
}

// R310_clearText 함수
void R310_clearText() {
    g_R310_textDisplay.buffer[0] = '\0'; // 구조체 멤버 사용
}

// R310_showText 함수
void R310_showText(bool p_bInit) {
    if (g_R310_textDisplay.pointer == nullptr || g_R310_textDisplay.buffer[0] == '\0') { // 구조체 멤버 사용
         R310_clearText();
         g_R310_aniControl.playState = ANI_PLY_STATE_IDLE; // 구조체 멤버 사용
         g_R310_robotStatus.lastAnimationTime = millis(); // 구조체 멤버 사용
         return;
    }
    FastLED.clear();
    R310_drawEye(EYE_RIGHT, (uint8_t)g_R310_textDisplay.pointer[0]); // 구조체 멤버 사용
    if (g_R310_textDisplay.pointer[1] != '\0') { // 구조체 멤버 사용
        R310_drawEye(EYE_LEFT, (uint8_t)g_R310_textDisplay.pointer[1]); // 구조체 멤버 사용
    }
    FastLED.show();
}

// R310_processCommand 함수
void R310_processCommand(const char* p_command) {
    R310_clearText();

    // ... (기존 strcmp를 통한 명령 처리 로직 동일) ...
    // 단, R310_set_RobotState -> R310_setRobotState 로 함수명 변경
    // R310_set_RobotState(R_STATE_AWAKE); -> R310_setRobotState(R_STATE_AWAKE);
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
        R310_setRobotState(R_STATE_AWAKE);
    } else if (strcmp(p_command, "sleeping") == 0) {
        R310_setRobotState(R_STATE_SLEEPING);
    }

    else {
        strncpy(g_R310_textDisplay.buffer, p_command, G_R310_MAX_TEXT_LENGTH); // 구조체 멤버 사용
        g_R310_textDisplay.buffer[G_R310_MAX_TEXT_LENGTH] = '\0'; // 구조체 멤버 사용
        g_R310_aniControl.playState = ANI_PLY_STATE_TEXT; // 구조체 멤버 사용
    }
}
