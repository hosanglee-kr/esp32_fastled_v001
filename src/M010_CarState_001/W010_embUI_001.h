#pragma once
// W010_embUI_001.h
// ====================================================================================================
// 프로젝트: 자동차 후방 로봇 눈
// 설명: ESP32 + MPU6050 활용, 차량 움직임 감지 및 LED Matrix 로봇 눈 표정 표현.
//       본 코드는 EmbUI 라이브러리를 사용하여 웹 기반 설정 관리 및 실시간 상태 모니터링 기능을 제공합니다.
// 개발 환경: PlatformIO Arduino Core (ESP32)
// 사용 라이브러리: EmbUI (DmytroKorniienko), ArduinoJson (bblanchon)
// ====================================================================================================

// --- 시리얼 출력 제어 매크로 ---
// M010_main3_010.h 에 정의된 DEBUG_P1 매크로를 활용
#include "A01_debug_001.h"

// ====================================================================================================
// lib_deps =
//        https://github.com/DmytroKorniienko/EmbUI.git#2.7.0
//        bblanchon/ArduinoJson@^7.4.1
// ====================================================================================================

#include <EmbUI.h> // EmbUI 라이브러리 포함
#include <ArduinoJson.h> // ArduinoJson 라이브러리 포함 (v7)

// M010_main3_010.h 에 정의된 전역 변수 및 함수 선언을 사용하기 위해 포함
// 이 파일은 M010_main3_010.h 보다 먼저 컴파일되어야 합니다.
#include "M010_main3_010.h" 

// ====================================================================================================
// 전역 변수 선언 (W010_으로 시작)
// ====================================================================================================
EmbUI embui; // EmbUI 객체 생성

// ====================================================================================================
// 함수 선언 (프로토타입)
// ====================================================================================================
void W010_EmbUI_init();                                     // EmbUI 초기화 및 Wi-Fi 설정
void W010_EmbUI_setupWebPages();                            // 웹페이지 UI 구성 및 설정 항목 바인딩
void W010_EmbUI_loadConfigToWebUI();                        // g_M010_Config 값을 웹 UI로 로드
void W010_EmbUI_handleConfigSave(String name, String value); // 웹 UI에서 설정 변경 시 호출되는 콜백
void W010_EmbUI_handleCommand(String cmd);                  // 웹 UI에서 특정 명령어 수신 시 처리
void W010_EmbUI_updateCarStatusWeb();                       // 자동차 상태를 웹페이지에 주기적으로 업데이트
void W010_EmbUI_run();                                      // EmbUI 메인 루프 함수

// ====================================================================================================
// 함수 정의 (W010_으로 시작)
// ====================================================================================================

/**
 * @brief EmbUI를 초기화하고 Wi-Fi 연결을 설정합니다.
 * Access Point (AP) 모드로 시작하여 ESP32가 설정 웹페이지를 호스팅하도록 합니다.
 * 추후 Wi-Fi STA 모드로 전환하여 기존 네트워크에 연결할 수 있습니다.
 */
void W010_EmbUI_init() {
    dbgP1_println_F(F("EmbUI 초기화 중..."));
    embui.begin(); // EmbUI 시작 (Serial.begin()은 setup()에서 먼저 호출되어야 함)

    // AP 모드 설정 (초기 설정 시 사용)
    //embui.setApPass("your_ap_password"); // 필요하다면 AP 비밀번호 설정
    //embui.setHostname("robot_eye_car"); // 장치 호스트 이름 설정

    // Wi-Fi STA 모드 설정 (이전에 저장된 WiFi 정보로 자동 연결 시도)
    embui.loadUIFromFS(); // LittleFS에서 UI 레이아웃 로드
    dbgP1_println_F(F("EmbUI 초기화 완료. 웹 인터페이스 접근 가능."));
}

/**
 * @brief EmbUI 웹페이지 UI를 구성하고, g_M010_Config 구조체의 멤버 변수들을 웹 UI에 바인딩합니다.
 * 사용자가 웹 인터페이스를 통해 설정을 조회하고 변경할 수 있도록 필드를 정의합니다.
 */
