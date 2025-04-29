// C120_MTS_002.H
#ifndef C120_MTS_002_H
#define C120_MTS_002_H

#include <Arduino.h>

class C120_MTX_Matrix {
public:
    // 생성자
    // 생성자는 헤더 파일에 구현 가능
    // p_width: 매트릭스 너비, p_height: 매트릭스 높이
    C120_MTX_Matrix(int p_width, int p_height) : v_matrixWidth(p_width), v_matrixHeight(p_height) {
        // 픽셀 배열 메모리 할당 (개념적 구현)
        // 실제 라이브러리에서는 필요 없을 수 있습니다.
        // 헤더 파일에 동적 할당을 넣는 것은 권장되지 않지만, 예시를 위해 포함합니다.
        v_pixels = new int*[v_matrixWidth];
        for (int v_i = 0; v_i < v_matrixWidth; ++v_i) {
            v_pixels[v_i] = new int[v_matrixHeight];
        }
    }

    // 소멸자 (메모리 해제) - 실제 라이브러리 사용 시 필요 없을 수 있습니다.
    // 헤더 파일에 소멸자를 넣는 것은 권장되지 않지만, 예시를 위해 포함합니다.
    // ~C120_MTX_Matrix() {
    //     for (int v_i = 0; v_i < v_matrixWidth; ++v_i) {
    //         delete[] v_pixels[v_i];
    //     }
    //     delete[] v_pixels;
    // }

    // LED 매트릭스 초기화 (사용하는 라이브러리에 맞게 구현)
    // 함수 구현부를 헤더 파일에 직접 작성
    void C120_MTX_begin() {
        // LED 매트릭스 초기화 코드 (사용하는 라이브러리 함수 호출)
        // 예: someLedMatrixLibrary.begin();
        // 예: FastLED.addLeds<...>(...).setCorrection(TypicalLEDStrip);
        Serial.println("Conceptual C120_MTX_begin() called.");
    }

    // 매트릭스 전체 클리어 (모든 픽셀 끄기)
    // 함수 구현부를 헤더 파일에 직접 작성
    void C120_MTX_clear() {
        // 매트릭스 전체 픽셀 끄기 (사용하는 라이브러리 함수 호출 또는 픽셀 배열 초기화)
        // 예: someLedMatrixLibrary.clear();
        // 예: FastLED.clear();
        for (int v_i = 0; v_i < v_matrixWidth; ++v_i) {
            for (int v_j = 0; v_j < v_matrixHeight; ++v_j) {
                C120_MTX_setPixel(v_i, v_j, 0); // 모든 픽셀 끄기
            }
        }
        Serial.println("Conceptual C120_MTX_clear() called.");
    }

    // 특정 픽셀 켜기
    // 함수 구현부를 헤더 파일에 직접 작성
    // p_x: 픽셀 X 좌표, p_y: 픽셀 Y 좌표, p_color: 색상 (사용하는 라이브러리에 따라 다름, 예: 1 for ON, 0 for OFF)
    void C120_MTX_setPixel(int p_x, int p_y, int p_color) {
        // 특정 픽셀 색상 설정 (사용하는 라이브러리 함수 호출 또는 픽셀 배열 업데이트)
        // 좌표 유효성 검사
        if (p_x >= 0 && p_x < v_matrixWidth && p_y >= 0 && p_y < v_matrixHeight) {
            // 예: someLedMatrixLibrary.drawPixel(p_x, p_y, p_color);
            // 예: FastLED[xyToLinear(p_x, p_y)] = p_color; // 좌표 변환 함수 필요
            // 개념적 구현: 픽셀 배열에 저장
            // v_pixels[p_x][p_y] = p_color;
            // Serial.print("Setting pixel ("); Serial.print(p_x); Serial.print(","); Serial.print(p_y); Serial.print(") to "); Serial.println(p_color);
        }
    }

