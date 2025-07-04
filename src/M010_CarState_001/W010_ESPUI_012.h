#pragma once
// W010_embUI_012.h
// -- json설정값과 다국어 설정 분리
// ====================================================================================================
// 프로젝트: 자동차 후방 로봇 눈
// 설명: ESP32 + MPU6050 활용, 차량 움직임 감지 및 LED Matrix 로봇 눈 표정 표현.
//       본 코드는 ESPUI 라이브러리를 사용하여 웹 기반 설정 관리 및 실시간 상태 모니터링 기능을 제공합니다.
// 개발 환경: PlatformIO Arduino Core (ESP32)
// 사용 라이브러리: ESPUI (s00500), ArduinoJson (bblanchon), LittleFS (내장 또는 추가)
// ====================================================================================================

// --- 시리얼 출력 제어 매크로 ---
// M010_main3_010.h 에 정의된 DEBUG_P1 매크로를 활용
#include "A01_debug_001.h"

// ====================================================================================================
// lib_deps 설정 (PlatformIO.ini)
// s00500/ESPUI @ ^2.3.0
// bblanchon/ArduinoJson@^7.4.1
// ====================================================================================================

#include <ESPUI.h> // ESPUI 라이브러리 포함
#include <ArduinoJson.h> // ArduinoJson 라이브러리 포함
#include <LittleFS.h> // LittleFS 파일 시스템 라이브러리 포함 (ESP32의 경우 <FS.h>와 함께 사용)

// M010_main3_010.h 에 정의된 전역 변수 및 함수 선언을 사용하기 위해 포함
#include "M010_main3_012.h" // T_M010_Config, T_M010_CarStatus, M010_Config_save, M010_Config_load, M010_Config_initDefaults 등 포함

extern T_M010_Config       g_M010_Config;
extern T_M010_CarStatus    g_M010_CarStatus; // 자동차 상태 구조체 인스턴스


// JSON 파일 경로 정의 (언어 코드에 따라 동적으로 결정)
#define G_W010_UI_LANG_FILE_PREFIX           "/W010_ESPUI_ui_lang_005_"
#define G_W010_UI_LANG_FILE_SUBFIX           ".json"

#define G_W010_UI_DEFAULT_CONFIG_FILE        "/W010_ESPUI_ui_defaults_005.json"


// 현재 선택된 언어 코드 (예: "ko", "en")
String          g_W010_currentLang          = "ko"; // 기본 언어는 한국어

JsonDocument    g_W010_uiLayoutDoc; // UI 레이아웃 및 기본값을 위한 JSON 문서
JsonDocument    g_W010_uiLangDoc;   // 다국어 문자열을 위한 JSON 문서

// ====================================================================================================
// 전역 변수 선언
// ====================================================================================================

// ESPUI 컨트롤 ID를 저장할 전역 변수들
// enum 값과 ESPUI.addControl()이 반환하는 실제 ID를 매핑하는 역할
// 이렇게 전역 변수로 관리해야 콜백에서 p_control->id 와 비교할 수 있습니다.

uint16_t g_W010_Tab_Config_Id;
uint16_t g_W010_Tab_Status_Id;

uint16_t g_W010_C_ID_ALARAM_LABEL_Id;
uint16_t g_W010_C_ID_ERROR_LABEL_Id;
uint16_t g_W010_C_ID_LANG_SELECT_Id; // 언어 선택 드롭다운 컨트롤 ID

uint16_t g_W010_C_ID_MVSTATE_ACCELFILTER_ALPHA_Id;
uint16_t g_W010_C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN_Id;
uint16_t g_W010_C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN_Id;
uint16_t g_W010_C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX_Id;
uint16_t g_W010_C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX_Id;
uint16_t g_W010_C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX_Id;
uint16_t g_W010_C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN_Id;
uint16_t g_W010_C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN_Id;
uint16_t g_W010_C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD_Id; 
uint16_t g_W010_C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD_Id;
uint16_t g_W010_C_ID_MVSTATE_BUMP_SPEEDKMH_MIN_Id;
uint16_t g_W010_C_ID_MVSTATE_BUMP_COOLDOWNMS_Id;
uint16_t g_W010_C_ID_MVSTATE_DECEL_DURATIONMS_HOLD_Id;
uint16_t g_W010_C_ID_MVSTATE_BUMP_DURATIONMS_HOLD_Id;
uint16_t g_W010_C_ID_MVSTATE_SIGNALWAIT1_SECONDS_Id;
uint16_t g_W010_C_ID_MVSTATE_SIGNALWAIT2_SECONDS_Id;
uint16_t g_W010_C_ID_MVSTATE_STOPPED1_SECONDS_Id;
uint16_t g_W010_C_ID_MVSTATE_STOPPED2_SECONDS_Id;
uint16_t g_W010_C_ID_MVSTATE_PARK_SECONDS_Id;
uint16_t g_W010_C_ID_SERIALPRINT_INTERVALMS_Id;
uint16_t g_W010_C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD_Id;
uint16_t g_W010_C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD_Id;
uint16_t g_W010_C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD_Id;
uint16_t g_W010_C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD_Id;
uint16_t g_W010_C_ID_TURNSTATE_SPEEDKMH_MINSPEED_Id;
uint16_t g_W010_C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD_Id;
uint16_t g_W010_C_ID_TURNSTATE_STABLEDURATIONMS_Id;
uint16_t g_W010_C_ID_SAVE_CONFIG_BTN_Id;
uint16_t g_W010_C_ID_LOAD_CONFIG_BTN_Id;
uint16_t g_W010_C_ID_RESET_CONFIG_BTN_Id;

uint16_t g_W010_C_ID_CARMOVEMENTSTATE_LABEL_Id;
uint16_t g_W010_C_ID_CARTURNSTATE_LABEL_Id;
uint16_t g_W010_C_ID_SPEED_KMH_LABEL_Id;
uint16_t g_W010_C_ID_ACCELX_MS2_LABEL_Id;
uint16_t g_W010_C_ID_ACCELY_MS2_LABEL_Id;
uint16_t g_W010_C_ID_ACCELZ_MS2_LABEL_Id;
uint16_t g_W010_C_ID_YAWANGLE_DEG_LABEL_Id;
uint16_t g_W010_C_ID_PITCHANGLE_DEG_LABEL_Id;
uint16_t g_W010_C_ID_YAWANGLEVELOCITY_DEGPS_LABEL_Id;
uint16_t g_W010_C_ID_ISEMERGENCYBRAKING_LABEL_Id;
uint16_t g_W010_C_ID_ISSPEEDBUMPDETECTED_LABEL_Id;
uint16_t g_W010_C_ID_CURRENTSTOPTIME_SEC_LABEL_Id;


// ESPUI 컨트롤 ID 정의 (이 Enum은 UI 구성에 사용되는 고유 식별자입니다.)
// 실제로 ESPUI에서 할당되는 ID와는 다릅니다.
// 이 Enum은 JSON의 enum_id와 논리적으로 매핑됩니다.
enum {
    // Config Page Controls
    C_ID_MVSTATE_ACCELFILTER_ALPHA,
    C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN,
    C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN,
    C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX,
    C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX,
    C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX,
    C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN,
    C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN,
    C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD,
    C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD,
    C_ID_BUMP_SPEEDKMH_MIN,
    C_ID_BUMP_COOLDOWNMS,
    C_ID_DECEL_DURATIONMS_HOLD,
    C_ID_BUMP_DURATIONMS_HOLD,
    C_ID_MVSTATE_SIGNALWAIT1_SECONDS,
    C_ID_MVSTATE_SIGNALWAIT2_SECONDS,
    C_ID_MVSTATE_STOPPED1_SECONDS,
    C_ID_MVSTATE_STOPPED2_SECONDS,
    C_ID_MVSTATE_PARK_SECONDS,
    C_ID_SERIALPRINT_INTERVALMS,
    C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD,
    C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD,
    C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD,
    C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD,
    C_ID_TURNSTATE_SPEEDKMH_MINSPEED,
    C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD,
    C_ID_TURNSTATE_STABLEDURATIONMS,
    C_ID_SAVE_CONFIG_BTN,
    C_ID_LOAD_CONFIG_BTN,
    C_ID_RESET_CONFIG_BTN,
    C_ID_LANGUAGE_SELECT, // 언어 선택 드롭다운

