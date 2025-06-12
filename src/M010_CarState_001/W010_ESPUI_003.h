#pragma once
// W010_embUI_001.h
// ====================================================================================================
// 프로젝트: 자동차 후방 로봇 눈
// 설명: ESP32 + MPU6050 활용, 차량 움직임 감지 및 LED Matrix 로봇 눈 표정 표현.
//       본 코드는 ESPUI 라이브러리를 사용하여 웹 기반 설정 관리 및 실시간 상태 모니터링 기능을 제공합니다.
// 개발 환경: PlatformIO Arduino Core (ESP32)
// 사용 라이브러리: ESPUI (s00500), ArduinoJson (bblanchon)
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
#include <ArduinoJson.h>

// M010_main3_010.h 에 정의된 전역 변수 및 함수 선언을 사용하기 위해 포함
#include "M010_main3_011.h"

// ====================================================================================================
// 전역 변수 선언
// ====================================================================================================

// ESPUI는 페이지 자체를 컨트롤로 추가하고, 그 ID를 페이지 ID로 사용합니다.
uint16_t g_configPageId;
uint16_t g_statusPageId;

uint16_t g_AlaramId ;
uint16_t g_ErrorId ;

// ESPUI 컨트롤 ID 정의 (각 컨트롤마다 고유한 ID를 부여)
// 이렇게 enum으로 관리하면 가독성이 좋고 오타를 줄일 수 있습니다.
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
};


// ====================================================================================================
// 함수 선언 (프로토타입)
// ====================================================================================================
void W010_EmbUI_init(); // 함수 이름은 EmbUI 그대로 두지만, 내부 구현은 ESPUI를 사용
void W010_EmbUI_setupWebPages();
void W010_EmbUI_loadConfigToWebUI();
void W010_ESPUI_callback(Control* control, int type, void* userData);
//void W010_ESPUI_callback(Control* control, int type); // ESPUI 콜백 함수 시그니처에 맞게 변경
void W010_EmbUI_updateCarStatusWeb();
void W010_EmbUI_run();

// ====================================================================================================
// 함수 정의
// ====================================================================================================

/**
 * @brief ESPUI를 초기화하고 Wi-Fi 연결을 설정합니다.
 * Access Point (AP) 모드로 시작하여 ESP32가 설정 웹페이지를 호스팅합니다.
 * 추후 Wi-Fi STA 모드로 전환하여 기존 네트워크에 연결할 수 있습니다.
 */