void W010_EmbUI_setupWebPages() {
    dbgP1_println_F(F("EmbUI 웹페이지 UI 설정 중..."));

    // 메인 페이지에 "설정" 섹션 추가
    embui.addSection("config", "시스템 설정");
    
    // g_M010_Config의 멤버 변수들을 웹 UI에 추가
    // 각 변수에 대해 적절한 UI 위젯(Input, Number, Select 등)을 사용합니다.
    // label, param, value, type, min, max, step 등의 속성 지정.
    // onchange 콜백을 통해 변경사항 저장 로직 연결.

    // mvState_accelFilter_Alpha (가속도 필터 알파값)
    embui.addControl("config", ControlType::Number, "가속도 필터 Alpha", "mvState_accelFilter_Alpha", g_M010_Config.mvState_accelFilter_Alpha, 0.0, 1.0, 0.01);

    // 자동차 움직임 상태 감지 임계값
    embui.addControl("config", ControlType::Number, "전진 시작 속도 (km/h)", "mvState_Forward_speedKmh_Threshold_Min", g_M010_Config.mvState_Forward_speedKmh_Threshold_Min, 0.0, 10.0, 0.1);
    embui.addControl("config", ControlType::Number, "후진 시작 속도 (km/h)", "mvState_Reverse_speedKmh_Threshold_Min", g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min, 0.0, 10.0, 0.1);
    embui.addControl("config", ControlType::Number, "정지 판단 최대 속도 (km/h)", "mvState_Stop_speedKmh_Threshold_Max", g_M010_Config.mvState_Stop_speedKmh_Threshold_Max, 0.0, 5.0, 0.1);
    embui.addControl("config", ControlType::Number, "정지 판단 Y Accel (m/s^2)", "mvState_Stop_accelMps2_Threshold_Max", g_M010_Config.mvState_Stop_accelMps2_Threshold_Max, 0.0, 1.0, 0.01);
    embui.addControl("config", ControlType::Number, "정지 판단 Yaw Gyro (deg/s)", "mvState_Stop_gyroDps_Threshold_Max", g_M010_Config.mvState_Stop_gyroDps_Threshold_Max, 0.0, 2.0, 0.01);

    // 상태 지속 시간 (ms)
    embui.addControl("config", ControlType::Number, "정지 안정 최소 시간 (ms)", "mvState_stop_durationMs_Stable_Min", g_M010_Config.mvState_stop_durationMs_Stable_Min, 50, 1000, 10);
    embui.addControl("config", ControlType::Number, "움직임 안정 최소 시간 (ms)", "mvState_move_durationMs_Stable_Min", g_M010_Config.mvState_move_durationMs_Stable_Min, 50, 1000, 10);
    //embui.addControl("config", ControlType::Number, "정상 움직임 복귀 시간 (ms)", "mvState_normalMove_durationMs", g_M010_Config.mvState_normalMove_durationMs); // (현재 사용되지 않음)

    // 급감속/방지턱 감지 관련 임계값
    embui.addControl("config", ControlType::Number, "급감속 Accel Y (m/s^2)", "mvState_Decel_accelMps2_Threshold", g_M010_Config.mvState_Decel_accelMps2_Threshold, -10.0, -1.0, 0.1);
    embui.addControl("config", ControlType::Number, "방지턱 Accel Z (m/s^2)", "mvState_Bump_accelMps2_Threshold", g_M010_Config.mvState_Bump_accelMps2_Threshold, 1.0, 10.0, 0.1);
    embui.addControl("config", ControlType::Number, "방지턱 최소 속도 (km/h)", "mvState_Bump_SpeedKmh_Min", g_M010_Config.mvState_Bump_SpeedKmh_Min, 0.0, 30.0, 0.1);

    // 감지 지속 및 쿨다운 시간
    embui.addControl("config", ControlType::Number, "방지턱 쿨다운 (ms)", "mvState_Bump_CooldownMs", g_M010_Config.mvState_Bump_CooldownMs, 100, 5000, 100);
    embui.addControl("config", ControlType::Number, "급감속 유지 시간 (ms)", "mvState_Decel_durationMs_Hold", g_M010_Config.mvState_Decel_durationMs_Hold, 100, 30000, 100);
    embui.addControl("config", ControlType::Number, "방지턱 유지 시간 (ms)", "mvState_Bump_durationMs_Hold", g_M010_Config.mvState_Bump_durationMs_Hold, 100, 30000, 100);

    // 정차/주차 시간 기준 (초)
    //embui.addControl("config", ControlType::Number, "정차 유예 시간 (ms)", "mvState_PeriodMs_stopGrace", g_M010_Config.mvState_PeriodMs_stopGrace); // (현재 사용되지 않음)
    embui.addControl("config", ControlType::Number, "신호대기1 (초)", "mvState_signalWait1_Seconds", g_M010_Config.mvState_signalWait1_Seconds, 5, 300, 1);
    embui.addControl("config", ControlType::Number, "신호대기2 (초)", "mvState_signalWait2_Seconds", g_M010_Config.mvState_signalWait2_Seconds, 5, 600, 1);
    embui.addControl("config", ControlType::Number, "정차1 (초)", "mvState_stopped1_Seconds", g_M010_Config.mvState_stopped1_Seconds, 30, 1800, 1);
    embui.addControl("config", ControlType::Number, "정차2 (초)", "mvState_stopped2_Seconds", g_M010_Config.mvState_stopped2_Seconds, 60, 3600, 1);
    embui.addControl("config", ControlType::Number, "주차 (초)", "mvState_park_Seconds", g_M010_Config.mvState_park_Seconds, 120, 7200, 1);

    // 시리얼 출력 간격
    embui.addControl("config", ControlType::Number, "시리얼 출력 간격 (ms)", "serialPrint_intervalMs", g_M010_Config.serialPrint_intervalMs, 100, 10000, 100);

    // 회전 감지 관련 상수 (Yaw 각속도 기반)
    embui.addControl("config", ControlType::Number, "직진 Yaw Thres (deg/s)", "turnState_Center_yawAngleVelocityDegps_Thresold", g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold, 0.1, 10.0, 0.1);
    embui.addControl("config", ControlType::Number, "LR1 Yaw Thres (deg/s)", "turnState_LR_1_yawAngleVelocityDegps_Thresold", g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold, 5.0, 30.0, 0.5);
    embui.addControl("config", ControlType::Number, "LR2 Yaw Thres (deg/s)", "turnState_LR_2_yawAngleVelocityDegps_Thresold", g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold, 10.0, 60.0, 0.5);
    embui.addControl("config", ControlType::Number, "LR3 Yaw Thres (deg/s)", "turnState_LR_3_yawAngleVelocityDegps_Thresold", g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold, 20.0, 90.0, 0.5);
    embui.addControl("config", ControlType::Number, "회전 최소 속도 (km/h)", "turnState_speedKmh_MinSpeed", g_M010_Config.turnState_speedKmh_MinSpeed, 0.0, 10.0, 0.1);
    embui.addControl("config", ControlType::Number, "고속 회전 임계 (km/h)", "turnState_speedKmh_HighSpeed_Threshold", g_M010_Config.turnState_speedKmh_HighSpeed_Threshold, 10.0, 100.0, 1.0);
    embui.addControl("config", ControlType::Number, "회전 안정 시간 (ms)", "turnState_StableDurationMs", g_M010_Config.turnState_StableDurationMs, 50, 500, 10);

    // 설정 저장, 로드, 초기화 버튼 추가
    embui.addControl("config", ControlType::Button, "설정 저장", "save_config", "저장", "btn success");
    embui.addControl("config", ControlType::Button, "설정 불러오기", "load_config", "불러오기", "btn info");
    embui.addControl("config", ControlType::Button, "설정 초기화", "reset_config", "초기화", "btn danger");
    
    // "상태" 섹션 추가 - 실시간 데이터 표시용
    embui.addSection("status", "자동차 현재 상태");
    embui.addControl("status", ControlType::Label, "움직임 상태:", "carMovementState");
    embui.addControl("status", ControlType::Label, "회전 상태:", "carTurnState");
    embui.addControl("status", ControlType::Label, "추정 속도:", "speed_kmh");
    embui.addControl("status", ControlType::Label, "AccelX (m/s^2):", "accelX_ms2");
    embui.addControl("status", ControlType::Label, "AccelY (m/s^2):", "accelY_ms2");
    embui.addControl("status", ControlType::Label, "AccelZ (m/s^2):", "accelZ_ms2");
    embui.addControl("status", ControlType::Label, "Yaw (도):", "yawAngle_deg");
    embui.addControl("status", ControlType::Label, "Pitch (도):", "pitchAngle_deg");
    embui.addControl("status", ControlType::Label, "Yaw 속도 (도/초):", "yawAngleVelocity_degps");
    embui.addControl("status", ControlType::Label, "급감속 감지:", "isEmergencyBraking");
    embui.addControl("status", ControlType::Label, "방지턱 감지:", "isSpeedBumpDetected");
    embui.addControl("status", ControlType::Label, "정차 지속 시간:", "currentStopTime_sec"); // 초 단위로 표시

    // EmbUI 콜백 함수 등록
    // 웹 UI에서 설정값이 변경될 때 호출될 함수
    embui.onChanged(W010_EmbUI_handleConfigSave);
    // 웹 UI에서 버튼 클릭 등 특정 명령이 발생할 때 호출될 함수
    embui.onCommand(W010_EmbUI_handleCommand);

    dbgP1_println_F(F("EmbUI 웹페이지 UI 설정 완료."));
}

