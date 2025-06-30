#pragma once
// W010_embUI_010.h
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
#include "M010_main3_011.h"


extern  T_M010_Config       g_M010_Config;
extern  T_M010_CarStatus    g_M010_CarStatus; // 자동차 상태 구조체 인스턴스


// JSON 파일 경로 정의 (언어 코드에 따라 동적으로 결정)
#define         G_W010_UI_LANG_FILE_PREFIX           "/W010_ESPUI_ui_lang_005_"
#define         G_W010_UI_LANG_FILE_SUBFIX           ".json"

#define         G_W010_UI_DEFAULT_CONFIG_FILE        "/W010_ESPUI_ui_defaults_005.json"


#define         G_W010_LAST_LANG_FILE               "/last_lang.txt" // 마지막 언어 설정을 저장할 파일

// 현재 선택된 언어 코드 (예: "ko", "en")
String          g_W010_currentLanguage          = "ko"; // 기본 언어는 한국어

JsonDocument    g_W010_uiLayoutDoc; // UI 레이아웃 및 기본값을 위한 JSON 문서
JsonDocument    g_W010_uiLangDoc;   // 다국어 문자열을 위한 JSON 문서

// JSON에서 로드된 UI 설정을 저장할 json doc 객체
// JsonDocument g_W010_uiConfigDoc; 


// ====================================================================================================
// 전역 변수 선언
// ====================================================================================================

// ESPUI는 페이지 자체를 컨트롤로 추가하고, 그 ID를 페이지 ID로 사용합니다.
uint16_t g_W010_Tab_Config_Id;
uint16_t g_W010_Tab_Status_Id;

uint16_t g_W010_Control_Alaram_Id;
uint16_t g_W010_Control_Error_Id;
uint16_t g_W010_Control_Language_Id; // 언어 선택 드롭다운 컨트롤 ID


// ESPUI 컨트롤 ID 정의
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
    C_ID_MVSTATE_BUMP_SPEEDKMH_MIN,
    C_ID_MVSTATE_BUMP_COOLDOWNMS,
    C_ID_MVSTATE_DECEL_DURATIONMS_HOLD,
    C_ID_MVSTATE_BUMP_DURATIONMS_HOLD,
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
};

// 매직 문자열을 위한 상수 정의
#define BTN_CMD_SAVE_CONFIG     F("save_config")
#define BTN_CMD_LOAD_CONFIG     F("load_config")
#define BTN_CMD_RESET_CONFIG    F("reset_config")

// JSON enum_id와 C++ enum 값 매핑을 위한 구조체
struct ControlMapEntry {
    const char* idStr;
    int enumVal;
};

// 모든 컨트롤 ID를 매핑하는 배열
const ControlMapEntry controlMap[] = {
    // Config Page Controls
    {"C_ID_MVSTATE_ACCELFILTER_ALPHA"                       , C_ID_MVSTATE_ACCELFILTER_ALPHA},
    {"C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN"          , C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN},
    {"C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN"          , C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN},
    {"C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX"             , C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX},
    {"C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX"            , C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX},
    {"C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX"              , C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX},
    {"C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN"              , C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN},
    {"C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN"              , C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN},
    {"C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD"               , C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD},
    {"C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD"                , C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD},
    {"C_ID_MVSTATE_BUMP_SPEEDKMH_MIN"                       , C_ID_MVSTATE_BUMP_SPEEDKMH_MIN},
    {"C_ID_MVSTATE_BUMP_COOLDOWNMS"                         , C_ID_MVSTATE_BUMP_COOLDOWNMS},
    {"C_ID_MVSTATE_DECEL_DURATIONMS_HOLD"                   , C_ID_MVSTATE_DECEL_DURATIONMS_HOLD},
    {"C_ID_MVSTATE_BUMP_DURATIONMS_HOLD"                    , C_ID_MVSTATE_BUMP_DURATIONMS_HOLD},
    {"C_ID_MVSTATE_SIGNALWAIT1_SECONDS"                     , C_ID_MVSTATE_SIGNALWAIT1_SECONDS},
    {"C_ID_MVSTATE_SIGNALWAIT2_SECONDS"                     , C_ID_MVSTATE_SIGNALWAIT2_SECONDS},
    {"C_ID_MVSTATE_STOPPED1_SECONDS"                        , C_ID_MVSTATE_STOPPED1_SECONDS},
    {"C_ID_MVSTATE_STOPPED2_SECONDS"                        , C_ID_MVSTATE_STOPPED2_SECONDS},
    {"C_ID_MVSTATE_PARK_SECONDS"                            , C_ID_MVSTATE_PARK_SECONDS},
    {"C_ID_SERIALPRINT_INTERVALMS"                          , C_ID_SERIALPRINT_INTERVALMS},
    {"C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD" , C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD},
    {"C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD"   , C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD},
    {"C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD"   , C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD},
    {"C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD"   , C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD},
    {"C_ID_TURNSTATE_SPEEDKMH_MINSPEED"                     , C_ID_TURNSTATE_SPEEDKMH_MINSPEED},
    {"C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD"          , C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD},
    {"C_ID_TURNSTATE_STABLEDURATIONMS"                      , C_ID_TURNSTATE_STABLEDURATIONMS},
    {"C_ID_SAVE_CONFIG_BTN"                                 , C_ID_SAVE_CONFIG_BTN},
    {"C_ID_LOAD_CONFIG_BTN"                                 , C_ID_LOAD_CONFIG_BTN},
    {"C_ID_RESET_CONFIG_BTN"                                , C_ID_RESET_CONFIG_BTN},
    {"C_ID_LANGUAGE_SELECT"                                 , C_ID_LANGUAGE_SELECT},

    // Status Page Controls
    {"C_ID_CARMOVEMENTSTATE_LABEL"                          , C_ID_CARMOVEMENTSTATE_LABEL},
    {"C_ID_CARTURNSTATE_LABEL"                              , C_ID_CARTURNSTATE_LABEL},
    {"C_ID_SPEED_KMH_LABEL"                                 , C_ID_SPEED_KMH_LABEL},
    {"C_ID_ACCELX_MS2_LABEL"                                , C_ID_ACCELX_MS2_LABEL},
    {"C_ID_ACCELY_MS2_LABEL"                                , C_ID_ACCELY_MS2_LABEL},
    {"C_ID_ACCELZ_MS2_LABEL"                                , C_ID_ACCELZ_MS2_LABEL},
    {"C_ID_YAWANGLE_DEG_LABEL"                              , C_ID_YAWANGLE_DEG_LABEL},
    {"C_ID_PITCHANGLE_DEG_LABEL"                            , C_ID_PITCHANGLE_DEG_LABEL},
    {"C_ID_YAWANGLEVELOCITY_DEGPS_LABEL"                    , C_ID_YAWANGLEVELOCITY_DEGPS_LABEL},
    {"C_ID_ISEMERGENCYBRAKING_LABEL"                        , C_ID_ISEMERGENCYBRAKING_LABEL},
    {"C_ID_ISSPEEDBUMPDETECTED_LABEL"                       , C_ID_ISSPEEDBUMPDETECTED_LABEL},
    {"C_ID_CURRENTSTOPTIME_SEC_LABEL"                       , C_ID_CURRENTSTOPTIME_SEC_LABEL}
};
const size_t controlMapSize = sizeof(controlMap) / sizeof(controlMap[0]);

#include <Preferences.h>
Preferences g_W010_preferences; // Preferences 객체
//String g_W010_currentLanguage; // 현재 언어 설정


/**
 * @brief 현재 선택된 언어를 영구 저장소에 저장합니다.
 * Preferences 라이브러리를 사용하여 NVS에 저장합니다.
 * @param langCode 저장할 언어 코드 (예: "ko", "en")
 */
