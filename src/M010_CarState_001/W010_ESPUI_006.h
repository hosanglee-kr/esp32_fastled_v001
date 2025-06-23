#pragma once
// W010_embUI_006.h
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

#define     G_W010_UI_CONFIG_JSON_FILE     "/W010_ESPUI_ui_config_001.json"

// ====================================================================================================
// 전역 변수 선언
// ====================================================================================================

// ESPUI는 페이지 자체를 컨트롤로 추가하고, 그 ID를 페이지 ID로 사용합니다.
uint16_t g_W010_Tab_Config_Id;
uint16_t g_W010_Tab_Status_Id;

uint16_t g_W010_Control_Alaram_Id;
uint16_t g_W010_Control_Error_Id;

// JSON에서 로드된 UI 설정을 저장할 동적 문서 객체
JsonDocument g_W010_uiConfigDoc; // 외부에 선언된 전역 변수임을 알림

// ESPUI 컨트롤 ID 정의 (각 컨트롤마다 고유한 ID를 부여)
// 이렇게 enum으로 관리하면 가독성이 좋고 오타를 줄일 수 있습니다.
// JSON에서 ID를 동적으로 가져올 것이므로, 기존 enum은 유지하되,
// 웹 UI 생성 시 JSON 데이터를 활용하여 컨트롤을 생성하도록 변경할 예정입니다.
// 기존 enum 값들은 callback 함수에서 컨트롤의 ID를 식별하는 데 계속 사용됩니다.
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
    // mvState_normalMove_durationMs는 웹 UI 컨트롤에 바인딩되지 않으므로 ID가 필요 없습니다.
    C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD,
    C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD,
    C_ID_MVSTATE_BUMP_SPEEDKMH_MIN,
    C_ID_MVSTATE_BUMP_COOLDOWNMS,
    C_ID_MVSTATE_DECEL_DURATIONMS_HOLD,
    C_ID_MVSTATE_BUMP_DURATIONMS_HOLD,
    // mvState_PeriodMs_stopGrace는 웹 UI 컨트롤에 바인딩되지 않으므로 ID가 필요 없습니다.
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

    // 알림/오류 메시지 라벨은 별도 ID가 필요 없으므로 enum에서 제거.
    // 대신 전역 변수 g_W010_Control_Alaram_Id, g_W010_Control_Error_Id 사용
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
    {"C_ID_MVSTATE_ACCELFILTER_ALPHA", C_ID_MVSTATE_ACCELFILTER_ALPHA},
    {"C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN", C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN},
    {"C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN", C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN},
    {"C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX", C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX},
    {"C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX", C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX},
    {"C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX", C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX},
    {"C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN", C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN},
    {"C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN", C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN},
    {"C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD", C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD},
    {"C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD", C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD},
    {"C_ID_MVSTATE_BUMP_SPEEDKMH_MIN", C_ID_MVSTATE_BUMP_SPEEDKMH_MIN},
    {"C_ID_MVSTATE_BUMP_COOLDOWNMS", C_ID_MVSTATE_BUMP_COOLDOWNMS},
    {"C_ID_MVSTATE_DECEL_DURATIONMS_HOLD", C_ID_MVSTATE_DECEL_DURATIONMS_HOLD},
    {"C_ID_MVSTATE_BUMP_DURATIONMS_HOLD", C_ID_MVSTATE_BUMP_DURATIONMS_HOLD},
    {"C_ID_MVSTATE_SIGNALWAIT1_SECONDS", C_ID_MVSTATE_SIGNALWAIT1_SECONDS},
    {"C_ID_MVSTATE_SIGNALWAIT2_SECONDS", C_ID_MVSTATE_SIGNALWAIT2_SECONDS},
    {"C_ID_MVSTATE_STOPPED1_SECONDS", C_ID_MVSTATE_STOPPED1_SECONDS},
    {"C_ID_MVSTATE_STOPPED2_SECONDS", C_ID_MVSTATE_STOPPED2_SECONDS},
    {"C_ID_MVSTATE_PARK_SECONDS", C_ID_MVSTATE_PARK_SECONDS},
    {"C_ID_SERIALPRINT_INTERVALMS", C_ID_SERIALPRINT_INTERVALMS},
    {"C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD", C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD},
    {"C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD", C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD},
    {"C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD", C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD},
    {"C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD", C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD},
    {"C_ID_TURNSTATE_SPEEDKMH_MINSPEED", C_ID_TURNSTATE_SPEEDKMH_MINSPEED},
    {"C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD", C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD},
    {"C_ID_TURNSTATE_STABLEDURATIONMS", C_ID_TURNSTATE_STABLEDURATIONMS},
    {"C_ID_SAVE_CONFIG_BTN", C_ID_SAVE_CONFIG_BTN},
    {"C_ID_LOAD_CONFIG_BTN", C_ID_LOAD_CONFIG_BTN},
    {"C_ID_RESET_CONFIG_BTN", C_ID_RESET_CONFIG_BTN},
    {"C_ID_CARMOVEMENTSTATE_LABEL", C_ID_CARMOVEMENTSTATE_LABEL},
    {"C_ID_CARTURNSTATE_LABEL", C_ID_CARTURNSTATE_LABEL},
    {"C_ID_SPEED_KMH_LABEL", C_ID_SPEED_KMH_LABEL},
    {"C_ID_ACCELX_MS2_LABEL", C_ID_ACCELX_MS2_LABEL},
    {"C_ID_ACCELY_MS2_LABEL", C_ID_ACCELY_MS2_LABEL},
    {"C_ID_ACCELZ_MS2_LABEL", C_ID_ACCELZ_MS2_LABEL},
    {"C_ID_YAWANGLE_DEG_LABEL", C_ID_YAWANGLE_DEG_LABEL},
    {"C_ID_PITCHANGLE_DEG_LABEL", C_ID_PITCHANGLE_DEG_LABEL},
    {"C_ID_YAWANGLEVELOCITY_DEGPS_LABEL", C_ID_YAWANGLEVELOCITY_DEGPS_LABEL},
    {"C_ID_ISEMERGENCYBRAKING_LABEL", C_ID_ISEMERGENCYBRAKING_LABEL},
    {"C_ID_ISSPEEDBUMPDETECTED_LABEL", C_ID_ISSPEEDBUMPDETECTED_LABEL},
    {"C_ID_CURRENTSTOPTIME_SEC_LABEL", C_ID_CURRENTSTOPTIME_SEC_LABEL}
};
const size_t controlMapSize = sizeof(controlMap) / sizeof(controlMap[0]);


