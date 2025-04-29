//====================================================
// 자동차 후방 로봇 눈 프로젝트 - Refactoring 및 전체 구현
// ESP32 + MPU6050 + FastLED (2x 8x8 WS2812B Matrix, 직렬 연결)
//
// 기능 요구사항:
// - MPU6050 센서 데이터(가속도, 각속도) 및 시간 정보만 사용
// - 차량 신호 입력 없음
// - MPU6050 데이터 노이즈 필터링 포함 (LPF, 기울기 계산 시 보정)
// - 차량 상태 추론 (정지, 주행, 가속, 감속, 회전, 충격, 정지 시 기울어짐)
// - 추론된 상태에 따른 로봇 눈 표현 (FastLED 사용)
// - 8x8 RGB LED 매트릭스 2개 사용 (좌/우 눈), WS2812B 직렬 연결 (지그재그 배선 가정)
// - 네이밍 컨벤션 적용:
// - 전역 변수: g_C100_, g_C110_, g_C120_
// - 전역 상수: G_C100_, G_C110_, G_C120_
// - 함수명: C100_, C110_, C120_
// - 로컬 변수: v_
// - 함수 파라미터: p_
//====================================================

// --- 모듈 헤더 파일 포함 ---
#include "config_001.h"
#include "data_001.h"
#include "C110_IMU_001.h"
#include "C120_MTX_001.h"

// --- 디버깅 헬퍼 함수 (String 반환 버전) ---
String C100_currentStateToString(CarState p_state) {
 switch (p_state) {
	case G_C100_STATE_UNKNOWN: return "UNKNOWN";
	case G_C100_STATE_STOPPED: return "STOPPED";
	case G_C100_STATE_MOVING_NORMAL: return "MOVING_NORMAL";
	case G_C100_STATE_ACCELERATING: return "ACCELERATING";
	case G_C100_STATE_BRAKING: return "BRAKING";
	case G_C100_STATE_TURNING_LEFT: return "TURNING_LEFT";
	case G_C100_STATE_TURNING_RIGHT: return "TURNING_RIGHT";
	case G_C100_STATE_BUMP: return "BUMP";
	case G_C100_STATE_TILTED: return "TILTED";
	// G_C100_STATE_REVERSING는 현재 미사용
	default: return "INVALID";
 }
}

String C100_eyeStateToString(EyeState p_state) {
 switch (p_state) {
	case G_C100_E_NEUTRAL: return "NEUTRAL";
	case G_C100_E_BLINK: return "BLINK";
	case G_C100_E_LOOK_LEFT: return "LOOK_LEFT";
	case G_C100_E_LOOK_RIGHT: return "LOOK_RIGHT";
	case G_C100_E_LOOK_UP: return "LOOK_UP";
	case G_C100_E_LOOK_DOWN: return "LOOK_DOWN";
	case G_C100_E_SQUINT: return "SQUINT";
	case G_C100_E_SQUINT_TIGHT: return "SQUINT_TIGHT";
	case G_C100_E_ANGRY: return "ANGRY";
	case G_C100_E_ABSURD: return "ABSURD";
	case G_C100_E_GLARING: return "GLARING";
	case G_C100_E_SLEEPY: return "SLEEPY";
	case G_C100_E_CONFUSED: return "CONFUSED";
	default: return "INVALID";
 }
}

// --- 핵심 로직 함수 (CarState 및 EyeState 관리: C100) ---

