#ifndef W010_AWEB_MAIN_001_H
#define W010_AWEB_MAIN_001_H

// 유니코드 공백문자 및 BOM 제거를 위한 인코딩 설정
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull-compare"
#pragma GCC diagnostic ignored "-Wreorder"

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "M010_main3_010.h" // M010_main3_010.h 파일 include

// --- 전역 변수 및 객체 선언 ---
// 웹 서버 객체. 기본 HTTP 포트 80 사용.
AsyncWebServer W010_webServer(80);

// --- HTML 템플릿 정의 ---
// 간단한 웹 페이지 HTML. 설정값과 차량 상태를 표시합니다.
const char W010_INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>W010 차량 제어 및 설정</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }
        .container { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); max-width: 600px; margin: auto; }
        h2 { color: #333; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input[type="number"], input[type="text"] { width: calc(100% - 22px); padding: 10px; margin-bottom: 10px; border: 1px solid #ddd; border-radius: 4px; }
        button { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; margin-right: 5px; }
        button.action { background-color: #008CBA; }
        button:hover { opacity: 0.9; }
        .status { margin-top: 20px; padding: 10px; background-color: #e7e7e7; border-radius: 4px; }
        .status p { margin: 5px 0; }
    </style>
</head>
<body>
    <div class="container">
        <h2>W010 차량 제어 및 설정</h2>

        <h3>설정값 변경</h3>
        <form action="/update_settings" method="post">
            <label for="speed">최대 속도 (0-255):</label>
            <input type="number" id="speed" name="speed" min="0" max="255" value="%W010_MAX_SPEED%">

            <label for="turn_factor">회전 계수 (0-100):</label>
            <input type="number" id="turn_factor" name="turn_factor" min="0" max="100" value="%W010_TURN_FACTOR%">

            <button type="submit">설정 적용</button>
        </form>

        <h3>차량 이동/회전 제어</h3>
        <div>
            <button class="action" onclick="fetch('/move?direction=forward')">전진</button>
            <button class="action" onclick="fetch('/move?direction=backward')">후진</button>
            <button class="action" onclick="fetch('/turn?direction=left')">좌회전</button>
            <button class="action" onclick="fetch('/turn?direction=right')">우회전</button>
            <button class="action" onclick="fetch('/stop')">정지</button>
        </div>

        <h3>차량 현재 상태</h3>
        <div class="status">
            <p>이동 상태: <span id="moveStatus">%W010_MOVE_STATUS%</span></p>
            <p>회전 상태: <span id="turnStatus">%W010_TURN_STATUS%</span></p>
            <p>현재 최대 속도: <span id="currentSpeed">%W010_CURRENT_MAX_SPEED%</span></p>
            <p>현재 회전 계수: <span id="currentTurnFactor">%W010_CURRENT_TURN_FACTOR%</span></p>
        </div>
    </div>

    <script>
        // 주기적으로 상태를 업데이트하는 함수
        setInterval(function() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('moveStatus').innerText = data.moveStatus;
                    document.getElementById('turnStatus').innerText = data.turnStatus;
                    document.getElementById('currentSpeed').innerText = data.currentSpeed;
                    document.getElementById('currentTurnFactor').innerText = data.currentTurnFactor;
                })
                .catch(error => console.error('Error fetching status:', error));
        }, 1000); // 1초마다 업데이트
    </script>
</body>
</html>
)rawliteral";


// --- 함수 프로토타입 선언 ---
void W010_initWebServer();
void W010_handleRoot(AsyncWebServerRequest *request);
void W010_handleUpdateSettings(AsyncWebServerRequest *request);
void W010_handleMove(AsyncWebServerRequest *request);
void W010_handleTurn(AsyncWebServerRequest *request);
void W010_handleStop(AsyncWebServerRequest *request);
void W010_handleStatus(AsyncWebServerRequest *request);
String W010_processor(const String& var);

// --- 함수 구현 ---

