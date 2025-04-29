// C110_IMU_002.h
#ifndef C110_IMU_002_H
#define C110_IMU_002_H

#include <Arduino.h>
#include <Wire.h> // I2C 통신 라이브러리

// 사용하는 IMU 센서의 I2C 주소 (MPU6050의 경우 0x68 또는 0x69)
const int G_C110_IMU_ADDRESS = 0x68;

class C110_IMU_Sensor {
public:
    // 생성자
    // 생성자는 헤더 파일에 구현 가능
    C110_IMU_Sensor() {
        // 변수 초기화
        v_accelX = v_accelY = v_accelZ = 0;
        v_gyroX = v_gyroY = v_gyroZ = 0;
        v_pitch = v_roll = 0.0;
    }

    // IMU 센서 초기화
    // 함수 구현부를 헤더 파일에 직접 작성
    bool C110_IMU_begin() {
        // IMU 센서가 응답하는지 확인 (I2C 주소로 스캔)
        Wire.beginTransmission(G_C110_IMU_ADDRESS);
        if (Wire.endTransmission() != 0) {
            // 센서 응답 없음
            return false;
        }

        // IMU 센서 설정 (MPU6050 예시)
        // 전원 관리 레지스터 설정 (슬립 모드 해제)
        Wire.beginTransmission(G_C110_IMU_ADDRESS);
        Wire.write(0x6B); // PWR_MGMT_1 레지스터 주소
        Wire.write(0x00); // 슬립 모드 해제
        Wire.endTransmission();

        // 추가적인 설정 (예: 자이로 스케일, 가속도 스케일 등)은 여기에 추가합니다.
        // 사용하는 IMU 센서의 데이터시트를 참고하세요.

        return true; // 초기화 성공
    }

    // IMU 데이터 읽기
    // 함수 구현부를 헤더 파일에 직접 작성
    void C110_IMU_readData() {
        // IMU 센서 데이터 읽기 (MPU6050 예시)
        Wire.beginTransmission(G_C110_IMU_ADDRESS);
        Wire.write(0x3B); // ACCEL_XOUT_H 레지스터 주소
        Wire.endTransmission(false); // 전송 종료 후 I2C 버스를 점유 상태로 유지

        Wire.requestFrom(G_C110_IMU_ADDRESS, 14, true); // 14바이트 데이터 요청 (가속도 6바이트, 온도 2바이트, 자이로 6바이트)

        if (Wire.available() == 14) {
            v_accelX = Wire.read() << 8 | Wire.read();
            v_accelY = Wire.read() << 8 | Wire.read();
            v_accelZ = Wire.read() << 8 | Wire.read();
            // 온도 데이터는 읽지만 사용하지 않음
            Wire.read() << 8 | Wire.read();
            v_gyroX = Wire.read() << 8 | Wire.read();
            v_gyroY = Wire.read() << 8 | Wire.read();
            v_gyroZ = Wire.read() << 8 | Wire.read();

            // 읽은 가속도 데이터로 피치와 롤 계산
            C110_IMU_calculatePitchAndRoll();
        } else {
            // 데이터 읽기 실패 처리
            Serial.println("IMU 데이터 읽기 실패!");
        }
    }

    // 피치 값 가져오기
    // 함수 구현부를 헤더 파일에 직접 작성
    float C110_IMU_getPitch() {
        return v_pitch;
    }

    // 롤 값 가져오기
    // 함수 구현부를 헤더 파일에 직접 작성
    float C110_IMU_getRoll() {
        return v_roll;
    }

private:
    // 가속도 및 자이로 데이터 저장 변수
    int16_t v_accelX, v_accelY, v_accelZ;
    int16_t v_gyroX, v_gyroY, v_gyroZ;

    // 계산된 피치 및 롤 값
    float v_pitch;
    float v_roll;

    // 가속도 데이터로부터 피치 및 롤 계산 함수
    // 함수 구현부를 헤더 파일에 직접 작성
    void C110_IMU_calculatePitchAndRoll() {
        // 가속도 데이터로부터 피치와 롤 계산 (간단한 방법)
        // 더 정확한 값을 위해서는 자이로 데이터와 함께 상보 필터 또는 칼만 필터를 사용해야 합니다.
        v_pitch = atan2(v_accelY, sqrt(v_accelX * v_accelX + v_accelZ * v_accelZ)) * 180 / PI;
        v_roll = atan2(-v_accelX, sqrt(v_accelY * v_accelY + v_accelZ * v_accelZ)) * 180 / PI;

        // atan2(y, x)는 -PI에서 +PI 라디안 값을 반환합니다.
        // 이를 각도로 변환하고 필요에 따라 범위를 조정합니다.
    }
};

#endif
