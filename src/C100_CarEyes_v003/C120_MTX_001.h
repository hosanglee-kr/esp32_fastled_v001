#ifndef _MATRIX_H_
#define _MATRIX_H_

#include "config_001.h" // 상수 정의 포함
#include "data_001.h" // 전역 변수 정의 포함
#include <FastLED.h> // FastLED 라이브러리 포함
#include <Arduino.h> // constrain, map 등 Arduino 함수 포함

// --- LED Matrix 제어 및 그리기 함수 (C120) ---

// LED 매트릭스 픽셀 인덱스 계산 함수 (!!! 중요 !!! 실제 하드웨어 배선에 맞게 수정 필요!)
// 2개의 8x8 지그재그(Serpentine) 매트릭스가 직렬 연결된 것을 가정한 구현.
// p_matrix_index: 0 또는 1 (좌/우 눈)
// p_x, p_y: 해당 매트릭스 내에서의 픽셀 좌표 (0 ~ G_C120_MATRIX_WIDTH-1, 0 ~ G_C120_MATRIX_HEIGHT-1)
int C120_getLedIndex(uint8_t p_matrix_index, uint8_t p_x, uint8_t p_y) {
	if (p_matrix_index >= G_C120_NUM_MATRICES || p_x >= G_C120_MATRIX_WIDTH || p_y >= G_C120_MATRIX_HEIGHT) {
		// 유효 범위를 벗어나면 -1 등 오류 값 반환
		return -1;
	}

	int v_index = -1;
	int v_base_index = p_matrix_index * G_C120_NUM_LEDS_PER_MATRIX; // 해당 매트릭스의 시작 LED 인덱스

	// 매트릭스 내부의 지그재그(Serpentine) 배선 계산
	// 행 인덱스 p_y가 짝수일 때와 홀수일 때 x 방향이 반전됩니다.
	if (p_y % 2 == 0) { // 짝수 행 (0, 2, 4, 6) -> 왼쪽에서 오른쪽 (x 증가)
		v_index = v_base_index + (p_y * G_C120_MATRIX_WIDTH + p_x);
	} else { // 홀수 행 (1, 3, 5, 7) -> 오른쪽에서 왼쪽 (x 감소)
		v_index = v_base_index + (p_y * G_C120_MATRIX_WIDTH + (G_C120_MATRIX_WIDTH - 1 - p_x));
	}

	// 계산된 인덱스가 전체 LED 범위 내에 있는지 최종 확인 (사실 위 로직이면 범위 안 벗어남)
	if (v_index < 0 || v_index >= G_C120_TOTAL_NUM_LEDS) return -1;

	return v_index;
}

// 모든 LED를 끄는 함수
void C120_clearDisplay() {
	FastLED.clear(); // 모든 LED 색상을 CRGB::Black으로 설정
}

// 특정 픽셀에 색상을 설정하는 함수 (C120_getLedIndex 활용)
void C120_drawPixel(uint8_t p_matrix_index, uint8_t p_x, uint8_t p_y, CRGB p_color) {
	int v_index = C120_getLedIndex(p_matrix_index, p_x, p_y);
	if (v_index != -1) { // 유효한 인덱스일 경우
		g_C120_leds[v_index] = p_color;
	}
}

//====================================================
// --- 눈 모양 그리기 함수들 (구현) ---
// 여기에 각 눈 모양 디자인을 8x8 픽셀 아트로 구현합니다.
// C120_drawPixel 또는 leds[] 배열과 C120_getLedIndex를 사용합니다.
//====================================================

// 기본 눈 모양 그리기 (간단한 원형 눈 + 가운데 눈동자)
void C120_drawEyeNeutral(uint8_t p_matrix_index) {
	int v_center_x = G_C120_MATRIX_WIDTH / 2; // 4
	int v_center_y = G_C120_MATRIX_HEIGHT / 2; // 4
	int v_eye_radius_sq = 3 * 3; // 눈 테두리 반경 제곱 예시 (픽셀 중심 기준)
	int v_pupil_radius_sq = 1 * 1; // 눈동자 반경 제곱 예시

	for (uint8_t v_y = 0; v_y < G_C120_MATRIX_HEIGHT; v_y++) {
		for (uint8_t v_x = 0; v_x < G_C120_MATRIX_WIDTH; v_x++) {
			// 픽셀 중심 좌표 (0,0 픽셀의 중심은 0.5, 0.5)
			float v_pixel_center_x = v_x + 0.5;
			float v_pixel_center_y = v_y + 0.5;

			// 눈 영역 중심으로부터 픽셀 중심까지의 거리 제곱
			float dx = v_pixel_center_x - (v_center_x + 0.5); // 매트릭스 중앙 픽셀의 중심을 기준으로
			float dy = v_pixel_center_y - (v_center_y + 0.5);
			float dist_sq = dx * dx + dy * dy;

			// 눈 테두리 그리기 (정확한 원은 아니지만 근사)
            // 중심으로부터 일정 거리 내에 있는 픽셀을 켜서 원 모양을 만듦
			if (dist_sq <= v_eye_radius_sq + 1 && dist_sq > v_eye_radius_sq - 2) { // 테두리 두께 및 경계 조절
				C120_drawPixel(p_matrix_index, v_x, v_y, CRGB::White);
			}

			// 눈동자 그리기 (가운데)
			if (dist_sq <= v_pupil_radius_sq) {
				C120_drawPixel(p_matrix_index, v_x, v_y, CRGB::White); // 눈동자 색상
			}
		}
	}
}

