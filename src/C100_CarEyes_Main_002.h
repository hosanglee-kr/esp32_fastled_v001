// main.cpp
#include <Arduino.h>
#include <Wire.h> // I2C 통신 라이브러리

// 사용자 정의 헤더 파일 포함 (구현부가 포함됨)
#include "C110_IMU_002.h"
#include "C120_MTS_002.H"

// 하드웨어 설정 (사용하는 LED 매트릭스 및 IMU에 맞게 수정하세요)
// 예: 16x16 LED 매트릭스 (사용하는 라이브러리에 따라 설정 방식이 다릅니다)
const int G_C100_LED_MATRIX_WIDTH = 16;
const int G_C100_LED_MATRIX_HEIGHT = 16;

// I2C 핀 설정 (ESP32 기본 I2C 핀)
const int G_C100_I2C_SDA_PIN = 21;
const int G_C100_I2C_SCL_PIN = 22;

// IMU 센서 객체 생성
C110_IMU_Sensor g_C100_imu;

// LED 매트릭스 객체 생성 (사용하는 라이브러리에 따라 생성 방식이 다릅니다)
// 이 예제에서는 개념적인 C120_MTX_Matrix 클래스를 사용합니다. 실제 라이브러리 객체로 교체해야 합니다.
C120_MTX_Matrix g_C100_ledMatrix(G_C100_LED_MATRIX_WIDTH, G_C100_LED_MATRIX_HEIGHT);

// float 값을 다른 범위의 float 값으로 매핑하는 헬퍼 함수
float C100_mapFloat(float p_x, float p_in_min, float p_in_max, float p_out_min, float p_out_max) {
  return (p_x - p_in_min) * (p_out_max - p_out_min) / (p_in_max - p_in_min) + p_out_min;
}

void C100_init() {
    // 시리얼 통신 시작 (디버깅용)
    //Serial.begin(115200);
    Serial.println("ESP32 Robot Eye Simulation Start!");

    // I2C 통신 시작
    Wire.begin(G_C100_I2C_SDA_PIN, G_C100_I2C_SCL_PIN);

    // IMU 센서 초기화
    if (!g_C100_imu.C110_IMU_begin()) {
        Serial.println("IMU 센서 초기화 실패!");
        // 초기화 실패 시 더 이상 진행하지 않거나 오류 처리를 수행합니다.
        while (1);
    }
    Serial.println("IMU 센서 초기화 완료.");

    // LED 매트릭스 초기화 (사용하는 라이브러리에 맞게 수정)
    g_C100_ledMatrix.C120_MTX_begin(); // 개념적인 초기화 함수 호출
    Serial.println("LED 매트릭스 초기화 완료.");

    // 초기 화면 클리어
    g_C100_ledMatrix.C120_MTX_clear();
}

void C100_run() {
    // IMU 데이터 읽기
    g_C100_imu.C110_IMU_readData();

    // 피치와 롤 값 가져오기
    float v_pitch = g_C100_imu.C110_IMU_getPitch();
    float v_roll = g_C100_imu.C110_IMU_getRoll();

    // 시리얼 모니터로 피치와 롤 값 출력 (디버깅용)
    Serial.print("Pitch: ");
    Serial.print(v_pitch);
    Serial.print(", Roll: ");
    Serial.println(v_roll);

    // IMU 값에 따라 눈동자 위치 계산
    // 피치(상하 기울기)는 Y축 움직임에, 롤(좌우 기울기)은 X축 움직임에 매핑
    // IMU 값의 범위를 LED 매트릭스 픽셀 범위로 변환해야 합니다.
    // 이 변환은 사용하는 IMU 센서의 특성과 원하는 눈동자 움직임 범위에 따라 조정해야 합니다.
    // 예시: -30도 ~ +30도 범위를 -5 ~ +5 픽셀 범위로 매핑
    int v_pupilX = C100_mapFloat(v_roll, -30.0, 30.0, -(G_C100_LED_MATRIX_WIDTH / 4), (G_C100_LED_MATRIX_WIDTH / 4));
    int v_pupilY = C100_mapFloat(v_pitch, -30.0, 30.0, -(G_C100_LED_MATRIX_HEIGHT / 4), (G_C100_LED_MATRIX_HEIGHT / 4));

    // 계산된 눈동자 위치를 시리얼 모니터로 출력 (디버깅용)
    Serial.print("Pupil X: ");
    Serial.print(v_pupilX);
    Serial.print(", Pupil Y: ");
    Serial.println(v_pupilY);

    // LED 매트릭스 클리어
    g_C100_ledMatrix.C120_MTX_clear();

    // LED 매트릭스에 눈 그리기 (계산된 눈동자 위치 사용)
    g_C100_ledMatrix.C120_MTX_drawEye(v_pupilX, v_pupilY); // 개념적인 그리기 함수 호출

    // LED 매트릭스 업데이트 (사용하는 라이브러리에 따라 다름)
    // 예: someLedMatrixLibrary.display();

    // 짧은 딜레이
    delay(50); // IMU 읽기 및 화면 업데이트 주기
}