    // Status Page Controls
    C_ID_CARMOVEMENTSTATE_LABEL,
    C_ID_CARTURNSTATE_LABEL,
    C_ID_SPEED_KMH_LABEL,
    C_ID_ACCELX_MS2_LABEL,
    C_ID_ACCELY_MS2_LABEL,
    C_ID_ACCELZ_MS2_LABEL,
    C_ID_YAWANGLE_DEG_LABEL,
    C_ID_PITCHANGLE_DEG_LABEL,
    C_ID_YAWANGLEVELOCITY_DEGPS_LABEL,
    C_ID_ISEMERGENCYBRAKING_LABEL,
    C_ID_ISSPEEDBUMPDETECTED_LABEL,
    C_ID_CURRENTSTOPTIME_SEC_LABEL,
    
    // 알림 및 오류 레이블 (JSON에는 enum_id로 정의되어 있지 않지만, 편의상 여기에 추가)
    // 이들은 g_W010_Ctl_Alaram_Id, g_W010_Control_Error_Id 와 직접 연결됩니다.
    // JSON의 "C_ID_ALARM_LABEL", "C_ID_ERROR_LABEL"는 실제 컨트롤 ID가 아니라
    // 레이블을 위한 플레이스홀더로 사용된 것으로 보입니다.
    // 하지만 콜백에서 enum ID로 처리하기 위해 여기에 추가했습니다.
    C_ID_ALARM_LABEL,
    C_ID_ERROR_LABEL
};

// 매직 문자열을 위한 상수 정의
// #define BTN_CMD_SAVE_CONFIG     F("save_config")
// #define BTN_CMD_LOAD_CONFIG     F("load_config")
// #define BTN_CMD_RESET_CONFIG    F("reset_config")

// JSON enum_id와 C++ enum 값 매핑을 위한 구조체
struct UiControl_Map_ST {
    const char* enum_Id_str;
    int         enum_no;
    uint16_t*   ui_ctl_var_ptr; // ESPUI.addControl()이 반환하는 실제 ID를 저장할 포인터
};

// 모든 컨트롤 ID를 매핑하는 배열
// ** 중요: 이 배열의 순서는 JSON 파일의 "controls" 배열 순서와 일치해야 합니다.
//          또는 enum_idStr을 사용하여 동적으로 찾을 수 있도록 구현해야 합니다.
//          여기서는 각 컨트롤의 ESPUI ID를 저장할 전역 변수 포인터를 추가했습니다.
const UiControl_Map_ST g_W010_UiControlMap_Arr[] = {
    // Config Page Controls
    {"C_ID_MVSTATE_ACCELFILTER_ALPHA"                       , C_ID_MVSTATE_ACCELFILTER_ALPHA                    , &g_W010_C_ID_MVSTATE_ACCELFILTER_ALPHA_Id},
    {"C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN"          , C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN       , &g_W010_C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN_Id},
    {"C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN"          , C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN       , &g_W010_C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN_Id},
    {"C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX"             , C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX          , &g_W010_C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX_Id},
    {"C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX"            , C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX         , &g_W010_C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX_Id},
    {"C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX"              , C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX           , &g_W010_C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX_Id},
    {"C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN"              , C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN           , &g_W010_C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN_Id},
    {"C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN"              , C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN           , &g_W010_C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN_Id},
    {"C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD"               , C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD            , &g_W010_C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD_Id}, // 수정됨
    {"C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD"                , C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD             , &g_W010_C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD_Id},  // 수정됨
    {"C_ID_MVSTATE_BUMP_SPEEDKMH_MIN"                       , C_ID_BUMP_SPEEDKMH_MIN                            , &g_W010_C_ID_MVSTATE_BUMP_SPEEDKMH_MIN_Id},         // 수정됨
    {"C_ID_MVSTATE_BUMP_COOLDOWNMS"                         , C_ID_BUMP_COOLDOWNMS                              , &g_W010_C_ID_MVSTATE_BUMP_COOLDOWNMS_Id},          // 수정됨
    {"C_ID_MVSTATE_DECEL_DURATIONMS_HOLD"                   , C_ID_DECEL_DURATIONMS_HOLD                        , &g_W010_C_ID_MVSTATE_DECEL_DURATIONMS_HOLD_Id},    // 수정됨
    {"C_ID_MVSTATE_BUMP_DURATIONMS_HOLD"                    , C_ID_BUMP_DURATIONMS_HOLD                         , &g_W010_C_ID_MVSTATE_BUMP_DURATIONMS_HOLD_Id},     // 수정됨
    {"C_ID_MVSTATE_SIGNALWAIT1_SECONDS"                     , C_ID_MVSTATE_SIGNALWAIT1_SECONDS                  , &g_W010_C_ID_MVSTATE_SIGNALWAIT1_SECONDS_Id},
    {"C_ID_MVSTATE_SIGNALWAIT2_SECONDS"                     , C_ID_MVSTATE_SIGNALWAIT2_SECONDS                  , &g_W010_C_ID_MVSTATE_SIGNALWAIT2_SECONDS_Id},
    {"C_ID_MVSTATE_STOPPED1_SECONDS"                        , C_ID_MVSTATE_STOPPED1_SECONDS                     , &g_W010_C_ID_MVSTATE_STOPPED1_SECONDS_Id},
    {"C_ID_MVSTATE_STOPPED2_SECONDS"                        , C_ID_MVSTATE_STOPPED2_SECONDS                     , &g_W010_C_ID_MVSTATE_STOPPED2_SECONDS_Id},
    {"C_ID_MVSTATE_PARK_SECONDS"                            , C_ID_MVSTATE_PARK_SECONDS                         , &g_W010_C_ID_MVSTATE_PARK_SECONDS_Id},
    {"C_ID_SERIALPRINT_INTERVALMS"                          , C_ID_SERIALPRINT_INTERVALMS                       , &g_W010_C_ID_SERIALPRINT_INTERVALMS_Id},
    {"C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD" , C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD , &g_W010_C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD_Id},
    {"C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD"   , C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD   , &g_W010_C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD_Id},
    {"C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD"   , C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD   , &g_W010_C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD_Id},
    {"C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD"   , C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD   , &g_W010_C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD_Id},
    {"C_ID_TURNSTATE_SPEEDKMH_MINSPEED"                     , C_ID_TURNSTATE_SPEEDKMH_MINSPEED                  , &g_W010_C_ID_TURNSTATE_SPEEDKMH_MINSPEED_Id},
    {"C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD"          , C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD       , &g_W010_C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD_Id},
    {"C_ID_TURNSTATE_STABLEDURATIONMS"                      , C_ID_TURNSTATE_STABLEDURATIONMS                   , &g_W010_C_ID_TURNSTATE_STABLEDURATIONMS_Id},
    {"C_ID_SAVE_CONFIG_BTN"                                 , C_ID_SAVE_CONFIG_BTN                              , &g_W010_C_ID_SAVE_CONFIG_BTN_Id},
    {"C_ID_LOAD_CONFIG_BTN"                                 , C_ID_LOAD_CONFIG_BTN                              , &g_W010_C_ID_LOAD_CONFIG_BTN_Id},
    {"C_ID_RESET_CONFIG_BTN"                                , C_ID_RESET_CONFIG_BTN                             , &g_W010_C_ID_RESET_CONFIG_BTN_Id},
    {"C_ID_LANGUAGE_SELECT"                                 , C_ID_LANGUAGE_SELECT                              , &g_W010_C_ID_LANG_SELECT_Id}, // 이 ID는 이미 전역으로 선언됨

    // Status Page Controls
    {"C_ID_CARMOVEMENTSTATE_LABEL"                          , C_ID_CARMOVEMENTSTATE_LABEL                       , &g_W010_C_ID_CARMOVEMENTSTATE_LABEL_Id},
    {"C_ID_CARTURNSTATE_LABEL"                              , C_ID_CARTURNSTATE_LABEL                           , &g_W010_C_ID_CARTURNSTATE_LABEL_Id},
    {"C_ID_SPEED_KMH_LABEL"                                 , C_ID_SPEED_KMH_LABEL                              , &g_W010_C_ID_SPEED_KMH_LABEL_Id},
    {"C_ID_ACCELX_MS2_LABEL"                                , C_ID_ACCELX_MS2_LABEL                             , &g_W010_C_ID_ACCELX_MS2_LABEL_Id},
    {"C_ID_ACCELY_MS2_LABEL"                                , C_ID_ACCELY_MS2_LABEL                             , &g_W010_C_ID_ACCELY_MS2_LABEL_Id},
    {"C_ID_ACCELZ_MS2_LABEL"                                , C_ID_ACCELZ_MS2_LABEL                             , &g_W010_C_ID_ACCELZ_MS2_LABEL_Id},
    {"C_ID_YAWANGLE_DEG_LABEL"                              , C_ID_YAWANGLE_DEG_LABEL                           , &g_W010_C_ID_YAWANGLE_DEG_LABEL_Id},
    {"C_ID_PITCHANGLE_DEG_LABEL"                            , C_ID_PITCHANGLE_DEG_LABEL                         , &g_W010_C_ID_PITCHANGLE_DEG_LABEL_Id},
    {"C_ID_YAWANGLEVELOCITY_DEGPS_LABEL"                    , C_ID_YAWANGLEVELOCITY_DEGPS_LABEL                 , &g_W010_C_ID_YAWANGLEVELOCITY_DEGPS_LABEL_Id},
    {"C_ID_ISEMERGENCYBRAKING_LABEL"                        , C_ID_ISEMERGENCYBRAKING_LABEL                     , &g_W010_C_ID_ISEMERGENCYBRAKING_LABEL_Id},
    {"C_ID_ISSPEEDBUMPDETECTED_LABEL"                       , C_ID_ISSPEEDBUMPDETECTED_LABEL                    , &g_W010_C_ID_ISSPEEDBUMPDETECTED_LABEL_Id},
    {"C_ID_CURRENTSTOPTIME_SEC_LABEL"                       , C_ID_CURRENTSTOPTIME_SEC_LABEL                    , &g_W010_C_ID_CURRENTSTOPTIME_SEC_LABEL_Id},
    {"C_ID_ALARM_LABEL"                                     , C_ID_ALARM_LABEL                                  , &g_W010_C_ID_ALARAM_LABEL_Id}, // 이 ID는 이미 전역으로 선언됨
    {"C_ID_ERROR_LABEL"                                     , C_ID_ERROR_LABEL                                  , &g_W010_C_ID_ERROR_LABEL_Id}  // 이 ID는 이미 전역으로 선언됨
};
const size_t g_W010_UiControlMap_Arr_Size = sizeof(g_W010_UiControlMap_Arr) / sizeof(g_W010_UiControlMap_Arr[0]);