// 깜빡임 눈 모양 그리기 (애니메이션 단계 전달)
void C120_drawEyeBlink(uint8_t p_matrix_index, unsigned long p_animation_phase) {
	// p_animation_phase: 깜빡임 애니메이션이 시작된 후 경과된 시간 (ms)
	// g_C100_blink_duration: 총 깜빡임 애니메이션 지속 시간 (ms)

	// 시간 경과에 따라 눈꺼풀 모양을 조절하여 깜빡임 표현
	// 0% ~ 50%까지는 감는 애니, 50% ~ 100%까지는 뜨는 애니
	unsigned long v_half_duration = g_C100_blink_duration / 2;
	int v_center_y = G_C120_MATRIX_HEIGHT / 2; // 4

	if (p_animation_phase < v_half_duration) {
		// 눈 감는 중간 단계 (위/아래 눈꺼풀이 가운데로 모임)
		// 시간 경과에 따라 눈꺼풀 라인의 Y 위치를 조절
		// 0ms -> 위 Y=0, 아래 Y=7 / v_half_duration -> 위 Y=3, 아래 Y=4 (완전히 감김)
		int v_eyelid_y_top = map(p_animation_phase, 0UL, v_half_duration, 0, v_center_y -1); // 위 눈꺼풀 (0UL은 unsigned long 형변환)
        int v_eyelid_y_bottom = map(p_animation_phase, 0UL, v_half_duration, G_C120_MATRIX_HEIGHT - 1, v_center_y); // 아래 눈꺼풀

		// 계산된 Y 좌표를 벗어나지 않도록 제한
		v_eyelid_y_top = constrain(v_eyelid_y_top, 0, v_center_y);
        v_eyelid_y_bottom = constrain(v_eyelid_y_bottom, v_center_y-1, G_C120_MATRIX_HEIGHT - 1);


		// 눈꺼풀 라인 그리기
		for (uint8_t v_x = 0; v_x < G_C120_MATRIX_WIDTH; v_x++) {
			C120_drawPixel(p_matrix_index, v_x, v_eyelid_y_top, CRGB::White); // 위 눈꺼풀 라인
            C120_drawPixel(p_matrix_index, v_x, v_eyelid_y_bottom, CRGB::White); // 아래 눈꺼풀 라인
		}

	} else {
		// 눈 뜨는 중간 단계 또는 완전히 뜬 상태
		// 시간 경과에 따라 눈꺼풀 라인의 Y 위치를 다시 원래 위치로 이동
        // v_half_duration -> 위 Y=3, 아래 Y=4 / g_C100_blink_duration -> 위 Y=0, 아래 Y=7
		int v_eyelid_y_top = map(p_animation_phase, v_half_duration, g_C100_blink_duration, v_center_y - 1, 0); // 위 눈꺼풀
        int v_eyelid_y_bottom = map(p_animation_phase, v_half_duration, g_C100_blink_duration, v_center_y, G_C120_MATRIX_HEIGHT - 1); // 아래 눈꺼풀

        // 계산된 Y 좌표를 벗어나지 않도록 제한
        v_eyelid_y_top = constrain(v_eyelid_y_top, 0, v_center_y-1);
        v_eyelid_y_bottom = constrain(v_eyelid_y_bottom, v_center_y, G_C120_MATRIX_HEIGHT - 1);

        // 눈꺼풀 라인 그리기 (눈이 완전히 떠지기 전까지)
        // 완전히 떠진 상태 (기본 눈)가 아니면 눈꺼풀 라인을 그리고, 맞으면 기본 눈을 그림
        if (v_eyelid_y_top > 0 || v_eyelid_y_bottom < G_C120_MATRIX_HEIGHT - 1) {
             for (uint8_t v_x = 0; v_x < G_C120_MATRIX_WIDTH; v_x++) {
                C120_drawPixel(p_matrix_index, v_x, v_eyelid_y_top, CRGB::White);
                C120_drawPixel(p_matrix_index, v_x, v_eyelid_y_bottom, CRGB::White);
             }
        } else {
            // 완전히 떠진 상태
		    C120_drawEyeNeutral(p_matrix_index);
        }
	}
}

