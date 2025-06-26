#pragma once
// W010_embUI_009.h
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

// JSON 파일 경로 정의 (언어 코드에 따라 동적으로 결정)

#define    G_W010_UI_CONFIG_BASE_FILE_PREFIX    "/W010_ESPUI_ui_config_003_"
#define    G_W010_UI_CONFIG_BASE_FILE_SUBFIX    ".json"

//#define     G_W010_UI_CONFIG_BASE_FILE        "/W010_ESPUI_ui_config_001"

#define     G_W010_LAST_LANG_FILE               "/last_lang.txt" // 마지막 언어 설정을 저장할 파일

// 현재 선택된 언어 코드 (예: "ko", "en")
String      g_W010_currentLanguage              = "ko"; // 기본 언어는 한국어



// ====================================================================================================
// 전역 변수 선언
// ====================================================================================================

// ESPUI는 페이지 자체를 컨트롤로 추가하고, 그 ID를 페이지 ID로 사용합니다.
uint16_t g_W010_Tab_Config_Id;
uint16_t g_W010_Tab_Status_Id;

uint16_t g_W010_Control_Alaram_Id;
uint16_t g_W010_Control_Error_Id;
uint16_t g_W010_Control_Language_Id; // 언어 선택 드롭다운 컨트롤 ID


JsonDocument g_W010_uiLayoutDoc; // UI 레이아웃 및 기본값을 위한 JSON 문서
JsonDocument g_W010_uiLangDoc;   // 다국어 문자열을 위한 JSON 문서

// JSON에서 로드된 UI 설정을 저장할 json doc 객체
// JsonDocument g_W010_uiConfigDoc; 


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


// ====================================================================================================
// 함수 선언 (프로토타입)
// ====================================================================================================
bool W010_EmbUI_loadUIConfig(const String& langCode); // UI 설정 JSON 로드 함수 (언어 코드 추가)
String W010_EmbUI_getCommonString(const char* key, const char* defaultVal = ""); // 다국어 문자열 가져오는 헬퍼 함수
String W010_EmbUI_getControlLabel(const String& enumIdStr); // 컨트롤 레이블 가져오는 헬퍼 함수
void W010_EmbUI_init(); // 함수 이름은 EmbUI 그대로 두지만, 내부 구현은 ESPUI를 사용
void W010_EmbUI_setupWebPages();
void W010_EmbUI_loadConfigToWebUI();
void W010_ESPUI_callback(Control* p_Control, int p_controlType, void* p_userData);
void W010_EmbUI_updateCarStatusWeb();
void W010_EmbUI_run();
void W010_EmbUI_loadLastLanguage(); // 마지막 선택된 언어를 로드
void W010_EmbUI_saveLastLanguage(); // 현재 언어 설정을 저장
void W010_EmbUI_rebuildUI(); // UI를 다시 그리는 함수

// ====================================================================================================
// 함수 정의
// ====================================================================================================

/**
 * @brief UI 설정 JSON 파일을 LittleFS에서 로드하고 파싱합니다.
 * @param langCode 로드할 언어 코드 (예: "ko", "en")
 * @return 로드 성공 시 true, 실패 시 false
 */
bool W010_EmbUI_loadUIConfig(const String& langCode) {
    String filePath = String(G_W010_UI_CONFIG_BASE_FILE_PREFIX) + langCode + G_W010_UI_CONFIG_BASE_FILE_SUBFIX;
    //String filePath = String(G_W010_UI_CONFIG_BASE_FILE) + "_" + langCode + ".json";
    dbgP1_printf("UI 설정 파일 로드 중: %s\n", filePath.c_str());

    File v_configFile = LittleFS.open(filePath, "r");
    if (!v_configFile) {
        dbgP1_printf("파일 열기 실패: %s\n", filePath.c_str());
        return false;
    }

    DeserializationError v_error = deserializeJson(g_W010_uiConfigDoc, v_configFile);
    if (v_error) {
        dbgP1_printf("JSON 파싱 실패: %s\n", v_error.c_str());
        v_configFile.close();
        return false;
    }

    v_configFile.close();
    dbgP1_printf("UI 설정 파일 로드 및 파싱 완료: %s\n", filePath.c_str());
    return true;
}