void W010_EmbUI_saveLastLanguage(const String& langCode) {
    dbgP1_printf(F("Saving last language: %s\n"), langCode.c_str());

    // "embui" 네임스페이스로 Preferences를 엽니다.
    if (!g_W010_preferences.begin("embui", false)) { // false는 읽기/쓰기 모드
        dbgP1_println(F("Failed to open preferences for saving language."));
        return;
    }

    // "last_lang" 키에 언어 코드를 저장합니다.
    g_W010_preferences.putString("last_lang", langCode);
    
    // Preferences를 닫습니다. (변경 사항을 플래시에 기록)
    g_W010_preferences.end();

    dbgP1_println(F("Last language saved successfully."));
}

// ====================================================================================================
// 함수 선언 (프로토타입)
// ====================================================================================================
// bool W010_EmbUI_loadUIConfig(const String& langCode); // UI 설정 JSON 로드 함수 (언어 코드 추가)
String W010_EmbUI_getCommonString(const char* key, const char* defaultVal = ""); // 다국어 문자열 가져오는 헬퍼 함수
String W010_EmbUI_getLabelFromLangDoc(const String& p_enumIdStr); // 언어 문서에서 enum_id에 해당하는 label을 찾는 함수
String W010_EmbUI_getControlLabel(const String& enumIdStr); // 컨트롤 레이블 가져오는 헬퍼 함수
bool W010_EmbUI_loadUILayoutDefaults(); // 
bool W010_EmbUI_loadUILanguage(const String& langCode); // 언어 파일 로드 함수
void W010_EmbUI_init(); // 함수 이름은 EmbUI 그대로 두지만, 내부 구현은 ESPUI를 사용
void W010_EmbUI_setupWebPages();
void W010_EmbUI_loadConfigToWebUI();
void W010_ESPUI_callback(Control* p_control, int p_value);
void W010_EmbUI_updateCarStatusWeb();
String W010_EmbUI_getCarTurnStateEnumString(T_M010_CarTurnState state);
String W010_EmbUI_getCarMovementStateEnumString(T_M010_CarMovementState state);
void W010_EmbUI_updateStatusLabels();
void W010_EmbUI_run();
void W010_EmbUI_loadLastLanguage(); // 마지막 선택된 언어를 로드
void W010_EmbUI_saveLastLanguage(); // 현재 언어 설정을 저장
void W010_EmbUI_rebuildUI(); // UI를 다시 그리는 함수




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


String W010_EmbUI_getControlLabel(const String& p_enumIdStr) {
    // 레이아웃 문서에서 enum_id를 찾아 해당 컨트롤의 레이블을 언어 문서에서 가져옵니다.
    // 레이아웃 문서의 "tabs" 구조를 따라가면서 enum_id를 찾습니다.
    JsonArray tabs = g_W010_uiLayoutDoc["tabs"].as<JsonArray>(); // g_W010_uiLayoutDoc 사용
    for (JsonObject tab : tabs) {
        JsonArray controls = tab["controls"].as<JsonArray>();
        for (JsonObject control : controls) {
            if (control["enum_id"].as<String>().equals(p_enumIdStr)) {
                // 해당 enum_id에 맞는 레이블을 언어 문서에서 찾습니다.
                // 언어 문서의 "tabs" 구조를 다시 따라가야 합니다.
                // 더 효율적인 방법: 언어 문서에 enum_id -> label 맵을 직접 구성.
                // 여기서는 기존 getControlLabel 로직을 언어 문서에 그대로 적용했다고 가정.
                return W010_EmbUI_getLabelFromLangDoc(p_enumIdStr);
            }
        }
    }
    return String(""); // 찾지 못함
}

// 새로운 헬퍼 함수: 언어 문서에서 enum_id에 해당하는 label을 찾는 함수
String W010_EmbUI_getLabelFromLangDoc(const String& p_enumIdStr) {
    JsonArray tabs = g_W010_uiLangDoc["tabs"].as<JsonArray>(); // g_W010_uiLangDoc 사용
    for (JsonObject tab : tabs) {
        JsonArray controls = tab["controls"].as<JsonArray>();
        for (JsonObject control : controls) {
            if (control["enum_id"].as<String>().equals(p_enumIdStr)) {
                JsonVariant labelVariant = control["label"];
                if (!labelVariant.isNull()) {
                    return labelVariant.as<String>();
                }
            }
        }
    }
    return String(""); // 찾지 못함
}



bool W010_EmbUI_loadUILayoutDefaults() {
	const char* filePath = G_W010_UI_DEFAULT_CONFIG_FILE; // 고정된 기본값 파일
    // const char* filePath = "/W010_ESPUI_ui_layout_defaults.json"; // 고정된 기본값 파일
    dbgP1_printf("UI 레이아웃 및 기본값 파일 로드 중: %s\n", filePath);

    File configFile = LittleFS.open(filePath, "r");
    if (!configFile) {
        dbgP1_printf("파일 열기 실패: %s\n", filePath);
        return false;
    }

    DeserializationError error = deserializeJson(g_W010_uiLayoutDoc, configFile);
    configFile.close();
    if (error) {
        dbgP1_printf("JSON 파싱 실패: %s\n", error.c_str());
        return false;
    }
    dbgP1_printf("UI 레이아웃 및 기본값 파일 로드 및 파싱 완료: %s\n", filePath);
    return true;
}




bool W010_EmbUI_loadUILanguage(const String& langCode) {
    String filePath = String(G_W010_UI_LANG_FILE_PREFIX) + langCode + G_W010_UI_LANG_FILE_SUBFIX;
    dbgP1_printf("UI 언어 파일 로드 중: %s\n", filePath.c_str());

    File langFile = LittleFS.open(filePath, "r");
    if (!langFile) {
        dbgP1_printf("파일 열기 실패: %s\n", filePath.c_str());
        return false;
    }

    DeserializationError error = deserializeJson(g_W010_uiLangDoc, langFile);
    langFile.close();
    if (error) {
        dbgP1_printf("JSON 파싱 실패: %s\n", error.c_str());
        return false;
    }
    dbgP1_printf("UI 언어 파일 로드 및 파싱 완료: %s\n", filePath.c_str());
    return true;
}

void W010_EmbUI_init() {
    dbgP1_println(F("ESPUI 초기화 중..."));

    if (!LittleFS.begin()) {
        dbgP1_println(F("LittleFS 마운트 실패! UI 설정 로드 불가."));
        return;
    }

    if (!W010_EmbUI_loadUILayoutDefaults()) {
        dbgP1_println(F("UI 레이아웃 및 기본값 파일 로드 실패. UI 구성에 문제 발생 가능."));
    }

    W010_EmbUI_loadLastLanguage();

    if (!W010_EmbUI_loadUILanguage(g_W010_currentLanguage)) {
        dbgP1_printf("UI 언어 파일 (%s) 로드 실패. 기본 언어(ko)로 재시도.\n", g_W010_currentLanguage.c_str());
        g_W010_currentLanguage = "ko";
        if (!W010_EmbUI_loadUILanguage(g_W010_currentLanguage)) {
            dbgP1_println(F("기본 언어 UI 설정 파일도 로드 실패. 다국어 지원 불가."));
        }
    }

    ESPUI.begin("Robot Eye Car", "roboteye", "carpassword");
    
    // 이전에 setupWebPages()에서 생성된 언어 선택 컨트롤에 콜백 할당
    // g_W010_Control_Language_Id는 setupWebPages()에서 ESPUI.addControl() 후 할당됩니다.
    // 따라서 setupWebPages()가 호출된 이후에 이 할당이 이루어져야 합니다.
    Control* v_langControl = ESPUI.getControl(g_W010_Control_Language_Id);
    if (v_langControl) {
        // 'userData' 멤버가 없으므로 이 줄을 제거하거나 대체해야 합니다.
        // v_langControl->userData = reinterpret_cast<void*>(static_cast<uintptr_t>(C_ID_LANGUAGE_SELECT)); // 이 줄 삭제 또는 주석 처리

        // 콜백 할당은 가능
        v_langControl->callback = &W010_ESPUI_callback;
    } else {
        dbgP1_println(F("언어 선택 컨트롤을 찾을 수 없습니다. 콜백 할당 실패."));
    }

    dbgP1_println(W010_EmbUI_getCommonString("messages.ui_init_done", "ESPUI initialization done."));
}