// 특정 방향 보기 눈 모양 그리기 (상대 좌표 전달)
void C120_drawEyeLook(uint8_t p_matrix_index, int8_t p_target_x, int8_t p_target_y) {
	// p_target_x, p_target_y: 눈동자가 이동할 방향 지시 (-1:왼쪽/위, 1:오른쪽/아래, 0:중앙)
	// 이 값을 기반으로 눈동자의 중심 픽셀 좌표 오프셋을 계산
	int v_center_x = G_C120_MATRIX_WIDTH / 2; // 4
	int v_center_y = G_C120_MATRIX_HEIGHT / 2; // 4
	int v_pupil_radius_sq = 1 * 1; // 눈동자 반경 제곱 예시

	// p_target 값을 눈동자가 움직일 수 있는 최대 오프셋으로 매핑
	int v_max_offset_x = 2; // 눈동자가 좌우로 움직일 수 있는 최대 픽셀 수
	int v_max_offset_y = 1; // 눈동자가 상하로 움직일 수 있는 최대 픽셀 수

	int v_pupil_center_x = v_center_x + p_target_x * v_max_offset_x;
	int v_pupil_center_y = v_center_y + p_target_y * v_max_offset_y;

    // 눈동자가 이동할 수 있는 실제 픽셀 범위 내로 제한
    v_pupil_center_x = constrain(v_pupil_center_x, v_center_x - v_max_offset_x, v_center_x + v_max_offset_x);
    v_pupil_center_y = constrain(v_pupil_center_y, v_center_y - v_max_offset_y, v_center_y + v_max_offset_y);


	// 눈 테두리 그리기 (기본 눈 모양 사용 - 눈동자 제외)
    // C120_drawEyeNeutral 함수에서 눈동자 그리는 부분을 분리하거나, 여기서 눈동자 영역을 지우고 다시 그림
	C120_drawEyeNeutral(p_matrix_index); // 일단 전체 눈을 그리고, 눈동자 위치만 다시 그리는 방식

    // 기존 눈동자 영역을 지우기 (가운데 3x3 영역 대략적으로)
    int v_base_pupil_center_x = G_C120_MATRIX_WIDTH / 2;
    int v_base_pupil_center_y = G_C120_MATRIX_HEIGHT / 2;
    for (int v_py = -1; v_py <= 1; v_py++) {
		for (int v_px = -1; v_px <= 1; v_px++) {
            C120_drawPixel(p_matrix_index, v_base_pupil_center_x + v_px, v_base_pupil_center_y + v_py, CRGB::Black);
        }
    }


	// 눈동자 그리기 (계산된 위치에)
	for (int v_py = -1; v_py <= 1; v_py++) { // 3x3 픽셀 영역에서 눈동자 그리기
		for (int v_px = -1; v_px <= 1; v_px++) {
			// 원형 눈동자 (반경 1 픽셀)
			if (v_px * v_px + v_py * v_py <= v_pupil_radius_sq) { // 원 범위 내 픽셀
				C120_drawPixel(p_matrix_index, v_pupil_center_x + v_px, v_pupil_center_y + v_py, CRGB::White); // 눈동자 색상
			}
		}
	}
	// 필요 시 눈동자 색상 변경 가능
}