/**
 * @brief JSON에서 현재 선택된 언어에 해당하는 공통 문자열을 가져옵니다.
 * @param key JSON 경로 (예: "car_movement_states.E_M010_CARMOVESTATE_UNKNOWN")
 * @param defaultVal 키를 찾지 못했을 때 반환할 기본값
 * @return 해당 언어의 문자열. 없으면 defaultVal 반환.
 */
String W010_EmbUI_getCommonString(const char* key, const char* p_defaultVal) {
    JsonVariant v_value = g_W010_uiConfigDoc["common_strings"][key];
    if (!v_value.isNull()) {
        return v_value.as<String>();
    }
    return String(p_defaultVal);
}

/**
 * @brief 주어진 enum ID 문자열에 해당하는 컨트롤의 레이블을 현재 로드된 언어 파일에서 가져옵니다.
 * @param enumIdStr 컨트롤의 enum_id 문자열
 * @return 해당 컨트롤의 레이블 문자열. 없으면 빈 문자열 반환.
 */
String W010_EmbUI_getControlLabel(const String& p_enumIdStr) {
    JsonArray v_tabs = g_W010_uiConfigDoc["tabs"].as<JsonArray>();
    for (JsonObject v_tab : v_tabs) {
        JsonArray v_controls = v_tab["controls"].as<JsonArray>();
        for (JsonObject v_control : v_controls) {
            if (v_control["enum_id"].as<String>().equals(p_enumIdStr)) {
                JsonVariant v_labelVariant = v_control["label"]; // 언어별 파일이므로 직접 접근
                if (!v_labelVariant.isNull()) {
                    return v_labelVariant.as<String>();
                }
            }
        }
    }
    return String(""); // 찾지 못함
}

bool W010_EmbUI_loadUILayoutDefaults() {
    const char* filePath = "/W010_ESPUI_ui_layout_defaults.json"; // 고정된 기본값 파일
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
    String filePath = String(G_W010_UI_CONFIG_BASE_FILE_PREFIX) + langCode + G_W010_UI_CONFIG_BASE_FILE_SUBFIX;
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

    // 1. UI 레이아웃 및 기본값 파일 로드 (한 번만 로드)
    if (!W010_EmbUI_loadUILayoutDefaults()) {
        dbgP1_println(F("UI 레이아웃 및 기본값 파일 로드 실패. UI 구성에 문제 발생 가능."));
        // 이 경우 default_value를 사용할 수 없으므로, 코드를 통해 기본값을 직접 설정하거나
        // 최소한의 UI만 구성하도록 폴백 로직 필요
    }

    // 2. 마지막 언어 설정 로드 시도 및 해당 언어 파일 로드
    W010_EmbUI_loadLastLanguage(); // g_W010_currentLanguage 설정

    if (!W010_EmbUI_loadUILanguage(g_W010_currentLanguage)) {
        dbgP1_printf("UI 언어 파일 (%s) 로드 실패. 기본 언어(ko)로 재시도.\n", g_W010_currentLanguage.c_str());
        g_W010_currentLanguage = "ko"; // 실패 시 기본 언어로 강제 설정
        if (!W010_EmbUI_loadUILanguage(g_W010_currentLanguage)) {
            dbgP1_println(F("기본 언어 UI 설정 파일도 로드 실패. 다국어 지원 불가."));
        }
    }

    ESPUI.begin("Robot Eye Car", "roboteye", "carpassword");
    dbgP1_println(W010_EmbUI_getCommonString("messages.ui_init_done"));
}


/**
 * @brief ESPUI를 초기화하고 Wi-Fi 연결을 설정합니다.
 * Access Point (AP) 모드로 시작하여 ESP32가 설정 웹페이지를 호스팅합니다.
 * 추후 Wi-Fi STA 모드로 전환하여 기존 네트워크에 연결할 수 있습니다.
 */
