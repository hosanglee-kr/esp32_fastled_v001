#ifndef _IMU_H_
#define _IMU_H_

#include "config.h" // 상수 정의 포함
#include "data.h"   // 전역 변수 정의 포함
#include <Adafruit_MPU6050.h> // MPU 라이브러리 포함
#include <Wire.h>			// I2C 통신 포함
#include <math.h>			// 수학 함수 포함
#include <Arduino.h>        // Serial 등 포함

// --- IMU 관련 함수 (C110) ---

// MPU6050 센서 데이터 읽기 함수
void C110_readMPUData() {
	g_C110_mpu.getEvent(&g_C110_a, &g_C110_g, &g_C110_temp);  // 가속도, 자이로, 온도 최신 값 읽기

	// 시리얼 모니터로 Raw 데이터 확인 (디버깅 시 유용)
	/*
	Serial.print("Raw Accel (m/s^2): "); Serial.print(g_C110_a.acceleration.x); Serial.print(", "); Serial.print(g_C110_a.acceleration.y); Serial.print(", "); Serial.println(g_C110_a.acceleration.z);
	Serial.print("Raw Gyro (deg/s): "); Serial.print(g_C110_g.gyro.x); Serial.print(", "); Serial.print(g_C110_g.gyro.y); Serial.print(", "); Serial.println(g_C110_g.gyro.z);
	*/
}

// 노이즈 필터링 적용 함수 (LPF 및 정지 시 기울기 계산 포함)
// MPU6050 내부 필터 사용 후 추가적인 소프트웨어 LPF 적용
void C110_applyFiltering() {
	// 1차 LPF 적용 (가속도 및 자이로 각 축)
    // MPU 내부 필터 설정에 따라 이 LPF의 alpha 값을 튜닝하거나 사용 중단 고려
	g_C110_filtered_ax		= g_C110_alpha * g_C110_a.acceleration.x + (1.0 - g_C110_alpha) * g_C110_filtered_ax;
	g_C110_filtered_ay		= g_C110_alpha * g_C110_a.acceleration.y + (1.0 - g_C110_alpha) * g_C110_filtered_ay;
	g_C110_filtered_az		= g_C110_alpha * g_C110_a.acceleration.z + (1.0 - g_C110_alpha) * g_C110_filtered_az;
	g_C110_filtered_gx		= g_C110_alpha * g_C110_g.gyro.x + (1.0 - g_C110_alpha) * g_C110_filtered_gx;
	g_C110_filtered_gy		= g_C110_alpha * g_C110_g.gyro.y + (1.0 - g_C110_alpha) * g_C110_filtered_gy;
	g_C110_filtered_gz		= g_C110_alpha * g_C110_g.gyro.z + (1.0 - g_C110_alpha) * g_C110_filtered_gz;

	// 시리얼 모니터로 필터링된 데이터 확인 (튜닝 시 필수)
	Serial.print("Filtered Accel (m/s^2): "); Serial.print(g_C110_filtered_ax); Serial.print(", "); Serial.print(g_C110_filtered_ay); Serial.print(", "); Serial.println(g_C110_filtered_az);
	Serial.print("Filtered Gyro (deg/s): "); Serial.print(g_C110_filtered_gx); Serial.print(", "); Serial.print(g_C110_filtered_gy); Serial.print(", "); Serial.println(g_C110_filtered_gz);


	// 정지 상태 판단을 위한 가속도/자이로 벡터 크기 계산
	// 가속도 크기는 중력가속도(약 9.8) 근처여야 함
	float v_accel_magnitude = sqrt(g_C110_filtered_ax * g_C110_filtered_ax + g_C110_filtered_ay * g_C110_filtered_ay + g_C110_filtered_az * g_C110_filtered_az);
	float v_gyro_magnitude	= sqrt(g_C110_filtered_gx * g_C110_filtered_gx + g_C110_filtered_gy * g_C110_filtered_gy + g_C110_filtered_gz * g_C110_filtered_gz);

	// 시리얼 모니터로 필터링된 크기 확인 (튜닝 시 필수)
	Serial.print("Accel Mag: "); Serial.print(v_accel_magnitude); Serial.print(", Gyro Mag: "); Serial.println(v_gyro_magnitude);

	// --- 정지 상태일 때만 가속도 센서로 Roll/Pitch 각도 계산 ---
	// MPU6050이 완전히 수평일 때 ax=0, ay=0, az=~9.8 로 가정한 계산 (장착 방향에 따라 atan2의 인자 순서 및 부호 변경 필수!)
	// 상보 필터 사용 시 이 부분을 대체하거나 보완
	// MPU 센서의 축 방향에 따라 atan2 인자 순서 및 부호가 달라짐.
	// 일반적으로 MPU6050의 Y축이 전진 방향, X축이 좌우, Z축이 상하(중력) 방향으로 장착된다고 가정 시:
	// Roll (X축 기준 회전): Y-Z 평면의 기울기. atan2(Ay, Az)
	// Pitch (Y축 기준 회전): X-Z 평면의 기울기. atan2(-Ax, sqrt(Ay*Ay + Az*Az))
	// 하지만 차량 후방 장착 시 축 방향이 바뀔 수 있으므로, 센서 방향 확인 후 튜닝 필요.
	// 예시: MPU의 X축이 차량의 좌우, Y축이 차량의 전후(뒤쪽-양수), Z축이 상하(중력 방향-음수) 인 경우
	// Z축은 보통 중력 방향(아래)을 가리키며, MPU 라이브러리는 중력 방향을 양수로 처리할 수 있으므로
	// 장착 방향과 센서 라이브러리 설정에 맞춰 atan2 인자 튜닝이 필요함.
	// 여기서는 'MPU Z축이 아래를 향하고 (중력 방향), Y축이 전방을 향함' 을 가정한 예시입니다.
	// 실제와 다를 경우 atan2(y, x)의 y와 x 자리에 들어갈 필터링된 가속도 축(ax, ay, az)을 교체하거나 부호를 변경하세요.

	// 나누기 0 방지
	float v_denominator_roll = (abs(g_C110_filtered_az) < 0.001 && abs(g_C110_filtered_ay) < 0.001) ? 0.001 : g_C110_filtered_az; // Az와 Ay 모두 0에 가까우면 0.001 사용
	float v_denominator_pitch_sq = g_C110_filtered_ay * g_C110_filtered_ay + g_C110_filtered_az * g_C110_filtered_az;
    float v_denominator_pitch = sqrt(v_denominator_pitch_sq);
    if (abs(v_denominator_pitch) < 0.001) v_denominator_pitch = 0.001;


	// 정지 또는 기울어짐 상태이거나, 이전 상태가 정지/기울어짐이었을 때만 각도 계산 유효
	if (g_C100_currentCarState == G_C100_STATE_STOPPED || g_C100_previousCarState == G_C100_STATE_STOPPED ||
		g_C100_currentCarState == G_C100_STATE_TILTED || g_C100_previousCarState == G_C100_STATE_TILTED) {

		// Roll: X축 기준 회전. Y-Z 평면의 기울기. atan2(Ay, Az) 사용.
		g_C110_accel_roll  = atan2(g_C110_filtered_ay, v_denominator_roll) * 180.0 / PI;
		// Pitch: Y축 기준 회전. X-Z 평면의 기울기. atan2(-Ax, sqrt(Ay*Ay + Az*Az)) 사용. (X축 방향 반전 필요시 -g_C110_filtered_ax)
		// v_denominator_pitch가 0에 가까우면 atan2 결과가 불안정할 수 있으므로 조건 처리 필요
		if (v_denominator_pitch > 0.001) { // 안정적인 경우에만 계산
			g_C110_accel_pitch = atan2(-g_C110_filtered_ax, v_denominator_pitch) * 180.0 / PI;
		} else {
            // YZ 평면 가속도 합이 0에 가까우면 Z축 가속도만 의미 있음.
            // Pitch는 XZ 평면 기울기이므로 Az가 중요. Ax와 Az의 부호로 0도 또는 180도에 가까운 값 추정.
            if (g_C110_filtered_az > 0) g_C110_accel_pitch = (g_C110_filtered_ax > 0) ? -90.0 : 90.0; // Az 양수: 센서 아래 방향, Ax 부호에 따라 ±90도
            else g_C110_accel_pitch = (g_C110_filtered_ax > 0) ? 90.0 : -90.0; // Az 음수: 센서 위 방향, Ax 부호에 따라 ±90도 (완전 수직 상태)
             if (abs(g_C110_filtered_ax) < 0.001) g_C110_accel_pitch = 0; // Ax도 0이면 Pitch는 0
        }


		Serial.print("Accel Roll: "); Serial.print(g_C110_accel_roll); Serial.print(", Pitch: "); Serial.println(g_C110_accel_pitch);
	}
}