// 찡그림 눈 모양 그리기 (강도 전달)
void C120_drawEyeSquint(uint8_t p_matrix_index, bool p_tight) {
	// p_tight: true면 강하게, false면 약하게 찡그림

	int v_center_y = G_C120_MATRIX_HEIGHT / 2; // 4
	int v_squint_level = p_tight ? 2 : 1; // 찡그림 강도에 따른 눈꺼풀 이동 거리

	// 눈꺼풀 라인 그리기 (가로선 - 찡그린 모양)
	// 위/아래 눈꺼풀이 가운데로 모이는 형태
	for (uint8_t v_x = 0; v_x < G_C120_MATRIX_WIDTH; v_x++) {
		// 위 찡그린 라인
		if (v_center_y - v_squint_level >= 0) C120_drawPixel(p_matrix_index, v_x, v_center_y - v_squint_level, CRGB::White);
		// 아래 찡그린 라인
		if (v_center_y + v_squint_level < G_C120_MATRIX_HEIGHT) C120_drawPixel(p_matrix_index, v_x, v_center_y + v_squint_level, CRGB::White);

		// 강하게 찡그릴 때 (p_tight == true) 가운데 라인 추가
		if (p_tight) {
             C120_drawPixel(p_matrix_index, v_x, v_center_y, CRGB::White);
        }
	}

	// 눈동자 그리기 (찡그렸을 때는 작게 또는 약간 가려지게)
	// 사용자 제공 코드는 눈동자 반경 0으로 설정하여 눈동자를 그리지 않음. 이 방식을 따름.
}

// 화남 눈 모양 그리기 (찡그림 + 눈썹)
void C120_drawEyeAngry(uint8_t p_matrix_index) {
	C120_drawEyeSquint(p_matrix_index, true); // 강하게 찡그린 모양 기본

	// 눈썹 모양 추가 (안쪽 아래로 쏠리게)
	int v_center_x = G_C120_MATRIX_WIDTH / 2; // 4
	int v_eyebrow_y = G_C120_MATRIX_HEIGHT / 2 - 3; // 눈꺼풀 위쪽 예시

	if (p_matrix_index == 0) { // 왼쪽 눈썹 (오른쪽 아래로)
		C120_drawPixel(p_matrix_index, v_center_x - 1, v_eyebrow_y, CRGB::White);
		C120_drawPixel(p_matrix_index, v_center_x - 2, v_eyebrow_y + 1, CRGB::White);
	} else { // 오른쪽 눈썹 (왼쪽 아래로)
		C120_drawPixel(p_matrix_index, v_center_x, v_eyebrow_y, CRGB::White);
		C120_drawPixel(p_matrix_index, v_center_x + 1, v_eyebrow_y + 1, CRGB::White);
	}
    // 사용자 제공 코드는 눈동자를 별도로 그리지 않음. 찡그린 눈동자를 따름.
}

// 황당/놀람 눈 모양 그리기 (눈 크게 + 작은 눈동자)
void C120_drawEyeAbsurd(uint8_t p_matrix_index) {
	int v_center_x = G_C120_MATRIX_WIDTH / 2; // 4
	int v_center_y = G_C120_MATRIX_HEIGHT / 2; // 4
	int v_eye_radius_sq = 4 * 4; // 눈 테두리 크게
	int v_pupil_radius_sq = 0 * 0; // 눈동자 아주 작게 또는 없애기 (사용자 코드 기준)

	for (uint8_t v_y = 0; v_y < G_C120_MATRIX_HEIGHT; v_y++) {
		for (uint8_t v_x = 0; v_x < G_C120_MATRIX_WIDTH; v_x++) {
            // 픽셀 중심 좌표를 사용한 원 그리기
            float v_pixel_center_x = v_x + 0.5;
			float v_pixel_center_y = v_y + 0.5;

			float dx = v_pixel_center_x - (v_center_x + 0.5);
			float dy = v_pixel_center_y - (v_center_y + 0.5);
			float dist_sq = dx * dx + dy * dy;

			// 눈 테두리 그리기 (큰 원)
			if (dist_sq <= v_eye_radius_sq + 1 && dist_sq > (v_eye_radius_sq - 2)) { // 테두리 두께 예시
				C120_drawPixel(p_matrix_index, v_x, v_y, CRGB::White);
			}

			// 눈동자 그리기 (아주 작게 - 사용자 코드 기준)
			if (v_pupil_radius_sq > 0 && dist_sq <= v_pupil_radius_sq) { // 반경이 0보다 클 때만 그림
				C120_drawPixel(p_matrix_index, v_x, v_y, CRGB::White);
			}
		}
	}
	// 충격/감속 시 눈동자 순간 이동 애니메이션 로직 필요 시 여기에 시간(millis() - g_C100_state_start_time) 활용하여 구현
}