/**
 * @brief 웹 서버를 초기화하고 라우팅 핸들러를 설정합니다.
 */
void W010_initWebServer() {
    // 루트 경로 (/) 에 대한 GET 요청 처리
    W010_webServer.on("/", HTTP_GET, W010_handleRoot);

    // 설정값 업데이트 (POST 요청) 처리
    W010_webServer.on("/update_settings", HTTP_POST, W010_handleUpdateSettings);

    // 차량 이동 명령 (GET 요청) 처리
    W010_webServer.on("/move", HTTP_GET, W010_handleMove);

    // 차량 회전 명령 (GET 요청) 처리
    W010_webServer.on("/turn", HTTP_GET, W010_handleTurn);

    // 차량 정지 명령 (GET 요청) 처리
    W010_webServer.on("/stop", HTTP_GET, W010_handleStop);

    // 차량 상태 요청 (GET 요청) 처리 (JSON 응답)
    W010_webServer.on("/status", HTTP_GET, W010_handleStatus);

    // 404 Not Found 처리
    W010_webServer.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not Found");
    });

    // 웹 서버 시작
    W010_webServer.begin();
    Serial.println("W010: AsyncWebServer 시작 완료.");
}

/**
 * @brief 루트 경로 (/) 요청을 처리하고 HTML 페이지를 전송합니다.
 * 플레이스홀더를 실제 값으로 대체합니다.
 * @param request 웹 요청 객체
 */
void W010_handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", W010_INDEX_HTML, W010_processor);
}

/**
 * @brief HTML 템플릿의 플레이스홀더를 실제 값으로 대체하는 프로세서 함수입니다.
 * @param var 플레이스홀더 변수 이름
 * @return 대체될 문자열
 */
String W010_processor(const String& var) {
    if (var == "W010_MAX_SPEED") {
        return String(M010_g_vehicleSettings.maxSpeed);
    }
    if (var == "W010_TURN_FACTOR") {
        return String(M010_g_vehicleSettings.turnFactor);
    }
    if (var == "W010_MOVE_STATUS") {
        return M010_getMovementStatusString(M010_g_vehicleState.movementStatus);
    }
    if (var == "W010_TURN_STATUS") {
        return M010_getRotationStatusString(M010_g_vehicleState.rotationStatus);
    }
    if (var == "W010_CURRENT_MAX_SPEED") {
        return String(M010_g_vehicleSettings.maxSpeed);
    }
    if (var == "W010_CURRENT_TURN_FACTOR") {
        return String(M010_g_vehicleSettings.turnFactor);
    }
    return String(); // 일치하는 플레이스홀더가 없으면 빈 문자열 반환
}

/**
 * @brief 설정값 변경 요청을 처리합니다.
 * 'speed'와 'turn_factor' 값을 받아 M010_g_vehicleSettings에 업데이트합니다.
 * @param request 웹 요청 객체
 */
void W010_handleUpdateSettings(AsyncWebServerRequest *request) {
    if (request->hasParam("speed", true) && request->hasParam("turn_factor", true)) {
        int newSpeed = request->getParam("speed", true)->value().toInt();
        int newTurnFactor = request->getParam("turn_factor", true)->value().toInt();

        // 유효성 검사 (M010_main3_010.h의 설정 범위 고려)
        if (newSpeed >= 0 && newSpeed <= 255) {
            M010_g_vehicleSettings.maxSpeed = newSpeed;
            Serial.printf("W010: 최대 속도 변경: %d\n", M010_g_vehicleSettings.maxSpeed);
        } else {
            Serial.printf("W010: 잘못된 최대 속도 값: %d\n", newSpeed);
        }

        if (newTurnFactor >= 0 && newTurnFactor <= 100) {
            M010_g_vehicleSettings.turnFactor = newTurnFactor;
            Serial.printf("W010: 회전 계수 변경: %d\n", M010_g_vehicleSettings.turnFactor);
        } else {
            Serial.printf("W010: 잘못된 회전 계수 값: %d\n", newTurnFactor);
        }

        // 설정 변경 후, 메인 페이지로 리디렉션
        request->redirect("/");
    } else {
        request->send(400, "text/plain", "Bad Request: Missing parameters.");
    }
}

