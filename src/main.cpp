#include <Arduino.h>

// The build version comes from an environment variable. Use the VERSION
// define wherever the version is needed.


#define S10
#ifdef S10
	#include "S10.h"
#endif

void setup() {

	Serial.begin(115200);


	#ifdef S10
		S10_setup();
	#endif

	Serial.println("11111");
}

void loop() {
	#ifdef S10
		S10_loop();
	#endif
}
