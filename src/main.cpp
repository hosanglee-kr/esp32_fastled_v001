#include <Arduino.h>

//#define F100
#ifdef F100
	#include "Fxx_basicExample/F100_fxWave2D_003.h"
#endif
	
//#define F110
#ifdef F110
	#include "Fxx_basicExample/F110_wave2D_002.h"
#endif

//#define F120
#ifdef F120
	#include "Fxx_basicExample/F120_Blur2d_001.h"
#endif

//#define F130
#ifdef F130
	#include "Fxx_basicExample/F130_Wave2d_001.h"
#endif

#ifdef F140
	#include "Fxx_basicExample/F140_fxWater_001.h"
#endif

#ifdef R100
	#include "R100_RobotEyes_T10_002.h"
#endif

//#define R110
#ifdef R200
	#include "R200_RobotEyes_T20_002.h"
#endif


#ifdef R210
	#include "R210_RobotEyes_T20_004_singleColor.h"
#endif

#ifdef R220
	#include "R220_RobotEyes_T20_004_multiColor.h"
#endif

#ifdef R300
    #include "R300_RobotEyes_T30_006/R300_Main_010.h"
#endif


#ifdef R310
    #include "R310_RobotEyes_T31_008/R310_main_015.h"
    // #include "R310_RobotEyes_T31_008/R310_Main_013.h"
#endif

#ifdef M010
    #include "M010_CarState_001/M010_main3_012.h"
    // #include "M010_CarState_001/M010_main3_009.h"
    // #include "M010_CarState_001/M010_main3_008.h"
	// #include "M010_CarState_001/M010_main2_007.h"
    // #include "M010_CarState_001/M010_main_005.h"
#endif

#ifdef W010
	#include "M010_CarState_001/W010_ESPUI_012.h"
    //#include "M010_CarState_001/W010_embUI_002.h"
#endif



#ifdef C100
    #include "C100_CarEyes_v003/C100_CarEyes_Main_001.h"
	//#include "C100_CarEyes_001.h"
#endif

void setup() {

	Serial.begin(115200);
	
	#ifdef F100
	     F100_init();
    #endif

    #ifdef F110
	     F110_init();
    #endif

	#ifdef F120
		F120_init();
	#endif

	#ifdef F130
		F130_init();
	#endif

	
	#ifdef F140
		F140_init();
	#endif

	#ifdef R100
		R100_init();
	#endif

	#ifdef R200
		R200_init();
	#endif

	#ifdef R210
		R210_init();
	#endif

	#ifdef R220
		R220_init();
	#endif

	#ifdef R300
	    R300_init();
	#endif

	#ifdef R310
	    R310_init();
	#endif

	
	#ifdef M010
	    M010_init();
	#endif
	
	#ifdef W010
	    W010_EmbUI_init();                                     // EmbUI 초기화 및 Wi-Fi 설정
        W010_EmbUI_setupWebPages();                            // 웹페이지 UI 구성 및 설정 항목 바인딩
        W010_EmbUI_loadConfigToWebUI();
	#endif

	#ifdef C100
		C100_init();
	#endif


	Serial.println("11111");
}

void loop() {
	#ifdef F100
	     F100_run();
    #endif
	
    #ifdef F110
	     F110_run();
    #endif

    #ifdef F120
	     F120_run();
    #endif

    #ifdef F130
	     F130_run();
    #endif

	#ifdef F140
		F140_init();
	#endif

	#ifdef R100
		R100_run();
	#endif

	#ifdef R200
		R200_run();
	#endif


	#ifdef R210
		R210_run();
	#endif

	#ifdef R220
		R220_run();
	#endif

	#ifdef R300
	    R300_run();
	#endif

	#ifdef R310
	    R310_run();
	#endif

	#ifdef M010
	    M010_run();
	#endif

	#ifdef W010
	    W010_EmbUI_run();
	#endif
	
	#ifdef C100
		C100_run();
	#endif
}