// void W010_EmbUI_init() {
//     dbgP1_println(F("ESPUI 초기화 중..."));

//     if (!LittleFS.begin()) {
//         dbgP1_println(F("LittleFS 마운트 실패! UI 설정 로드 불가."));
//         return;
//     }

//     // 1. UI 레이아웃 및 기본값 파일 로드 (한 번만 로드)
//     if (!W010_EmbUI_loadUILayoutDefaults()) {
//         dbgP1_println(F("UI 레이아웃 및 기본값 파일 로드 실패. UI 구성에 문제 발생 가능."));
//         // 이 경우 default_value를 사용할 수 없으므로, 코드를 통해 기본값을 직접 설정하거나
//         // 최소한의 UI만 구성하도록 폴백 로직 필요
//     }

//     // 2. 마지막 언어 설정 로드 시도 및 해당 언어 파일 로드
//     W010_EmbUI_loadLastLanguage(); // g_W010_currentLanguage 설정

//     if (!W010_EmbUI_loadUILanguage(g_W010_currentLanguage)) {
//         dbgP1_printf("UI 언어 파일 (%s) 로드 실패. 기본 언어(ko)로 재시도.\n", g_W010_currentLanguage.c_str());
//         g_W010_currentLanguage = "ko"; // 실패 시 기본 언어로 강제 설정
//         if (!W010_EmbUI_loadUILanguage(g_W010_currentLanguage)) {
//             dbgP1_println(F("기본 언어 UI 설정 파일도 로드 실패. 다국어 지원 불가."));
//         }
//     }

//     ESPUI.begin("Robot Eye Car", "roboteye", "carpassword");

// 	Control* v_langControl = ESPUI.getControl(g_W010_Control_Language_Id);
//     if (v_langControl) {
//         v_langControl->callback = &W010_ESPUI_callback;
//         v_langControl->userData = reinterpret_cast<void*>(static_cast<uintptr_t>(C_ID_LANGUAGE_SELECT));
//     } else {
//         dbgP1_println(F("언어 선택 컨트롤을 찾을 수 없습니다. 콜백 할당 실패."));
//     }

//     dbgP1_println(W010_EmbUI_getCommonString("messages.ui_init_done", "ESPUI initialization done."));
// }



/**
 * @brief ESPUI 웹페이지 UI를 구성하고, g_M010_Config 구조체의 멤버 변수들을 웹 UI에 바인딩합니다.
 * 사용자가 웹 인터페이스를 통해 설정을 조회하고 변경할 수 있도록 필드를 정의합니다.
 */