// 차량 상태 추론 로직 함수
void C100_inferCarState() {
	unsigned long v_currentTime = millis();
	CarState v_nextCarState = g_C100_currentCarState; // 기본적으로 현재 상태 유지

	// 필터링된 가속도 및 자이로 벡터 크기 계산 (C110_applyFiltering에서 이미 계산)
	float v_accel_magnitude = sqrt(g_C110_filtered_ax * g_C110_filtered_ax + g_C110_filtered_ay * g_C110_filtered_ay + g_C110_filtered_az * g_C110_filtered_az);
	float v_gyro_magnitude = sqrt(g_C110_filtered_gx * g_C110_filtered_gx + g_C110_filtered_gy * g_C110_filtered_gy + g_C110_filtered_gz * g_C110_filtered_gz);

	// --- 상태 판단 우선순위 (순서 중요!) ---
	// 순간적인 상태 (충격, 급감속, 가속, 회전)를 먼저 판단하고, 아니면 정지 또는 일반 주행 판단

	// 1. 충격/요철 감지 (다른 움직임 상태가 아닐 때의 순간적인 큰 가속도 변화)
	// 간단한 구현: 필터링된 가속도 절대값 임계값 초과 및 자이로 변화 적을 때
	// 더 정확한 구현: 이전 가속도 값과의 변화량, 또는 특정 주파수 대역 분석 필요
	bool v_isMovingState = (g_C100_currentCarState != G_C100_STATE_STOPPED && g_C100_currentCarState != G_C100_STATE_TILTED && g_C100_currentCarState != G_C100_STATE_UNKNOWN);

	// 충격 감지는 자이로 변화가 크지 않으면서 순간적인 가속도 변화가 클 때 (정지, 가감속, 회전 상태가 아닐 때)
	// 또는 이미 움직이는 상태에서 순간적인 큰 가속도 변화가 있을 때
	if ( !(g_C100_currentCarState == G_C100_STATE_BRAKING && (v_currentTime - g_C100_state_start_time) < 800) && // 급감속 초기 상태가 아닐 때 (충격과 감속 구분)
         v_gyro_magnitude < G_C110_GYRO_TURN_THRESHOLD_DEGS * 2 && // 회전이 심하지 않을 때 (충격과 회전 구분)
		 (abs(g_C110_filtered_ax) > G_C110_ACCEL_BUMP_THRESHOLD_MS2 ||
          abs(g_C110_filtered_ay) > G_C110_ACCEL_BUMP_THRESHOLD_MS2 ||
          abs(g_C110_filtered_az - 9.8) > G_C110_ACCEL_BUMP_THRESHOLD_MS2 // Z축은 중력 기준
         )
	) {
		// 이전 상태가 충격 상태가 아니었을 때만 충격 상태로 진입 처리하여 짧은 시간 유지
		if (g_C100_currentCarState != G_C100_STATE_BUMP) {
			v_nextCarState = G_C100_STATE_BUMP;
			g_C100_state_start_time = v_currentTime; // 충격 상태 시작 시간 기록
		}
	}
	// 충격 상태는 매우 짧게 유지 후 이전 상태로 복귀하는 로직
	else if (g_C100_currentCarState == G_C100_STATE_BUMP) {
		if ((v_currentTime - g_C100_state_start_time) > 500) { // 예시: 충격 후 0.5초 지나면
			v_nextCarState = g_C100_previousCarState; // 충격 직전 상태로 복귀 (또는 G_C100_STATE_MOVING_NORMAL)
			// 복귀 시 이전 상태의 시작 시간 정보는 살리지 않음. 새로운 상태 진입으로 간주.
		}
	}

	// 2. 정지 상태 감지 (충격 감지 로직 이후에 수행)
	// 가속도 크기가 중력가속도 근처이고 변화가 적으며, 자이로 값도 매우 작을 때
	// 현재 상태가 충격 상태가 아닐면서 정지 조건을 만족할 때 판단
	if (v_nextCarState != G_C100_STATE_BUMP) { // 충격 상태가 아닐 때만 정지/기울어짐 판단
		if (abs(v_accel_magnitude - 9.8) < G_C110_ACCEL_IDLE_THRESHOLD_MS2 && v_gyro_magnitude < G_C110_GYRO_IDLE_THRESHOLD_DEGS) {
			if (g_C100_currentCarState != G_C100_STATE_STOPPED && g_C100_currentCarState != G_C100_STATE_TILTED) {
				// 정지 또는 기울어짐 상태로 방금 진입했으면 시간 기록 시작
				v_nextCarState = G_C100_STATE_STOPPED; // 일단 정지로 판단
				g_C100_state_start_time = v_currentTime;
				g_C100_stop_duration = 0; // 새로 정지했으니 시간 0으로 초기화
			} else {
				// 이미 정지 또는 기울어짐 상태면 지속 시간 업데이트
				g_C100_stop_duration = v_currentTime - g_C100_state_start_time;

				// 정지 상태일 때만 기울어짐 추가 판단
				// C110_applyFiltering에서 계산된 g_C110_accel_roll, g_C110_accel_pitch 사용
				if (abs(g_C110_accel_roll) > G_C110_TILT_ANGLE_THRESHOLD_DEG || abs(g_C110_accel_pitch) > G_C110_TILT_ANGLE_THRESHOLD_DEG) {
					v_nextCarState = G_C100_STATE_TILTED; // 정지 상태이면서 기울어졌음
				} else {
					v_nextCarState = G_C100_STATE_STOPPED; // 기울어지지 않은 정지 상태
				}
			}
		}
		// 정지 또는 기울어짐 상태가 아니라면 정지 시간 초기화
		else {
			g_C100_stop_duration = 0; // 정지 풀림
			if (g_C100_currentCarState == G_C100_STATE_STOPPED || g_C100_currentCarState == G_C100_STATE_TILTED) {
				// 정지/기울어짐 상태에서 방금 벗어난 경우 시간 기록 시작
				g_C100_state_start_time = v_currentTime;
			}
			// 계속 움직이는 중이면 g_C100_state_start_time 유지
		}
	}

	// 3. 회전 감지 (정지/충격 감지 로직 이후에 수행)
	// Z축 자이로 값이 특정 임계값 이상일 때 (부호로 방향 구분)
	// 현재 상태가 정지, 기울어짐, 충격 상태가 아닐 때만 회전 판단
	if (v_nextCarState != G_C100_STATE_STOPPED && v_nextCarState != G_C100_STATE_TILTED && v_nextCarState != G_C100_STATE_BUMP) {
		if (g_C110_filtered_gz > G_C110_GYRO_TURN_THRESHOLD_DEGS) {
			v_nextCarState = G_C100_STATE_TURNING_RIGHT;
		} else if (g_C110_filtered_gz < -G_C110_GYRO_TURN_THRESHOLD_DEGS) {
			v_nextCarState = G_C100_STATE_TURNING_LEFT;
		}
		// 회전 상태로 진입했으면 시간 기록 시작 (이미 움직이는 중이었더라도 특정 상태 진입으로 간주)
		if (g_C100_currentCarState != v_nextCarState) g_C100_state_start_time = v_currentTime;
	}

	// 4. 가속/감속 감지 (정지, 충격, 회전 감지 로직 이후에 수행)
	// Y축 가속도 값이 특정 임계값 이상일 때 (부호로 방향 구분)
	// 현재 상태가 다른 특정 움직임 상태가 아닐 때만 가감속 판단
	if (v_nextCarState != G_C100_STATE_STOPPED && v_nextCarState != G_C100_STATE_TILTED && v_nextCarState != G_C100_STATE_BUMP &&
		v_nextCarState != G_C100_STATE_TURNING_LEFT && v_nextCarState != G_C100_STATE_TURNING_RIGHT)
	{
		if (g_C110_filtered_ay > G_C110_ACCEL_FORWARD_THRESHOLD_MS2) {
			v_nextCarState = G_C100_STATE_ACCELERATING;
		} else if (g_C110_filtered_ay < G_C110_ACCEL_BRAKE_THRESHOLD_MS2) {
			v_nextCarState = G_C100_STATE_BRAKING;
		}
		// 가감속 상태로 진입했으면 시간 기록 시작
		if (g_C100_currentCarState != v_nextCarState) g_C100_state_start_time = v_currentTime;
	}

	// 5. 위 모든 특정 상태가 아니면 일반 주행 또는 불분명 상태
	// 현재 상태가 정지/기울어짐에서 벗어났거나, 움직이는 상태였으나 특정 움직임 임계값 아래로 내려온 경우
	// (충격 상태에서 복귀한 경우도 이 조건에 포함될 수 있음)
	// 현재 v_nextCarState가 충격 상태에서 복귀한 상태가 아니라면
	if (v_nextCarState != G_C100_STATE_BUMP) {
		if (v_nextCarState == g_C100_currentCarState) { // 위에서 상태 변화가 결정되지 않았다면
			if (v_nextCarState == G_C100_STATE_UNKNOWN || // 초기 상태거나
				((g_C100_previousCarState == G_C100_STATE_STOPPED || g_C100_previousCarState == G_C100_STATE_TILTED) && (g_C100_currentCarState != G_C100_STATE_STOPPED && g_C100_currentCarState != G_C100_STATE_TILTED)) // 정지/기울어짐에서 방금 벗어났거나
				)
			{
				v_nextCarState = G_C100_STATE_MOVING_NORMAL;
				g_C100_state_start_time = v_currentTime; // 새로운 상태 진입 시간 기록
			}
			// 움직이는 상태였지만 특정 가감속/회전/충격 임계값 아래로 내려온 경우
			else if (v_isMovingState &&
					 g_C100_currentCarState != G_C100_STATE_ACCELERATING && g_C100_currentCarState != G_C100_STATE_BRAKING &&
					 g_C100_currentCarState != G_C100_STATE_TURNING_LEFT && g_C100_currentCarState != G_C100_STATE_TURNING_RIGHT
					 ) // 충격 상태는 이미 위에서 제외됨
			{
				// 현재 상태가 이미 MOVING_NORMAL 이면 상태 유지
				if (g_C100_currentCarState != G_C100_STATE_MOVING_NORMAL) {
					v_nextCarState = G_C100_STATE_MOVING_NORMAL;
					g_C100_state_start_time = v_currentTime; // 새로운 상태 진입 시간 기록
				}
			}
		}
	}

	// 시리얼 모니터로 차량 상태 변화 확인 (디버깅 시 유용)
	if (v_nextCarState != g_C100_currentCarState) {
		Serial.print("Car state changed from ");
		Serial.print(C100_currentStateToString(g_C100_previousCarState)); // 이전 상태 출력 수정
		Serial.print(" to ");
		Serial.println(C100_currentStateToString(v_nextCarState));
	}

	g_C100_previousCarState = g_C100_currentCarState; // 현재 상태를 이전 상태로 업데이트
	g_C100_currentCarState = v_nextCarState; // 최종 결정된 다음 상태를 현재 상태로 업데이트
}