/*
void W010_EmbUI_init() {
    dbgP1_println(F("ESPUI 초기화 중...")); // 초기화 메시지는 우선 고정

    if (!LittleFS.begin()) {
        dbgP1_println(F("LittleFS 마운트 실패! UI 설정 로드 불가."));
        return;
    } else {
        W010_EmbUI_loadLastLanguage(); // 마지막으로 선택된 언어 로드 시도
        if (!W010_EmbUI_loadUIConfig(g_W010_currentLanguage)) {
            dbgP1_printf("UI 설정 JSON 파일 (%s) 로드 실패. 기본 언어(ko)로 재시도.\n", g_W010_currentLanguage.c_str());
            g_W010_currentLanguage = "ko"; // 실패 시 기본 언어로 강제 설정
            if (!W010_EmbUI_loadUIConfig(g_W010_currentLanguage)) {
                dbgP1_println(F("기본 언어 UI 설정 파일도 로드 실패. UI 구성에 문제 발생 가능."));
            }
        }
    }

    ESPUI.begin("Robot Eye Car", "roboteye", "carpassword");

    dbgP1_println(W010_EmbUI_getCommonString("messages.ui_init_done"));
}
*/

/**
 * @brief ESPUI 웹페이지 UI를 구성하고, g_M010_Config 구조체의 멤버 변수들을 웹 UI에 바인딩합니다.
 * 사용자가 웹 인터페이스를 통해 설정을 조회하고 변경할 수 있도록 필드를 정의합니다.
 */