void W010_EmbUI_setupWebPages() {
    dbgP1_println(W010_EmbUI_getCommonString("messages.ui_setup_start", "UI setup start..."));

    JsonArray v_tabs = g_W010_uiLayoutDoc["tabs"].as<JsonArray>();

    for (JsonObject v_tab : v_tabs) {
        String tabId = v_tab["id"].as<String>();
        String tabTitle = W010_EmbUI_getLabelFromLangDoc(tabId + "_title");

        uint16_t v_currentTab_Id;
        if (tabId.equals(F("config"))) {
            g_W010_Tab_Config_Id = ESPUI.addControl(Tab, tabTitle.c_str(), tabTitle);
            v_currentTab_Id = g_W010_Tab_Config_Id;

            // C_ID_LANGUAGE_SELECT는 콜백을 직접 addControl에 넘기지 않습니다.
            // 대신 ESPUI.getControl().callback = ... 방식으로 할당됩니다.
            g_W010_Control_Language_Id = ESPUI.addControl(
                                                            ControlType::Select,
                                                            W010_EmbUI_getCommonString("messages.lang_select_label", "Language Select").c_str(),
                                                            "", // 초기값은 비워두고 나중에 업데이트
                                                            ControlColor::Wetasphalt,
                                                            v_currentTab_Id
                                                        );
            
            ESPUI.addControl( ControlType::Option, W010_EmbUI_getCommonString("messages.lang_ko", "Korean").c_str(), "ko", ControlColor::Alizarin, g_W010_Control_Language_Id);
            ESPUI.addControl( ControlType::Option, W010_EmbUI_getCommonString("messages.lang_en", "English").c_str(), "en", ControlColor::Alizarin, g_W010_Control_Language_Id);

        } else if (tabId.equals(F("status"))) {
            g_W010_Tab_Status_Id = ESPUI.addControl(ControlType::Tab, tabTitle.c_str(), F(""), ControlColor::Wetasphalt);
            ESPUI.setVertical(g_W010_Tab_Status_Id, true);
            v_currentTab_Id = g_W010_Tab_Status_Id;
        } else {
            dbgP1_printf(String(W010_EmbUI_getCommonString("messages.unknown_tab_id", "Unknown Tab ID:") + " %s\n").c_str(), tabId.c_str());
            continue;
        }

		JsonArray v_controls = v_tab["controls"].as<JsonArray>();

        for (JsonObject v_control : v_controls) {
            String enumIdStr = v_control["enum_id"].as<String>();
            String label = W010_EmbUI_getLabelFromLangDoc(enumIdStr);
            JsonVariant defaultValue = v_control["default_value"];

            int controlEnumId = -1; // M010_CAR_CONTROL_ID_E enum 값
            // controlMap에서 enumIdStr에 해당하는 실제 enum 값을 찾음
            for (size_t i = 0; i < controlMapSize; ++i) {
                if (enumIdStr.equals(controlMap[i].idStr)) {
                    controlEnumId = controlMap[i].enumVal;
                    break;
                }
            }
            
            if (enumIdStr.equals(F("g_W010_Control_Alaram_Id"))) {
                g_W010_Control_Alaram_Id = ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, v_currentTab_Id);
                continue;
            } else if (enumIdStr.equals(F("g_W010_Control_Error_Id"))) {
                g_W010_Control_Error_Id = ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, v_currentTab_Id);
                continue;
            } else if (enumIdStr.equals(F("C_ID_LANGUAGE_SELECT"))) { 
                continue; // 이미 위에서 addControl 했으므로 건너뛰기
            }

            if (controlEnumId == -1) {
                dbgP1_printf(String(W010_EmbUI_getCommonString("messages.unknown_control_id", "Unknown control ID:") + " %s\n").c_str(), enumIdStr.c_str());
                continue;
            }

            if (tabId.equals(F("config"))) {
                // Button과 Number 컨트롤에 콜백을 할당하는 방식 변경
                // std::function을 사용하는 오버로딩에 맞춥니다.
                if (enumIdStr.endsWith(F("_BTN"))) {
                    // C_ID_XXX를 람다 캡처로 전달하여 콜백 내에서 사용할 수 있도록 함
                    ESPUI.addControl(ControlType::Button, label.c_str(), defaultValue.as<String>().c_str(), ControlColor::Emerald, v_currentTab_Id, 
                        [controlEnumId](Control* p_ctrl, int p_val) {
                            // 람다 함수 내부에서 W010_ESPUI_callback의 실제 로직을 호출하거나 여기에 직접 구현
                            // 여기서는 기존 W010_ESPUI_callback 함수를 호출하고,
                            // controlEnumId를 Control 객체 대신 직접 전달하는 방식으로 처리
                            // (주의: Control* p_ctrl->id를 사용하는 방식으로 콜백을 전면 재정비하는 것이 더 깔끔할 수 있음)
                            // 현재는 기존 W010_ESPUI_callback 함수의 구현을 최대한 활용하기 위해 우회적으로 전달합니다.
                            // W010_ESPUI_callback 함수 내에서 p_control->id 로 분기하도록 변경했으므로,
                            // Button/Number의 경우에도 p_control->id로 직접 식별해야 합니다.
                            // 즉, 아래 ControlType::Number와 동일하게 콜백을 직접 인자로 넘기면 됩니다.
                            W010_ESPUI_callback(p_ctrl, p_val);
                        });
                } else { // Number Control
                    // C_ID_XXX를 람다 캡처로 전달하여 콜백 내에서 사용할 수 있도록 함
                    ESPUI.addControl(ControlType::Number, label.c_str(), String(defaultValue.as<float>(), 3), ControlColor::Alizarin, v_currentTab_Id, 
                        [controlEnumId](Control* p_ctrl, int p_val) {
                             W010_ESPUI_callback(p_ctrl, p_val);
                        });
                }
            } else if (tabId.equals(F("status"))) {
                // Label 컨트롤은 콜백이 필요 없으며, userData도 필요 없음
                ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, v_currentTab_Id);
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
    JsonArray tabs = g_W010_uiLayoutDoc["tabs"].as<JsonArray>();
    for (JsonObject tab : tabs) {
        String tabId = tab["id"].as<String>();
        String tabTitle = W010_EmbUI_getLabelFromLangDoc(tabId + "_title");
        
        uint16_t currentTabEspuiId = 0;
        if (tabId.equals(F("config"))) currentTabEspuiId = g_W010_Tab_Config_Id;
        else if (tabId.equals(F("status"))) currentTabEspuiId = g_W010_Tab_Status_Id;

        if (currentTabEspuiId != 0) {
			ESPUI.updateControlLabel(currentTabEspuiId, tabTitle.c_str());
        }

        JsonArray controls = tab["controls"].as<JsonArray>();
        for (JsonObject control : controls) {
            String enumIdStr = control["enum_id"].as<String>();
            String label = W010_EmbUI_getLabelFromLangDoc(enumIdStr); // 언어 파일에서 레이블 가져오기

            int controlEnumId = -1;
            for (size_t i = 0; i < controlMapSize; ++i) {
                if (enumIdStr.equals(controlMap[i].idStr)) {
                    controlEnumId = controlMap[i].enumVal;
                    break;
                }
            }
            if (controlEnumId == -1) continue;

			ESPUI.updateControlLabel(controlEnumId, label.c_str());
        }
    }

    // 설정값 업데이트 (숫자만 표시)
    ESPUI.updateControlValue(C_ID_MVSTATE_ACCELFILTER_ALPHA                         , String(g_M010_Config.mvState_accelFilter_Alpha, 3));
    ESPUI.updateControlValue(C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN            , String(g_M010_Config.mvState_Forward_speedKmh_Threshold_Min, 3));
    ESPUI.updateControlValue(C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN            , String(g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min, 3));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX               , String(g_M010_Config.mvState_Stop_speedKmh_Threshold_Max, 3));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX              , String(g_M010_Config.mvState_Stop_accelMps2_Threshold_Max, 3));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX                , String(g_M010_Config.mvState_Stop_gyroDps_Threshold_Max, 3));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN                , String(g_M010_Config.mvState_stop_durationMs_Stable_Min));
    ESPUI.updateControlValue(C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN                , String(g_M010_Config.mvState_move_durationMs_Stable_Min));
    ESPUI.updateControlValue(C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD                 , String(g_M010_Config.mvState_Decel_accelMps2_Threshold, 3));
    ESPUI.updateControlValue(C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD                  , String(g_M010_Config.mvState_Bump_accelMps2_Threshold, 3));
    ESPUI.updateControlValue(C_ID_MVSTATE_BUMP_SPEEDKMH_MIN                         , String(g_M010_Config.mvState_Bump_SpeedKmh_Min, 3));
    ESPUI.updateControlValue(C_ID_MVSTATE_BUMP_COOLDOWNMS                           , String(g_M010_Config.mvState_Bump_CooldownMs));
    ESPUI.updateControlValue(C_ID_MVSTATE_DECEL_DURATIONMS_HOLD                     , String(g_M010_Config.mvState_Decel_durationMs_Hold));
    ESPUI.updateControlValue(C_ID_MVSTATE_BUMP_DURATIONMS_HOLD                      , String(g_M010_Config.mvState_Bump_durationMs_Hold));
    ESPUI.updateControlValue(C_ID_MVSTATE_SIGNALWAIT1_SECONDS                       , String(g_M010_Config.mvState_signalWait1_Seconds));
    ESPUI.updateControlValue(C_ID_MVSTATE_SIGNALWAIT2_SECONDS                       , String(g_M010_Config.mvState_signalWait2_Seconds));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOPPED1_SECONDS                          , String(g_M010_Config.mvState_stopped1_Seconds));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOPPED2_SECONDS                          , String(g_M010_Config.mvState_stopped2_Seconds));
    ESPUI.updateControlValue(C_ID_MVSTATE_PARK_SECONDS                              , String(g_M010_Config.mvState_park_Seconds));
    ESPUI.updateControlValue(C_ID_SERIALPRINT_INTERVALMS                            , String(g_M010_Config.serialPrint_intervalMs));
    ESPUI.updateControlValue(C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD   , String(g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold, 3));
    ESPUI.updateControlValue(C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD     , String(g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold, 3));
    ESPUI.updateControlValue(C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD     , String(g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold, 3));
    ESPUI.updateControlValue(C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD     , String(g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold, 3));
    ESPUI.updateControlValue(C_ID_TURNSTATE_SPEEDKMH_MINSPEED                       , String(g_M010_Config.turnState_speedKmh_MinSpeed, 3));
    ESPUI.updateControlValue(C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD            , String(g_M010_Config.turnState_speedKmh_HighSpeed_Threshold, 3));
    ESPUI.updateControlValue(C_ID_TURNSTATE_STABLEDURATIONMS                        , String(g_M010_Config.turnState_StableDurationMs));

    // 언어 선택 드롭다운 값 업데이트
    ESPUI.updateControlValue(g_W010_Control_Language_Id, g_W010_currentLanguage);

    dbgP1_println(W010_EmbUI_getCommonString("messages.config_load_to_web_done", "Config loaded to web UI."));
}


/**
 * @brief ESPUI 컨트롤 이벤트를 처리하는 콜백 함수.
 * @param p_control 이벤트가 발생한 ESPUI 컨트롤 객체.
 * @param p_value ESPUI 라이브러리 내부에서 사용하는 컨트롤의 추가 값 (버튼의 경우 1).
 */

 // --- W010_ESPUI_callback 함수 (수정) ---