#include <Preferences.h>
Preferences g_W010_preferences; // Preferences 객체

// ====================================================================================================
// 함수 선언 (프로토타입)
// ====================================================================================================
String W010_EmbUI_getCommonString(const char* key, const char* defaultVal = ""); // 다국어 문자열 가져오는 헬퍼 함수
String W010_EmbUI_getLabelFromLangDoc(const String& p_enumIdStr); // 언어 문서에서 enum_id에 해당하는 label을 찾는 함수
bool   W010_EmbUI_load_Json_UiLayout(); // 
bool   W010_EmbUI_load_Json_UiLanguage(const String& langCode); // 언어 파일 로드 함수
void   W010_EmbUI_init(); // 함수 이름은 EmbUI 그대로 두지만, 내부 구현은 ESPUI를 사용
void   W010_EmbUI_setupWebPages();
void   W010_EmbUI_loadConfigToWebUI();
void   W010_ESPUI_callback(Control* p_control, int p_value);
void   W010_EmbUI_updateCarStatusWeb();
String W010_EmbUI_getCarTurnStateEnumString(T_M010_CarTurnState state);
String W010_EmbUI_getCarMovementStateEnumString(T_M010_CarMovementState state);
void   W010_EmbUI_run();
void   W010_EmbUI_load_Pref_LastLang(); // 마지막 선택된 언어를 로드
void   W010_EmbUI_save_Pref_LastLang(); // 현재 언어 설정을 저장
void   W010_EmbUI_rebuildUI(); // UI를 다시 그리는 함수
int    W010_EmbUI_getEnumIdFromEspuiId(uint16_t espuiId);  // ESPUI ID를 enum 값으로 변환하는 헬퍼 함수

// ====================================================================================================
// 함수 정의
// ====================================================================================================

String W010_EmbUI_getCommonString(const char* key, const char* p_defaultVal) {
    JsonVariant value = g_W010_uiLangDoc["common_strings"][key]; // g_W010_uiLangDoc 사용
    if (!value.isNull()) {
        return value.as<String>();
    }
    return String(p_defaultVal);
}

// 언어 문서에서 enum_id에 해당하는 label을 찾는 함수
String W010_EmbUI_getLabelFromLangDoc(const String& p_enumIdStr) {
    JsonArray v_json_tabs_arr = g_W010_uiLangDoc["tabs"].as<JsonArray>(); // g_W010_uiLangDoc 사용
    for (JsonObject v_json_tab : v_json_tabs_arr) {
        JsonArray v_json_ctrs_arr = v_json_tab["controls"].as<JsonArray>();
        for (JsonObject v_json_ctr : v_json_ctrs_arr) {
            if (v_json_ctr["enum_id"].as<String>().equals(p_enumIdStr)) {
                JsonVariant v_jsonVar_ctr_label = v_json_ctr["label"];
                if (!v_jsonVar_ctr_label.isNull()) {
                    return v_jsonVar_ctr_label.as<String>();
                }
            }
        }
    }
    return String(""); // 찾지 못함
}

bool W010_EmbUI_load_Json_UiLayout() {
	const char* v_filePath = G_W010_UI_DEFAULT_CONFIG_FILE; // 고정된 기본값 파일
    dbgP1_printf("UI 레이아웃 및 기본값 파일 로드 중: %s\n", v_filePath);

    File v_configFile = LittleFS.open(v_filePath, "r");
    if (!v_configFile) {
        dbgP1_printf("파일 열기 실패: %s\n", v_filePath);
        return false;
    }

    DeserializationError error = deserializeJson(g_W010_uiLayoutDoc, v_configFile);
    v_configFile.close();
    if (error) {
        dbgP1_printf("JSON 파싱 실패: %s\n", error.c_str());
        return false;
    }
    dbgP1_printf("UI 레이아웃 및 기본값 파일 로드 및 파싱 완료: %s\n", v_filePath);
    return true;
}