// 차량 상태에 따라 눈 표현 상태 업데이트 함수
void C100_updateEyeState() {
	unsigned long v_currentTime = millis();
	EyeState v_nextEyeState = g_C100_currentEyeState; // 기본적으로 현재 눈 표현 유지

	// 차량 상태별 눈 표현 결정
	switch (g_C100_currentCarState) {
		case G_C100_STATE_STOPPED:
			// 정지 시간 경과에 따른 눈 표현 변화
			if (g_C100_stop_duration < G_C100_SHORT_STOP_DURATION) {
				v_nextEyeState = G_C100_E_NEUTRAL;
				// 짧은 정지 중에는 깜빡임/둘러보기 타이머 리셋 또는 일시 정지
				// (만약 이전 상태가 정지가 아니었다면 타이머 리셋)
				if(g_C100_previousCarState != G_C100_STATE_STOPPED && g_C100_previousCarState != G_C100_STATE_TILTED) {
                   g_C100_last_blink_time = v_currentTime;
                }

			} else if (g_C100_stop_duration < G_C100_LONG_STOP_DURATION) {
				// 긴 정지: 주기적 깜빡임 또는 둘러보기 애니메이션
				// 애니메이션 중에는 해당 애니메이션 상태 유지
				if (g_C100_currentEyeState == G_C100_E_BLINK) {
					// 깜빡임 애니메이션 지속 시간 체크
					if (v_currentTime - g_C100_last_blink_time > g_C100_blink_duration) {
						v_nextEyeState = G_C100_E_NEUTRAL; // 애니메이션 끝나면 기본으로
						g_C100_last_blink_time = v_currentTime; // 다음 타이머 시작 (랜덤 간격 적용)
						g_C100_blink_interval = random(3000, 7000);
					}
				} else if (g_C100_currentEyeState >= G_C100_E_LOOK_LEFT && g_C100_currentEyeState <= G_C100_E_LOOK_DOWN) { // 둘러보기 상태 범위
					// 둘러보기 애니메이션 지속 시간 체크
					if (v_currentTime - g_C100_last_blink_time > g_C100_look_animation_duration) {
						v_nextEyeState = G_C100_E_NEUTRAL; // 애니메이션 끝나면 기본으로
						g_C100_last_blink_time = v_currentTime; // 다음 타이머 시작 (랜덤 간격 적용)
						g_C100_blink_interval = random(3000, 7000);
					}
				} else { // 현재 G_C100_E_NEUTRAL 상태 등에서 새로운 애니메이션 시작 타이밍 체크
					if (v_currentTime - g_C100_last_blink_time > g_C100_blink_interval) {
						// 무작위로 깜빡임 또는 둘러보기 선택
						if (random(10) < 7) { // 70% 확률로 깜빡임
							v_nextEyeState = G_C100_E_BLINK;
							g_C100_blink_duration = random(100, 300); // 깜빡임 시간 랜덤
						} else { // 30% 확률로 둘러보기
							int v_look_dir = random(4); // 0:LEFT, 1:RIGHT, 2:UP, 3:DOWN
							if (v_look_dir == 0)
								v_nextEyeState = G_C100_E_LOOK_LEFT;
							else if (v_look_dir == 1)
								v_nextEyeState = G_C100_E_LOOK_RIGHT;
							else if (v_look_dir == 2)
								v_nextEyeState = G_C100_E_LOOK_UP;
							else
								v_nextEyeState = G_C100_E_LOOK_DOWN;
							g_C100_look_animation_duration = random(400, 800); // 둘러보기 시간 랜덤
						}
						g_C100_last_blink_time = v_currentTime; // 새 애니메이션 시작 시간 기록
						// 다음 애니메이션 간격은 이미 위에서 랜덤하게 설정됨.
					}
				}
			} else { // 매우 긴 정지
				v_nextEyeState = G_C100_E_SLEEPY;
				// 졸린 눈 상태에서도 아주 느린 깜빡임 등 추가 애니메이션 로직 필요 시 구현
				// 예를 들어, 졸린 눈 깜빡임 타이머 별도 관리
			}
			// 정지 상태 진입 시 blink 타이머 리셋은 짧은 정지에서 처리됨.
			// 긴 정지로 넘어가면 주기적 애니 시작.
			break;

		case G_C100_STATE_MOVING_NORMAL:
			v_nextEyeState = G_C100_E_NEUTRAL; // 일반 주행 시 기본 눈
															 // 주행 중에도 가끔 아주 느린 깜빡임 추가 가능
			if (g_C100_currentEyeState == G_C100_E_BLINK) { // 깜빡임 애니메이션 중이면 지속
				if (v_currentTime - g_C100_last_blink_time > g_C100_blink_duration) {
					v_nextEyeState = G_C100_E_NEUTRAL; // 끝나면 기본으로
					g_C100_last_blink_time = v_currentTime; // 다음 타이머 시작
					g_C100_blink_interval = random(7000, 15000); // 주행 중 깜빡임 간격은 더 길게
				}
			} else { // G_C100_E_NEUTRAL 상태 등에서 깜빡임 시작 타이밍 체크
				if (v_currentTime - g_C100_last_blink_time > g_C100_blink_interval) {
					v_nextEyeState = G_C100_E_BLINK;
					g_C100_blink_duration = random(100, 300);
					g_C100_last_blink_time = v_currentTime;
					g_C100_blink_interval = random(7000, 15000);
				}
			}
			break;

		case G_C100_STATE_ACCELERATING:
			v_nextEyeState = G_C100_E_SQUINT; // 가속 집중/약간 긴장 (눈동자 위치 조정은 drawCurrentEyeState에서 오버레이)
			break;

		case G_C100_STATE_BRAKING:
			// 급감속 시작 시 순간적으로 강한 표현 (ABSURD 또는 ANGRY) 후 찡그림(SQUINT_TIGHT)으로 전환
			if (g_C100_currentCarState != g_C100_previousCarState) { // 감속 상태로 방금 진입
				v_nextEyeState = G_C100_E_ABSURD; // 또는 G_C100_E_ANGRY (선택)
				// 상태 시작 시간 기록은 inferCarState에서 이미 함.
			} else { // 감속 상태 유지 중
				// ABSURD/ANGRY 상태를 짧게 유지 후 SQUINT_TIGHT로 전환
				if ((g_C100_currentEyeState == G_C100_E_ABSURD || g_C100_currentEyeState == G_C100_E_ANGRY) && (v_currentTime - g_C100_state_start_time) > 500) { // 예시: 0.5초 유지
						v_nextEyeState = G_C100_E_SQUINT_TIGHT;
				} else if (!(g_C100_currentEyeState == G_C100_E_ABSURD || g_C100_currentEyeState == G_C100_E_ANGRY)) {
                    // 이미 SQUINT_TIGHT 상태이거나, 처음부터 SQUINT_TIGHT로 설정된 경우 유지
					v_nextEyeState = G_C100_E_SQUINT_TIGHT;
				}
				// 그 외의 경우는 현재 눈 상태(ABSURD/ANGRY) 유지 (0.5초 동안)
			}
			// 눈동자 순간 이동 애니메이션은 C100_drawCurrentEyeState 함수에서 G_C100_STATE_BRAKING일 때 처리
			break;

		case G_C100_STATE_TURNING_LEFT:
			v_nextEyeState = G_C100_E_LOOK_LEFT; // 왼쪽 보기 (찡그림 오버레이는 drawCurrentEyeState)
			break;

		case G_C100_STATE_TURNING_RIGHT:
			v_nextEyeState = G_C100_E_LOOK_RIGHT; // 오른쪽 보기 (찡그림 오버레이는 drawCurrentEyeState)
			break;

		case G_C100_STATE_BUMP:
			// 충격 시작 시 순간적으로 강한 표현 (ABSURD 또는 GLARING)
			if (g_C100_currentCarState != g_C100_previousCarState) { // 충격 상태로 방금 진입
				v_nextEyeState = G_C100_E_GLARING; // 또는 G_C100_E_ABSURD (선택)
				// 상태 시작 시간 기록은 inferCarState에서 이미 함.
			}
			// 충격 상태는 C100_inferCarState에서 짧게 유지하고 다른 상태로 빠르게 전환되므로, 여기서 별도 시간 체크는 최소화
			// 눈동자 순간 이동 애니메이션은 C100_drawCurrentEyeState 함수에서 G_C100_STATE_BUMP일 때 처리
			break;

		case G_C100_STATE_TILTED:
			// 기울어짐 상태에서는 눈동자 위치만 기울기 방향으로 조정 (drawCurrentEyeState에서 오버레이)
			v_nextEyeState = G_C100_E_NEUTRAL; // 눈 형태는 기본으로 유지
			break;

		case G_C100_STATE_UNKNOWN:
			v_nextEyeState = G_C100_E_CONFUSED; // 초기 또는 알 수 없는 상태 시 혼란스러운 눈
			break;

			// case G_C100_STATE_REVERSING: // MPU만으로는 어려움
			// break;
	}

	// 최종 결정된 눈 표현 상태 업데이트
	if (v_nextEyeState != g_C100_currentEyeState) {
		Serial.print("Eye state changed to "); // 이전 눈 상태 출력은 제거
		Serial.println(C100_eyeStateToString(v_nextEyeState));
		g_C100_currentEyeState = v_nextEyeState;
		// 필요하다면 상태 변화 시 애니메이션 시작 시간 등 초기화
		// (예: 깜빡임/둘러보기 타이머는 정지 상태에서만 유효하게 작동하도록 로직 보강 - 위에서 이미 일부 처리)
		// (예2: 충격/감속 애니메이션은 상태 진입 시 한 번만 실행되도록 플래그 사용 - 현재는 state_start_time 활용)
	}
}