void W010_ESPUI_callback(Control* p_control, int p_value) {
    // 'userData' 멤버가 없으므로 해당 줄 삭제
    // int controlEnumId = reinterpret_cast<uintptr_t>(p_control->userData); // 삭제

    // 'name' 멤버도 없을 가능성이 높으므로, 디버그 출력 수정
    // Control 객체에 'label' 또는 'title' 같은 멤버가 있을 수 있습니다.
    // 일단 'name'을 'id'로 대체하거나, 단순히 출력하지 않도록 합니다.
    // 가장 안전한 방법은 Control 객체의 ID만을 사용하는 것입니다.
    dbgP1_printf(F("%s ID=%d, Value=%s, Type=%d\n"), 
                 W010_EmbUI_getCommonString("messages.callback_detected", "ESPUI Callback detected:").c_str(),
                 p_control->id, p_control->value.c_str(), p_control->type);
    
    // 언어 선택 컨트롤 처리 (p_control->id를 사용하여 직접 식별)
    if (p_control->id == g_W010_Control_Language_Id) {
        g_W010_currentLanguage = p_control->value;
        W010_EmbUI_saveLastLanguage(g_W010_currentLanguage);
        W010_EmbUI_rebuildUI();
        ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.lang_change_success", "Language changed successfully!"));
        ESPUI.updateControlValue(g_W010_Control_Error_Id, "");
        return;
    }

    // 버튼 컨트롤 처리 (p_control->id를 사용하여 직접 식별)
    if (p_control->type == ControlType::Button && p_value) { // p_value가 1일 때 버튼이 눌린 것
        // 각 버튼의 ESPUI ID를 전역 변수로 관리하고 있다고 가정하고 비교합니다.
        // 예: extern uint16_t g_W010_Control_SaveBtn_Id;
        //     extern uint16_t g_W010_Control_LoadBtn_Id;
        //     extern uint16_t g_W010_Control_ResetBtn_Id;

        // 여기서는 예시로 Enum ID와 ESPUI ID 간의 명시적인 매핑이 없으므로
        // 임시로 ESPUI ID의 범위나 특징을 이용하여 판단합니다.
        // 더 견고한 방법은 각 버튼에 해당하는 enum ID (C_ID_SAVE_CONFIG_BTN 등)를
        // ESPUI.addControl 시 반환되는 ID와 매핑하는 전역 맵을 만드는 것입니다.
        // 예를 들어: std::map<uint16_t, int> espuiIdToEnumIdMap;
        // 그리고 setupWebPages에서 espuiIdToEnumIdMap[espuiId] = controlEnumId; 로 저장.
        // 콜백에서 int enumId = espuiIdToEnumIdMap[p_control->id]; 로 조회.

        // 현재 상황에서는 p_control->id 와 M010_CAR_CONTROL_ID_E 를 연결하는
        // 명시적인 방법이 없으므로, 가장 간단하게 '이름' 대신 'ID'를 직접 비교합니다.
        // 이 방법은 setupWebPages에서 생성된 각 버튼의 ESPUI ID가
        // 예측 가능하거나 전역 변수로 저장되어 있어야 합니다.
        // 만약 setupWebPages에서 ControlType::Button 생성 시
        // p_control->userData에 enumId를 넣는 방식이 가능한 ESPUI 버전이라면
        // 그렇게 하는 것이 가장 좋습니다. 현재 오류를 보니 불가능한 것으로 보입니다.

        // 따라서, 각 버튼에 해당하는 ESPUI.addControl()의 반환 ID를
        // 별도의 전역 변수에 저장하고 여기서 그 변수들을 직접 비교하는 것이 현실적입니다.
        // 예를 들어:
        // if (p_control->id == g_W010_Control_SaveConfigBtn_Id) { ... }

        // 임시로 Enum ID의 문자열을 다시 가져와 레이블과 비교하는 대신,
        // C_ID_SAVE_CONFIG_BTN 등의 ENUM 값을 직접 사용해 식별하기 위해
        // ControlMap을 역으로 찾아야 합니다.
        int foundEnumId = -1;
        for (size_t i = 0; i < controlMapSize; ++i) {
            // 이전에 p_control->name을 사용했지만, name 멤버가 없으므로
            // ESPUI.addControl() 시 Label 인자로 사용된 controlMap[i].idStr을
            // Control 객체에 저장할 수 있었다면 좋았을 겁니다.
            // 현재는 가장 확실한 방법은 각 컨트롤의 ESPUI ID를 미리 알아내는 것입니다.
            // 여기서는 `Control::id`와 `enum_id`를 연결하는 방법이 필요합니다.
            // 만약 ESPUI.addControl()시 `userData` 인자가 없다면,
            // 콜백에서 `p_control->id`를 통해 `M010_CAR_CONTROL_ID_E`를 알아내야 합니다.

            // 가장 확실한 방법은 setupWebPages에서 컨트롤 생성 시 반환되는
            // ESPUI.addControl()의 ID를 미리 정의된 전역 변수에 할당하고,
            // 여기서 그 전역 변수를 사용하는 것입니다.
            // 예를 들어:
            // uint16_t myButtonId = ESPUI.addControl(ControlType::Button, ...);
            // ...
            // if (p_control->id == myButtonId) { /* do something */ }

            // 현재 구조로는 Control Map의 String ID와 ESPUI Control ID를 직접 매칭하기 어렵습니다.
            // 따라서 각 버튼/넘버 컨트롤에 해당하는 ESPUI ID를 전역 변수로 정의하고,
            // setupWebPages에서 할당한 후, 여기서 그 ID를 직접 비교하는 것이 가장 실용적입니다.

            // 일단 이전처럼 controlMap을 통해 문자열 enumIdStr을 얻은 후
            // 이를 기반으로 동작하는 코드를 유지하지만, 이 부분은 수정이 필요할 수 있습니다.
            // p_control->name이 없으므로, 아래 조건문을 p_control->id로 변경해야 합니다.
            // 하지만 현재 ESPUI.addControl()에서 controlEnumId를 직접 연결할 수 없으므로,
            // ESPUI.h에 정의된 `addControl` 오버로딩을 다시 확인하는 것이 중요합니다.

            // (현재 코드의 한계)
            // p_control->name 대신 p_control->id로 버튼을 식별해야 합니다.
            // 이를 위해서는 각 버튼의 ESPUI ID를 전역 변수에 저장해야 합니다.
            // 임시로 다음과 같이 변경합니다.
            // 만약 ESPUI.h에 Control::id가 ControlType::Button에 대해 고유하게 매핑된다면
            // 아래의 switch (p_control->id) 문으로 직접 처리할 수 있습니다.
            
            // --- 대체 방안: p_control->id로 직접 분기 ---
            if (p_control->id == /* C_ID_SAVE_CONFIG_BTN 에 해당하는 ESPUI ID */) {
                foundEnumId = C_ID_SAVE_CONFIG_BTN;
                break;
            } else if (p_control->id == /* C_ID_LOAD_CONFIG_BTN 에 해당하는 ESPUI ID */) {
                foundEnumId = C_ID_LOAD_CONFIG_BTN;
                break;
            } else if (p_control->id == /* C_ID_RESET_CONFIG_BTN 에 해당하는 ESPUI ID */) {
                foundEnumId = C_ID_RESET_CONFIG_BTN;
                break;
            }
        } // for loop end

        if (foundEnumId != -1) {
            switch (foundEnumId) {
                case C_ID_SAVE_CONFIG_BTN:
                    if (M010_Config_save()) {
                        ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.config_save_success", "Configuration saved successfully."));
                        ESPUI.updateControlValue(g_W010_Control_Error_Id, ""); 
                    } else {
                        ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.config_save_fail", "Failed to save configuration to file system."));
                        ESPUI.updateControlValue(g_W010_Control_Alaram_Id, "");
                    }
                    break;
                case C_ID_LOAD_CONFIG_BTN:
                    if (M010_Config_load()) {
                        W010_EmbUI_loadConfigToWebUI();
                        ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.config_load_success", "Configuration loaded successfully."));
                        ESPUI.updateControlValue(g_W010_Control_Error_Id, "");
                    } else {
                        ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.config_load_fail", "Configuration file not found. Default values applied."));
                        ESPUI.updateControlValue(g_W010_Control_Alaram_Id, "");
                    }
                    break;
                case C_ID_RESET_CONFIG_BTN:
                    M010_Config_initDefaults();
                    if (M010_Config_load()) {
                        W010_EmbUI_loadConfigToWebUI();
                        ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.config_reset_success", "Configuration reset to default."));
                        ESPUI.updateControlValue(g_W010_Control_Error_Id, "");
                    } else {
                        ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.config_reset_fail", "Error saving default configuration."));
                        ESPUI.updateControlValue(g_W010_Control_Alaram_Id, "");
                    }
                    break;
                default:
                    ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.unknown_command", "Unknown command detected."));
                    ESPUI.updateControlValue(g_W010_Control_Alaram_Id, "");
                    dbgP1_printf(F("Unknown button control ID: %d\n"), p_control->id);
                    break;
            }
        } else {
             ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.unknown_command", "Unknown button control ID."));
             ESPUI.updateControlValue(g_W010_Control_Alaram_Id, "");
             dbgP1_printf(F("Button control ID not mapped to enum: %d\n"), p_control->id);
        }

    } else if (p_control->type == ControlType::Number) {
        // 숫자 입력 필드 처리
        // 각 Number 컨트롤의 ESPUI ID를 전역 변수로 관리하고 있다고 가정합니다.
        // 그리고 이 ID를 M010_CAR_CONTROL_ID_E enum 값과 매핑하는 것이 가장 좋습니다.
        
        int foundEnumId = -1;
        // 마찬가지로 p_control->id 를 M010_CAR_CONTROL_ID_E 에 매핑해야 합니다.
        // 예를 들어: std::map<uint16_t, int> espuiIdToEnumIdMap; 를 사용.
        // 현재는 map이 없으므로, 직접 ESPUI ID를 비교하여 처리하는 예시를 보여줍니다.
        // 이 부분은 모든 Number 컨트롤에 대해 각각 if-else if 문으로 ID를 비교해야 합니다.
        
        // --- Number 컨트롤 예시: p_control->id로 직접 분기 ---
        if (p_control->id == /* C_ID_MVSTATE_ACCELFILTER_ALPHA 에 해당하는 ESPUI ID */) {
            foundEnumId = C_ID_MVSTATE_ACCELFILTER_ALPHA;
        } else if (p_control->id == /* C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN 에 해당하는 ESPUI ID */) {
            foundEnumId = C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN;
        }
        // ... 모든 Number 컨트롤 ID와 Enum ID 매핑을 여기에 추가 ...
        // else if (p_control->id == /* C_ID_TURNSTATE_STABLEDURATIONMS 에 해당하는 ESPUI ID */) {
        //     foundEnumId = C_ID_TURNSTATE_STABLEDURATIONMS;
        // }


        if (foundEnumId != -1) {
            switch (foundEnumId) {
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
                case C_ID_MVSTATE_BUMP_SPEEDKMH_MIN:
                    g_M010_Config.mvState_Bump_SpeedKmh_Min = p_control->value.toFloat();
                    break;
                case C_ID_MVSTATE_BUMP_COOLDOWNMS:
                    g_M010_Config.mvState_Bump_CooldownMs = p_control->value.toInt();
                    break;
                case C_ID_MVSTATE_DECEL_DURATIONMS_HOLD:
                    g_M010_Config.mvState_Decel_durationMs_Hold = p_control->value.toInt();
                    break;
                case C_ID_MVSTATE_BUMP_DURATIONMS_HOLD:
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
                    ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.unknown_command", "Unknown command detected for Number control."));
                    ESPUI.updateControlValue(g_W010_Control_Alaram_Id, "");
                    dbgP1_printf(F("Unknown Number control ID: %d\n"), p_control->id);
                    break;
            }
        } else {
             ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.unknown_command", "Number control ID not mapped to enum."));
             ESPUI.updateControlValue(g_W010_Control_Alaram_Id, "");
             dbgP1_printf(F("Number control ID not found in map: %d\n"), p_control->id);
        }
    } else {
        // 다른 유형의 컨트롤 (Label 등)은 여기서 직접 처리할 필요가 없을 수 있습니다.
        ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.unknown_command", "Unknown command detected for control type."));
        ESPUI.updateControlValue(g_W010_Control_Alaram_Id, "");
        dbgP1_printf(F("Unhandled control type: %d, ID: %d\n"), p_control->type, p_control->id);
    }
}