bool W010_EmbUI_load_Json_UiLanguage(const String& langCode) {
    String v_filePath = String(G_W010_UI_LANG_FILE_PREFIX) + langCode + G_W010_UI_LANG_FILE_SUBFIX;
    dbgP1_printf("UI 언어 파일 로드 중: %s\n", v_filePath.c_str());

    File v_langFile = LittleFS.open(v_filePath, "r");
    if (!v_langFile) {
        dbgP1_printf("파일 열기 실패: %s\n", v_filePath.c_str());
        return false;
    }

    DeserializationError error = deserializeJson(g_W010_uiLangDoc, v_langFile);
    v_langFile.close();
    if (error) {
        dbgP1_printf("JSON 파싱 실패: %s\n", error.c_str());
        return false;
    }
    dbgP1_printf("UI 언어 파일 로드 및 파싱 완료: %s\n", v_filePath.c_str());
    return true;
}

void W010_EmbUI_init() {
    dbgP1_println(F("ESPUI 초기화 중..."));

    if (!LittleFS.begin()) {
        dbgP1_println(F("LittleFS 마운트 실패! UI 설정 로드 불가."));
        return;
    }

    if (!W010_EmbUI_load_Json_UiLayout()) {
        dbgP1_println(F("UI 레이아웃 및 기본값 파일 로드 실패. UI 구성에 문제 발생 가능."));
    }

    W010_EmbUI_load_Pref_LastLang(); // Preferences에서 마지막 언어 로드

    if (!W010_EmbUI_load_Json_UiLanguage(g_W010_currentLang)) {
        dbgP1_printf("UI 언어 파일 (%s) 로드 실패. 기본 언어(ko)로 재시도.\n", g_W010_currentLang.c_str());
        g_W010_currentLang = "ko";
        if (!W010_EmbUI_load_Json_UiLanguage(g_W010_currentLang)) {
            dbgP1_println(F("기본 언어 UI 설정 파일도 로드 실패. 다국어 지원 불가."));
        }
    }

    ESPUI.begin("Robot Eye Car", "roboteye", "carpassword");
    
    // setupWebPages()에서 모든 컨트롤이 생성된 후 언어 선택 컨트롤에 콜백 할당
    // setupWebPages()를 먼저 호출하여 컨트롤 ID가 할당되도록 합니다.
    W010_EmbUI_setupWebPages(); 
    
    Control* v_langControl = ESPUI.getControl(g_W010_C_ID_LANG_SELECT_Id);
    if (v_langControl) {
        v_langControl->callback = &W010_ESPUI_callback;
    } else {
        dbgP1_println(F("언어 선택 컨트롤을 찾을 수 없습니다. 콜백 할당 실패."));
    }

    dbgP1_println(W010_EmbUI_getCommonString("messages.ui_init_done", "ESPUI initialization done."));
}

/**
 * @brief ESPUI 웹페이지 UI를 구성하고, g_M010_Config 구조체의 멤버 변수들을 웹 UI에 바인딩합니다.
 * 사용자가 웹 인터페이스를 통해 설정을 조회하고 변경할 수 있도록 필드를 정의합니다.
 */
void W010_EmbUI_setupWebPages() {
    dbgP1_println(W010_EmbUI_getCommonString("messages.ui_setup_start", "UI setup start..."));

    JsonArray v_json_tabs_arr = g_W010_uiLayoutDoc["tabs"].as<JsonArray>();

    for (JsonObject v_json_tab : v_json_tabs_arr) {
        String v_ctl_tabId   = v_json_tab["id"].as<String>();
        String v_ctl_tabName = W010_EmbUI_getLabelFromLangDoc(v_ctl_tabId + "_title");

        uint16_t v_currentTab_Id;
        if (v_ctl_tabId.equals(F("config"))) {
            g_W010_Tab_Config_Id = ESPUI.addControl(Tab, v_ctl_tabName.c_str(), v_ctl_tabName);
            ESPUI.setVertical(g_W010_Tab_Config_Id, true); // Config 탭 세로 정렬
            
            v_currentTab_Id = g_W010_Tab_Config_Id;

            // 언어 선택 드롭다운은 여기서 ID를 미리 저장합니다.
            g_W010_C_ID_LANG_SELECT_Id = ESPUI.addControl(
                                                            ControlType::Select,
                                                            W010_EmbUI_getCommonString("messages.lang_select_label", "Language Select").c_str(),
                                                            "", // 초기값은 비워두고 나중에 업데이트
                                                            ControlColor::Wetasphalt,
                                                            v_currentTab_Id
                                                        );
            
            ESPUI.addControl( ControlType::Option, W010_EmbUI_getCommonString("messages.lang_ko", "Korean").c_str(), "ko", ControlColor::Alizarin, g_W010_C_ID_LANG_SELECT_Id);
            ESPUI.addControl( ControlType::Option, W010_EmbUI_getCommonString("messages.lang_en", "English").c_str(), "en", ControlColor::Alizarin, g_W010_C_ID_LANG_SELECT_Id);

        } else if (v_ctl_tabId.equals(F("status"))) {
            g_W010_Tab_Status_Id = ESPUI.addControl(ControlType::Tab, v_ctl_tabName.c_str(), F(""), ControlColor::Wetasphalt);
            ESPUI.setVertical(g_W010_Tab_Status_Id, true); // Status 탭 세로 정렬
            v_currentTab_Id = g_W010_Tab_Status_Id;
        } else {
            dbgP1_printf(String(W010_EmbUI_getCommonString("messages.unknown_tab_id", "Unknown Tab ID:") + " %s\n").c_str(), v_ctl_tabId.c_str());
            continue;
        }

		JsonArray v_json_ctrs_arr = v_json_tab["controls"].as<JsonArray>();

        for (JsonObject v_json_ctr : v_json_ctrs_arr) {
            String v_enum_Id_str = v_json_ctr["enum_id"].as<String>();
            String label = W010_EmbUI_getLabelFromLangDoc(v_enum_Id_str);
            JsonVariant defaultValue = v_json_ctr["default_value"];

            // g_W010_UiControlMap_Arr에서 enum_Id_str에 해당하는 실제 enum 값과 ESPUI ID 포인터를 찾음
            int controlEnumId = -1;
            uint16_t* espuiIdStoragePtr = nullptr;
            for (size_t i = 0; i < g_W010_UiControlMap_Arr_Size; ++i) {
                if (v_enum_Id_str.equals(g_W010_UiControlMap_Arr[i].enum_Id_str)) {
                    controlEnumId = g_W010_UiControlMap_Arr[i].enum_no;
                    espuiIdStoragePtr = g_W010_UiControlMap_Arr[i].ui_ctl_var_ptr;
                    break;
                }
            }
            
            if (v_enum_Id_str.equals(F("C_ID_ALARM_LABEL"))) {
                g_W010_C_ID_ALARAM_LABEL_Id = ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, v_currentTab_Id);
                continue;
            } else if (v_enum_Id_str.equals(F("C_ID_ERROR_LABEL"))) {
                g_W010_C_ID_ERROR_LABEL_Id = ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, v_currentTab_Id);
                continue;
            } else if (v_enum_Id_str.equals(F("C_ID_LANGUAGE_SELECT"))) { 
                continue; // 이미 위에서 addControl 했으므로 건너뛰기
            }

            if (controlEnumId == -1 || espuiIdStoragePtr == nullptr) {
                dbgP1_printf(String(W010_EmbUI_getCommonString("messages.unknown_control_id", "Unknown control ID:") + " %s\n").c_str(), v_enum_Id_str.c_str());
                continue;
            }

            if (v_ctl_tabId.equals(F("config"))) {
                if (v_enum_Id_str.endsWith(F("_BTN"))) {
                    // 버튼 컨트롤: 콜백에서 p_control->id로 식별 가능하도록 등록하고 ID 저장
                    *espuiIdStoragePtr = ESPUI.addControl(ControlType::Button, label.c_str(), defaultValue.as<String>().c_str(), ControlColor::Emerald, v_currentTab_Id, &W010_ESPUI_callback);
                } else { // Number Control
                    // Number 컨트롤: 콜백에서 p_control->id로 식별 가능하도록 등록하고 ID 저장
                    *espuiIdStoragePtr = ESPUI.addControl(ControlType::Number, label.c_str(), String(defaultValue.as<float>(), 3), ControlColor::Alizarin, v_currentTab_Id, &W010_ESPUI_callback);
                }
            } else if (v_ctl_tabId.equals(F("status"))) {
                // Label 컨트롤: ID만 저장하고 콜백은 필요 없음
                *espuiIdStoragePtr = ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, v_currentTab_Id);
            }
        }
    }
    dbgP1_println(W010_EmbUI_getCommonString("messages.ui_setup_done", "UI setup done."));
}