// ====================================================================================================
// 함수 선언 (프로토타입)
// ====================================================================================================
bool W010_EmbUI_loadUIConfig(); // UI 설정 JSON 로드 함수
void W010_EmbUI_init(); // 함수 이름은 EmbUI 그대로 두지만, 내부 구현은 ESPUI를 사용
void W010_EmbUI_setupWebPages();
void W010_EmbUI_loadConfigToWebUI();
void W010_ESPUI_callback(Control* p_Control, int p_controlType, void* p_userData);
void W010_EmbUI_updateCarStatusWeb();
void W010_EmbUI_run();

// ====================================================================================================
// 함수 정의
// ====================================================================================================

/**
 * @brief UI 설정 JSON 파일을 LittleFS에서 로드하고 파싱합니다.
 * @return 로드 성공 시 true, 실패 시 false
 */
bool W010_EmbUI_loadUIConfig() {
    dbgP1_println_F(F("UI 설정 파일 로드 중..."));
    // LittleFS.begin()은 W010_EmbUI_init()에서 이미 호출되었다고 가정합니다.

    File v_configFile = LittleFS.open(G_W010_UI_CONFIG_JSON_FILE, "r");
    if (!v_configFile) {
        dbgP1_println_F(F("ui_config.json 파일 열기 실패!"));
        return false;
    }

    DeserializationError v_error = deserializeJson(g_W010_uiConfigDoc, v_configFile);
    if (v_error) {
        dbgP1_printf_F(F("JSON 파싱 실패: %s\n"), v_error.c_str());
        v_configFile.close();
        return false;
    }

    v_configFile.close();
    dbgP1_println_F(F("UI 설정 파일 로드 및 파싱 완료."));
    return true;
}


