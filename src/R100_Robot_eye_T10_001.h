#include "LedControl.h"
LedControl lc=LedControl(11, 13, 3, 2);

const int addrL = 0;  // first LED matrix - Left robot eye
const int addrR = 1;  // second LED matrix - Right robot eye

void R100_init() {
  /*The MAX72XX is in power-saving mode on startup*/
  lc.shutdown(addrL,false);
  lc.shutdown(addrR,false);
  /* Set the brightness to max values */
  lc.setIntensity(addrL,15);
  lc.setIntensity(addrR,15);
  /* and clear the display */
  lc.clearDisplay(addrL);
  lc.clearDisplay(addrR);

  // turn on all LEDs for a test
  for(int row=0;row<8;row++) {
    lc.setRow(addrL, row, 255);
    lc.setRow(addrR, row, 255);
    delay(100);
  }
  delay(300);
}

void showNeutral() {
  byte left[8] = {
B00000000,
B00111100,
B01000010,
B01011010,
B01011010,
B01000010,
B00111100,
B00000000};

  displayEmotion(left, left);
}
void left1(){
 byte left[8] = {
   0b00000000,
  0b00111100,
  0b01001110,
  0b01001110,
  0b01111110,
  0b01111110,
  0b00111100,
  0b00000000};
  displayEmotion(left, left);
}
void right1(){
 byte left[8] = {
  0b00000000,
  0b00111100,
  0b01111110,
  0b01111110,
  0b01001110,
  0b01001110,
  0b00111100,
  0b00000000
};
  displayEmotion(left, left);
}
void he(){
 byte left[8] = {
   0b01000000,
  0b10011000,
  0b10100100,
  0b10111100,
  0b10111100,
  0b10100100,
  0b10011000,
  0b01000000
};
  displayEmotion(left, left);
}
void hec(){
 byte left[8] = {
 0b01000000,
  0b10011000,
  0b10111100,
  0b10111100,
  0b10111100,
  0b10111100,
  0b10011000,
  0b01000000};
  displayEmotion(left,left);
}
void displayEmotion(byte left[8], byte right[8]) {
  lc.clearDisplay(addrL);
  lc.clearDisplay(addrR);
  for(int row=0;row<8;row++) {
    lc.setRow(addrL,row,left[row]);
    lc.setRow(addrR,row,right[row]);
  }
}

void R100_run() {
 // showNeutral();
 // delay(4000);
 left1();
  delay(4000);
  right1();
  delay(4000);
he();
  delay(4000);
  hec();
 delay(4000);
}