/**
 * @brief g_M010_Config 구조체에 저장된 현재 설정값을 ESPUI 웹 UI로 로드하여 표시합니다.
 * 주로 시스템 시작 시 또는 '불러오기' 버튼 클릭 시 호출됩니다.
 * 다국어 레이블을 적용하여 컨트롤 업데이트
 */
void W010_EmbUI_loadConfigToWebUI() {
    dbgP1_println(W010_EmbUI_getCommonString("messages.config_load_to_web_start", "Loading config to web UI..."));

    // 모든 컨트롤의 레이블 업데이트 (언어 변경 시)
    JsonArray v_json_tabs_arr = g_W010_uiLayoutDoc["tabs"].as<JsonArray>();
    for (JsonObject v_json_tab : v_json_tabs_arr) {
        String v_ctl_tabId = v_json_tab["id"].as<String>();
        String v_ctl_tabName = W010_EmbUI_getLabelFromLangDoc(v_ctl_tabId + "_title");
        
        uint16_t currentTabEspuiId = 0;
        if (v_ctl_tabId.equals(F("config"))) currentTabEspuiId = g_W010_Tab_Config_Id;
        else if (v_ctl_tabId.equals(F("status"))) currentTabEspuiId = g_W010_Tab_Status_Id;

        if (currentTabEspuiId != 0) {
			ESPUI.updateControlLabel(currentTabEspuiId, v_ctl_tabName.c_str());
        }

        JsonArray v_json_ctrs_arr = v_json_tab["controls"].as<JsonArray>();
        for (JsonObject v_json_ctr : v_json_ctrs_arr) {
            String v_enum_Id_str = v_json_ctr["enum_id"].as<String>();
            String label = W010_EmbUI_getLabelFromLangDoc(v_enum_Id_str); // 언어 파일에서 레이블 가져오기

            // ESPUI ID를 찾아서 레이블 업데이트
            for (size_t i = 0; i < g_W010_UiControlMap_Arr_Size; ++i) {
                if (v_enum_Id_str.equals(g_W010_UiControlMap_Arr[i].enum_Id_str)) {
                    if (*g_W010_UiControlMap_Arr[i].ui_ctl_var_ptr != 0) { // ESPUI ID가 할당된 경우에만
                        ESPUI.updateControlLabel(*g_W010_UiControlMap_Arr[i].ui_ctl_var_ptr, label.c_str());
                    }
                    break;
                }
            }
        }
    }

      // mvState_Config (이동 상태 감지 관련 설정)
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_ACCELFILTER_ALPHA_Id, String(g_M010_Config.mvState_accelFilter_Alpha, 3)); 
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN_Id, String(g_M010_Config.mvState_Forward_speedKmh_Threshold_Min, 3)); 
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN_Id, String(g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min, 3)); 
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX_Id, String(g_M010_Config.mvState_Stop_speedKmh_Threshold_Max, 3)); 
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX_Id, String(g_M010_Config.mvState_Stop_accelMps2_Threshold_Max, 3));
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX_Id, String(g_M010_Config.mvState_Stop_gyroDps_Threshold_Max, 3)); 
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN_Id, String(g_M010_Config.mvState_stop_durationMs_Stable_Min)); // 이름 수정
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN_Id, String(g_M010_Config.mvState_move_durationMs_Stable_Min)); // 이름 수정
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD_Id, String(g_M010_Config.mvState_Decel_accelMps2_Threshold, 3));
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD_Id, String(g_M010_Config.mvState_Bump_accelMps2_Threshold, 3));
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_BUMP_SPEEDKMH_MIN_Id, String(g_M010_Config.mvState_Bump_SpeedKmh_Min, 3));
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_BUMP_COOLDOWNMS_Id, String(g_M010_Config.mvState_Bump_CooldownMs));
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_DECEL_DURATIONMS_HOLD_Id, String(g_M010_Config.mvState_Decel_durationMs_Hold));
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_BUMP_DURATIONMS_HOLD_Id, String(g_M010_Config.mvState_Bump_durationMs_Hold));

    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_SIGNALWAIT1_SECONDS_Id, String(g_M010_Config.mvState_signalWait1_Seconds)); 
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_SIGNALWAIT2_SECONDS_Id, String(g_M010_Config.mvState_signalWait2_Seconds)); 
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_STOPPED1_SECONDS_Id, String(g_M010_Config.mvState_stopped1_Seconds)); 
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_STOPPED2_SECONDS_Id, String(g_M010_Config.mvState_stopped2_Seconds)); 
    ESPUI.updateControlValue(g_W010_C_ID_MVSTATE_PARK_SECONDS_Id, String(g_M010_Config.mvState_park_Seconds)); 
    
    // Serial Print Interval
    ESPUI.updateControlValue(g_W010_C_ID_SERIALPRINT_INTERVALMS_Id, String(g_M010_Config.serialPrint_intervalMs)); 

    // TurnState_Config (회전 상태 감지 관련 설정)
    ESPUI.updateControlValue(g_W010_C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD_Id, String(g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold, 3)); 
    ESPUI.updateControlValue(g_W010_C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD_Id, String(g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold, 3)); 
    ESPUI.updateControlValue(g_W010_C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD_Id, String(g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold, 3)); 
    ESPUI.updateControlValue(g_W010_C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD_Id, String(g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold, 3)); 
    ESPUI.updateControlValue(g_W010_C_ID_TURNSTATE_SPEEDKMH_MINSPEED_Id, String(g_M010_Config.turnState_speedKmh_MinSpeed, 3)); 
    ESPUI.updateControlValue(g_W010_C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD_Id, String(g_M010_Config.turnState_speedKmh_HighSpeed_Threshold, 3)); 
    ESPUI.updateControlValue(g_W010_C_ID_TURNSTATE_STABLEDURATIONMS_Id, String(g_M010_Config.turnState_StableDurationMs)); // 이름 수정
	
    // 언어 선택 드롭다운 값 업데이트
    ESPUI.updateControlValue(g_W010_C_ID_LANG_SELECT_Id, g_W010_currentLang);

    dbgP1_println(W010_EmbUI_getCommonString("messages.config_load_to_web_done", "Config loaded to web UI."));
}

/**
 * @brief ESPUI 컨트롤 이벤트를 처리하는 콜백 함수.
 * @param p_control 이벤트가 발생한 ESPUI 컨트롤 객체.
 * @param p_value ESPUI 라이브러리 내부에서 사용하는 컨트롤의 추가 값 (버튼의 경우 1).
 */
