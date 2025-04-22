#include <Arduino.h>

//#define F100
#ifdef F100
	#include "F100_fxWave2D_002.h"
#endif
	
//#define F110
#ifdef F110
	#include "F110_wave2D_002.h"
#endif

//#define F120
#ifdef F120
	#include "F120_Blur2d_001.h"
#endif

//#define F130
#ifdef F130
	#include "F130_Wave2d_001.h"
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
}
