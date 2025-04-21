#include <Arduino.h>

// #define F100
#ifdef F100
	#include "F100_fxWave2D_001.h"
#endif
	
#define F110
#ifdef F110
	#include "F110_wave2D_001.h"
#endif

void setup() {
	// delay(5000);

	Serial.begin(115200);
	
	#ifdef F100
	     F100_init();
    #endif

    #ifdef F110
	     F110_init();
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
}