void W010_ESPUI_callback(Control* p_control, int p_value) {
    dbgP1_printf(F("%s ID=%d, Value=%s, Type=%d\n"), 
                 W010_EmbUI_getCommonString("messages.callback_detected", "ESPUI Callback detected:").c_str(),
                 p_control->id, p_control->value.c_str(), p_control->type);
    
    // ESPUI ID를 찾아서 해당하는 Control Enum ID를 가져옴
    int controlEnumId = W010_EmbUI_getEnumIdFromEspuiId(p_control->id);

    if (controlEnumId == -1) {
        ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, W010_EmbUI_getCommonString("messages.unknown_command", "Unknown control ID received in callback."));
        ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, "");
        dbgP1_printf(F("Unknown control ID in callback: %d\n"), p_control->id);
        return;
    }

    // 언어 선택 컨트롤 처리 (g_W010_C_ID_LANG_SELECT_Id는 이미 전역으로 직접 참조 가능)
    if (p_control->id == g_W010_C_ID_LANG_SELECT_Id) {
        g_W010_currentLang = p_control->value;
        W010_EmbUI_save_Pref_LastLang(); // 인자 없는 버전 호출
        W010_EmbUI_rebuildUI();
        ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, W010_EmbUI_getCommonString("messages.lang_change_success", "Language changed successfully!"));
        ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, "");
        return;
    }

    // 버튼 컨트롤 처리 (p_value가 1일 때 버튼이 눌린 것)
    if (p_control->type == ControlType::Button && p_value) { 
        switch (controlEnumId) {
            case C_ID_SAVE_CONFIG_BTN:
                if (M010_Config_save()) {
                    ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, W010_EmbUI_getCommonString("messages.config_save_success", "Configuration saved successfully."));
                    ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, ""); 
                } else {
                    ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, W010_EmbUI_getCommonString("messages.config_save_fail", "Failed to save configuration to file system."));
                    ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, "");
                }
                break;
            case C_ID_LOAD_CONFIG_BTN:
                if (M010_Config_load()) {
                    W010_EmbUI_loadConfigToWebUI();
                    ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, W010_EmbUI_getCommonString("messages.config_load_success", "Configuration loaded successfully."));
                    ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, "");
                } else {
                    ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, W010_EmbUI_getCommonString("messages.config_load_fail", "Configuration file not found. Default values applied."));
                    ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, "");
                }
                break;
            case C_ID_RESET_CONFIG_BTN:
                M010_Config_initDefaults();
                if (M010_Config_save()) { // 기본값으로 초기화 후 저장 시도
                    W010_EmbUI_loadConfigToWebUI();
                    ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, W010_EmbUI_getCommonString("messages.config_reset_success", "Configuration reset to default."));
                    ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, "");
                } else { // 기본값 저장 실패 시
                    ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, W010_EmbUI_getCommonString("messages.config_reset_fail", "Error saving default configuration."));
                    ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, "");
                }
                break;
            default:
                ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, W010_EmbUI_getCommonString("messages.unknown_command", "Unknown command detected."));
                ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, "");
                dbgP1_printf(F("Unknown button control ID: %d\n"), p_control->id);
                break;
        }
    } else if (p_control->type == ControlType::Number) {
        // 숫자 입력 필드 처리
        switch (controlEnumId) {
            case C_ID_MVSTATE_ACCELFILTER_ALPHA:
                g_M010_Config.mvState_accelFilter_Alpha = p_control->value.toFloat();
                break;
            case C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN:
                g_M010_Config.mvState_Forward_speedKmh_Threshold_Min = p_control->value.toFloat();
                break;
            case C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN:
                g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min = p_control->value.toFloat();
                break;
            case C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX:
                g_M010_Config.mvState_Stop_speedKmh_Threshold_Max = p_control->value.toFloat();
                break;
            case C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX:
                g_M010_Config.mvState_Stop_accelMps2_Threshold_Max = p_control->value.toFloat();
                break;
            case C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX:
                g_M010_Config.mvState_Stop_gyroDps_Threshold_Max = p_control->value.toFloat();
                break;
            case C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN:
                g_M010_Config.mvState_stop_durationMs_Stable_Min = p_control->value.toInt();
                break;
            case C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN:
                g_M010_Config.mvState_move_durationMs_Stable_Min = p_control->value.toInt();
                break;
            case C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD:
                g_M010_Config.mvState_Decel_accelMps2_Threshold = p_control->value.toFloat();
                break;
            case C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD:
                g_M010_Config.mvState_Bump_accelMps2_Threshold = p_control->value.toFloat();
                break;
            case C_ID_BUMP_SPEEDKMH_MIN:
                g_M010_Config.mvState_Bump_SpeedKmh_Min = p_control->value.toFloat();
                break;
            case C_ID_BUMP_COOLDOWNMS:
                g_M010_Config.mvState_Bump_CooldownMs = p_control->value.toInt();
                break;
            case C_ID_DECEL_DURATIONMS_HOLD:
                g_M010_Config.mvState_Decel_durationMs_Hold = p_control->value.toInt();
                break;
            case C_ID_BUMP_DURATIONMS_HOLD:
                g_M010_Config.mvState_Bump_durationMs_Hold = p_control->value.toInt();
                break;
            case C_ID_MVSTATE_SIGNALWAIT1_SECONDS:
                g_M010_Config.mvState_signalWait1_Seconds = p_control->value.toInt();
                break;
            case C_ID_MVSTATE_SIGNALWAIT2_SECONDS:
                g_M010_Config.mvState_signalWait2_Seconds = p_control->value.toInt();
                break;
            case C_ID_MVSTATE_STOPPED1_SECONDS:
                g_M010_Config.mvState_stopped1_Seconds = p_control->value.toInt();
                break;
            case C_ID_MVSTATE_STOPPED2_SECONDS:
                g_M010_Config.mvState_stopped2_Seconds = p_control->value.toInt();
                break;
            case C_ID_MVSTATE_PARK_SECONDS:
                g_M010_Config.mvState_park_Seconds = p_control->value.toInt();
                break;
            case C_ID_SERIALPRINT_INTERVALMS:
                g_M010_Config.serialPrint_intervalMs = p_control->value.toInt();
                break;
            case C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold = p_control->value.toFloat();
                break;
            case C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold = p_control->value.toFloat();
                break;
            case C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold = p_control->value.toFloat();
                break;
            case C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold = p_control->value.toFloat();
                break;
            case C_ID_TURNSTATE_SPEEDKMH_MINSPEED:
                g_M010_Config.turnState_speedKmh_MinSpeed = p_control->value.toFloat();
                break;
            case C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD:
                g_M010_Config.turnState_speedKmh_HighSpeed_Threshold = p_control->value.toFloat();
                break;
            case C_ID_TURNSTATE_STABLEDURATIONMS:
                g_M010_Config.turnState_StableDurationMs = p_control->value.toInt();
                break;
            default:
                ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, W010_EmbUI_getCommonString("messages.unknown_command", "Unknown command detected for Number control."));
                ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, "");
                dbgP1_printf(F("Unknown Number control ID: %d\n"), p_control->id);
                break;
        }
    } else {
        // 다른 유형의 컨트롤 (Label 등)은 여기서 직접 처리할 필요가 없을 수 있습니다.
        ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, W010_EmbUI_getCommonString("messages.unknown_command", "Unknown command detected for control type."));
        ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, "");
        dbgP1_printf(F("Unhandled control type: %d, ID: %d\n"), p_control->type, p_control->id);
    }
}

/**
 * @brief ESPUI ID를 M010_CAR_CONTROL_ID_E enum 값으로 변환하는 헬퍼 함수
 * @param espuiId ESPUI 컨트롤의 실제 ID
 * @return 매핑되는 enum 값, 없으면 -1 반환
 */
int W010_EmbUI_getEnumIdFromEspuiId(uint16_t espuiId) {
    for (size_t i = 0; i < g_W010_UiControlMap_Arr_Size; ++i) {
        if (g_W010_UiControlMap_Arr[i].ui_ctl_var_ptr != nullptr && *g_W010_UiControlMap_Arr[i].ui_ctl_var_ptr == espuiId) {
            return g_W010_UiControlMap_Arr[i].enum_no;
        }
    }
    // 알림 및 오류 레이블은 g_W010_UiControlMap_Arr에 직접 enum_no을 가지고 있지 않으므로 별도 처리
    if (espuiId == g_W010_C_ID_ALARAM_LABEL_Id) return C_ID_ALARM_LABEL;
    if (espuiId == g_W010_C_ID_ERROR_LABEL_Id) return C_ID_ERROR_LABEL;

    return -1; // 찾지 못함
}