// 현재 눈 표현 상태에 맞는 눈을 LED 매트릭스에 그리는 함수
// 이 함수는 FastLED 함수들과 C120_getLedIndex를 사용하여 픽셀을 그림
void C100_drawCurrentEyeState() {
	unsigned long v_currentTime = millis(); // 현재 시간 (애니메이션에 사용)

	// 각 눈(매트릭스)별로 그리기 함수 호출
	for (uint8_t v_eye = 0; v_eye < G_C120_NUM_MATRICES; v_eye++) {
		// 현재 눈 상태에 따라 해당 그리기 함수 호출
		// 여기서 눈 모양을 먼저 그리고, 필요한 경우 아래에서 특정 차량 상태에 따른 오버레이를 그립니다.
		switch (g_C100_currentEyeState) {
			case G_C100_E_NEUTRAL:
				C120_drawEyeNeutral(v_eye);
				break;
			case G_C100_E_BLINK:
				// 깜빡임 애니메이션 진행 단계에 따라 그리기
				C120_drawEyeBlink(v_eye, v_currentTime - g_C100_last_blink_time);
				break;
			case G_C100_E_LOOK_LEFT:
				C120_drawEyeLook(v_eye, -1, 0); // 예시: 왼쪽 방향 지시 (-1, 0)
				break;
			case G_C100_E_LOOK_RIGHT:
				C120_drawEyeLook(v_eye, 1, 0); // 예시: 오른쪽 방향 지시 (1, 0)
				break;
			case G_C100_E_LOOK_UP:
				C120_drawEyeLook(v_eye, 0, -1); // 예시: 위쪽 방향 지시 (0, -1)
				break;
			case G_C100_E_LOOK_DOWN:
				C120_drawEyeLook(v_eye, 0, 1); // 예시: 아래쪽 방향 지시 (0, 1)
				break;
			case G_C100_E_SQUINT:
				C120_drawEyeSquint(v_eye, false); // 약하게 찡그림 (별도 오버레이 로직 없이 이 함수가 다 그림)
				break;
			case G_C100_E_SQUINT_TIGHT:
				C120_drawEyeSquint(v_eye, true); // 강하게 찡그림 (별도 오버레이 로직 없이 이 함수가 다 그림)
				break;
			case G_C100_E_ANGRY:
				C120_drawEyeAngry(v_eye);
				break;
			case G_C100_E_ABSURD:
				C120_drawEyeAbsurd(v_eye);
				// 충격/감속 시 눈동자 순간 이동 애니메이션 로직 필요 시 C120_drawEyeAbsurd 내에서 시간(v_currentTime - g_C100_state_start_time) 활용하여 구현
				break;
			case G_C100_E_GLARING:
				C120_drawEyeGlaring(v_eye);
				break;
			case G_C100_E_SLEEPY:
				C120_drawEyeSleepy(v_eye);
				break;
			case G_C100_E_CONFUSED:
				C120_drawEyeConfused(v_eye);
				break;
				// ... 기타 눈 모양 상태 처리 ...
		}
	}

	// 특정 차량 상태일 때 눈 모양 외 추가적인 그리기 또는 오버레이
	// 이 부분은 위에 그린 기본 눈 모양 위에 덧그리는 방식이 될 수 있습니다.
	// 단, 위에 그린 눈 모양 함수들이 전체 8x8을 덮어쓰므로, 오버레이를 그리려면
	// 기본 눈 모양을 다시 그리거나, 오버레이를 그리기 전에 해당 픽셀을 지우는 등의 처리가 필요합니다.
	// 현재는 간결성을 위해 오버레이를 별도 EyeState로 관리하거나,
	// drawCurrentEyeState에서 필요한 경우 기본 눈을 그린 후 눈동자만 다시 그리는 방식으로 구현합니다.

	// 기울어짐 상태일 때 눈동자 위치 조정 오버레이 (기본 눈 상태에서만)
	if (g_C100_currentCarState == G_C100_STATE_TILTED && g_C100_currentEyeState == G_C100_E_NEUTRAL) {
		// 정지 시 기울어짐 상태에서는 기본 눈 형태(G_C100_E_NEUTRAL)를 유지하고 눈동자 위치만 기울기 방향으로 조정
		for (uint8_t v_eye = 0; v_eye < G_C120_NUM_MATRICES; v_eye++) {
			// g_C110_accel_roll, g_C110_accel_pitch 값을 이용하여 눈동자 상대 위치 계산 후 그리기
			// 맵핑 범위는 실제 테스트 후 튜닝 필요
			// map 함수는 long 타입을 사용하므로 float을 int로 변환 또는 캐스팅
			int8_t v_pupil_offset_x = map((long)(g_C110_accel_roll * 10.0), -200L, 200L, -2, 2); // 예시: 롤 각도 -20~20도를 10배하여 -200~200으로 확장 후 -2~2으로 매핑 (상수에도 L 접미사 추가)
			int8_t v_pupil_offset_y = map((long)(g_C110_accel_pitch * 10.0), -200L, 200L, -2, 2); // 예시: 피치 각도 -20~20도를 10배하여 -200~200으로 확장 후 -2~2으로 매핑
			v_pupil_offset_x = constrain(v_pupil_offset_x, -2, 2); // 오프셋 범위 제한 (8x8 매트릭스에 맞게)
			v_pupil_offset_y = constrain(v_pupil_offset_y, -2, 2);

			// 눈동자만 다시 그리는 로직 (기존 눈 모양 위에 덧그림)
			int v_center_x = G_C120_MATRIX_WIDTH / 2;
			int v_center_y = G_C120_MATRIX_HEIGHT / 2;
			int v_pupil_radius_sq = 1 * 1; // 눈동자 크기 (기본 눈동자와 동일하게)

			int v_pupil_center_x = v_center_x + v_pupil_offset_x;
			int v_pupil_center_y = v_center_y + v_pupil_offset_y;

			// 기존 눈동자 영역을 지우고 새 위치에 그리는 방식
            // C120_drawEyeNeutral이 눈동자를 그렸다고 가정하고, 해당 영역을 Black으로 지움
            for (int v_py = -1; v_py <= 1; v_py++) { // 기본 눈동자 영역 (가운데 3x3 대략)
				for (int v_px = -1; v_px <= 1; v_px++) {
                     C120_drawPixel(v_eye, v_center_x + v_px, v_center_y + v_py, CRGB::Black);
                }
            }

			// 새로운 눈동자 위치에 그리기
			for (int v_py = -1; v_py <= 1; v_py++) {
				for (int v_px = -1; v_px <= 1; v_px++) {
					// 원형 눈동자
					if (v_px * v_px + v_py * v_py <= v_pupil_radius_sq + 0.5) { // 약간 여유있는 반경
						C120_drawPixel(v_eye, v_pupil_center_x + v_px, v_pupil_center_y + v_py, CRGB::White); // 눈동자 색상
					}
				}
			}
		}
	}
    // 가속 상태일 때 눈동자 위치 조정 오버레이 (약하게 찡그린 눈 상태에서만)
	else if (g_C100_currentCarState == G_C100_STATE_ACCELERATING && g_C100_currentEyeState == G_C100_E_SQUINT) {
		// 가속 시 찡그림(G_C100_E_SQUINT)과 함께 눈동자를 중앙 또는 약간 위로 모으는 효과
		for (uint8_t v_eye = 0; v_eye < G_C120_NUM_MATRICES; v_eye++) {
			// 눈동자만 따로 그리는 함수를 만들거나, C120_drawEyeLook 로직을 여기에 가져와 눈동자 픽셀만 그리기
			// 여기서는 간단히 눈동자 위치를 재지정하여 덧그림
			int v_center_x = G_C120_MATRIX_WIDTH / 2;
			int v_center_y = G_C120_MATRIX_HEIGHT / 2;
			int v_pupil_radius_sq = 1 * 1; // 눈동자 크기

			// 눈동자 위치 계산 (약간 위로 모으는 효과)
            // 가속 강도에 비례하여 오프셋 조절도 가능
			int v_pupil_center_x = v_center_x;
			int v_pupil_center_y = v_center_y - 1; // 눈동자를 1픽셀 위로 이동

            // 기존 눈동자 영역 (찡그린 눈의 눈동자 위치)을 지우고 새로 그리는 방식 필요
            // C120_drawEyeSquint 함수는 눈동자를 그리지 않으므로 지울 필요 없음.
            // 그냥 새로운 위치에 눈동자만 그리면 됨.

			// 눈동자 그리기
			for (int v_py = -1; v_py <= 1; v_py++) { // 3x3 픽셀 영역에서 눈동자 그리기
				for (int v_px = -1; v_px <= 1; v_px++) {
					// 원형 눈동자
					if (v_px * v_px + v_py * v_py <= v_pupil_radius_sq + 0.5) { // 약간 여유있는 반경
						C120_drawPixel(v_eye, v_pupil_center_x + v_px, v_pupil_center_y + v_py, CRGB::White);
					}
				}
			}
		}
	}
    // 회전 시 찡그림 오버레이 (LOOK_LEFT/RIGHT 상태일 때) - 기존 로직 유지
    else if ((g_C100_currentCarState == G_C100_STATE_TURNING_LEFT && g_C100_currentEyeState == G_C100_E_LOOK_LEFT) ||
        (g_C100_currentCarState == G_C100_STATE_TURNING_RIGHT && g_C100_currentEyeState == G_C100_E_LOOK_RIGHT)) {

        // 회전 강도(자이로 Z값)에 비례하여 찡그림 강도 조절 가능
        // 자이로 임계값부터 최대 회전 값까지 0.0 ~ 1.0으로 매핑
        float v_squint_intensity = map(abs(g_C110_filtered_gz), (float)G_C110_GYRO_TURN_THRESHOLD_DEGS, (float)G_C110_GYRO_TURN_MAX_SQUINT_DEGS, 0.0, 1.0);
        v_squint_intensity = constrain(v_squint_intensity, 0.0, 1.0); // 강도 범위를 0~1로 제한

        // 찡그림 오버레이 그리기 (눈꺼풀 라인)
        // 강도에 따라 눈꺼풀 라인의 Y 위치를 조절
        int v_center_y = G_C120_MATRIX_HEIGHT / 2; // 4

        // 찡그림 강도에 따라 눈꺼풀이 내려오는 정도를 계산 (0% 강도일 때 Y=3, 100% 강도일 때 Y=4)
        int v_squint_y_top = v_center_y - 1 + (int)(v_squint_intensity * 1.0); // 찡그림 상단 라인 Y
        //int v_squint_y_bottom = v_center_y + 1 - (int)(v_squint_intensity * 1.0); // 찡그림 하단 라인 Y (약간 위로 올라옴) // 하단 라인은 생략

        // Y 좌표 범위를 벗어나지 않도록 제한
        v_squint_y_top = constrain(v_squint_y_top, v_center_y - 1, v_center_y + 1);
        //v_squint_y_bottom = constrain(v_squint_y_bottom, v_center_y - 1, v_center_y + 1);


        for (uint8_t v_eye = 0; v_eye < G_C120_NUM_MATRICES; v_eye++) {
             // 눈꺼풀 라인 그리기 (기존 눈 모양 위에 덧그림)
             // 눈동자 주변 픽셀은 덮어쓰지 않도록 주의해야 하지만, 단순화하여 전체 너비에 그림.
             // 더 정교하게 하려면 눈동자 위치를 피해서 그려야 함.
             for (uint8_t v_x = 0; v_x < G_C120_MATRIX_WIDTH; v_x++) {
                 // 눈동자가 있는 위치는 제외하고 그리기 (예시)
                 int v_pupil_center_x = G_C120_MATRIX_WIDTH / 2 + ( (g_C100_currentCarState == G_C100_STATE_TURNING_LEFT) ? -2 : 2 ); // LOOK_LEFT/RIGHT의 눈동자 위치
                 int v_pupil_center_y = G_C120_MATRIX_HEIGHT / 2; // LOOK_LEFT/RIGHT의 눈동자 위치 (중앙)
                 if (!((v_x >= v_pupil_center_x - 1 && v_x <= v_pupil_center_x + 1) && (v_squint_y_top >= v_pupil_center_y - 1 && v_squint_y_top <= v_pupil_center_y + 1)) ) {
                     C120_drawPixel(v_eye, v_x, v_squint_y_top, CRGB::White); // 눈 색상으로 라인 그림
                 }

                 // 강도가 높을수록 아래쪽 라인도 추가
                 if (v_squint_intensity > 0.5) {
                     int v_squint_y_top_plus_1 = v_squint_y_top + 1;
                     if (!((v_x >= v_pupil_center_x - 1 && v_x <= v_pupil_center_x + 1) && (v_squint_y_top_plus_1 >= v_pupil_center_y - 1 && v_squint_y_top_plus_1 <= v_pupil_center_y + 1)) ) {
                         C120_drawPixel(v_eye, v_x, v_squint_y_top_plus_1, CRGB::White); // 바로 아래 라인 추가
                     }
                 }
             }
             // 하단 눈꺼풀 라인도 유사하게 추가 가능 (필요 시)
        }
    }
	// ... 기타 차량 상태별 추가 표현 ...
}

