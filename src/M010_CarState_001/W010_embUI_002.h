#ifndef W010_embUI_002_h
#define W010_embUI_002_h

#include <EmbUI.h> // EmbUI 라이브러리 include
#include "M010_main3_010.h" // 프로젝트의 다른 헤더 파일 (VAR_ 및 CMD_ 정의 포함 가정)

// =======================================================
// 중요: 아래 VAR_... 및 CMD_... 정의는 M010_main3_010.h
// 또는 적절한 공통 헤더 파일에 있어야 합니다.
// 컴파일 오류를 위해 여기에 임시로 추가합니다.
// 실제 사용 시에는 중복 정의를 피하기 위해 적절한 곳으로 이동시키세요.
// =======================================================
#ifndef VAR_ACCEL_FILTER_ALPHA
    #define VAR_ACCEL_FILTER_ALPHA "accelAlpha"
    #define VAR_FORWARD_SPEED_MIN "fwdMinSpeed"
    #define VAR_STOP_SPEED_MAX "stopMaxSpeed"
    #define VAR_STOP_TIME_MS "stopTimeMs"
    #define VAR_TURN_YAW_THRESHOLD "turnYawThresh"
    #define VAR_TURN_MIN_SPEED "turnMinSpeed"
    #define VAR_TURN_COOLDOWN "turnCooldown"
    #define VAR_BUMP_SPEED_MIN "bumpMinSpeed"
    #define VAR_BUMP_ACCEL_THRESHOLD "bumpAccelThresh"
    #define VAR_BUMP_COOLDOWN "bumpCooldown"

    #define VAR_CAR_MOVEMENT_STATE "carMovement"
    #define VAR_CAR_TURN_STATE "carTurn"
    #define VAR_SPEED_KMH "speedKmh"
    #define VAR_ACCEL_X "accelX"
    #define VAR_ACCEL_Y "accelY"
    #define VAR_ACCEL_Z "accelZ"
    #define VAR_YAW_ANGLE "yawAngle"
    #define VAR_PITCH_ANGLE "pitchAngle"
    #define VAR_ROLL_ANGLE "rollAngle"
    #define VAR_CURRENT_STOP_TIME "stopTime"

    #define CMD_SAVE_CONFIG "cmdSave"
    #define CMD_LOAD_CONFIG "cmdLoad"
    #define CMD_RESET_CONFIG "cmdReset"
#endif
// =======================================================