void W010_EmbUI_setupWebPages() {
    dbgP1_println(W010_EmbUI_getCommonString("messages.ui_setup_start"));

    JsonArray tabs = g_W010_uiConfigDoc["tabs"].as<JsonArray>();

    for (JsonObject tab : tabs) {
        String tabId = tab["id"].as<String>();
        String tabTitle = tab["title"].as<String>(); // 현재 언어 파일에서 직접 가져옴
       
        uint16_t currentTabId;
        if (tabId.equals(F("config"))) {
            g_W010_Tab_Config_Id = ESPUI.addControl(Tab, tabTitle.c_str(), tabTitle);
            currentTabId = g_W010_Tab_Config_Id;

            // 언어 선택 드롭다운 추가
            g_W010_Control_Language_Id = ESPUI.addControl(ControlType::Select,
                                                           W010_EmbUI_getCommonString("messages.lang_select_label").c_str(),
                                                           "",
                                                           ControlColor::Wetasphalt,
                                                           currentTabId,
                                                           &W010_ESPUI_callback,
                                                           reinterpret_cast<void*>(static_cast<uintptr_t>(C_ID_LANGUAGE_SELECT)));
            // 드롭다운 옵션 추가
            // uint16_t select1 = ESPUI.addControl( ControlType::Select, "Select Title", "Initial Value", ControlColor::Alizarin, tab1, &selectExample );
            // ESPUI.addControl( ControlType::Option, "Option1", "Opt1", ControlColor::Alizarin, select1);
            // ESPUI.addControl( ControlType::Option, "Option2", "Opt2", ControlColor::Alizarin, select1);
            
            // auto mainselector = ESPUI.addControl(Select, "Selector", "Selector", Wetasphalt, maintab, generalCallback);
            // for(auto const& v : optionValues) {
            //     ESPUI.addControl(Option, v.c_str(), v, None, mainselector);
            // }

            // uint16_t addControl(ControlType type, const char* label);
            // uint16_t addControl(ControlType type, const char* label, const String& value);
            // uint16_t addControl(ControlType type, const char* label, const String& value, ControlColor color);
            // uint16_t addControl(ControlType type, const char* label, const String& value, ControlColor color, uint16_t parentControl);
            // uint16_t addControl(ControlType type, const char* label, const String& value, ControlColor color, uint16_t parentControl, std::function<void(Control*, int)> callback);


            ESPUI.addControl( ControlType::Option, W010_EmbUI_getCommonString("messages.lang_ko").c_str(), "ko", ControlColor::Alizarin, g_W010_Control_Language_Id);
            ESPUI.addControl( ControlType::Option, W010_EmbUI_getCommonString("messages.lang_en").c_str(), "en", ControlColor::Alizarin, g_W010_Control_Language_Id);
            //ESPUI.addControlOption(g_W010_Control_Language_Id, W010_EmbUI_getCommonString("messages.lang_ko"), "ko");
            //ESPUI.addControlOption(g_W010_Control_Language_Id, W010_EmbUI_getCommonString("messages.lang_en"), "en");
            
            // 현재 선택된 언어를 드롭다운에 반영
            ESPUI.updateControlValue(g_W010_Control_Language_Id, g_W010_currentLanguage);


        } else if (tabId.equals(F("status"))) {
            g_W010_Tab_Status_Id = ESPUI.addControl(ControlType::Tab, tabTitle.c_str(), F(""), ControlColor::Wetasphalt);
            ESPUI.setVertical(g_W010_Tab_Status_Id, true);
            currentTabId = g_W010_Tab_Status_Id;
        } else {
            dbgP1_printf(String(W010_EmbUI_getCommonString("messages.unknown_tab_id") + " %s\n").c_str(), tabId.c_str());
            continue;
        }

		JsonArray controls = tab["controls"].as<JsonArray>();

        for (JsonObject control : controls) {
            String enumIdStr = control["enum_id"].as<String>();
            String label = W010_EmbUI_getControlLabel(enumIdStr); // 다국어 레이블 가져오기
            JsonVariant defaultValue = control["default_value"];

            int controlEnumId = -1;
            for (size_t i = 0; i < controlMapSize; ++i) {
                if (enumIdStr.equals(controlMap[i].idStr)) {
                    controlEnumId = controlMap[i].enumVal;
                    break;
                }
            }
            
            if (enumIdStr.equals(F("g_W010_Control_Alaram_Id"))) {
                g_W010_Control_Alaram_Id = ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, currentTabId);
                continue;
            } else if (enumIdStr.equals(F("g_W010_Control_Error_Id"))) {
                g_W010_Control_Error_Id = ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, currentTabId);
                continue;
            } else if (enumIdStr.equals(F("C_ID_LANGUAGE_SELECT"))) { // 언어 선택 드롭다운은 이미 추가했으므로 건너뛰기
                continue;
            }

            if (controlEnumId == -1) {
                dbgP1_printf(String(W010_EmbUI_getCommonString("messages.unknown_control_id") + " %s\n").c_str(), enumIdStr.c_str());
                continue;
            }

            if (tabId.equals(F("config"))) {
                if (enumIdStr.endsWith(F("_BTN"))) {
                    ESPUI.addControl(ControlType::Button, label.c_str(), label.c_str(), ControlColor::Emerald, currentTabId, &W010_ESPUI_callback, reinterpret_cast<void*>(static_cast<uintptr_t>(controlEnumId)));
                } else {
                    ESPUI.addControl(ControlType::Number, label.c_str(), String(defaultValue.as<float>(), 3), ControlColor::Alizarin, currentTabId, &W010_ESPUI_callback, reinterpret_cast<void*>(static_cast<uintptr_t>(controlEnumId)));
                }
            } else if (tabId.equals(F("status"))) {
                ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, currentTabId, nullptr, reinterpret_cast<void*>(static_cast<uintptr_t>(controlEnumId)));
            }
        }
    }
    dbgP1_println(W010_EmbUI_getCommonString("messages.ui_setup_done"));
}

/**
 * @brief g_M010_Config 구조체에 저장된 현재 설정값을 ESPUI 웹 UI로 로드하여 표시합니다.
 * 주로 시스템 시작 시 또는 '불러오기' 버튼 클릭 시 호출됩니다.
 * 다국어 레이블을 적용하여 컨트롤 업데이트
 */