/**
 * @brief g_M010_CarStatus 구조체의 현재 자동차 상태 정보를 ESPUI 웹페이지에 주기적으로 업데이트합니다.
 * "status" 페이지에 정의된 Label 컨트롤들의 값을 실시간으로 갱신합니다.
 */
void W010_EmbUI_updateCarStatusWeb() {
    String v_movementStateStr;
    switch (g_M010_CarStatus.carMovementState) {
        case E_M010_CARMOVESTATE_UNKNOWN:
            v_movementStateStr = W010_EmbUI_getCommonString("car_movement_states.E_M010_CARMOVESTATE_UNKNOWN");
            break;
        case E_M010_CARMOVESTATE_STOPPED_INIT:
            v_movementStateStr = W010_EmbUI_getCommonString("car_movement_states.E_M010_CARMOVESTATE_STOPPED_INIT");
            break;
        case E_M010_CARMOVESTATE_SIGNAL_WAIT1:
            v_movementStateStr = W010_EmbUI_getCommonString("car_movement_states.E_M010_CARMOVESTATE_SIGNAL_WAIT1") + F(" (") + String(g_M010_Config.mvState_signalWait1_Seconds) + W010_EmbUI_getCommonString("messages.signal_wait1_suffix");
            break;
        case E_M010_CARMOVESTATE_SIGNAL_WAIT2:
            v_movementStateStr = W010_EmbUI_getCommonString("car_movement_states.E_M010_CARMOVESTATE_SIGNAL_WAIT2") + F(" (") + String(g_M010_Config.mvState_signalWait2_Seconds) + W010_EmbUI_getCommonString("messages.signal_wait2_suffix");
            break;
        case E_M010_CARMOVESTATE_STOPPED1:
            v_movementStateStr = W010_EmbUI_getCommonString("car_movement_states.E_M010_CARMOVESTATE_STOPPED1") + F(" (") + String(g_M010_Config.mvState_stopped1_Seconds / 60) + W010_EmbUI_getCommonString("messages.stopped1_suffix");
            break;
        case E_M010_CARMOVESTATE_STOPPED2:
            v_movementStateStr = W010_EmbUI_getCommonString("car_movement_states.E_M010_CARMOVESTATE_STOPPED2") + F(" (") + String(g_M010_Config.mvState_stopped2_Seconds / 60) + W010_EmbUI_getCommonString("messages.stopped2_suffix");
            break;
        case E_M010_CARMOVESTATE_PARKED:
            v_movementStateStr = W010_EmbUI_getCommonString("car_movement_states.E_M010_CARMOVESTATE_PARKED") + W010_EmbUI_getCommonString("messages.parked_prefix") + String(g_M010_Config.mvState_park_Seconds / 60) + W010_EmbUI_getCommonString("messages.parked_suffix");
            break;
        case E_M010_CARMOVESTATE_FORWARD:
            v_movementStateStr = W010_EmbUI_getCommonString("car_movement_states.E_M010_CARMOVESTATE_FORWARD");
            break;
        case E_M010_CARMOVESTATE_REVERSE:
            v_movementStateStr = W010_EmbUI_getCommonString("car_movement_states.E_M010_CARMOVESTATE_REVERSE");
            break;
        default: // 안전을 위해 추가
            v_movementStateStr = W010_EmbUI_getCommonString("car_movement_states.UNKNOWN_ENUM_STATE", "Unknown");
            break;
    }

    String v_turnStateStr;
    switch (g_M010_CarStatus.carTurnState) {
        case E_M010_CARTURNSTATE_CENTER:
            v_turnStateStr = W010_EmbUI_getCommonString("car_turn_states.E_M010_CARTURNSTATE_CENTER");
            break;
        case E_M010_CARTURNSTATE_LEFT_1:
            v_turnStateStr = W010_EmbUI_getCommonString("car_turn_states.E_M010_CARTURNSTATE_LEFT_1");
            break;
        case E_M010_CARTURNSTATE_LEFT_2:
            v_turnStateStr = W010_EmbUI_getCommonString("car_turn_states.E_M010_CARTURNSTATE_LEFT_2");
            break;
        case E_M010_CARTURNSTATE_LEFT_3:
            v_turnStateStr = W010_EmbUI_getCommonString("car_turn_states.E_M010_CARTURNSTATE_LEFT_3");
            break;
        case E_M010_CARTURNSTATE_RIGHT_1:
            v_turnStateStr = W010_EmbUI_getCommonString("car_turn_states.E_M010_CARTURNSTATE_RIGHT_1");
            break;
        case E_M010_CARTURNSTATE_RIGHT_2:
            v_turnStateStr = W010_EmbUI_getCommonString("car_turn_states.E_M010_CARTURNSTATE_RIGHT_2");
            break;
        case E_M010_CARTURNSTATE_RIGHT_3:
            v_turnStateStr = W010_EmbUI_getCommonString("car_turn_states.E_M010_CARTURNSTATE_RIGHT_3");
            break;
        default: // 안전을 위해 추가
            v_turnStateStr = W010_EmbUI_getCommonString("car_turn_states.UNKNOWN_ENUM_TURN_STATE", "Unknown");
            break;
    }

    // 상태 레이블 업데이트
    ESPUI.updateControlValue(g_W010_C_ID_CARMOVEMENTSTATE_LABEL_Id, W010_EmbUI_getLabelFromLangDoc("C_ID_CARMOVEMENTSTATE_LABEL") + " " + v_movementStateStr);
    ESPUI.updateControlValue(g_W010_C_ID_CARTURNSTATE_LABEL_Id, W010_EmbUI_getLabelFromLangDoc("C_ID_CARTURNSTATE_LABEL") + " " + v_turnStateStr);
    ESPUI.updateControlValue(g_W010_C_ID_SPEED_KMH_LABEL_Id, String(g_M010_CarStatus.speed_kmh, 2) + W010_EmbUI_getCommonString("units.speed"));
    ESPUI.updateControlValue(g_W010_C_ID_ACCELX_MS2_LABEL_Id, String(g_M010_CarStatus.accelX_ms2, 2) + W010_EmbUI_getCommonString("units.accel"));
    ESPUI.updateControlValue(g_W010_C_ID_ACCELY_MS2_LABEL_Id, String(g_M010_CarStatus.accelY_ms2, 2) + W010_EmbUI_getCommonString("units.accel"));
    ESPUI.updateControlValue(g_W010_C_ID_ACCELZ_MS2_LABEL_Id, String(g_M010_CarStatus.accelZ_ms2, 2) + W010_EmbUI_getCommonString("units.accel"));
    ESPUI.updateControlValue(g_W010_C_ID_YAWANGLE_DEG_LABEL_Id, String(g_M010_CarStatus.yawAngle_deg, 2) + W010_EmbUI_getCommonString("units.angle"));
    ESPUI.updateControlValue(g_W010_C_ID_PITCHANGLE_DEG_LABEL_Id, String(g_M010_CarStatus.pitchAngle_deg, 2) + W010_EmbUI_getCommonString("units.angle"));
    ESPUI.updateControlValue(g_W010_C_ID_YAWANGLEVELOCITY_DEGPS_LABEL_Id, String(g_M010_CarStatus.yawAngleVelocity_degps, 2) + W010_EmbUI_getCommonString("units.angular_velocity"));
    ESPUI.updateControlValue(g_W010_C_ID_ISEMERGENCYBRAKING_LABEL_Id, g_M010_CarStatus.isEmergencyBraking ? W010_EmbUI_getCommonString("boolean_states.true") : W010_EmbUI_getCommonString("boolean_states.false"));
    ESPUI.updateControlValue(g_W010_C_ID_ISSPEEDBUMPDETECTED_LABEL_Id, g_M010_CarStatus.isSpeedBumpDetected ? W010_EmbUI_getCommonString("boolean_states.true") : W010_EmbUI_getCommonString("boolean_states.false"));
    ESPUI.updateControlValue(g_W010_C_ID_CURRENTSTOPTIME_SEC_LABEL_Id, String(g_M010_CarStatus.currentStopTime_ms / 1000) + W010_EmbUI_getCommonString("units.time_seconds"));
}