// EmbUI의 기본 페이지 및 메뉴 구성
void W010_EmbUI_build_pages() {
    LOG("I", "Building EmbUI pages");

    // "설정" 페이지 구성 (add_page 대신 builder 사용 시도)
    // EmbUI 라이브러리 버전이 add_page를 지원하지 않는 경우,
    // UI 요소들을 직접 builder 콜백 내에서 구성해야 합니다.
    // section() 함수가 없는 경우, 단순히 그룹화된 UI 요소들을 배치합니다.
    embui.builder()._H("div", "config_page", "id='config_page' class='page' data-title='설정'", [](){
        // "일반 설정" 섹션
        embui.builder()._H("fieldset", "", "class='fieldset'", [](){
            embui.builder()._H("legend", "", "", [](){ embui.builder()._text("일반 설정"); });
            embui.builder().number(FPSTR(VAR_ACCEL_FILTER_ALPHA), g_M010_Config.mvState_accelFilter_Alpha, "가속도 필터 알파 (0~1)");
            embui.builder().number(FPSTR(VAR_FORWARD_SPEED_MIN), g_M010_Config.mvState_Forward_speedKmh_Threshold_Min, "전진 최소 속도 (km/h)");
            embui.builder().number(FPSTR(VAR_STOP_SPEED_MAX), g_M010_Config.mvState_Stop_speedKmh_Threshold_Max, "정지 최대 속도 (km/h)");
            embui.builder().number(FPSTR(VAR_STOP_TIME_MS), g_M010_Config.mvState_Stop_TimeMs_Threshold, "정지 시간 임계값 (ms)");
        });

        // "좌/우회전 설정" 섹션
        embui.builder()._H("fieldset", "", "class='fieldset'", [](){
            embui.builder()._H("legend", "", "", [](){ embui.builder()._text("좌/우회전 설정"); });
            embui.builder().number(FPSTR(VAR_TURN_YAW_THRESHOLD), g_M010_Config.mvState_Turn_YawAngle_Threshold, "회전 요 각도 임계값 (도)");
            embui.builder().number(FPSTR(VAR_TURN_MIN_SPEED), g_M010_Config.mvState_Turn_speedKmh_Min, "회전 최소 속도 (km/h)");
            embui.builder().number(FPSTR(VAR_TURN_COOLDOWN), g_M010_Config.mvState_Turn_CooldownMs, "회전 쿨다운 (ms)");
        });

        // "방지턱 설정" 섹션
        embui.builder()._H("fieldset", "", "class='fieldset'", [](){
            embui.builder()._H("legend", "", "", [](){ embui.builder()._text("방지턱 설정"); });
            embui.builder().number(FPSTR(VAR_BUMP_SPEED_MIN), g_M010_Config.mvState_Bump_SpeedKmh_Min, "방지턱 최소 속도 (km/h)");
            embui.builder().number(FPSTR(VAR_BUMP_ACCEL_THRESHOLD), g_M010_Config.mvState_Bump_AccelMs2_Threshold, "방지턱 가속도 임계값 (m/s^2)");
            embui.builder().number(FPSTR(VAR_BUMP_COOLDOWN), g_M010_Config.mvState_Bump_CooldownMs, "방지턱 쿨다운 (ms)");
        });

        // "설정 제어" 섹션
        embui.builder()._H("fieldset", "", "class='fieldset'", [](){
            embui.builder()._H("legend", "", "", [](){ embui.builder()._text("설정 제어"); });
            // button_t::submit은 그대로 사용하고, 버튼 색상은 style 인자로 대체
            embui.builder().button(button_t::submit, FPSTR(CMD_SAVE_CONFIG), "설정 저장", "btn-success"); // 초록색 버튼
            embui.builder().button(button_t::submit, FPSTR(CMD_LOAD_CONFIG), "설정 불러오기", "btn-info");    // 파란색 버튼
            embui.builder().button(button_t::submit, FPSTR(CMD_RESET_CONFIG), "설정 초기화", "btn-danger"); // 빨간색 버튼
        });
    });

    // "상태" 페이지 구성
    embui.builder()._H("div", "status_page", "id='status_page' class='page' data-title='상태'", [](){
        embui.builder()._H("fieldset", "", "class='fieldset'", [](){
            embui.builder()._H("legend", "", "", [](){ embui.builder()._text("차량 상태 정보"); });
            // text 함수는 ID, value, label을 받음. value는 비워두고 label에 설명을 넣습니다.
            embui.builder().text(FPSTR(VAR_CAR_MOVEMENT_STATE), "", "움직임 상태:");
            embui.builder().text(FPSTR(VAR_CAR_TURN_STATE), "", "회전 상태:");
            embui.builder().text(FPSTR(VAR_SPEED_KMH), "", "속도 (km/h):");
            embui.builder().text(FPSTR(VAR_ACCEL_X), "", "가속도 X (m/s^2):");
            embui.builder().text(FPSTR(VAR_ACCEL_Y), "", "가속도 Y (m/s^2):");
            embui.builder().text(FPSTR(VAR_ACCEL_Z), "", "가속도 Z (m/s^2):");
            embui.builder().text(FPSTR(VAR_YAW_ANGLE), "", "요 각도 (도):");
            embui.builder().text(FPSTR(VAR_PITCH_ANGLE), "", "피치 각도 (도):");
            embui.builder().text(FPSTR(VAR_ROLL_ANGLE), "", "롤 각도 (도):");
            embui.builder().text(FPSTR(VAR_CURRENT_STOP_TIME), "", "정차 지속 시간:");
        });
    });
}

