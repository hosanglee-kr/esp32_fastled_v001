
#pragma once
// W010_embUI_005.h
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
    if (!LittleFS.begin()) {
        dbgP1_println_F(F("LittleFS 마운트 실패!"));
        return false;
    }

    File configFile = LittleFS.open(G_W010_UI_CONFIG_JSON_FILE, "r");
    if (!configFile) {
        dbgP1_println_F(F("ui_config.json 파일 열기 실패!"));
        LittleFS.end();
        return false;
    }

    // JSON 문서의 크기 설정 (대략적인 예상 크기, 실제 JSON 크기에 따라 조절)
    // DynamicJsonDocument doc(1024 * 4); // 4KB 예상
    // g_W010_uiConfigDoc은 전역으로 선언되어 있으므로 여기서 초기화하지 않음.
    // 하지만, 크기를 명시적으로 지정해야 한다면 아래와 같이 사용 가능.
    // g_W010_uiConfigDoc.clear(); // 기존 데이터가 있다면 초기화
    // g_W010_uiConfigDoc.capacity = 1024 * 4; // 크기 설정 (ArduinoJson v6 이상)

    DeserializationError error = deserializeJson(g_W010_uiConfigDoc, configFile);
    if (error) {
        dbgP1_printf_F(F("JSON 파싱 실패: %s\n"), error.c_str());
        configFile.close();
        LittleFS.end();
        return false;
    }

    configFile.close();
    LittleFS.end();
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

	// LittleFS 초기화 및 UI 설정 로드
    if (!LittleFS.begin()) {
        dbgP1_println_F(F("LittleFS 마운트 실패! UI 설정 로드 불가."));
        // 파일 시스템 에러 처리
    } else {
        if (!W010_EmbUI_loadUIConfig()) {
            dbgP1_println_F(F("UI 설정 JSON 파일 로드 실패. 기본 UI 상수값 사용 또는 UI 구성에 문제 발생 가능."));
            // JSON 로드 실패 시 대체 로직 또는 경고 처리
        }
        LittleFS.end(); // 파일 시스템 사용 후 해제
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
        if (tabId.equals("config")) {
            g_W010_Tab_Config_Id = ESPUI.addControl(Tab, tabTitle, tabTitle);
            currentTabId = g_W010_Tab_Config_Id;
        } else if (tabId.equals("status")) {
            g_W010_Tab_Status_Id = ESPUI.addControl(ControlType::Tab, tabTitle, "", ControlColor::Wetasphalt);
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

            // enum_id 문자열을 enum 값으로 매핑
            int controlEnumId = -1;
            // 각 enum 값에 대한 case 문을 추가하여 매핑합니다.
            // 이 부분은 수동으로 매핑해야 합니다.
            // 더 효율적인 방법은 String to Enum 매핑 함수를 만드는 것입니다.
            if (enumIdStr.equals("C_ID_MVSTATE_ACCELFILTER_ALPHA")) controlEnumId = C_ID_MVSTATE_ACCELFILTER_ALPHA;
            else if (enumIdStr.equals("C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN")) controlEnumId = C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN;
            else if (enumIdStr.equals("C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN")) controlEnumId = C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN;
            else if (enumIdStr.equals("C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX")) controlEnumId = C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX;
            else if (enumIdStr.equals("C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX")) controlEnumId = C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX;
            else if (enumIdStr.equals("C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX")) controlEnumId = C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX;
            else if (enumIdStr.equals("C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN")) controlEnumId = C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN;
            else if (enumIdStr.equals("C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN")) controlEnumId = C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN;
            else if (enumIdStr.equals("C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD")) controlEnumId = C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD;
            else if (enumIdStr.equals("C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD")) controlEnumId = C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD;
            else if (enumIdStr.equals("C_ID_MVSTATE_BUMP_SPEEDKMH_MIN")) controlEnumId = C_ID_MVSTATE_BUMP_SPEEDKMH_MIN;
            else if (enumIdStr.equals("C_ID_MVSTATE_BUMP_COOLDOWNMS")) controlEnumId = C_ID_MVSTATE_BUMP_COOLDOWNMS;
            else if (enumIdStr.equals("C_ID_MVSTATE_DECEL_DURATIONMS_HOLD")) controlEnumId = C_ID_MVSTATE_DECEL_DURATIONMS_HOLD;
            else if (enumIdStr.equals("C_ID_MVSTATE_BUMP_DURATIONMS_HOLD")) controlEnumId = C_ID_MVSTATE_BUMP_DURATIONMS_HOLD;
            else if (enumIdStr.equals("C_ID_MVSTATE_SIGNALWAIT1_SECONDS")) controlEnumId = C_ID_MVSTATE_SIGNALWAIT1_SECONDS;
            else if (enumIdStr.equals("C_ID_MVSTATE_SIGNALWAIT2_SECONDS")) controlEnumId = C_ID_MVSTATE_SIGNALWAIT2_SECONDS;
            else if (enumIdStr.equals("C_ID_MVSTATE_STOPPED1_SECONDS")) controlEnumId = C_ID_MVSTATE_STOPPED1_SECONDS;
            else if (enumIdStr.equals("C_ID_MVSTATE_STOPPED2_SECONDS")) controlEnumId = C_ID_MVSTATE_STOPPED2_SECONDS;
            else if (enumIdStr.equals("C_ID_MVSTATE_PARK_SECONDS")) controlEnumId = C_ID_MVSTATE_PARK_SECONDS;
            else if (enumIdStr.equals("C_ID_SERIALPRINT_INTERVALMS")) controlEnumId = C_ID_SERIALPRINT_INTERVALMS;
            else if (enumIdStr.equals("C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD")) controlEnumId = C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD;
            else if (enumIdStr.equals("C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD")) controlEnumId = C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD;
            else if (enumIdStr.equals("C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD")) controlEnumId = C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD;
            else if (enumIdStr.equals("C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD")) controlEnumId = C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD;
            else if (enumIdStr.equals("C_ID_TURNSTATE_SPEEDKMH_MINSPEED")) controlEnumId = C_ID_TURNSTATE_SPEEDKMH_MINSPEED;
            else if (enumIdStr.equals("C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD")) controlEnumId = C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD;
            else if (enumIdStr.equals("C_ID_TURNSTATE_STABLEDURATIONMS")) controlEnumId = C_ID_TURNSTATE_STABLEDURATIONMS;
            else if (enumIdStr.equals("C_ID_SAVE_CONFIG_BTN")) controlEnumId = C_ID_SAVE_CONFIG_BTN;
            else if (enumIdStr.equals("C_ID_LOAD_CONFIG_BTN")) controlEnumId = C_ID_LOAD_CONFIG_BTN;
            else if (enumIdStr.equals("C_ID_RESET_CONFIG_BTN")) controlEnumId = C_ID_RESET_CONFIG_BTN;
            else if (enumIdStr.equals("C_ID_CARMOVEMENTSTATE_LABEL")) controlEnumId = C_ID_CARMOVEMENTSTATE_LABEL;
            else if (enumIdStr.equals("C_ID_CARTURNSTATE_LABEL")) controlEnumId = C_ID_CARTURNSTATE_LABEL;
            else if (enumIdStr.equals("C_ID_SPEED_KMH_LABEL")) controlEnumId = C_ID_SPEED_KMH_LABEL;
            else if (enumIdStr.equals("C_ID_ACCELX_MS2_LABEL")) controlEnumId = C_ID_ACCELX_MS2_LABEL;
            else if (enumIdStr.equals("C_ID_ACCELY_MS2_LABEL")) controlEnumId = C_ID_ACCELY_MS2_LABEL;
            else if (enumIdStr.equals("C_ID_ACCELZ_MS2_LABEL")) controlEnumId = C_ID_ACCELZ_MS2_LABEL;
            else if (enumIdStr.equals("C_ID_YAWANGLE_DEG_LABEL")) controlEnumId = C_ID_YAWANGLE_DEG_LABEL;
            else if (enumIdStr.equals("C_ID_PITCHANGLE_DEG_LABEL")) controlEnumId = C_ID_PITCHANGLE_DEG_LABEL;
            else if (enumIdStr.equals("C_ID_YAWANGLEVELOCITY_DEGPS_LABEL")) controlEnumId = C_ID_YAWANGLEVELOCITY_DEGPS_LABEL;
            else if (enumIdStr.equals("C_ID_ISEMERGENCYBRAKING_LABEL")) controlEnumId = C_ID_ISEMERGENCYBRAKING_LABEL;
            else if (enumIdStr.equals("C_ID_ISSPEEDBUMPDETECTED_LABEL")) controlEnumId = C_ID_ISSPEEDBUMPDETECTED_LABEL;
            else if (enumIdStr.equals("C_ID_CURRENTSTOPTIME_SEC_LABEL")) controlEnumId = C_ID_CURRENTSTOPTIME_SEC_LABEL;
            else if (enumIdStr.equals("g_W010_Control_Alaram_Id")) { // 알림/오류 라벨은 enum_id 대신 전역 변수를 직접 사용
                g_W010_Control_Alaram_Id = ESPUI.addControl(ControlType::Label, label, defaultValue.as<String>(), ControlColor::Wetasphalt, currentTabId);
                continue; // 다음 컨트롤로 넘어감
            }
            else if (enumIdStr.equals("g_W010_Control_Error_Id")) { // 알림/오류 라벨은 enum_id 대신 전역 변수를 직접 사용
                g_W010_Control_Error_Id = ESPUI.addControl(ControlType::Label, label, defaultValue.as<String>(), ControlColor::Wetasphalt, currentTabId);
                continue; // 다음 컨트롤로 넘어감
            }


            if (controlEnumId == -1) {
                dbgP1_printf_F(F("JSON에 정의된 알 수 없는 enum_id: %s\n"), enumIdStr.c_str());
                continue;
            }

            // 컨트롤 타입에 따라 다르게 생성
            if (tabId.equals("config")) {
                // Config 탭의 컨트롤들은 Number 또는 Button
                if (enumIdStr.endsWith("_BTN")) { // 버튼인지 확인
                    ESPUI.addControl(ControlType::Button, label, label.c_str(), ControlColor::Emerald, currentTabId, &W010_ESPUI_callback, (void*)controlEnumId);
                } else {
                    // Number 컨트롤의 초기값은 String(g_M010_Config.xxx, decimal_places) 형식으로 제공되어야 합니다.
                    // 여기서는 초기값을 0으로 설정하고, 나중에 loadConfigToWebUI에서 실제 값을 로드합니다.
                    ESPUI.addControl(ControlType::Number, label, String(defaultValue.as<float>(), 3), ControlColor::Alizarin, currentTabId, &W010_ESPUI_callback, (void*)controlEnumId);
                }
            } else if (tabId.equals("status")) {
                // Status 탭의 컨트롤들은 Label
                ESPUI.addControl(ControlType::Label, label, defaultValue.as<String>(), ControlColor::Wetasphalt, currentTabId, nullptr, (void*)controlEnumId);
            }
        }
    }

    dbgP1_println_F(F("ESPUI 웹페이지 UI 설정 완료 (JSON 활용)."));
}