/**
 * @brief g_M010_Config 구조체에 저장된 현재 설정값을 EmbUI 웹 UI로 로드하여 표시합니다.
 * 주로 시스템 시작 시 또는 '불러오기' 버튼 클릭 시 호출됩니다.
 */
void W010_EmbUI_loadConfigToWebUI() {
    dbgP1_println_F(F("웹 UI에 설정값 로드 중..."));
    
    // 각 설정 항목에 대해 EmbUI의 updateControl 함수를 사용하여 웹 UI 값을 갱신합니다.
    embui.updateControl("mvState_accelFilter_Alpha",                String(g_M010_Config.mvState_accelFilter_Alpha, 3));
    embui.updateControl("mvState_Forward_speedKmh_Threshold_Min",   String(g_M010_Config.mvState_Forward_speedKmh_Threshold_Min, 2));
    embui.updateControl("mvState_Reverse_speedKmh_Threshold_Min",   String(g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min, 2));
    embui.updateControl("mvState_Stop_speedKmh_Threshold_Max",      String(g_M010_Config.mvState_Stop_speedKmh_Threshold_Max, 2));
    embui.updateControl("mvState_Stop_accelMps2_Threshold_Max",     String(g_M010_Config.mvState_Stop_accelMps2_Threshold_Max, 2));
    embui.updateControl("mvState_Stop_gyroDps_Threshold_Max",       String(g_M010_Config.mvState_Stop_gyroDps_Threshold_Max, 2));
    embui.updateControl("mvState_stop_durationMs_Stable_Min",       String(g_M010_Config.mvState_stop_durationMs_Stable_Min));
    embui.updateControl("mvState_move_durationMs_Stable_Min",       String(g_M010_Config.mvState_move_durationMs_Stable_Min));
    //embui.updateControl("mvState_normalMove_durationMs",            String(g_M010_Config.mvState_normalMove_durationMs));
    embui.updateControl("mvState_Decel_accelMps2_Threshold",        String(g_M010_Config.mvState_Decel_accelMps2_Threshold, 2));
    embui.updateControl("mvState_Bump_accelMps2_Threshold",         String(g_M010_Config.mvState_Bump_accelMps2_Threshold, 2));
    embui.updateControl("mvState_Bump_SpeedKmh_Min",                String(g_M010_Config.mvState_Bump_SpeedKmh_Min, 2));
    embui.updateControl("mvState_Bump_CooldownMs",                  String(g_M010_Config.mvState_Bump_CooldownMs));
    embui.updateControl("mvState_Decel_durationMs_Hold",            String(g_M010_Config.mvState_Decel_durationMs_Hold));
    embui.updateControl("mvState_Bump_durationMs_Hold",             String(g_M010_Config.mvState_Bump_durationMs_Hold));
    //embui.updateControl("mvState_PeriodMs_stopGrace",               String(g_M010_Config.mvState_PeriodMs_stopGrace));
    embui.updateControl("mvState_signalWait1_Seconds",              String(g_M010_Config.mvState_signalWait1_Seconds));
    embui.updateControl("mvState_signalWait2_Seconds",              String(g_M010_Config.mvState_signalWait2_Seconds));
    embui.updateControl("mvState_stopped1_Seconds",                 String(g_M010_Config.mvState_stopped1_Seconds));
    embui.updateControl("mvState_stopped2_Seconds",                 String(g_M010_Config.mvState_stopped2_Seconds));
    embui.updateControl("mvState_park_Seconds",                     String(g_M010_Config.mvState_park_Seconds));
    embui.updateControl("serialPrint_intervalMs",                   String(g_M010_Config.serialPrint_intervalMs));
    embui.updateControl("turnState_Center_yawAngleVelocityDegps_Thresold", String(g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold, 2));
    embui.updateControl("turnState_LR_1_yawAngleVelocityDegps_Thresold",   String(g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold, 2));
    embui.updateControl("turnState_LR_2_yawAngleVelocityDegps_Thresold",   String(g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold, 2));
    embui.updateControl("turnState_LR_3_yawAngleVelocityDegps_Thresold",   String(g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold, 2));
    embui.updateControl("turnState_speedKmh_MinSpeed",                     String(g_M010_Config.turnState_speedKmh_MinSpeed, 2));
    embui.updateControl("turnState_speedKmh_HighSpeed_Threshold",          String(g_M010_Config.turnState_speedKmh_HighSpeed_Threshold, 2));
    embui.updateControl("turnState_StableDurationMs",                      String(g_M010_Config.turnState_StableDurationMs));

    dbgP1_println_F(F("웹 UI에 설정값 로드 완료."));
}