// --- 설정 (Setup) 함수 ---
void C100_init() { // Arduino 스케치 기본 setup 함수

	Serial.println("Starting C100 Robot Eye Project...");

	Wire.begin(G_C110_MPU_I2C_SDA, G_C110_MPU_I2C_SCL);	 // MPU I2C 통신 시작

	// MPU6050 초기화
	if (!g_C110_mpu.begin()) {
		Serial.println("MPU6050 센서 초기화 실패! 배선 및 전원을 확인하세요.");
		while (1) delay(10); // 센서 초기화 실패 시 무한 대기
	}
	Serial.println("MPU6050 센서 초기화 완료.");

	// MPU6050 설정 (측정 범위 및 필터 대역폭 설정)
	// 설정에 따라 임계값 (Threshold) 튜닝 필수!
    // 내부 디지털 필터 활성화 및 대역폭 설정 (센서 데이터 자체 필터링)
	g_C110_mpu.setAccelerometerRange(MPU6050_RANGE_8_G); // 가속도 범위 설정 (예: +/- 8g)
	g_C110_mpu.setGyroRange(MPU6050_RANGE_500_DEG);		 // 자이로 범위 설정 (예: +/- 500 deg/s)
	g_C110_mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);	 // 디지털 저역 통과 필터 설정
	// MPU6050_BAND_260_HZ (필터링 약함) ~ MPU6050_BAND_5_HZ (필터링 강함)
    Serial.print("MPU6050 Filter Bandwidth set to 21 Hz.");

	// FastLED 초기화
	FastLED.addLeds<G_C120_LED_TYPE, G_C120_LED_DATA_PIN, G_C120_COLOR_ORDER>(g_C120_leds, G_C120_TOTAL_NUM_LEDS).setCorrection(TypicalLEDStrip); // LED 설정
	FastLED.setBrightness(80); // 전체 밝기 설정 (0-255), 너무 밝으면 눈부심 및 전력 소모 증가

	// 초기 필터 값 설정 (첫 센서 값으로 초기화)
	C110_readMPUData();	 // 초기 값 한번 읽기 (내부 필터가 적용된 값)
	g_C110_filtered_ax		= g_C110_a.acceleration.x;
	g_C110_filtered_ay		= g_C110_a.acceleration.y;
	g_C110_filtered_az		= g_C110_a.acceleration.z;
	g_C110_filtered_gx		= g_C110_g.gyro.x;
	g_C110_filtered_gy		= g_C110_g.gyro.y;
	g_C110_filtered_gz		= g_C110_g.gyro.z;
	// g_C110_last_filter_time = millis(); // 상보 필터 사용 시 초기화

	g_C100_currentCarState	= G_C100_STATE_UNKNOWN;	 // 초기 차량 상태
	g_C100_previousCarState = G_C100_STATE_UNKNOWN;
	g_C100_currentEyeState	= G_C100_E_CONFUSED;	 // 초기 눈 표현은 CONFUSED로 시작

	g_C100_state_start_time = millis();	 // 상태 시작 시간 기록 시작
	g_C100_last_blink_time	= millis();	 // 깜빡임 타이머 시작
	randomSeed(analogRead(0));			 // 랜덤 시드 초기화 (무작위 깜빡임/둘러보기 사용 시)

	Serial.println("Setup complete. Starting loop...");
}