/**
 * @brief 차량 이동 명령을 처리합니다.
 * 'direction' 파라미터에 따라 차량 이동 상태를 업데이트합니다.
 * @param request 웹 요청 객체
 */
void W010_handleMove(AsyncWebServerRequest *request) {
    if (request->hasParam("direction")) {
        String direction = request->getParam("direction")->value();
        if (direction == "forward") {
            M010_g_vehicleState.movementStatus = M010_MOVE_FORWARD;
            Serial.println("W010: 차량 이동: 전진");
        } else if (direction == "backward") {
            M010_g_vehicleState.movementStatus = M010_MOVE_BACKWARD;
            Serial.println("W010: 차량 이동: 후진");
        } else {
            M010_g_vehicleState.movementStatus = M010_MOVE_STOPPED; // 알 수 없는 명령은 정지로 처리
            Serial.printf("W010: 알 수 없는 이동 명령: %s\n", direction.c_str());
        }
    } else {
        M010_g_vehicleState.movementStatus = M010_MOVE_STOPPED; // 파라미터 없으면 정지
    }
    request->send(200, "text/plain", "OK"); // 응답 전송
}

/**
 * @brief 차량 회전 명령을 처리합니다.
 * 'direction' 파라미터에 따라 차량 회전 상태를 업데이트합니다.
 * @param request 웹 요청 객체
 */
void W010_handleTurn(AsyncWebServerRequest *request) {
    if (request->hasParam("direction")) {
        String direction = request->getParam("direction")->value();
        if (direction == "left") {
            M010_g_vehicleState.rotationStatus = M010_ROTATE_LEFT;
            Serial.println("W010: 차량 회전: 좌회전");
        } else if (direction == "right") {
            M010_g_vehicleState.rotationStatus = M010_ROTATE_RIGHT;
            Serial.println("W010: 차량 회전: 우회전");
        } else {
            M010_g_vehicleState.rotationStatus = M010_ROTATE_NONE; // 알 수 없는 명령은 회전 안 함으로 처리
            Serial.printf("W010: 알 수 없는 회전 명령: %s\n", direction.c_str());
        }
    } else {
        M010_g_vehicleState.rotationStatus = M010_ROTATE_NONE; // 파라미터 없으면 회전 안 함
    }
    request->send(200, "text/plain", "OK"); // 응답 전송
}

/**
 * @brief 차량 정지 명령을 처리합니다.
 * 이동 및 회전 상태를 정지로 업데이트합니다.
 * @param request 웹 요청 객체
 */
void W010_handleStop(AsyncWebServerRequest *request) {
    M010_g_vehicleState.movementStatus = M010_MOVE_STOPPED;
    M010_g_vehicleState.rotationStatus = M010_ROTATE_NONE;
    Serial.println("W010: 차량 정지.");
    request->send(200, "text/plain", "OK"); // 응답 전송
}

/**
 * @brief 차량의 현재 상태를 JSON 형식으로 반환합니다.
 * @param request 웹 요청 객체
 */
void W010_handleStatus(AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"moveStatus\": \"" + M010_getMovementStatusString(M010_g_vehicleState.movementStatus) + "\",";
    json += "\"turnStatus\": \"" + M010_getRotationStatusString(M010_g_vehicleState.rotationStatus) + "\",";
    json += "\"currentSpeed\": " + String(M010_g_vehicleSettings.maxSpeed) + ",";
    json += "\"currentTurnFactor\": " + String(M010_g_vehicleSettings.turnFactor);
    json += "}";
    request->send(200, "application/json", json);
}

#pragma GCC diagnostic pop // 경고 비활성화 복원

#endif // W010_AWEB_MAIN_001_H