/**
 * @brief ESPUI를 초기화하고 Wi-Fi 연결을 설정합니다.
 * Access Point (AP) 모드로 시작하여 ESP32가 설정 웹페이지를 호스팅합니다.
 * 추후 Wi-Fi STA 모드로 전환하여 기존 네트워크에 연결할 수 있습니다.
 */
void W010_EmbUI_init() {
    dbgP1_println_F(F("ESPUI 초기화 중..."));

    // LittleFS 초기화 및 UI 설정 로드 (한 번만 마운트)
    if (!LittleFS.begin()) {
        dbgP1_println_F(F("LittleFS 마운트 실패! UI 설정 로드 불가."));
        // 파일 시스템 에러 처리, 더 이상 진행하지 않거나 기본값으로 폴백
        return;
    } else {
        if (!W010_EmbUI_loadUIConfig()) {
            dbgP1_println_F(F("UI 설정 JSON 파일 로드 실패. 기본 UI 상수값 사용 또는 UI 구성에 문제 발생 가능."));
            // JSON 로드 실패 시 대체 로직 또는 경고 처리
        }
    }

    // WiFi 설정은 ESPUI.begin() 전에 이루어져야 합니다.
    // ESPUI.begin()은 기본적으로 AP 모드를 시작하며, 이미 WiFi가 연결되어 있다면 STA 모드를 유지합니다.
    // 여기서는 간단히 AP 모드로 시작하는 예시를 보여줍니다.
    // 실제 프로젝트에서는 Wi-FiManager 등을 사용하여 STA 모드와 AP 모드를 유연하게 관리하는 것이 좋습니다.
    // WiFi.softAP("RobotEyeCarAP", "your_ap_password"); // 이 부분을 사용하려면 <WiFi.h>를 추가해야 합니다.
    // IPAddress apIP = WiFi.softAPIP();
    // dbgP1_printf_F(F("AP IP Address: %s\n"), apIP.toString().c_str());

    ESPUI.begin("Robot Eye Car", "roboteye", "carpassword"); // ESPUI 시작 (WiFi AP 이름, 사용자, 비밀번호)
                                                            // Serial.begin()은 setup()에서 먼저 호출되어야 함

    dbgP1_println_F(F("ESPUI 초기화 완료. 웹 인터페이스 접근 가능."));
}

/**
 * @brief ESPUI 웹페이지 UI를 구성하고, g_M010_Config 구조체의 멤버 변수들을 웹 UI에 바인딩합니다.
 * 사용자가 웹 인터페이스를 통해 설정을 조회하고 변경할 수 있도록 필드를 정의합니다.
 */