/**
 * @brief 웹 UI에서 설정값이 변경될 때 호출되는 콜백 함수입니다.
 * 변경된 값을 g_M010_Config 구조체에 반영합니다.
 * @param name 변경된 설정 항목의 이름 (param 속성)
 * @param value 변경된 값 (문자열)
 */
void W010_EmbUI_handleConfigSave(String name, String value) {
    dbgP1_printf_F(F("웹 UI에서 설정 변경 감지: %s = %s\n"), name.c_str(), value.c_str());

    // ArduinoJson을 사용하여 임시 JsonDocument 생성 및 값 업데이트
    JsonDocument v_config_doc;
    
    // 기존 g_M010_Config 값을 JSON 문서로 복사하여 현재 상태를 유지
    v_config_doc["mvState_accelFilter_Alpha"]                   = g_M010_Config.mvState_accelFilter_Alpha;
    v_config_doc["mvState_Forward_speedKmh_Threshold_Min"]      = g_M010_Config.mvState_Forward_speedKmh_Threshold_Min;
    v_config_doc["mvState_Reverse_speedKmh_Threshold_Min"]      = g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min;
    v_config_doc["mvState_Stop_speedKmh_Threshold_Max"]         = g_M010_Config.mvState_Stop_speedKmh_Threshold_Max;
    v_config_doc["mvState_Stop_accelMps2_Threshold_Max"]        = g_M010_Config.mvState_Stop_accelMps2_Threshold_Max;
    v_config_doc["mvState_Stop_gyroDps_Threshold_Max"]          = g_M010_Config.mvState_Stop_gyroDps_Threshold_Max;
    v_config_doc["mvState_stop_durationMs_Stable_Min"]          = g_M010_Config.mvState_stop_durationMs_Stable_Min;
    v_config_doc["mvState_move_durationMs_Stable_Min"]          = g_M010_Config.mvState_move_durationMs_Stable_Min;
    v_config_doc["mvState_normalMove_durationMs"]               = g_M010_Config.mvState_normalMove_durationMs;
    v_config_doc["mvState_Decel_accelMps2_Threshold"]           = g_M010_Config.mvState_Decel_accelMps2_Threshold;
    v_config_doc["mvState_Bump_accelMps2_Threshold"]            = g_M010_Config.mvState_Bump_accelMps2_Threshold;
    v_config_doc["mvState_Bump_SpeedKmh_Min"]                   = g_M010_Config.mvState_Bump_SpeedKmh_Min;
    v_config_doc["mvState_Bump_CooldownMs"]                     = g_M010_Config.mvState_Bump_CooldownMs;
    v_config_doc["mvState_Decel_durationMs_Hold"]               = g_M010_Config.mvState_Decel_durationMs_Hold;
    v_config_doc["mvState_Bump_durationMs_Hold"]                = g_M010_Config.mvState_Bump_durationMs_Hold;
    v_config_doc["mvState_PeriodMs_stopGrace"]                  = g_M010_Config.mvState_PeriodMs_stopGrace;
    v_config_doc["mvState_signalWait1_Seconds"]                 = g_M010_Config.mvState_signalWait1_Seconds;
    v_config_doc["mvState_signalWait2_Seconds"]                 = g_M010_Config.mvState_signalWait2_Seconds;
    v_config_doc["mvState_stopped1_Seconds"]                    = g_M010_Config.mvState_stopped1_Seconds;
    v_config_doc["mvState_stopped2_Seconds"]                    = g_M010_Config.mvState_stopped2_Seconds;
    v_config_doc["mvState_park_Seconds"]                        = g_M010_Config.mvState_park_Seconds;
    v_config_doc["serialPrint_intervalMs"]                      = g_M010_Config.serialPrint_intervalMs;
    v_config_doc["turnState_Center_yawAngleVelocityDegps_Thresold"] = g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold;
    v_config_doc["turnState_LR_1_yawAngleVelocityDegps_Thresold"]   = g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold;
    v_config_doc["turnState_LR_2_yawAngleVelocityDegps_Thresold"]   = g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold;
    v_config_doc["turnState_LR_3_yawAngleVelocityDegps_Thresold"]   = g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold;
    v_config_doc["turnState_speedKmh_MinSpeed"]                     = g_M010_Config.turnState_speedKmh_MinSpeed;
    v_config_doc["turnState_speedKmh_HighSpeed_Threshold"]          = g_M010_Config.turnState_speedKmh_HighSpeed_Threshold;
    v_config_doc["turnState_StableDurationMs"]                      = g_M010_Config.turnState_StableDurationMs;

    // 변경된 파라미터 값만 업데이트
    // 숫자형 값은 float 또는 int로 변환하여 적용
    if      (name.equals("mvState_accelFilter_Alpha"))                 v_config_doc[name] = value.toFloat();
    else if (name.equals("mvState_Forward_speedKmh_Threshold_Min"))    v_config_doc[name] = value.toFloat();
    else if (name.equals("mvState_Reverse_speedKmh_Threshold_Min"))    v_config_doc[name] = value.toFloat();
    else if (name.equals("mvState_Stop_speedKmh_Threshold_Max"))       v_config_doc[name] = value.toFloat();
    else if (name.equals("mvState_Stop_accelMps2_Threshold_Max"))      v_config_doc[name] = value.toFloat();
    else if (name.equals("mvState_Stop_gyroDps_Threshold_Max"))        v_config_doc[name] = value.toFloat();
    else if (name.equals("mvState_stop_durationMs_Stable_Min"))        v_config_doc[name] = value.toInt();
    else if (name.equals("mvState_move_durationMs_Stable_Min"))        v_config_doc[name] = value.toInt();
    else if (name.equals("mvState_normalMove_durationMs"))             v_config_doc[name] = value.toInt();
    else if (name.equals("mvState_Decel_accelMps2_Threshold"))         v_config_doc[name] = value.toFloat();
    else if (name.equals("mvState_Bump_accelMps2_Threshold"))          v_config_doc[name] = value.toFloat();
    else if (name.equals("mvState_Bump_SpeedKmh_Min"))                 v_config_doc[name] = value.toFloat();
    else if (name.equals("mvState_Bump_CooldownMs"))                   v_config_doc[name] = value.toInt();
    else if (name.equals("mvState_Decel_durationMs_Hold"))             v_config_doc[name] = value.toInt();
    else if (name.equals("mvState_Bump_durationMs_Hold"))              v_config_doc[name] = value.toInt();
    else if (name.equals("mvState_PeriodMs_stopGrace"))                v_config_doc[name] = value.toInt();
    else if (name.equals("mvState_signalWait1_Seconds"))               v_config_doc[name] = value.toInt();
    else if (name.equals("mvState_signalWait2_Seconds"))               v_config_doc[name] = value.toInt();
    else if (name.equals("mvState_stopped1_Seconds"))                  v_config_doc[name] = value.toInt();
    else if (name.equals("mvState_stopped2_Seconds"))                  v_config_doc[name] = value.toInt();
    else if (name.equals("mvState_park_Seconds"))                      v_config_doc[name] = value.toInt();
    else if (name.equals("serialPrint_intervalMs"))                    v_config_doc[name] = value.toInt();
    else if (name.equals("turnState_Center_yawAngleVelocityDegps_Thresold")) v_config_doc[name] = value.toFloat();
    else if (name.equals("turnState_LR_1_yawAngleVelocityDegps_Thresold"))   v_config_doc[name] = value.toFloat();
    else if (name.equals("turnState_LR_2_yawAngleVelocityDegps_Thresold"))   v_config_doc[name] = value.toFloat();
    else if (name.equals("turnState_LR_3_yawAngleVelocityDegps_Thresold"))   v_config_doc[name] = value.toFloat();
    else if (name.equals("turnState_speedKmh_MinSpeed"))                     v_config_doc[name] = value.toFloat();
    else if (name.equals("turnState_speedKmh_HighSpeed_Threshold"))          v_config_doc[name] = value.toFloat();
    else if (name.equals("turnState_StableDurationMs"))                      v_config_doc[name] = value.toInt();
    else {
        dbgP1_printf_F(F("알 수 없는 설정 항목: %s\n"), name.c_str());
        return;
    }
    
    // JsonDocument의 값을 다시 g_M010_Config 구조체로 복사하여 업데이트
    g_M010_Config.mvState_accelFilter_Alpha                 = v_config_doc["mvState_accelFilter_Alpha"];
    g_M010_Config.mvState_Forward_speedKmh_Threshold_Min    = v_config_doc["mvState_Forward_speedKmh_Threshold_Min"];
    g_M010_Config.mvState_Reverse_speedKmh_Threshold_Min    = v_config_doc["mvState_Reverse_speedKmh_Threshold_Min"];
    g_M010_Config.mvState_Stop_speedKmh_Threshold_Max       = v_config_doc["mvState_Stop_speedKmh_Threshold_Max"];
    g_M010_Config.mvState_Stop_accelMps2_Threshold_Max      = v_config_doc["mvState_Stop_accelMps2_Threshold_Max"];
    g_M010_Config.mvState_Stop_gyroDps_Threshold_Max        = v_config_doc["mvState_Stop_gyroDps_Threshold_Max"];
    g_M010_Config.mvState_stop_durationMs_Stable_Min        = v_config_doc["mvState_stop_durationMs_Stable_Min"];
    g_M010_Config.mvState_move_durationMs_Stable_Min        = v_config_doc["mvState_move_durationMs_Stable_Min"];
    g_M010_Config.mvState_normalMove_durationMs             = v_config_doc["mvState_normalMove_durationMs"];
    g_M010_Config.mvState_Decel_accelMps2_Threshold         = v_config_doc["mvState_Decel_accelMps2_Threshold"];
    g_M010_Config.mvState_Bump_accelMps2_Threshold          = v_config_doc["mvState_Bump_accelMps2_Threshold"];
    g_M010_Config.mvState_Bump_SpeedKmh_Min                 = v_config_doc["mvState_Bump_SpeedKmh_Min"];
    g_M010_Config.mvState_Bump_CooldownMs                   = v_config_doc["mvState_Bump_CooldownMs"];
    g_M010_Config.mvState_Decel_durationMs_Hold             = v_config_doc["mvState_Decel_durationMs_Hold"];
    g_M010_Config.mvState_Bump_durationMs_Hold              = v_config_doc["mvState_Bump_durationMs_Hold"];
    g_M010_Config.mvState_PeriodMs_stopGrace                = v_config_doc["mvState_PeriodMs_stopGrace"];
    g_M010_Config.mvState_signalWait1_Seconds               = v_config_doc["mvState_signalWait1_Seconds"];
    g_M010_Config.mvState_signalWait2_Seconds               = v_config_doc["mvState_signalWait2_Seconds"];
    g_M010_Config.mvState_stopped1_Seconds                  = v_config_doc["mvState_stopped1_Seconds"];
    g_M010_Config.mvState_stopped2_Seconds                  = v_config_doc["mvState_stopped2_Seconds"];
    g_M010_Config.mvState_park_Seconds                      = v_config_doc["mvState_park_Seconds"];
    g_M010_Config.serialPrint_intervalMs                    = v_config_doc["serialPrint_intervalMs"];
    g_M010_Config.turnState_Center_yawAngleVelocityDegps_Thresold   = v_config_doc["turnState_Center_yawAngleVelocityDegps_Thresold"];
    g_M010_Config.turnState_LR_1_yawAngleVelocityDegps_Thresold     = v_config_doc["turnState_LR_1_yawAngleVelocityDegps_Thresold"];
    g_M010_Config.turnState_LR_2_yawAngleVelocityDegps_Thresold     = v_config_doc["turnState_LR_2_yawAngleVelocityDegps_Thresold"];
    g_M010_Config.turnState_LR_3_yawAngleVelocityDegps_Thresold     = v_config_doc["turnState_LR_3_yawAngleVelocityDegps_Thresold"];
    g_M010_Config.turnState_speedKmh_MinSpeed                       = v_config_doc["turnState_speedKmh_MinSpeed"];
    g_M010_Config.turnState_speedKmh_HighSpeed_Threshold            = v_config_doc["turnState_speedKmh_HighSpeed_Threshold"];
    g_M010_Config.turnState_StableDurationMs                        = v_config_doc["turnState_StableDurationMs"];

    // 변경된 설정값으로 인해 시리얼 출력 주기 등 EmbUI 자체의 설정도 변경될 수 있으므로 적용
    embui.setPrintInterval(g_M010_Config.serialPrint_intervalMs);
}