    // 눈 그리기 함수
    // 함수 구현부를 헤더 파일에 직접 작성
    // p_pupilX: 눈동자 X 위치 (매트릭스 중앙 기준 상대 좌표)
    // p_pupilY: 눈동자 Y 위치 (매트릭스 중앙 기준 상대 좌표)
    void C120_MTX_drawEye(int p_pupilX, int p_pupilY) {
        // 매트릭스 중앙 좌표 계산
        int v_centerX = v_matrixWidth / 2;
        int v_centerY = v_matrixHeight / 2;

        // 눈 모양 그리기 (개념적: 큰 원)
        // 실제로는 더 복잡한 눈 모양을 그릴 수 있습니다.
        int v_eyeRadius = min(v_matrixWidth, v_matrixHeight) / 2 - 1; // 매트릭스 크기에 맞게 조정
        C120_MTX_drawCircle(v_centerX, v_centerY, v_eyeRadius, 1); // 흰색 눈 (개념적)

        // 눈동자 위치 계산 (중앙 기준 상대 좌표 + 중앙 좌표)
        int v_actualPupilX = v_centerX + p_pupilX;
        int v_actualPupilY = v_centerY + p_pupilY;

        // 눈동자 그리기 (개념적: 작은 원)
        int v_pupilRadius = G_C120_MTX_PUPIL_SIZE / 2;
        C120_MTX_drawCircle(v_actualPupilX, v_actualPupilY, v_pupilRadius, 1); // 검은색 눈동자 (개념적)

        // 이 함수 내에서 실제 LED 매트릭스 라이브러리의 그리기 함수를 호출해야 합니다.
        // 예:
        // someLedMatrixLibrary.fillScreen(BLACK); // 배경 클리어
        // someLedMatrixLibrary.drawCircle(v_centerX, v_centerY, v_eyeRadius, WHITE); // 눈 흰자위
        // someLedMatrixLibrary.fillCircle(v_actualPupilX, v_actualPupilY, v_pupilRadius, BLACK); // 눈동자
    }

private:
    int v_matrixWidth;
    int v_matrixHeight;

    // LED 매트릭스 픽셀 데이터를 저장할 2D 배열 (개념적)
    // 실제 라이브러리에서는 내부적으로 관리할 수 있습니다.
    int** v_pixels; // 헤더 파일에 동적 할당 멤버 변수를 두는 것은 주의해야 합니다.

    // 눈동자 크기 (픽셀 단위)
    const int G_C120_MTX_PUPIL_SIZE = 4; // 예시: 4x4 픽셀 크기의 눈동자

    // 눈 모양을 그리는 헬퍼 함수 (개념적)
    // 여기서는 단순한 원 형태의 눈과 눈동자를 그립니다.
    // 함수 구현부를 헤더 파일에 직접 작성
    void C120_MTX_drawCircle(int p_centerX, int p_centerY, int p_radius, int p_color) {
        // 이 함수는 개념적인 구현입니다. 실제 LED 매트릭스 라이브러리의 원 그리기 함수를 사용하세요.
        // 예: someLedMatrixLibrary.drawCircle(p_centerX, p_centerY, p_radius, p_color);
        Serial.print("Conceptual C120_MTX_drawCircle() called at ("); Serial.print(p_centerX); Serial.print(","); Serial.print(p_centerY); Serial.print(") with radius "); Serial.print(p_radius); Serial.print(" and color "); Serial.println(p_color);

        // 간단한 예시 (모든 픽셀을 검사하여 원 안에 있는지 확인) - 매우 비효율적
        // for (int v_x = p_centerX - p_radius; v_x <= p_centerX + p_radius; ++v_x) {
        //     for (int v_y = p_centerY - p_radius; v_y <= p_centerY + p_radius; ++v_y) {
        //         int v_dx = v_x - p_centerX;
        //         int v_dy = v_y - p_centerY;
        //         if (v_dx * v_dx + v_dy * v_dy <= p_radius * p_radius) {
        //             C120_MTX_setPixel(v_x, v_y, p_color);
        //         }
        //     }
        // }
    }
};

#endif