void W010_EmbUI_setupWebPages() {
    dbgP1_println_F(F("ESPUI 웹페이지 UI 설정 중 (JSON 활용)..."));

    JsonArray tabs = g_W010_uiConfigDoc["tabs"].as<JsonArray>();

    for (JsonObject tab : tabs) {
        String tabId = tab["id"].as<String>();
        String tabTitle = tab["title"].as<String>();
        JsonArray controls = tab["controls"].as<JsonArray>();

        uint16_t currentTabId;
        if (tabId.equals(F("config"))) {
            g_W010_Tab_Config_Id = ESPUI.addControl(Tab, tabTitle.c_str(), tabTitle);
            currentTabId = g_W010_Tab_Config_Id;
        } else if (tabId.equals(F("status"))) {
            g_W010_Tab_Status_Id = ESPUI.addControl(ControlType::Tab, tabTitle.c_str(), F(""), ControlColor::Wetasphalt);
            ESPUI.setVertical(g_W010_Tab_Status_Id, true);
            currentTabId = g_W010_Tab_Status_Id;
        } else {
            dbgP1_printf_F(F("알 수 없는 탭 ID: %s\n"), tabId.c_str());
            continue;
        }

        for (JsonObject control : controls) {
            String enumIdStr = control["enum_id"].as<String>();
            String label = control["label"].as<String>();
            JsonVariant defaultValue = control["default_value"]; // 값을 직접 사용하지 않고, 레이블 생성 시에만 활용.

            int controlEnumId = -1;
            // 구조체 배열을 사용하여 enum_id 문자열을 enum 값으로 매핑
            for (size_t i = 0; i < controlMapSize; ++i) {
                if (enumIdStr.equals(controlMap[i].idStr)) {
                    controlEnumId = controlMap[i].enumVal;
                    break;
                }
            }
            
            // 알림/오류 라벨은 특별 처리
            if (enumIdStr.equals(F("g_W010_Control_Alaram_Id"))) {
                g_W010_Control_Alaram_Id = ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, currentTabId);
                continue; // 다음 컨트롤로 넘어감
            } else if (enumIdStr.equals(F("g_W010_Control_Error_Id"))) {
                g_W010_Control_Error_Id = ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, currentTabId);
                continue; // 다음 컨트롤로 넘어감
            }

            if (controlEnumId == -1) {
                dbgP1_printf_F(F("JSON에 정의된 알 수 없는 enum_id: %s\n"), enumIdStr.c_str());
                continue;
            }

            // 컨트롤 타입에 따라 다르게 생성
            if (tabId.equals(F("config"))) {
                // Config 탭의 컨트롤들은 Number 또는 Button
                if (enumIdStr.endsWith(F("_BTN"))) { // 버튼인지 확인
                    ESPUI.addControl(ControlType::Button, label.c_str(), label.c_str(), ControlColor::Emerald, currentTabId, &W010_ESPUI_callback, reinterpret_cast<void*>(static_cast<uintptr_t>(controlEnumId)));
                } else {
                    // Number 컨트롤의 초기값은 String(g_M010_Config.xxx, decimal_places) 형식으로 제공되어야 합니다.
                    // 여기서는 초기값을 defaultValue를 String으로 변환하여 사용하고, 나중에 loadConfigToWebUI에서 실제 값을 로드합니다.
                    ESPUI.addControl(ControlType::Number, label.c_str(), String(defaultValue.as<float>(), 3), ControlColor::Alizarin, currentTabId, &W010_ESPUI_callback, reinterpret_cast<void*>(static_cast<uintptr_t>(controlEnumId)));
                }
            } else if (tabId.equals(F("status"))) {
                // Status 탭의 컨트롤들은 Label
                ESPUI.addControl(ControlType::Label, label.c_str(), defaultValue.as<String>(), ControlColor::Wetasphalt, currentTabId, nullptr, reinterpret_cast<void*>(static_cast<uintptr_t>(controlEnumId)));
            }
        }
    }

    dbgP1_println_F(F("ESPUI 웹페이지 UI 설정 완료 (JSON 활용)."));
}

/**
 * @brief g_M010_Config 구조체에 저장된 현재 설정값을 ESPUI 웹 UI로 로드하여 표시합니다.
 * 주로 시스템 시작 시 또는 '불러오기' 버튼 클릭 시 호출됩니다.
 */
void W010_EmbUI_loadConfigToWebUI() {
    dbgP1_println_F(F("웹 UI에 설정값 로드 중..."));

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

    dbgP1_println_F(F("웹 UI에 설정값 로드 완료."));
}

/**
 * @brief ESPUI 웹 UI에서 컨트롤 값이 변경되거나 버튼이 클릭될 때 호출되는 콜백 함수입니다.
 * 변경된 값을 g_M010_Config 구조체에 반영하거나, 명령 버튼에 따른 동작을 수행합니다.
 * @param p_Control 변경되거나 클릭된 컨트롤 객체
 * @param p_controlType 컨트롤의 변경/클릭 유형 (Control::Type::Button, Control::Type::Number 등)
 */