// void W010_ESPUI_callback(Control* p_control, int p_value) {
//     // controlMap 배열에서 enum_id 문자열을 기반으로 실제 enum 값을 가져옵니다.
//     // p_control->userData는 ESPUI.addControl() 시 전달된 controlEnumId입니다.
//     int controlEnumId = reinterpret_cast<uintptr_t>(p_control->userData);

//     dbgP1_printf(W010_EmbUI_getCommonString("messages.callback_detected", "ESPUI Callback detected:") + " ID=%d, Name=%s, Value=%s, Type=%d\n", 
//                  p_control->id, p_control->name.c_str(), p_control->value.c_str(), p_control->type);

//     switch (controlEnumId) {
//         // M010_CAR_CONTROL_ID_E enum 값에 따른 설정값 업데이트
//         case C_ID_MVSTATE_ACCELFILTER_ALPHA:
//             g_M010_Config.mvState_accelFilter_Alpha = p_control->value.toFloat();
//             break;
//         case C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN:
//             g_M010_Config.mvState_Forward_speedKmh_Threshold_Min = p_control->value.toFloat();
//             break;
//         case C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN:
//             g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min = p_control->value.toFloat();
//             break;
//         case C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX:
//             g_M010_Config.mvState_Stop_speedKmh_Threshold_Max = p_control->value.toFloat();
//             break;
//         case C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX:
//             g_M010_Config.mvState_Stop_accelMps2_Threshold_Max = p_control->value.toFloat();
//             break;
//         case C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX:
//             g_M010_Config.mvState_Stop_gyroDps_Threshold_Max = p_control->value.toFloat();
//             break;
//         case C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN:
//             g_M010_Config.mvState_stop_durationMs_Stable_Min = p_control->value.toInt();
//             break;
//         case C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN:
//             g_M010_Config.mvState_move_durationMs_Stable_Min = p_control->value.toInt();
//             break;
//         case C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD:
//             g_M010_Config.mvState_Decel_accelMps2_Threshold = p_control->value.toFloat();
//             break;
//         case C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD:
//             g_M010_Config.mvState_Bump_accelMps2_Threshold = p_control->value.toFloat();
//             break;
//         case C_ID_MVSTATE_BUMP_SPEEDKMH_MIN:
//             g_M010_Config.mvState_Bump_SpeedKmh_Min = p_control->value.toFloat();
//             break;
//         case C_ID_MVSTATE_BUMP_COOLDOWNMS:
//             g_M010_Config.mvState_Bump_CooldownMs = p_control->value.toInt();
//             break;
//         case C_ID_MVSTATE_DECEL_DURATIONMS_HOLD:
//             g_M010_Config.mvState_Decel_durationMs_Hold = p_control->value.toInt();
//             break;
//         case C_ID_MVSTATE_BUMP_DURATIONMS_HOLD:
//             g_M010_Config.mvState_Bump_durationMs_Hold = p_control->value.toInt();
//             break;
//         case C_ID_MVSTATE_SIGNALWAIT1_SECONDS:
//             g_M010_Config.mvState_signalWait1_Seconds = p_control->value.toInt();
//             break;
//         case C_ID_MVSTATE_SIGNALWAIT2_SECONDS:
//             g_M010_Config.mvState_signalWait2_Seconds = p_control->value.toInt();
//             break;
//         case C_ID_MVSTATE_STOPPED1_SECONDS:
//             g_M010_Config.mvState_stopped1_Seconds = p_control->value.toInt();
//             break;
//         case C_ID_MVSTATE_STOPPED2_SECONDS:
//             g_M010_Config.mvState_stopped2_Seconds = p_control->value.toInt();
//             break;
//         case C_ID_MVSTATE_PARK_SECONDS:
//             g_M010_Config.mvState_park_Seconds = p_control->value.toInt();
//             break;
//         case C_ID_SERIALPRINT_INTERVALMS:
//             g_M010_Config.serialPrint_intervalMs = p_control->value.toInt();
//             break;
//         case C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD:
//             g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold = p_control->value.toFloat();
//             break;
//         case C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD:
//             g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold = p_control->value.toFloat();
//             break;
//         case C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD:
//             g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold = p_control->value.toFloat();
//             break;
//         case C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD:
//             g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold = p_control->value.toFloat();
//             break;
//         case C_ID_TURNSTATE_SPEEDKMH_MINSPEED:
//             g_M010_Config.turnState_speedKmh_MinSpeed = p_control->value.toFloat();
//             break;
//         case C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD:
//             g_M010_Config.turnState_speedKmh_HighSpeed_Threshold = p_control->value.toFloat();
//             break;
//         case C_ID_TURNSTATE_STABLEDURATIONMS:
//             g_M010_Config.turnState_StableDurationMs = p_control->value.toInt();
//             break;