// 상보 필터 구현 예시 (필요 시 사용, 현재는 가속도 기반 기울기 사용)
/*
void C110_complementaryFilter(float p_ax, float p_ay, float p_az, float p_gx, float p_gy, float p_gz, float p_dt) {
	// 자이로 데이터로 각도 변화 계산 (라디안)
	float v_gyro_roll_change_rad = p_gx * p_dt * PI / 180.0; // deg/s 를 rad/s 로 변환
	float v_gyro_pitch_change_rad = p_gy * p_dt * PI / 180.0;
	// float v_gyro_yaw_change_rad = p_gz * p_dt * PI / 180.0; // Yaw는 중력 영향 없어 가속도 보정 불가

	// 가속도 데이터로 Roll/Pitch 계산 (라디안)
	// atan2(y, x) 사용, MPU 장착 방향에 따라 인자 및 부호 조절 필수!
    // 나누기 0 방지
	float v_denominator_roll_accel = (abs(p_az) < 0.001 && abs(p_ay) < 0.001) ? 0.001 : p_az;
	float v_denominator_pitch_sq_accel = p_ay*p_ay + p_az*p_az;
    float v_denominator_pitch_accel = sqrt(v_denominator_pitch_sq_accel);
    if (abs(v_denominator_pitch_accel) < 0.001) v_denominator_pitch_accel = 0.001;

	float v_accel_roll_rad = atan2(p_ay, v_denominator_roll_accel);
	float v_accel_pitch_rad = atan2(-p_ax, v_denominator_pitch_accel);

	// 상보 필터 업데이트 (라디안 단위로 계산 후 필요 시 도 단위로 변환하여 저장)
	float v_filter_weight = 0.98; // 자이로 비중 (튜닝 필요, 1에 가까울수록 자이로 영향 큼)
    // g_C110_comp_roll = v_filter_weight * (g_C110_comp_roll + v_gyro_roll_change_rad) + (1.0 - v_filter_weight) * v_accel_roll_rad;
	// g_C110_comp_pitch = v_filter_weight * (g_C110_comp_pitch + v_gyro_pitch_change_rad) + (1.0 - v_filter_weight) * v_accel_pitch_rad;

    // 상보 필터 결과 값을 도 단위 전역 변수에 저장하는 예시
    // float g_C110_comp_roll_deg = g_C110_comp_roll * 180.0 / PI;
    // float g_C110_comp_pitch_deg = g_C110_comp_pitch * 180.0 / PI;

	// g_C110_comp_yaw = g_C110_comp_yaw + v_gyro_yaw_change; // Yaw는 자이로만 누적 (드리프트 발생)
}
*/


#endif // _IMU_H_