void W010_ESPUI_callback(Control* p_Control, int p_controlType, void* p_userData) {
    // p_userData를 uint16_t로 직접 캐스팅하여 사용 (Control ID와 일치)
    uint16_t v_controlId = reinterpret_cast<uintptr_t>(p_userData); 

    dbgP1_printf_F(F("ESPUI 콜백 감지: ID=%d, Name=%s, Value=%s, Type=%d\n"), v_controlId, p_Control->label, p_Control->value.c_str(), p_controlType);

    // Number 컨트롤의 값 변경 처리
    if (p_controlType == Number) {
        float v_floatValue  = p_Control->value.toFloat();
        int v_intValue      = p_Control->value.toInt();

        switch (v_controlId) { // p_Control->id 대신 v_controlId 사용
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
                dbgP1_printf_F(F("알 수 없는 Number 컨트롤 ID: %d\n"), v_controlId);
                break;
        }
    }
    // Button 클릭 처리
    else if (p_controlType == ControlType::Button) {
        String v_cmd = p_Control->value; // Button의 value 속성이 명령어 역할을 합니다.

        if (v_cmd.equals(BTN_CMD_SAVE_CONFIG)) {
            if (M010_Config_save()) {
                dbgP1_println_F(F("웹 UI: 설정값이 LittleFS에 저장되었습니다."));                
                ESPUI.updateControlValue(g_W010_Control_Alaram_Id, F("설정값이 성공적으로 저장되었습니다."));
            } else {
                dbgP1_println_F(F("웹 UI: 설정값 저장 실패!"));
                ESPUI.updateControlValue(g_W010_Control_Error_Id, F("파일 시스템에 설정값을 저장할 수 없습니다."));
            }
        } else if (v_cmd.equals(BTN_CMD_LOAD_CONFIG)) {
            if (M010_Config_load()) {
                dbgP1_println_F(F("웹 UI: 설정값이 LittleFS에서 로드되었습니다."));
                W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 로드된 값 반영
                ESPUI.updateControlValue(g_W010_Control_Alaram_Id, F("설정값이 성공적으로 불러와졌습니다."));
            } else {
                dbgP1_println_F(F("웹 UI: 설정값 로드 실패! 기본값이 사용됩니다."));
                M010_Config_initDefaults(); // 로드 실패 시 기본값으로 초기화
                W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 기본값 반영

                ESPUI.updateControlValue(g_W010_Control_Error_Id, F("설정 파일을 찾을 수 없습니다. 기본값이 적용됩니다."));
            }
        } else if (v_cmd.equals(BTN_CMD_RESET_CONFIG)) {
            M010_Config_initDefaults(); // 설정값을 기본값으로 초기화
            if (M010_Config_save()) { // 초기화된 기본값을 파일에 저장
                dbgP1_println_F(F("웹 UI: 설정값이 기본값으로 초기화되고 저장되었습니다."));
                W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 기본값 반영
                ESPUI.updateControlValue(g_W010_Control_Alaram_Id, F("설정값이 기본값으로 초기화되었습니다."));
            } else {
                dbgP1_println_F(F("웹 UI: 설정 초기화 후 저장 실패!"));
                ESPUI.updateControlValue(g_W010_Control_Error_Id, F("기본값 저장 중 오류가 발생했습니다."));
            }
        } else {
            dbgP1_printf_F(F("웹 UI: 알 수 없는 명령: %s\n"), v_cmd.c_str());
            ESPUI.updateControlValue(g_W010_Control_Error_Id, F("알 수 없는 명령입니다."));
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
        case E_M010_CARMOVESTATE_UNKNOWN:       v_movementStateStr = F("알 수 없음")                                                                     ; break;
        case E_M010_CARMOVESTATE_STOPPED_INIT:  v_movementStateStr = F("정차 중 (기본)")                                                                 ; break;
        case E_M010_CARMOVESTATE_SIGNAL_WAIT1:  v_movementStateStr = String(F("신호대기 1 (")) + String(g_M010_Config.mvState_signalWait1_Seconds) + F("s 미만)"); break;
        case E_M010_CARMOVESTATE_SIGNAL_WAIT2:  v_movementStateStr = String(F("신호대기 2 (")) + String(g_M010_Config.mvState_signalWait2_Seconds) + F("s 미만)"); break;
        case E_M010_CARMOVESTATE_STOPPED1:      v_movementStateStr = String(F("정차 1 (")) + String(g_M010_Config.mvState_stopped1_Seconds / 60) + F("분 미만)"); break;
        case E_M010_CARMOVESTATE_STOPPED2:      v_movementStateStr = String(F("정차 2 (")) + String(g_M010_Config.mvState_stopped2_Seconds / 60) + F("분 미만)"); break;
        case E_M010_CARMOVESTATE_PARKED:        v_movementStateStr = String(F("주차 중 (>= ")) + String(g_M010_Config.mvState_park_Seconds / 60) + F("분)"); break;
        case E_M010_CARMOVESTATE_FORWARD:       v_movementStateStr = F("전진 중"); break;
        case E_M010_CARMOVESTATE_REVERSE:       v_movementStateStr = F("후진 중"); break;
    }

    String v_turnStateStr;
    switch (g_M010_CarStatus.carTurnState) {
        case E_M010_CARTURNSTATE_CENTER:    v_turnStateStr = F("직진 또는 정지"); break;
        case E_M010_CARTURNSTATE_LEFT_1:    v_turnStateStr = F("약간 좌회전"); break;
        case E_M010_CARTURNSTATE_LEFT_2:    v_turnStateStr = F("중간 좌회전"); break;
        case E_M010_CARTURNSTATE_LEFT_3:    v_turnStateStr = F("급격한 좌회전"); break;
        case E_M010_CARTURNSTATE_RIGHT_1:   v_turnStateStr = F("약간 우회전"); break;
        case E_M010_CARTURNSTATE_RIGHT_2:   v_turnStateStr = F("중간 우회전"); break;
        case E_M010_CARTURNSTATE_RIGHT_3:   v_turnStateStr = F("급격한 우회전"); break;
    }

    ESPUI.updateControlValue(C_ID_CARMOVEMENTSTATE_LABEL, v_movementStateStr);
    ESPUI.updateControlValue(C_ID_CARTURNSTATE_LABEL, v_turnStateStr);
    ESPUI.updateControlValue(C_ID_SPEED_KMH_LABEL, String(g_M010_CarStatus.speed_kmh, 2) + F(" km/h"));
    ESPUI.updateControlValue(C_ID_ACCELX_MS2_LABEL, String(g_M010_CarStatus.accelX_ms2, 2) + F(" m/s^2"));
    ESPUI.updateControlValue(C_ID_ACCELY_MS2_LABEL, String(g_M010_CarStatus.accelY_ms2, 2) + F(" m/s^2"));
    ESPUI.updateControlValue(C_ID_ACCELZ_MS2_LABEL, String(g_M010_CarStatus.accelZ_ms2, 2) + F(" m/s^2"));
    ESPUI.updateControlValue(C_ID_YAWANGLE_DEG_LABEL, String(g_M010_CarStatus.yawAngle_deg, 2) + F(" 도"));
    ESPUI.updateControlValue(C_ID_PITCHANGLE_DEG_LABEL, String(g_M010_CarStatus.pitchAngle_deg, 2) + F(" 도"));
    ESPUI.updateControlValue(C_ID_YAWANGLEVELOCITY_DEGPS_LABEL, String(g_M010_CarStatus.yawAngleVelocity_degps, 2) + F(" 도/초"));
    ESPUI.updateControlValue(C_ID_ISEMERGENCYBRAKING_LABEL, g_M010_CarStatus.isEmergencyBraking ? F("감지됨") : F("아님"));
    ESPUI.updateControlValue(C_ID_ISSPEEDBUMPDETECTED_LABEL, g_M010_CarStatus.isSpeedBumpDetected ? F("감지됨") : F("아님"));
    ESPUI.updateControlValue(C_ID_CURRENTSTOPTIME_SEC_LABEL, String(g_M010_CarStatus.currentStopTime_ms / 1000) + F(" 초"));
}

/**
 * @brief ESPUI의 메인 루프를 실행하고, 주기적으로 자동차 상태를 웹에 업데이트합니다.
 * Arduino의 `loop()` 함수에서 이 함수를 호출해야 합니다.
 */
void W010_EmbUI_run() {
    // ESPUI.loop()은 ESPUI.begin()에서 내부적으로 WebServer를 시작하므로
    // 별도로 호출할 필요가 없습니다. (예전 버전에서는 필요했음)

    static u_int32_t v_lastWebUpdateTime_ms = 0;
    if (millis() - v_lastWebUpdateTime_ms >= g_M010_Config.serialPrint_intervalMs) {
        W010_EmbUI_updateCarStatusWeb();
        v_lastWebUpdateTime_ms = millis();
    }
}