//         // 언어 선택 드롭다운 (Select) 처리
//         case C_ID_LANGUAGE_SELECT:
//             g_W010_currentLanguage = p_control->value; // 선택된 언어 코드 (예: "ko", "en") 저장
//             W010_EmbUI_saveLastLanguage(g_W010_currentLanguage); // 마지막 선택 언어를 EEPROM 등에 저장
//             W010_EmbUI_rebuildUI(); // UI 재구성 (새로운 언어 파일 로드 및 UI 갱신)
//             ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.lang_change_success", "Language changed successfully!"));
//             break;

//         // 버튼 클릭 이벤트 처리 (p_value가 1일 때 버튼이 눌린 것)
//         case C_ID_SAVE_CONFIG_BTN:
//             if (p_value) { // 버튼이 눌렸을 때만 처리
//                 if (M010_Config_save()) {
//                     ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.config_save_success", "Configuration saved successfully."));
//                     // Error 메시지 초기화
//                     ESPUI.updateControlValue(g_W010_Control_Error_Id, ""); 
//                 } else {
//                     ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.config_save_fail", "Failed to save configuration to file system."));
//                     // Alarm 메시지 초기화
//                     ESPUI.updateControlValue(g_W010_Control_Alaram_Id, "");
//                 }
//             }
//             break;
//         case C_ID_LOAD_CONFIG_BTN:
//             if (p_value) {
//                 if (M010_Config_load()) {
//                     W010_EmbUI_loadConfigToWebUI(); // 설정값 로드 후 웹 UI 갱신
//                     ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.config_load_success", "Configuration loaded successfully."));
//                     ESPUI.updateControlValue(g_W010_Control_Error_Id, "");
//                 } else {
//                     ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.config_load_fail", "Configuration file not found. Default values applied."));
//                     ESPUI.updateControlValue(g_W010_Control_Alaram_Id, "");
//                 }
//             }
//             break;
//         case C_ID_RESET_CONFIG_BTN:
//             if (p_value) {
//                 M010_Config_initDefaults();
//                 if (M010_Config_save()) {
//                     W010_EmbUI_loadConfigToWebUI(); // 설정값 리셋 후 웹 UI 갱신
//                     ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.config_reset_success", "Configuration reset to default."));
//                     ESPUI.updateControlValue(g_W010_Control_Error_Id, "");
//                 } else {
//                     ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.config_reset_fail", "Error saving default configuration."));
//                     ESPUI.updateControlValue(g_W010_Control_Alaram_Id, "");
//                 }
//             }
//             break;

//         default:
//             // 정의되지 않은 컨트롤 ID에 대한 처리
//             ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.unknown_command", "Unknown command detected."));
//             ESPUI.updateControlValue(g_W010_Control_Alaram_Id, ""); // 알람 메시지 초기화
//             dbgP1_printf("Unknown command or control ID: %d\n", controlEnumId);
//             break;
//     }
// }
            


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
    }

    ESPUI.updateControlValue(C_ID_CARMOVEMENTSTATE_LABEL, v_movementStateStr);
    ESPUI.updateControlValue(C_ID_CARTURNSTATE_LABEL, v_turnStateStr);
    ESPUI.updateControlValue(C_ID_SPEED_KMH_LABEL, String(g_M010_CarStatus.speed_kmh, 2) + W010_EmbUI_getCommonString("units.speed"));
    ESPUI.updateControlValue(C_ID_ACCELX_MS2_LABEL, String(g_M010_CarStatus.accelX_ms2, 2) + W010_EmbUI_getCommonString("units.accel"));
    ESPUI.updateControlValue(C_ID_ACCELY_MS2_LABEL, String(g_M010_CarStatus.accelY_ms2, 2) + W010_EmbUI_getCommonString("units.accel"));
    ESPUI.updateControlValue(C_ID_ACCELZ_MS2_LABEL, String(g_M010_CarStatus.accelZ_ms2, 2) + W010_EmbUI_getCommonString("units.accel"));
    ESPUI.updateControlValue(C_ID_YAWANGLE_DEG_LABEL, String(g_M010_CarStatus.yawAngle_deg, 2) + W010_EmbUI_getCommonString("units.angle"));
    ESPUI.updateControlValue(C_ID_PITCHANGLE_DEG_LABEL, String(g_M010_CarStatus.pitchAngle_deg, 2) + W010_EmbUI_getCommonString("units.angle"));
    ESPUI.updateControlValue(C_ID_YAWANGLEVELOCITY_DEGPS_LABEL, String(g_M010_CarStatus.yawAngleVelocity_degps, 2) + W010_EmbUI_getCommonString("units.angular_velocity"));
    ESPUI.updateControlValue(C_ID_ISEMERGENCYBRAKING_LABEL, g_M010_CarStatus.isEmergencyBraking ? W010_EmbUI_getCommonString("boolean_states.true") : W010_EmbUI_getCommonString("boolean_states.false"));
    ESPUI.updateControlValue(C_ID_ISSPEEDBUMPDETECTED_LABEL, g_M010_CarStatus.isSpeedBumpDetected ? W010_EmbUI_getCommonString("boolean_states.true") : W010_EmbUI_getCommonString("boolean_states.false"));
    ESPUI.updateControlValue(C_ID_CURRENTSTOPTIME_SEC_LABEL, String(g_M010_CarStatus.currentStopTime_ms / 1000) + W010_EmbUI_getCommonString("units.time_seconds"));
}