void W010_EmbUI_loadConfigToWebUI() {
    dbgP1_println(W010_EmbUI_getCommonString("messages.config_load_to_web_start"));

    // 모든 컨트롤에 대해 현재 언어의 레이블로 업데이트
    JsonArray tabs = g_W010_uiConfigDoc["tabs"].as<JsonArray>();
    for (JsonObject tab : tabs) {
        // 탭 타이틀 업데이트
        String tabId = tab["id"].as<String>();
        String tabTitle = tab["title"].as<String>(); // 현재 언어 파일에서 직접 가져옴
        
        uint16_t currentTabEspuiId = 0;
        if (tabId.equals(F("config"))) currentTabEspuiId = g_W010_Tab_Config_Id;
        else if (tabId.equals(F("status"))) currentTabEspuiId = g_W010_Tab_Status_Id;

        if (currentTabEspuiId != 0) {
			ESPUI.updateControlLabel(currentTabEspuiId, tabTitle.c_str());
             // ESPUI.updateControl(currentTabEspuiId, tabTitle.c_str(), "");
        }

        JsonArray controls = tab["controls"].as<JsonArray>();
        for (JsonObject control : controls) {
            String enumIdStr = control["enum_id"].as<String>();
            String label = W010_EmbUI_getControlLabel(enumIdStr);

            int controlEnumId = -1;
            for (size_t i = 0; i < controlMapSize; ++i) {
                if (enumIdStr.equals(controlMap[i].idStr)) {
                    controlEnumId = controlMap[i].enumVal;
                    break;
                }
            }
            if (controlEnumId == -1) continue;

            // ESPUI 컨트롤 레이블 업데이트
			ESPUI.updateControlLabel(controlEnumId, label.c_str());
            //ESPUI.updateControl(controlEnumId, label.c_str(), "");
        }
    }

    ESPUI.updateControlValue(C_ID_MVSTATE_ACCELFILTER_ALPHA                         , String(g_M010_Config.mvState_accelFilter_Alpha, 3));
    ESPUI.updateControlValue(C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN            , String(g_M010_Config.mvState_Forward_speedKmh_Threshold_Min, 2));
    ESPUI.updateControlValue(C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN            , String(g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min, 2));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX               , String(g_M010_Config.mvState_Stop_speedKmh_Threshold_Max, 2));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX              , String(g_M010_Config.mvState_Stop_accelMps2_Threshold_Max, 2));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX                , String(g_M010_Config.mvState_Stop_gyroDps_Threshold_Max, 2));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN                , String(g_M010_Config.mvState_stop_durationMs_Stable_Min));
    ESPUI.updateControlValue(C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN                , String(g_M010_Config.mvState_move_durationMs_Stable_Min));
    ESPUI.updateControlValue(C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD                 , String(g_M010_Config.mvState_Decel_accelMps2_Threshold, 2));
    ESPUI.updateControlValue(C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD                  , String(g_M010_Config.mvState_Bump_accelMps2_Threshold, 2));
    ESPUI.updateControlValue(C_ID_MVSTATE_BUMP_SPEEDKMH_MIN                         , String(g_M010_Config.mvState_Bump_SpeedKmh_Min, 2));
    ESPUI.updateControlValue(C_ID_MVSTATE_BUMP_COOLDOWNMS                           , String(g_M010_Config.mvState_Bump_CooldownMs));
    ESPUI.updateControlValue(C_ID_MVSTATE_DECEL_DURATIONMS_HOLD                     , String(g_M010_Config.mvState_Decel_durationMs_Hold));
    ESPUI.updateControlValue(C_ID_MVSTATE_BUMP_DURATIONMS_HOLD                      , String(g_M010_Config.mvState_Bump_durationMs_Hold));
    ESPUI.updateControlValue(C_ID_MVSTATE_SIGNALWAIT1_SECONDS                       , String(g_M010_Config.mvState_signalWait1_Seconds));
    ESPUI.updateControlValue(C_ID_MVSTATE_SIGNALWAIT2_SECONDS                       , String(g_M010_Config.mvState_signalWait2_Seconds));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOPPED1_SECONDS                          , String(g_M010_Config.mvState_stopped1_Seconds));
    ESPUI.updateControlValue(C_ID_MVSTATE_STOPPED2_SECONDS                          , String(g_M010_Config.mvState_stopped2_Seconds));
    ESPUI.updateControlValue(C_ID_MVSTATE_PARK_SECONDS                              , String(g_M010_Config.mvState_park_Seconds));
    ESPUI.updateControlValue(C_ID_SERIALPRINT_INTERVALMS                            , String(g_M010_Config.serialPrint_intervalMs));
    ESPUI.updateControlValue(C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD   , String(g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold, 2));
    ESPUI.updateControlValue(C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD     , String(g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold, 2));
    ESPUI.updateControlValue(C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD     , String(g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold, 2));
    ESPUI.updateControlValue(C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD     , String(g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold, 2));
    ESPUI.updateControlValue(C_ID_TURNSTATE_SPEEDKMH_MINSPEED                       , String(g_M010_Config.turnState_speedKmh_MinSpeed, 2));
    ESPUI.updateControlValue(C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD            , String(g_M010_Config.turnState_speedKmh_HighSpeed_Threshold, 2));
    ESPUI.updateControlValue(C_ID_TURNSTATE_STABLEDURATIONMS                        , String(g_M010_Config.turnState_StableDurationMs));

    // 언어 선택 드롭다운 값 업데이트
    ESPUI.updateControlValue(g_W010_Control_Language_Id, g_W010_currentLanguage);

    dbgP1_println(W010_EmbUI_getCommonString("messages.config_load_to_web_done"));
}