/**
 * @brief 웹 UI에서 'save_config', 'load_config', 'reset_config'와 같은 명령 버튼이 클릭될 때 호출됩니다.
 * 해당 명령에 따라 M010_main3_010.h에 정의된 설정 관리 함수들을 호출합니다.
 * @param cmd 수신된 명령어 문자열
 */
void W010_EmbUI_handleCommand(String cmd) {
    dbgP1_printf_F(F("웹 UI 명령 수신: %s\n"), cmd.c_str());

    if (cmd.equals("save_config")) {
        if (M010_Config_save()) {
            dbgP1_println_F(F("웹 UI: 설정값이 LittleFS에 저장되었습니다."));
            embui.displayMessage("설정 저장 완료", "설정값이 성공적으로 저장되었습니다.", 2000);
        } else {
            dbgP1_println_F(F("웹 UI: 설정값 저장 실패!"));
            embui.displayMessage("설정 저장 실패", "파일 시스템에 설정값을 저장할 수 없습니다.", 3000, true);
        }
    } else if (cmd.equals("load_config")) {
        if (M010_Config_load()) {
            dbgP1_println_F(F("웹 UI: 설정값이 LittleFS에서 로드되었습니다."));
            W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 로드된 값 반영
            embui.displayMessage("설정 불러오기 완료", "설정값이 성공적으로 불러와졌습니다.", 2000);
        } else {
            dbgP1_println_F(F("웹 UI: 설정값 로드 실패! 기본값이 사용됩니다."));
            M010_Config_initDefaults(); // 로드 실패 시 기본값으로 초기화
            W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 기본값 반영
            embui.displayMessage("설정 불러오기 실패", "설정 파일을 찾을 수 없습니다. 기본값이 적용됩니다.", 3000, true);
        }
    } else if (cmd.equals("reset_config")) {
        M010_Config_initDefaults(); // 설정값을 기본값으로 초기화
        if (M010_Config_save()) { // 초기화된 기본값을 파일에 저장
            dbgP1_println_F(F("웹 UI: 설정값이 기본값으로 초기화되고 저장되었습니다."));
            W010_EmbUI_loadConfigToWebUI(); // 웹 UI에도 기본값 반영
            embui.displayMessage("설정 초기화 완료", "설정값이 기본값으로 초기화되었습니다.", 2000);
        } else {
            dbgP1_println_F(F("웹 UI: 설정 초기화 후 저장 실패!"));
            embui.displayMessage("설정 초기화 실패", "기본값 저장 중 오류가 발생했습니다.", 3000, true);
        }
    } else {
        dbgP1_printf_F(F("웹 UI: 알 수 없는 명령: %s\n"), cmd.c_str());
    }
}