// 웹 UI로부터 설정 데이터 수신 및 처리
void W010_EmbUI_action_config_data(Interface *interf, JsonObjectConst data, const char* action) {
    LOG("I", "Received config data action: %s", action);

    // 각 설정값을 업데이트
    if (data.containsKey(FPSTR(VAR_ACCEL_FILTER_ALPHA))) {
        g_M010_Config.mvState_accelFilter_Alpha = data[FPSTR(VAR_ACCEL_FILTER_ALPHA)].as<float>();
    }
    if (data.containsKey(FPSTR(VAR_FORWARD_SPEED_MIN))) {
        g_M010_Config.mvState_Forward_speedKmh_Threshold_Min = data[FPSTR(VAR_FORWARD_SPEED_MIN)].as<float>();
    }
    if (data.containsKey(FPSTR(VAR_STOP_SPEED_MAX))) {
        g_M010_Config.mvState_Stop_speedKmh_Threshold_Max = data[FPSTR(VAR_STOP_SPEED_MAX)].as<float>();
    }
    if (data.containsKey(FPSTR(VAR_STOP_TIME_MS))) {
        g_M010_Config.mvState_Stop_TimeMs_Threshold = data[FPSTR(VAR_STOP_TIME_MS)].as<uint32_t>();
    }
    if (data.containsKey(FPSTR(VAR_TURN_YAW_THRESHOLD))) {
        g_M010_Config.mvState_Turn_YawAngle_Threshold = data[FPSTR(VAR_TURN_YAW_THRESHOLD)].as<float>();
    }
    if (data.containsKey(FPSTR(VAR_TURN_MIN_SPEED))) {
        g_M010_Config.mvState_Turn_speedKmh_Min = data[FPSTR(VAR_TURN_MIN_SPEED)].as<float>();
    }
    if (data.containsKey(FPSTR(VAR_TURN_COOLDOWN))) {
        g_M010_Config.mvState_Turn_CooldownMs = data[FPSTR(VAR_TURN_COOLDOWN)].as<uint32_t>();
    }
    if (data.containsKey(FPSTR(VAR_BUMP_SPEED_MIN))) {
        g_M010_Config.mvState_Bump_SpeedKmh_Min = data[FPSTR(VAR_BUMP_SPEED_MIN)].as<float>();
    }
    if (data.containsKey(FPSTR(VAR_BUMP_ACCEL_THRESHOLD))) {
        g_M010_Config.mvState_Bump_AccelMs2_Threshold = data[FPSTR(VAR_BUMP_ACCEL_THRESHOLD)].as<float>();
    }
    if (data.containsKey(FPSTR(VAR_BUMP_COOLDOWN))) {
        g_M010_Config.mvState_Bump_CooldownMs = data[FPSTR(VAR_BUMP_COOLDOWN)].as<uint32_t>();
    }

    // 설정 저장, 불러오기, 초기화 액션 처리
    if (data.containsKey(FPSTR(CMD_SAVE_CONFIG))) {
        if (M010_Config_save()) {
            embui.sendNotification("설정 저장 완료", "설정값이 성공적으로 저장되었습니다.", 2000);
        } else {
            embui.sendNotification("설정 저장 실패", "파일 시스템에 설정값을 저장할 수 없습니다.", 3000, true);
        }
    } else if (data.containsKey(FPSTR(CMD_LOAD_CONFIG))) {
        if (M010_Config_load()) {
            embui.sendNotification("설정 불러오기 완료", "설정값이 성공적으로 불러와졌습니다.", 2000);
            W010_EmbUI_loadConfigToWebUI(); // 불러온 설정값을 웹 UI에 반영
        } else {
            embui.sendNotification("설정 불러오기 실패", "설정 파일을 찾을 수 없습니다. 기본값이 적용됩니다.", 3000, true);
        }
    } else if (data.containsKey(FPSTR(CMD_RESET_CONFIG))) {
        M010_Config_resetToDefaults();
        embui.sendNotification("설정 초기화 완료", "설정값이 기본값으로 초기화되었습니다.", 2000);
        W010_EmbUI_loadConfigToWebUI(); // 초기화된 설정값을 웹 UI에 반영
    }
}