/**
 * @brief ESPUI 웹 UI에서 컨트롤 값이 변경되거나 버튼이 클릭될 때 호출되는 콜백 함수입니다.
 * 변경된 값을 g_M010_Config 구조체에 반영하거나, 명령 버튼에 따른 동작을 수행합니다.
 * @param p_Control 변경되거나 클릭된 컨트롤 객체
 * @param p_controlType 컨트롤의 변경/클릭 유형 (Control::Type::Button, Control::Type::Number 등)
 */
void W010_ESPUI_callback(Control* p_Control, int p_controlType, void* p_userData) {
    uint16_t v_controlId = reinterpret_cast<uintptr_t>(p_userData); 

    dbgP1_printf(String(W010_EmbUI_getCommonString("messages.callback_detected") + "\n").c_str(), v_controlId, p_Control->label, p_Control->value.c_str(), p_controlType);

    if (p_controlType == Number) {
        float v_floatValue  = p_Control->value.toFloat();
        int v_intValue      = p_Control->value.toInt();

        switch (v_controlId) {
            case C_ID_MVSTATE_ACCELFILTER_ALPHA:
                g_M010_Config.mvState_accelFilter_Alpha = v_floatValue;
                break;
            case C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN:
                g_M010_Config.mvState_Forward_speedKmh_Threshold_Min = v_floatValue;
                break;
            case C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN:
                g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min = v_floatValue;
                break;
            case C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX:
                g_M010_Config.mvState_Stop_speedKmh_Threshold_Max = v_floatValue;
                break;
            case C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX:
                g_M010_Config.mvState_Stop_accelMps2_Threshold_Max = v_floatValue;
                break;
            case C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX:
                g_M010_Config.mvState_Stop_gyroDps_Threshold_Max = v_floatValue;
                break;
            case C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN:
                g_M010_Config.mvState_stop_durationMs_Stable_Min = v_intValue;
                break;
            case C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN:
                g_M010_Config.mvState_move_durationMs_Stable_Min = v_intValue;
                break;
            case C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD:
                g_M010_Config.mvState_Decel_accelMps2_Threshold = v_floatValue;
                break;
            case C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD:
                g_M010_Config.mvState_Bump_accelMps2_Threshold = v_floatValue;
                break;
            case C_ID_MVSTATE_BUMP_SPEEDKMH_MIN:
                g_M010_Config.mvState_Bump_SpeedKmh_Min = v_floatValue;
                break;
            case C_ID_MVSTATE_BUMP_COOLDOWNMS:
                g_M010_Config.mvState_Bump_CooldownMs = v_intValue;
                break;
            case C_ID_MVSTATE_DECEL_DURATIONMS_HOLD:
                g_M010_Config.mvState_Decel_durationMs_Hold = v_intValue;
                break;
            case C_ID_MVSTATE_BUMP_DURATIONMS_HOLD:
                g_M010_Config.mvState_Bump_durationMs_Hold = v_intValue;
                break;
            case C_ID_MVSTATE_SIGNALWAIT1_SECONDS:
                g_M010_Config.mvState_signalWait1_Seconds = v_intValue;
                break;
            case C_ID_MVSTATE_SIGNALWAIT2_SECONDS:
                g_M010_Config.mvState_signalWait2_Seconds = v_intValue;
                break;
            case C_ID_MVSTATE_STOPPED1_SECONDS:
                g_M010_Config.mvState_stopped1_Seconds = v_intValue;
                break;
            case C_ID_MVSTATE_STOPPED2_SECONDS:
                g_M010_Config.mvState_stopped2_Seconds = v_intValue;
                break;
            case C_ID_MVSTATE_PARK_SECONDS:
                g_M010_Config.mvState_park_Seconds = v_intValue;
                break;
            case C_ID_SERIALPRINT_INTERVALMS:
                g_M010_Config.serialPrint_intervalMs = v_intValue;
                break;
            case C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold = v_floatValue;
                break;
            case C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold = v_floatValue;
                break;
            case C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold = v_floatValue;
                break;
            case C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold = v_floatValue;
                break;
            case C_ID_TURNSTATE_SPEEDKMH_MINSPEED:
                g_M010_Config.turnState_speedKmh_MinSpeed = v_floatValue;
                break;
            case C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD:
                g_M010_Config.turnState_speedKmh_HighSpeed_Threshold = v_floatValue;
                break;
            case C_ID_TURNSTATE_STABLEDURATIONMS:
                g_M010_Config.turnState_StableDurationMs = v_intValue;
                break;
            default:
                dbgP1_printf(String(W010_EmbUI_getCommonString("messages.unknown_control_id") + " %d\n").c_str(), v_controlId);
                break;
        }
    } else if (p_controlType == ControlType::Button) {
        String v_cmd = p_Control->value;

        if (v_cmd.equals(BTN_CMD_SAVE_CONFIG)) {
            if (M010_Config_save()) {
                dbgP1_println(W010_EmbUI_getCommonString("messages.config_save_success"));                
                ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.config_save_success"));
            } else {
                dbgP1_println(W010_EmbUI_getCommonString("messages.config_save_fail"));
                ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.config_save_fail"));
            }
        } else if (v_cmd.equals(BTN_CMD_LOAD_CONFIG)) {
            if (M010_Config_load()) {
                dbgP1_println(W010_EmbUI_getCommonString("messages.config_load_success"));
                W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 로드된 값 반영
                ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.config_load_success"));
            } else {
                dbgP1_println(W010_EmbUI_getCommonString("messages.config_load_fail"));
                M010_Config_initDefaults(); // 로드 실패 시 기본값으로 초기화
                W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 기본값 반영

                ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.config_load_fail"));
            }
        } else if (v_cmd.equals(BTN_CMD_RESET_CONFIG)) {
            M010_Config_initDefaults(); // 설정값을 기본값으로 초기화
            if (M010_Config_save()) { // 초기화된 기본값을 파일에 저장
                dbgP1_println(W010_EmbUI_getCommonString("messages.config_reset_success"));
                W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 기본값 반영
                ESPUI.updateControlValue(g_W010_Control_Alaram_Id, W010_EmbUI_getCommonString("messages.config_reset_success"));
            } else {
                dbgP1_println(W010_EmbUI_getCommonString("messages.config_reset_fail"));
                ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.config_reset_fail"));
            }
        } else {
            dbgP1_printf(String(W010_EmbUI_getCommonString("messages.unknown_command") + ": %s\n").c_str(), v_cmd.c_str());
            ESPUI.updateControlValue(g_W010_Control_Error_Id, W010_EmbUI_getCommonString("messages.unknown_command"));
        }
    } else if (p_controlType == Select) {
        if (v_controlId == C_ID_LANGUAGE_SELECT) {
            String selectedLang = p_Control->value;
            if (selectedLang != g_W010_currentLanguage) {
                g_W010_currentLanguage = selectedLang;
                W010_EmbUI_saveLastLanguage(); // 선택된 언어 저장
                W010_EmbUI_rebuildUI(); // UI 다시 그리기
            }
        }
    }
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
    if (!W010_EmbUI_loadUIConfig(g_W010_currentLanguage)) {
        dbgP1_printf("UI 설정 JSON 파일 (%s) 로드 실패. 기본 언어(ko)로 재시도.\n", g_W010_currentLanguage.c_str());
        g_W010_currentLanguage = "ko"; // 실패 시 기본 언어로 강제 설정
        if (!W010_EmbUI_loadUIConfig(g_W010_currentLanguage)) {
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
