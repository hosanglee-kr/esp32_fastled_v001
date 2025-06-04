
// ====================================================================================================
// 커스텀 디버그 매크로 정의
// ====================================================================================================

// print 계열: F() 매크로 사용 가능, 개행 여부 및 함수 정보 출력 여부 제어
#define A01_DEBUG_print(v_debugType, v_newline, v_funcName, x, ...) {      \
    if (v_funcName == 'Y') {                                               \
        Serial.print(v_debugType);                                         \
        Serial.printf("{%s.%s()- line# %05d} " , __FILE__, __func__, __LINE__ ); \
    }                                                                      \
    Serial.print(x, ##__VA_ARGS__);                                        \
    if (v_newline == 'Y') {                                                \
        Serial.println();                                                  \
    }                                                                      \
} 

// printf 계열: 포맷 문자열과 가변 인자 사용, 함수 정보 출력 여부 제어
#define A01_DEBUG_printf(v_debugType, v_funcName, fmt, ...) {              \
    if (v_funcName == 'Y') {                                               \
        Serial.print((v_debugType));                                       \
        Serial.printf((" %s.%s()- line# %05d") , __FILE__, __func__, __LINE__); \
    }                                                                      \
    Serial.printf(fmt, ##__VA_ARGS__);                                     \
}

// v2 버전 (개선된 상세 정보 출력)
#define A01_DEBUG_print_v2(v_debugType, v_newline, v_funcName, x, ...) {   \
    if (v_funcName == 'Y') {                                               \
        Serial.print(v_debugType);                                         \
        Serial.printf("[%s:%s() line# %05d] ", __FILE__, __func__, __LINE__); /* 수정: 간결하게 라인 정보 출력 */ \
    }                                                                      \
    Serial.print(x, ##__VA_ARGS__);                                        \
    if (v_newline == 'Y') {                                                \
        Serial.println();                                                  \
    }                                                                      \
} 

// 특정 모듈/레벨 디버그 출력 매크로 정의 (DEBUG_P1 활성화 시)
#ifdef DEBUG_P1
    #define dbgP1_print_F(x, ...)       A01_DEBUG_print("[P1]", 'N', 'Y', x, ##__VA_ARGS__) // 함수 정보 포함, 개행 없음, F() 사용
    #define dbgP1_print(x, ...)         A01_DEBUG_print("[P1]", 'N', 'N', x, ##__VA_ARGS__) // 함수 정보 없음, 개행 없음

    #define dbgP1_println_F(x, ...)     A01_DEBUG_print("[P1]", 'Y', 'Y', x, ##__VA_ARGS__) // 함수 정보 포함, 개행 포함, F() 사용
    #define dbgP1_println(x, ...)       A01_DEBUG_print("[P1]", 'Y', 'N', x, ##__VA_ARGS__) // 함수 정보 없음, 개행 포함

    #define dbgP1_printf_F(fmt, ...)    A01_DEBUG_printf("[P1]", 'Y', fmt, ##__VA_ARGS__)    // 함수 정보 포함, printf 스타일
    #define dbgP1_printf(fmt, ...)      A01_DEBUG_printf("[P1]", 'N', fmt, ##__VA_ARGS__)    // 함수 정보 없음, printf 스타일
#else
    // DEBUG_P1이 비활성화되면 모든 dbgP1_ 매크로는 아무것도 하지 않음
    #define dbgP1_print_F(x, ...)       
    #define dbgP1_print(x, ...)         

    #define dbgP1_println_F(x, ...)     
    #define dbgP1_println(x, ...)       

    #define dbgP1_printf_F(fmt, ...)    
    #define dbgP1_printf(fmt, ...)      
#endif

// DEBUG_P2 (현재 비활성화 - 필요 시 위와 동일한 방식으로 정의)
#ifdef DEBUG_P2
    // #define dbgP2_print_F(...) 등 정의
#else
    // #define dbgP2_print_F(...) 정의
#endif