// 불쾌함/노려봄 눈 모양 그리기 (작은 눈 + 강조된 눈동자)
void C120_drawEyeGlaring(uint8_t p_matrix_index) {
	// 약하게 찡그린 모양 기본 (사용자 코드 기준)
    C120_drawEyeSquint(p_matrix_index, false);

	// 눈동자 강조 (색상 변경 또는 크기 약간 키우기)
	int v_center_x = G_C120_MATRIX_WIDTH / 2; // 4
	int v_center_y = G_C120_MATRIX_HEIGHT / 2; // 4
	int v_pupil_radius_sq = 1 * 1; // 눈동자 반경

    // 눈동자 그릴 위치 설정 (가운데)
    int v_pupil_center_x = v_center_x;
    int v_pupil_center_y = v_center_y;

    // 찡그린 눈꺼풀 때문에 눈동자 영역이 제한됨.
    // 찡그린 눈 모양 위에 눈동자를 덧그리는 방식으로 구현.
    // 눈동자 영역을 Black으로 지우고 Red로 다시 그림
    for (int v_py = -1; v_py <= 1; v_py++) {
		for (int v_px = -1; v_px <= 1; v_px++) {
             // 눈동자가 그려질 수 있는 영역인지 확인 (8x8 매트릭스 범위 내)
            if (v_pupil_center_x + v_px >= 0 && v_pupil_center_x + v_px < G_C120_MATRIX_WIDTH &&
                v_pupil_center_y + v_py >= 0 && v_pupil_center_y + v_py < G_C120_MATRIX_HEIGHT) {

                 // 기존 픽셀을 지우고 (눈꺼풀 라인 포함)
                 C120_drawPixel(p_matrix_index, v_pupil_center_x + v_px, v_pupil_center_y + v_py, CRGB::Black);

                // 눈동자 픽셀 그리기 (원형)
                if (v_px * v_px + v_py * v_py <= v_pupil_radius_sq + 0.5) { // 약간 여유있는 반경
				    C120_drawPixel(p_matrix_index, v_pupil_center_x + v_px, v_pupil_center_y + v_py, CRGB::Red); // 빨간 눈동자 예시
			    }
            }
		}
	}
    // 사용자 제공 코드는 눈꺼풀 라인 외 별도 날카로운 라인을 그리지 않음. 필요 시 추가 가능.
}

// 졸림 눈 모양 그리기 (눈꺼풀 처짐)
void C120_drawEyeSleepy(uint8_t p_matrix_index) {
	int v_center_x = G_C120_MATRIX_WIDTH / 2; // 4
	int v_center_y = G_C120_MATRIX_HEIGHT / 2; // 4
	int v_droop = 2; // 처지는 정도 예시 (사용자 코드 기준)

	// 눈꺼풀 라인 그리기 (위 라인은 아래로, 아래 라인은 위로) - 사용자 코드 기준
	for (uint8_t v_x = 0; v_x < G_C120_MATRIX_WIDTH; v_x++) {
		// 위 눈꺼풀 처짐 라인
		if (v_center_y - v_droop >= 0) C120_drawPixel(p_matrix_index, v_x, v_center_y - v_droop, CRGB::White);
		// 아래 눈꺼풀 올라옴 라인
		if (v_center_y + v_droop < G_C120_MATRIX_HEIGHT) C120_drawPixel(p_matrix_index, v_x, v_center_y + v_droop, CRGB::White);
	}

	// 눈동자 그리기 (아주 작게 또는 없애기) - 사용자 코드 기준은 눈동자 없음.
}

// 혼란스러움 눈 모양 그리기 (비대칭)
void C120_drawEyeConfused(uint8_t p_matrix_index) {
	// 기본 눈 모양 사용 (사용자 코드 기준)
	C120_drawEyeNeutral(p_matrix_index);

	// 눈썹 모양 비대칭으로 추가 (예시) - 사용자 코드 기준
	int v_center_x = G_C120_MATRIX_WIDTH / 2; // 4
	int v_eyebrow_y = G_C120_MATRIX_HEIGHT / 2 - 3; // 눈꺼풀 위쪽 예시

	if (p_matrix_index == 0) { // 왼쪽 눈썹 (오른쪽 위로)
		C120_drawPixel(p_matrix_index, v_center_x - 1, v_eyebrow_y, CRGB::White);
		C120_drawPixel(p_matrix_index, v_center_x - 2, v_eyebrow_y - 1, CRGB::White);
	} else { // 오른쪽 눈썹 (왼쪽 아래로 - 화난 눈썹 비슷)
		C120_drawPixel(p_matrix_index, v_center_x, v_eyebrow_y, CRGB::White);
		C120_drawPixel(p_matrix_index, v_center_x + 1, v_eyebrow_y + 1, CRGB::White);
	}
    // 사용자 코드는 눈동자 위치를 좌우 눈 다르게 설정하는 로직은 포함하지 않음. 필요 시 추가 가능.
}

#endif // _MATRIX_H_