// 현재 설정값을 웹 UI에 로드 (페이지 로드 시 또는 설정 불러오기 후)
void W010_EmbUI_loadConfigToWebUI() {
    embui.var_set(FPSTR(VAR_ACCEL_FILTER_ALPHA), String(g_M010_Config.mvState_accelFilter_Alpha, 2));
    embui.var_set(FPSTR(VAR_FORWARD_SPEED_MIN), String(g_M010_Config.mvState_Forward_speedKmh_Threshold_Min, 2));
    embui.var_set(FPSTR(VAR_STOP_SPEED_MAX), String(g_M010_Config.mvState_Stop_speedKmh_Threshold_Max, 2));
    embui.var_set(FPSTR(VAR_STOP_TIME_MS), String(g_M010_Config.mvState_Stop_TimeMs_Threshold));
    embui.var_set(FPSTR(VAR_TURN_YAW_THRESHOLD), String(g_M010_Config.mvState_Turn_YawAngle_Threshold, 2));
    embui.var_set(FPSTR(VAR_TURN_MIN_SPEED), String(g_M010_Config.mvState_Turn_speedKmh_Min, 2));
    embui.var_set(FPSTR(VAR_TURN_COOLDOWN), String(g_M010_Config.mvState_Turn_CooldownMs));
    embui.var_set(FPSTR(VAR_BUMP_SPEED_MIN), String(g_M010_Config.mvState_Bump_SpeedKmh_Min, 2));
    embui.var_set(FPSTR(VAR_BUMP_ACCEL_THRESHOLD), String(g_M010_Config.mvState_Bump_AccelMs2_Threshold, 2));
    embui.var_set(FPSTR(VAR_BUMP_COOLDOWN), String(g_M010_Config.mvState_Bump_CooldownMs));

    embui.sendUpdates(); // 현재 활성화된 UI 패널을 업데이트하도록 요청
}

// 차량 상태 정보를 웹 UI에 업데이트
void W010_EmbUI_updateCarStatusWeb() {
    String movementStateStr;
    switch (g_M010_CarStatus.movementState) {
        case CAR_STOPPED: movementStateStr = "정지"; break;
        case CAR_MOVING_FORWARD: movementStateStr = "전진"; break;
        case CAR_MOVING_BACKWARD: movementStateStr = "후진"; break;
        case CAR_BUMP_DETECTED: movementStateStr = "방지턱"; break;
        default: movementStateStr = "알 수 없음"; break;
    }

    String turnStateStr;
    switch (g_M010_CarStatus.turnState) {
        case CAR_STRAIGHT: turnStateStr = "직진"; break;
        case CAR_TURNING_LEFT: turnStateStr = "좌회전"; break;
        case CAR_TURNING_RIGHT: turnStateStr = "우회전"; break;
        default: turnStateStr = "알 수 없음"; break;
    }

    embui.var_set(FPSTR(VAR_CAR_MOVEMENT_STATE), movementStateStr);
    embui.var_set(FPSTR(VAR_CAR_TURN_STATE), turnStateStr);
    embui.var_set(FPSTR(VAR_SPEED_KMH), String(g_M010_CarStatus.speed_kmh, 2) + " km/h");
    embui.var_set(FPSTR(VAR_ACCEL_X), String(g_M010_CarStatus.accelX_ms2, 2) + " m/s^2");
    embui.var_set(FPSTR(VAR_ACCEL_Y), String(g_M010_CarStatus.accelY_ms2, 2) + " m/s^2");
    embui.var_set(FPSTR(VAR_ACCEL_Z), String(g_M010_CarStatus.accelZ_ms2, 2) + " m/s^2");
    embui.var_set(FPSTR(VAR_YAW_ANGLE), String(g_M010_CarStatus.yawAngle_deg, 2) + " 도");
    embui.var_set(FPSTR(VAR_PITCH_ANGLE), String(g_M010_CarStatus.pitchAngle_deg, 2) + " 도");
    embui.var_set(FPSTR(VAR_ROLL_ANGLE), String(g_M010_CarStatus.rollAngle_deg, 2) + " 도");
    embui.var_set(FPSTR(VAR_CURRENT_STOP_TIME), String(g_M010_CarStatus.stopDurationMs) + " ms");

    embui.sendUpdates(); // 모든 변경사항을 웹 UI에 한 번에 전송
}

#endif // W010_embUI_002_h