// --- 메인 루프 (Loop) 함수 ---
void C100_run() { // Arduino 스케치 기본 loop 함수
	unsigned long v_currentTime = millis();	 // 현재 시간

	// 1. MPU 데이터 읽기 (내부 필터 적용 값)
	C110_readMPUData();

	// 2. 소프트웨어 필터링 적용 (선택 사항, MPU 내부 필터 후 추가 스무딩) 및 기울기 계산
	C110_applyFiltering(); // 이 함수 내에서 소프트웨어 LPF와 가속도 기반 기울기 계산 수행

    // 상보 필터를 사용하는 경우, C110_applyFiltering 내부 로직을 수정하거나 이 위치에서 호출
    // if (g_C110_last_filter_time == 0) g_C110_last_filter_time = v_currentTime; // 초기화
    // float v_dt = (v_currentTime - g_C110_last_filter_time) / 1000.0; // 시간 간격 (초)
    // C110_complementaryFilter(g_C110_a.acceleration.x, g_C110_a.acceleration.y, g_C110_a.acceleration.z,
    //                          g_C110_g.gyro.x, g_C110_g.gyro.y, g_C110_g.gyro.z, v_dt);
    // g_C110_last_filter_time = v_currentTime; // 시간 업데이트

	// 3. 차량 상태 추론
	C100_inferCarState();

	// 4. 눈 표현 상태 업데이트 (상태 머신)
	C100_updateEyeState();

	// 5. 디스플레이에 눈 그리기
	C120_clearDisplay();		 // 매 프레임 LED 버퍼 초기화
	C100_drawCurrentEyeState();	 // 현재 눈 상태에 맞는 눈을 그림
	FastLED.show();				 // LED 업데이트 내용을 실제 LED에 표시

	// 센서 읽기 및 루프 실행 간격 제어 (너무 빠르면 필터링 효과 감소 및 부하 증가)
	// MPU6050의 샘플링 속도와 처리할 내용에 따라 적절히 조절
	delay(20);	// 예시: 20ms 간격 (약 50 FPS)
}