// 이 함수는 main.cpp의 loop() 함수 또는 별도의 타이머에서 주기적으로 호출되어야 합니다.
void W010_EmbUI_updateStatusLabels() {
  
    // 예시: 속도 레이블 업데이트
    String speedKmhUnit = W010_EmbUI_getCommonString("units.speed", " km/h");
    ESPUI.updateControlValue(C_ID_SPEED_KMH_LABEL, String(g_M010_CarStatus.speed_kmh, 2) + speedKmhUnit);

    // 예시: AccelX 레이블 업데이트
    String accelUnit = W010_EmbUI_getCommonString("units.accel", " m/s^2");
    ESPUI.updateControlValue(C_ID_ACCELX_MS2_LABEL, String(g_M010_CarStatus.accelX_ms2, 2) + accelUnit);
    ESPUI.updateControlValue(C_ID_ACCELY_MS2_LABEL, String(g_M010_CarStatus.accelY_ms2, 2) + accelUnit);
    ESPUI.updateControlValue(C_ID_ACCELZ_MS2_LABEL, String(g_M010_CarStatus.accelZ_ms2, 2) + accelUnit);

    // 예시: Yaw 각도 업데이트
    String angleUnit = W010_EmbUI_getCommonString("units.angle", " deg");
    ESPUI.updateControlValue(C_ID_YAWANGLE_DEG_LABEL, String(g_M010_CarStatus.yawAngle_deg, 2) + angleUnit);
    ESPUI.updateControlValue(C_ID_PITCHANGLE_DEG_LABEL, String(g_M010_CarStatus.pitchAngle_deg, 2) + angleUnit);

    // 예시: Yaw 각속도 업데이트
    String angularVelocityUnit = W010_EmbUI_getCommonString("units.angular_velocity", " deg/s");
    ESPUI.updateControlValue(C_ID_YAWANGLEVELOCITY_DEGPS_LABEL, String(g_M010_CarStatus.yawAngleVelocity_degps, 2) + angularVelocityUnit);

    // 급감속/방지턱 감지 상태 업데이트 (true/false 문자열도 언어 파일에서 가져옴)
    String isEmergencyBrakingStr = g_M010_CarStatus.isEmergencyBraking ? W010_EmbUI_getCommonString("boolean_states.true", "Detected") : W010_EmbUI_getCommonString("boolean_states.false", "Not Detected");
    ESPUI.updateControlValue(C_ID_ISEMERGENCYBRAKING_LABEL, isEmergencyBrakingStr);

    String isSpeedBumpDetectedStr = g_M010_CarStatus.isSpeedBumpDetected ? W010_EmbUI_getCommonString("boolean_states.true", "Detected") : W010_EmbUI_getCommonString("boolean_states.false", "Not Detected");
    ESPUI.updateControlValue(C_ID_ISSPEEDBUMPDETECTED_LABEL, isSpeedBumpDetectedStr);

    // 정차 지속 시간 업데이트
    String timeSecondsUnit = W010_EmbUI_getCommonString("units.time_seconds", " sec");
    ESPUI.updateControlValue(C_ID_CURRENTSTOPTIME_SEC_LABEL, String(g_M010_CarStatus.currentStopTime_ms/1000) + timeSecondsUnit);

    // 자동차 움직임 상태 업데이트
    String carMovementStateStr = W010_EmbUI_getCommonString(
        (String("car_movement_states.") + W010_EmbUI_getCarMovementStateEnumString(g_M010_CarStatus.carMovementState)).c_str(),
        "Unknown State"
    );
    ESPUI.updateControlValue(C_ID_CARMOVEMENTSTATE_LABEL, W010_EmbUI_getCommonString("C_ID_CARMOVEMENTSTATE_LABEL_prefix", "Movement State:") + " " + carMovementStateStr);

    // 자동차 회전 상태 업데이트
    String carTurnStateStr = W010_EmbUI_getCommonString(
        (String("car_turn_states.") + W010_EmbUI_getCarTurnStateEnumString(g_M010_CarStatus.carTurnState)).c_str(),
        "Unknown Turn State"
    );
    ESPUI.updateControlValue(C_ID_CARTURNSTATE_LABEL, W010_EmbUI_getCommonString("C_ID_CARTURNSTATE_LABEL_prefix", "Turn State:") + " " + carTurnStateStr);

    // g_W010_Control_Alaram_Id, g_W010_Control_Error_Id는 W010_ESPUI_callback에서 직접 업데이트하므로 여기서는 처리하지 않습니다.
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
 * Arduino의 `loop()` 함수에서 이 함수를 호출해야 합니다.
 */
void W010_EmbUI_run() {
    static u_int32_t v_lastWebUpdateTime_ms = 0;
    if (millis() - v_lastWebUpdateTime_ms >= g_M010_Config.serialPrint_intervalMs) {
        W010_EmbUI_updateCarStatusWeb();
        v_lastWebUpdateTime_ms = millis();
    }
}

/**
 * @brief LittleFS에서 마지막으로 선택된 언어 설정을 로드합니다.
 */
void W010_EmbUI_loadLastLanguage() {
    File langFile = LittleFS.open(G_W010_LAST_LANG_FILE, "r");
    if (langFile) {
        String langCode = langFile.readStringUntil('\n');
        langCode.trim();
        if (langCode.length() > 0) {
            g_W010_currentLanguage = langCode;
            dbgP1_printf("마지막으로 사용된 언어 로드: %s\n", g_W010_currentLanguage.c_str());
        }
        langFile.close();
    } else {
        dbgP1_println(F("마지막 언어 설정 파일이 없습니다. 기본값 (ko) 사용."));
    }
}

/**
 * @brief 현재 선택된 언어 설정을 LittleFS에 저장합니다.
 */
void W010_EmbUI_saveLastLanguage() {
    File langFile = LittleFS.open(G_W010_LAST_LANG_FILE, "w");
    if (langFile) {
        langFile.println(g_W010_currentLanguage);
        langFile.close();
        dbgP1_printf("현재 언어 설정 저장됨: %s\n", g_W010_currentLanguage.c_str());
    } else {
        dbgP1_println(F("언어 설정 파일 저장 실패!"));
    }
}

/**
 * @brief ESPUI UI를 완전히 제거하고 현재 언어 설정에 따라 다시 구성합니다.
 * 언어 변경 시 UI 전체를 리로드하는 데 사용됩니다.
 */


void W010_EmbUI_rebuildUI() {
    dbgP1_printf("UI 재구성 중... 새로운 언어: %s\n", g_W010_currentLanguage.c_str());

    // 1. 새로운 언어의 UI 설정 JSON 파일 로드
    // 이전 언어 설정은 메모리에 남아있을 수 있으므로 JsonDocument를 초기화하거나
    // 새 JSON 파일을 안전하게 로드합니다.
    // 여기서는 g_W010_uiConfigDoc 객체를 재활용하되, 오류 발생 시 기본 언어로 폴백합니다.
	
    if (!W010_EmbUI_loadUILanguage(g_W010_currentLanguage)) {
    //if (!W010_EmbUI_loadUIConfig(g_W010_currentLanguage)) {
        dbgP1_printf("UI 설정 JSON 파일 (%s) 로드 실패. 기본 언어(ko)로 재시도.\n", g_W010_currentLanguage.c_str());
        g_W010_currentLanguage = "ko"; // 실패 시 기본 언어로 강제 설정
        if (!W010_EmbUI_loadUILanguage(g_W010_currentLanguage)) {
            dbgP1_println(F("기본 언어 UI 설정 파일도 로드 실패. UI 업데이트 불가."));
            ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.ui_load_fail_critical"));
            return; // 치명적인 오류이므로 여기서 종료
        }
    }

    // 2. 모든 탭과 컨트롤의 레이블 업데이트
    // W010_EmbUI_loadConfigToWebUI() 함수는 이미 이 기능을 포함하고 있으므로,
    // 이 함수를 호출하여 웹 UI를 갱신합니다.
    W010_EmbUI_loadConfigToWebUI();

    // 3. 언어 변경 성공 메시지 표시 (선택 사항)
    ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.lang_change_success"));

    dbgP1_println(W010_EmbUI_getCommonString("messages.ui_rebuild_done"));
}