void W010_EmbUI_init() {
    dbgP1_println_F(F("ESPUI 초기화 중..."));

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
    dbgP1_println_F(F("ESPUI 웹페이지 UI 설정 중..."));

    // 'configPage' 생성 및 컨트롤 추가
    //auto maintab = ESPUI.addControl(Tab, "", "Basic controls");

    g_configPageId = ESPUI.addControl(Tab, "시스템 설정", "시스템 설정");

    //g_configPageId = ESPUI.addControl(Control::Type::Page, "시스템 설정", "", Control::Color::Alizarin, Control::Vertical);

    // 이제 g_configPageId를 부모 ID로 사용하여 해당 페이지에 컨트롤을 추가합니다.


    ESPUI.addControl(ControlType::Number, "가속도 필터 Alpha", String(g_M010_Config.mvState_accelFilter_Alpha, 3), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_ACCELFILTER_ALPHA);
    ESPUI.addControl(ControlType::Number, "전진 시작 속도 (km/h)", String(g_M010_Config.mvState_Forward_speedKmh_Threshold_Min, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN);
    ESPUI.addControl(ControlType::Number, "후진 시작 속도 (km/h)", String(g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN);
    ESPUI.addControl(ControlType::Number, "정지 판단 최대 속도 (km/h)", String(g_M010_Config.mvState_Stop_speedKmh_Threshold_Max, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX);
    ESPUI.addControl(ControlType::Number, "정지 판단 Y Accel (m/s^2)", String(g_M010_Config.mvState_Stop_accelMps2_Threshold_Max, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX);
    ESPUI.addControl(ControlType::Number, "정지 판단 Yaw Gyro (deg/s)", String(g_M010_Config.mvState_Stop_gyroDps_Threshold_Max, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX);
    ESPUI.addControl(ControlType::Number, "정지 안정 최소 시간 (ms)", String(g_M010_Config.mvState_stop_durationMs_Stable_Min), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN);
    ESPUI.addControl(ControlType::Number, "움직임 안정 최소 시간 (ms)", String(g_M010_Config.mvState_move_durationMs_Stable_Min), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN);
    ESPUI.addControl(ControlType::Number, "급감속 Accel Y (m/s^2)", String(g_M010_Config.mvState_Decel_accelMps2_Threshold, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD);
    ESPUI.addControl(ControlType::Number, "방지턱 Accel Z (m/s^2)", String(g_M010_Config.mvState_Bump_accelMps2_Threshold, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD);
    ESPUI.addControl(ControlType::Number, "방지턱 최소 속도 (km/h)", String(g_M010_Config.mvState_Bump_SpeedKmh_Min, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_BUMP_SPEEDKMH_MIN);
    ESPUI.addControl(ControlType::Number, "방지턱 쿨다운 (ms)", String(g_M010_Config.mvState_Bump_CooldownMs), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_BUMP_COOLDOWNMS);
    ESPUI.addControl(ControlType::Number, "급감속 유지 시간 (ms)", String(g_M010_Config.mvState_Decel_durationMs_Hold), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_DECEL_DURATIONMS_HOLD);
    ESPUI.addControl(ControlType::Number, "방지턱 유지 시간 (ms)", String(g_M010_Config.mvState_Bump_durationMs_Hold), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_BUMP_DURATIONMS_HOLD);
    ESPUI.addControl(ControlType::Number, "신호대기1 (초)", String(g_M010_Config.mvState_signalWait1_Seconds), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_SIGNALWAIT1_SECONDS);
    ESPUI.addControl(ControlType::Number, "신호대기2 (초)", String(g_M010_Config.mvState_signalWait2_Seconds), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_SIGNALWAIT2_SECONDS);
    ESPUI.addControl(ControlType::Number, "정차1 (초)", String(g_M010_Config.mvState_stopped1_Seconds), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_STOPPED1_SECONDS);
    ESPUI.addControl(ControlType::Number, "정차2 (초)", String(g_M010_Config.mvState_stopped2_Seconds), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_STOPPED2_SECONDS);
    ESPUI.addControl(ControlType::Number, "주차 (초)", String(g_M010_Config.mvState_park_Seconds), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_MVSTATE_PARK_SECONDS);
    ESPUI.addControl(ControlType::Number, "시리얼 출력 간격 (ms)", String(g_M010_Config.serialPrint_intervalMs), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_SERIALPRINT_INTERVALMS);
    ESPUI.addControl(ControlType::Number, "직진 Yaw Thres (deg/s)", String(g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD);
    ESPUI.addControl(ControlType::Number, "LR1 Yaw Thres (deg/s)", String(g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD);
    ESPUI.addControl(ControlType::Number, "LR2 Yaw Thres (deg/s)", String(g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD);
    ESPUI.addControl(ControlType::Number, "LR3 Yaw Thres (deg/s)", String(g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD);
    ESPUI.addControl(ControlType::Number, "회전 최소 속도 (km/h)", String(g_M010_Config.turnState_speedKmh_MinSpeed, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_TURNSTATE_SPEEDKMH_MINSPEED);
    ESPUI.addControl(ControlType::Number, "고속 회전 임계 (km/h)", String(g_M010_Config.turnState_speedKmh_HighSpeed_Threshold, 2), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD);
    ESPUI.addControl(ControlType::Number, "회전 안정 시간 (ms)", String(g_M010_Config.turnState_StableDurationMs), ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_TURNSTATE_STABLEDURATIONMS);

    ESPUI.addControl(ControlType::Button, "설정 저장", "save_config",   ControlColor::Emerald, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_SAVE_CONFIG_BTN);
    ESPUI.addControl(ControlType::Button, "설정 불러오기", "load_config", ControlColor::Carrot, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_LOAD_CONFIG_BTN);
    ESPUI.addControl(ControlType::Button, "설정 초기화", "reset_config", ControlColor::Alizarin, g_configPageId, &W010_ESPUI_callback, (void*)C_ID_RESET_CONFIG_BTN);


    // 'statusPage' 생성 및 컨트롤 추가
    g_statusPageId = ESPUI.addControl(ControlType::Tab, "자동차 현재 상태", "", ControlColor::Wetasphalt);
    ESPUI.setVertical(g_statusPageId, true);

    ///// g_statusPageId = ESPUI.addControl(Control::Type::Page, "자동차 현재 상태", "", ControlColor::Wetasphalt, Control::Vertical);

    // 이제 g_statusPageId를 부모 ID로 사용하여 해당 페이지에 컨트롤을 추가합니다.
    ESPUI.addControl(ControlType::Label, "움직임 상태:"       , "정보 없음", ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_CARMOVEMENTSTATE_LABEL);
    ESPUI.addControl(ControlType::Label, "회전 상태:"         , "정보 없음", ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_CARTURNSTATE_LABEL);
    ESPUI.addControl(ControlType::Label, "추정 속도:"         , "0.00 km/h", ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_SPEED_KMH_LABEL);
    ESPUI.addControl(ControlType::Label, "AccelX (m/s^2):"  , "0.00 m/s^2", ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_ACCELX_MS2_LABEL);
    ESPUI.addControl(ControlType::Label, "AccelY (m/s^2):"  , "0.00 m/s^2", ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_ACCELY_MS2_LABEL);
    ESPUI.addControl(ControlType::Label, "AccelZ (m/s^2):"  , "0.00 m/s^2", ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_ACCELZ_MS2_LABEL);
    ESPUI.addControl(ControlType::Label, "Yaw (도):"        , "0.00 도", ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_YAWANGLE_DEG_LABEL);
    ESPUI.addControl(ControlType::Label, "Pitch (도):"      , "0.00 도", ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_PITCHANGLE_DEG_LABEL);
    ESPUI.addControl(ControlType::Label, "Yaw 속도 (도/초):"    , "0.00 도/초", ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_YAWANGLEVELOCITY_DEGPS_LABEL);
    ESPUI.addControl(ControlType::Label, "급감속 감지:", "아님" , ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_ISEMERGENCYBRAKING_LABEL);
    ESPUI.addControl(ControlType::Label, "방지턱 감지:", "아님" , ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_ISSPEEDBUMPDETECTED_LABEL);
    ESPUI.addControl(ControlType::Label, "정차 지속 시간:", "0 초", ControlColor::Wetasphalt, g_statusPageId, nullptr, (void*)C_ID_CURRENTSTOPTIME_SEC_LABEL);


    g_AlaramId = ESPUI.addControl(ControlType::Label, "알림", "", ControlColor::Wetasphalt, g_statusPageId);
    g_ErrorId = ESPUI.addControl(ControlType::Label, "오류", "", ControlColor::Wetasphalt, g_statusPageId);

    dbgP1_println_F(F("ESPUI 웹페이지 UI 설정 완료."));
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
    ESPUI.updateControlValue(C_ID_MVSTATE_STOPPED2_SECONDS                          , String(g_M010_Config.mvState_stopped2_Seconds)); // ID 오타 수정: C_ID_MVSTATE_STOPPED2_SECONDS
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
 * @param control 변경되거나 클릭된 컨트롤 객체
 * @param type 컨트롤의 변경/클릭 유형 (Control::Type::Button, Control::Type::Number 등)
 */
void W010_ESPUI_callback(Control* control, int type, void* userData) {
    dbgP1_printf_F(F("ESPUI 콜백 감지: ID=%d, Name=%s, Value=%s, Type=%d\n"), control->id, control->label, control->value.c_str(), type);

    int controlId = *static_cast<int*>(userData); // userData를 int 포인터로 캐스팅 후 역참조

    // Number 컨트롤의 값 변경 처리
    if (type == Number) {
    //if (type == Control::Type::Number) {
        float floatValue = control->value.toFloat();
        int intValue = control->value.toInt();

        switch (control->id) {
            case C_ID_MVSTATE_ACCELFILTER_ALPHA:
                g_M010_Config.mvState_accelFilter_Alpha = floatValue;
                break;
            case C_ID_MVSTATE_FORWARD_SPEEDKMH_THRESHOLD_MIN:
                g_M010_Config.mvState_Forward_speedKmh_Threshold_Min = floatValue;
                break;
            case C_ID_MVSTATE_REVERSE_SPEEDKMH_THRESHOLD_MIN:
                g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min = floatValue;
                break;
            case C_ID_MVSTATE_STOP_SPEEDKMH_THRESHOLD_MAX:
                g_M010_Config.mvState_Stop_speedKmh_Threshold_Max = floatValue;
                break;
            case C_ID_MVSTATE_STOP_ACCELMPS2_THRESHOLD_MAX:
                g_M010_Config.mvState_Stop_accelMps2_Threshold_Max = floatValue;
                break;
            case C_ID_MVSTATE_STOP_GYRODPS_THRESHOLD_MAX:
                g_M010_Config.mvState_Stop_gyroDps_Threshold_Max = floatValue;
                break;
            case C_ID_MVSTATE_STOP_DURATIONMS_STABLE_MIN:
                g_M010_Config.mvState_stop_durationMs_Stable_Min = intValue;
                break;
            case C_ID_MVSTATE_MOVE_DURATIONMS_STABLE_MIN:
                g_M010_Config.mvState_move_durationMs_Stable_Min = intValue;
                break;
            case C_ID_MVSTATE_DECEL_ACCELMPS2_THRESHOLD:
                g_M010_Config.mvState_Decel_accelMps2_Threshold = floatValue;
                break;
            case C_ID_MVSTATE_BUMP_ACCELMPS2_THRESHOLD:
                g_M010_Config.mvState_Bump_accelMps2_Threshold = floatValue;
                break;
            case C_ID_MVSTATE_BUMP_SPEEDKMH_MIN:
                g_M010_Config.mvState_Bump_SpeedKmh_Min = floatValue;
                break;
            case C_ID_MVSTATE_BUMP_COOLDOWNMS:
                g_M010_Config.mvState_Bump_CooldownMs = intValue;
                break;
            case C_ID_MVSTATE_DECEL_DURATIONMS_HOLD:
                g_M010_Config.mvState_Decel_durationMs_Hold = intValue;
                break;
            case C_ID_MVSTATE_BUMP_DURATIONMS_HOLD:
                g_M010_Config.mvState_Bump_durationMs_Hold = intValue;
                break;
            case C_ID_MVSTATE_SIGNALWAIT1_SECONDS:
                g_M010_Config.mvState_signalWait1_Seconds = intValue;
                break;
            case C_ID_MVSTATE_SIGNALWAIT2_SECONDS:
                g_M010_Config.mvState_signalWait2_Seconds = intValue;
                break;
            case C_ID_MVSTATE_STOPPED1_SECONDS:
                g_M010_Config.mvState_stopped1_Seconds = intValue;
                break;
            case C_ID_MVSTATE_STOPPED2_SECONDS: // ID 오타 수정됨
                g_M010_Config.mvState_stopped2_Seconds = intValue;
                break;
            case C_ID_MVSTATE_PARK_SECONDS:
                g_M010_Config.mvState_park_Seconds = intValue;
                break;
            case C_ID_SERIALPRINT_INTERVALMS:
                g_M010_Config.serialPrint_intervalMs = intValue;
                break;
            case C_ID_TURNSTATE_CENTER_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold = floatValue;
                break;
            case C_ID_TURNSTATE_LR_1_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold = floatValue;
                break;
            case C_ID_TURNSTATE_LR_2_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold = floatValue;
                break;
            case C_ID_TURNSTATE_LR_3_YAWANGLEVELOCITYDEGPS_THRESOLD:
                g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold = floatValue;
                break;
            case C_ID_TURNSTATE_SPEEDKMH_MINSPEED:
                g_M010_Config.turnState_speedKmh_MinSpeed = floatValue;
                break;
            case C_ID_TURNSTATE_SPEEDKMH_HIGHSPEED_THRESHOLD:
                g_M010_Config.turnState_speedKmh_HighSpeed_Threshold = floatValue;
                break;
            case C_ID_TURNSTATE_STABLEDURATIONMS:
                g_M010_Config.turnState_StableDurationMs = intValue;
                break;
            default:
                dbgP1_printf_F(F("알 수 없는 Number 컨트롤 ID: %d\n"), control->id);
                break;
        }
    }
    // Button 클릭 처리
    else if (type == ControlType::Button) {
        String cmd = control->value; // Button의 value 속성이 명령어 역할을 합니다.

        if (cmd.equals("save_config")) {
            if (M010_Config_save()) {
                dbgP1_println_F(F("웹 UI: 설정값이 LittleFS에 저장되었습니다."));                
                ESPUI.print(g_AlaramId, "설정값이 성공적으로 저장되었습니다."); // ESPUI 메시지
                // ESPUI.print("알림", "설정값이 성공적으로 저장되었습니다.", "lime"); // ESPUI 메시지
            } else {
                dbgP1_println_F(F("웹 UI: 설정값 저장 실패!"));
                ESPUI.print(g_ErrorId, "파일 시스템에 설정값을 저장할 수 없습니다.");
                //ESPUI.print("경고", "파일 시스템에 설정값을 저장할 수 없습니다.", "red");
            }
        } else if (cmd.equals("load_config")) {
            if (M010_Config_load()) {
                dbgP1_println_F(F("웹 UI: 설정값이 LittleFS에서 로드되었습니다."));
                W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 로드된 값 반영
                ESPUI.print(g_AlaramId, "설정값이 성공적으로 불러와졌습니다.");
                // ESPUI.print("알림", "설정값이 성공적으로 불러와졌습니다.", "lime");
            } else {
                dbgP1_println_F(F("웹 UI: 설정값 로드 실패! 기본값이 사용됩니다."));
                M010_Config_initDefaults(); // 로드 실패 시 기본값으로 초기화
                W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 기본값 반영

                ESPUI.print(g_ErrorId, "설정 파일을 찾을 수 없습니다. 기본값이 적용됩니다.");
                //ESPUI.print("경고", "설정 파일을 찾을 수 없습니다. 기본값이 적용됩니다.", "red");
            }
        } else if (cmd.equals("reset_config")) {
            M010_Config_initDefaults(); // 설정값을 기본값으로 초기화
            if (M010_Config_save()) { // 초기화된 기본값을 파일에 저장
                dbgP1_println_F(F("웹 UI: 설정값이 기본값으로 초기화되고 저장되었습니다."));
                W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 기본값 반영
                ESPUI.print(g_AlaramId, "설정값이 기본값으로 초기화되었습니다.");
                // ESPUI.print("알림", "설정값이 기본값으로 초기화되었습니다.", "lime");
            } else {
                dbgP1_println_F(F("웹 UI: 설정 초기화 후 저장 실패!"));
                ESPUI.print(g_ErrorId, "기본값 저장 중 오류가 발생했습니다.");
                //ESPUI.print("경고", "기본값 저장 중 오류가 발생했습니다.", "red");
            }
        } else {
            dbgP1_printf_F(F("웹 UI: 알 수 없는 명령: %s\n"), cmd.c_str());
            ESPUI.print(g_ErrorId, "알 수 없는 명령입니다.");
            //ESPUI.print("오류", "알 수 없는 명령입니다.", "red");
        }
    }
}

/**
 * @brief g_M010_CarStatus 구조체의 현재 자동차 상태 정보를 ESPUI 웹페이지에 주기적으로 업데이트합니다.
 * "status" 페이지에 정의된 Label 컨트롤들의 값을 실시간으로 갱신합니다.
 */
void W010_EmbUI_updateCarStatusWeb() {
    String movementStateStr;
    switch (g_M010_CarStatus.carMovementState) {
        case E_M010_CARMOVESTATE_UNKNOWN:       movementStateStr = "알 수 없음"                                                                     ; break;
        case E_M010_CARMOVESTATE_STOPPED_INIT:  movementStateStr = "정차 중 (기본)"                                                                 ; break;
        case E_M010_CARMOVESTATE_SIGNAL_WAIT1:  movementStateStr = "신호대기 1 (" + String(g_M010_Config.mvState_signalWait1_Seconds) + "s 미만)"   ; break;
        case E_M010_CARMOVESTATE_SIGNAL_WAIT2:  movementStateStr = "신호대기 2 (" + String(g_M010_Config.mvState_signalWait2_Seconds) + "s 미만)"   ; break;
        case E_M010_CARMOVESTATE_STOPPED1:      movementStateStr = "정차 1 (" + String(g_M010_Config.mvState_stopped1_Seconds / 60) + "분 미만)"    ; break;
        case E_M010_CARMOVESTATE_STOPPED2:      movementStateStr = "정차 2 (" + String(g_M010_Config.mvState_stopped2_Seconds / 60) + "분 미만)"    ; break;
        case E_M010_CARMOVESTATE_PARKED:        movementStateStr = "주차 중 (>= " + String(g_M010_Config.mvState_park_Seconds / 60) + "분)"         ; break;
        case E_M010_CARMOVESTATE_FORWARD:       movementStateStr = "전진 중"; break;
        case E_M010_CARMOVESTATE_REVERSE:       movementStateStr = "후진 중"; break;
    }

    String turnStateStr;
    switch (g_M010_CarStatus.carTurnState) {
        case E_M010_CARTURNSTATE_CENTER:    turnStateStr = "직진 또는 정지"; break;
        case E_M010_CARTURNSTATE_LEFT_1:    turnStateStr = "약간 좌회전"; break;
        case E_M010_CARTURNSTATE_LEFT_2:    turnStateStr = "중간 좌회전"; break;
        case E_M010_CARTURNSTATE_LEFT_3:    turnStateStr = "급격한 좌회전"; break;
        case E_M010_CARTURNSTATE_RIGHT_1:   turnStateStr = "약간 우회전"; break;
        case E_M010_CARTURNSTATE_RIGHT_2:   turnStateStr = "중간 우회전"; break;
        case E_M010_CARTURNSTATE_RIGHT_3:   turnStateStr = "급격한 우회전"; break;
    }

    ESPUI.updateControlValue(C_ID_CARMOVEMENTSTATE_LABEL, movementStateStr);
    ESPUI.updateControlValue(C_ID_CARTURNSTATE_LABEL, turnStateStr);
    ESPUI.updateControlValue(C_ID_SPEED_KMH_LABEL, String(g_M010_CarStatus.speed_kmh, 2) + " km/h");
    ESPUI.updateControlValue(C_ID_ACCELX_MS2_LABEL, String(g_M010_CarStatus.accelX_ms2, 2) + " m/s^2");
    ESPUI.updateControlValue(C_ID_ACCELY_MS2_LABEL, String(g_M010_CarStatus.accelY_ms2, 2) + " m/s^2");
    ESPUI.updateControlValue(C_ID_ACCELZ_MS2_LABEL, String(g_M010_CarStatus.accelZ_ms2, 2) + " m/s^2");
    ESPUI.updateControlValue(C_ID_YAWANGLE_DEG_LABEL, String(g_M010_CarStatus.yawAngle_deg, 2) + " 도");
    ESPUI.updateControlValue(C_ID_PITCHANGLE_DEG_LABEL, String(g_M010_CarStatus.pitchAngle_deg, 2) + " 도");
    ESPUI.updateControlValue(C_ID_YAWANGLEVELOCITY_DEGPS_LABEL, String(g_M010_CarStatus.yawAngleVelocity_degps, 2) + " 도/초");
    ESPUI.updateControlValue(C_ID_ISEMERGENCYBRAKING_LABEL, g_M010_CarStatus.isEmergencyBraking ? "감지됨" : "아님");
    ESPUI.updateControlValue(C_ID_ISSPEEDBUMPDETECTED_LABEL, g_M010_CarStatus.isSpeedBumpDetected ? "감지됨" : "아님");
    ESPUI.updateControlValue(C_ID_CURRENTSTOPTIME_SEC_LABEL, String(g_M010_CarStatus.currentStopTime_ms / 1000) + " 초");
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