/**
 * @brief g_M010_CarStatus 구조체의 현재 자동차 상태 정보를 EmbUI 웹페이지에 주기적으로 업데이트합니다.
 * "status" 섹션에 정의된 Label 컨트롤들의 값을 실시간으로 갱신합니다.
 */
void W010_EmbUI_updateCarStatusWeb() {
    // 각 상태값에 따라 문자열 변환
    String movementStateStr;
    switch (g_M010_CarStatus.carMovementState) {
        case E_M010_CARMOVESTATE_UNKNOWN:      movementStateStr = "알 수 없음"; break;
        case E_M010_CARMOVESTATE_STOPPED_INIT: movementStateStr = "정차 중 (기본)"; break;
        case E_M010_CARMOVESTATE_SIGNAL_WAIT1: movementStateStr = "신호대기 1 (" + String(g_M010_Config.mvState_signalWait1_Seconds) + "s 미만)"; break;
        case E_M010_CARMOVESTATE_SIGNAL_WAIT2: movementStateStr = "신호대기 2 (" + String(g_M010_Config.mvState_signalWait2_Seconds) + "s 미만)"; break;
        case E_M010_CARMOVESTATE_STOPPED1:     movementStateStr = "정차 1 (" + String(g_M010_Config.mvState_stopped1_Seconds / 60) + "분 미만)"; break;
        case E_M010_CARMOVESTATE_STOPPED2:     movementStateStr = "정차 2 (" + String(g_M010_Config.mvState_stopped2_Seconds / 60) + "분 미만)"; break;
        case E_M010_CARMOVESTATE_PARKED:       movementStateStr = "주차 중 (>= " + String(g_M010_Config.mvState_park_Seconds / 60) + "분)"; break;
        case E_M010_CARMOVESTATE_FORWARD:      movementStateStr = "전진 중"; break;
        case E_M010_CARMOVESTATE_REVERSE:      movementStateStr = "후진 중"; break;
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

    // EmbUI의 updateControl 함수를 사용하여 "status" 섹션의 레이블 업데이트
    // 숫자 값은 String()으로 변환 시 소수점 자리를 지정하여 정확하게 표시
    embui.updateControl("carMovementState",     movementStateStr);
    embui.updateControl("carTurnState",         turnStateStr);
    embui.updateControl("speed_kmh",            String(g_M010_CarStatus.speed_kmh, 2) + " km/h");
    embui.updateControl("accelX_ms2",           String(g_M010_CarStatus.accelX_ms2, 2) + " m/s^2");
    embui.updateControl("accelY_ms2",           String(g_M010_CarStatus.accelY_ms2, 2) + " m/s^2");
    embui.updateControl("accelZ_ms2",           String(g_M010_CarStatus.accelZ_ms2, 2) + " m/s^2");
    embui.updateControl("yawAngle_deg",         String(g_M010_CarStatus.yawAngle_deg, 2) + " 도");
    embui.updateControl("pitchAngle_deg",       String(g_M010_CarStatus.pitchAngle_deg, 2) + " 도");
    embui.updateControl("yawAngleVelocity_degps", String(g_M010_CarStatus.yawAngleVelocity_degps, 2) + " 도/초");
    embui.updateControl("isEmergencyBraking",   g_M010_CarStatus.isEmergencyBraking ? "감지됨" : "아님");
    embui.updateControl("isSpeedBumpDetected",  g_M010_CarStatus.isSpeedBumpDetected ? "감지됨" : "아님");
    embui.updateControl("currentStopTime_sec",  String(g_M010_CarStatus.currentStopTime_ms / 1000) + " 초");
}

/**
 * @brief EmbUI의 메인 루프를 실행하고, 주기적으로 자동차 상태를 웹에 업데이트합니다.
 * Arduino의 `loop()` 함수에서 이 함수를 호출해야 합니다.
 */
void W010_EmbUI_run() {
    embui.loop(); // EmbUI의 내부 루프 실행 (Wi-Fi, 웹 서버, 이벤트 처리)

    // 설정된 주기(serialPrint_intervalMs를 활용)에 따라 웹 UI에 자동차 상태 업데이트
    // EmbUI는 자체적으로 printInterval을 가지고 있으므로, 이를 활용하여 중복 타이머 방지.
    // 여기서는 M010_main3_010.h 의 serialPrint_intervalMs 를 웹 UI 업데이트에도 사용
    // embui.loop() 내부적으로 printInterval을 사용하므로, 별도 타이머 필요 없이
    // embui.onUpdate([]{ W010_EmbUI_updateCarStatusWeb(); }); 와 같이 등록하여 활용하는 것이 더 좋습니다.
    // 하지만 현재 구조상 수동 호출이 필요하다면 아래와 같이 millis() 기반으로 호출합니다.

    static u_int32_t v_lastWebUpdateTime_ms = 0;
    if (millis() - v_lastWebUpdateTime_ms >= g_M010_Config.serialPrint_intervalMs) {
        W010_EmbUI_updateCarStatusWeb();
        v_lastWebUpdateTime_ms = millis();
    }
}