// 각 enum 값을 문자열로 변환하는 헬퍼 함수 (main.cpp 또는 M010_CarState_001.h에 정의되어 있을 것으로 예상)
// 이 함수들은 언어에 종속되지 않고 enum 값을 나타내는 고유 문자열을 반환해야 합니다.
String W010_EmbUI_getCarMovementStateEnumString(T_M010_CarMovementState state) {
    switch (state) {
        case E_M010_CARMOVESTATE_UNKNOWN        : return "E_M010_CARMOVESTATE_UNKNOWN";
        case E_M010_CARMOVESTATE_STOPPED_INIT   : return "E_M010_CARMOVESTATE_STOPPED_INIT";
        case E_M010_CARMOVESTATE_SIGNAL_WAIT1   : return "E_M010_CARMOVESTATE_SIGNAL_WAIT1";
        case E_M010_CARMOVESTATE_SIGNAL_WAIT2   : return "E_M010_CARMOVESTATE_SIGNAL_WAIT2";
        case E_M010_CARMOVESTATE_STOPPED1       : return "E_M010_CARMOVESTATE_STOPPED1";
        case E_M010_CARMOVESTATE_STOPPED2       : return "E_M010_CARMOVESTATE_STOPPED2";
        case E_M010_CARMOVESTATE_PARKED         : return "E_M010_CARMOVESTATE_PARKED";
        case E_M010_CARMOVESTATE_FORWARD        : return "E_M010_CARMOVESTATE_FORWARD";
        case E_M010_CARMOVESTATE_REVERSE        : return "E_M010_CARMOVESTATE_REVERSE";
        default: return "UNKNOWN_ENUM_STATE";
    }
}

String W010_EmbUI_getCarTurnStateEnumString(T_M010_CarTurnState state) {
    switch (state) {
        case E_M010_CARTURNSTATE_CENTER     : return "E_M010_CARTURNSTATE_CENTER";
        case E_M010_CARTURNSTATE_LEFT_1     : return "E_M010_CARTURNSTATE_LEFT_1";
        case E_M010_CARTURNSTATE_LEFT_2     : return "E_M010_CARTURNSTATE_LEFT_2";
        case E_M010_CARTURNSTATE_LEFT_3     : return "E_M010_CARTURNSTATE_LEFT_3";
        case E_M010_CARTURNSTATE_RIGHT_1    : return "E_M010_CARTURNSTATE_RIGHT_1";
        case E_M010_CARTURNSTATE_RIGHT_2    : return "E_M010_CARTURNSTATE_RIGHT_2";
        case E_M010_CARTURNSTATE_RIGHT_3    : return "E_M010_CARTURNSTATE_RIGHT_3";
        default                             : return "UNKNOWN_ENUM_TURN_STATE";
    }
}

/**
 * @brief ESPUI의 메인 루프를 실행하고, 주기적으로 자동차 상태를 웹에 업데이트합니다.
 * @note 이 함수는 Arduino의 `loop()` 함수에서 주기적으로 호출되어야 합니다.
 */
void W010_EmbUI_run() {
    static u_int32_t v_lastWebUpdateTime_ms = 0;
    if (millis() - v_lastWebUpdateTime_ms >= g_M010_Config.serialPrint_intervalMs) {
        W010_EmbUI_updateCarStatusWeb();
        v_lastWebUpdateTime_ms = millis();
    }
}

/**
 * @brief Preferences에서 마지막으로 선택된 언어 설정을 로드합니다.
 */
void W010_EmbUI_load_Pref_LastLang() {
    dbgP1_println(F("Loading last language from Preferences..."));

    // "embui" 네임스페이스로 Preferences를 엽니다.
    if (!g_W010_preferences.begin("embui", true)) { // true는 읽기 전용 모드
        dbgP1_println(F("Failed to open preferences for loading language."));
        return;
    }

    // "last_lang" 키에서 언어 코드를 읽어옵니다. 기본값은 "ko"
    String loadedLang = g_W010_preferences.getString("last_lang", "ko");
    
    // Preferences를 닫습니다.
    g_W010_preferences.end();

    if (loadedLang.length() > 0) {
        g_W010_currentLang = loadedLang;
        dbgP1_printf("Last used language loaded: %s\n", g_W010_currentLang.c_str());
    } else {
        dbgP1_println(F("No last language setting found. Using default (ko)."));
        g_W010_currentLang = "ko"; //Preferences에 값이 없을 경우 기본값 설정
    }
}

/**
 * @brief 현재 선택된 언어 설정을 Preferences에 저장합니다.
 */
void W010_EmbUI_save_Pref_LastLang() {
    dbgP1_printf(F("Saving current language to Preferences: %s\n"), g_W010_currentLang.c_str());

    // "embui" 네임스페이스로 Preferences를 엽니다.
    if (!g_W010_preferences.begin("embui", false)) { // false는 읽기/쓰기 모드
        dbgP1_println(F("Failed to open preferences for saving language."));
        return;
    }

    // "last_lang" 키에 현재 언어 코드를 저장합니다.
    g_W010_preferences.putString("last_lang", g_W010_currentLang);
    
    // Preferences를 닫습니다. (변경 사항을 플래시에 기록)
    g_W010_preferences.end();

    dbgP1_println(F("Current language saved successfully."));
}

/**
 * @brief ESPUI UI를 현재 언어 설정에 따라 다시 구성합니다.
 * 언어 변경 시 UI 전체를 리로드하는 데 사용됩니다.
 */
void W010_EmbUI_rebuildUI() {
    dbgP1_printf("UI 재구성 중... 새로운 언어: %s\n", g_W010_currentLang.c_str());

    // 1. 새로운 언어의 UI 설정 JSON 파일 로드
    if (!W010_EmbUI_load_Json_UiLanguage(g_W010_currentLang)) {
        dbgP1_printf("UI 설정 JSON 파일 (%s) 로드 실패. 기본 언어(ko)로 재시도.\n", g_W010_currentLang.c_str());
        g_W010_currentLang = "ko"; // 실패 시 기본 언어로 강제 설정
        if (!W010_EmbUI_load_Json_UiLanguage(g_W010_currentLang)) {
            dbgP1_println(F("기본 언어 UI 설정 파일도 로드 실패. UI 업데이트 불가."));
            ESPUI.updateControlValue(g_W010_C_ID_ERROR_LABEL_Id, W010_EmbUI_getCommonString("messages.ui_load_fail_critical"));
            return; // 치명적인 오류이므로 여기서 종료
        }
    }

    // 2. 모든 탭과 컨트롤의 레이블 업데이트 및 값 다시 로드
    W010_EmbUI_loadConfigToWebUI();

    // 3. 언어 변경 성공 메시지 표시 (선택 사항)
    ESPUI.updateControlValue(g_W010_C_ID_ALARAM_LABEL_Id, W010_EmbUI_getCommonString("messages.lang_change_success"));

    dbgP1_println(W010_EmbUI_getCommonString("messages.ui_rebuild_done"));
}
