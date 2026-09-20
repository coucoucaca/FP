/*
  @brief Checking for human presence with AMG88xx GridEYE 8x8 IR camera
  Based on Adafruit Library code. amg88xx_test.ino and pixels_test.ino   
  by Dean Miller for Adafruit Industries. http://www.adafruit.com/products/3538


  The concept of the code: 
  1. set up the camera and read the background temp as a baseline
  2. Find the delta of the baseline and the new temp(pixel)
  3. if it is over 2-4 difference then count the pixels

  Variables for GridEye can be dynamic since it is much simpler operation comapred to IMU.

*/

#ifndef IR_CAMERA_H
#define IR_CAMERA_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AMG88xx.h>

Adafruit_AMG88xx amg;
//set up array to store temp data 64 (mutable)
float pixels[AMG88xx_PIXEL_ARRAY_SIZE];

//configuration (immutable)
const float TEMP_DELTA_THRESHOLD = 2.0f;  // 2-4c
const int PIXEL_COUNT_THRESHOLD = 6;      // 6pix

//global variable (dynamic)
//baseline temp
float bg_temp_avg_final = 0.0f;

// function signiature
bool setup_ir_camera();
bool is_target_detected();


//set up and read the background temp and average out
inline bool setup_ir_camera() {

  Serial.begin(115200);

  bool status;

  status = amg.begin();

  if (!status) {
    Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
    // while (1)
    //   ;
    return false;
  }

  float totalBGtemp = 0.0f;

  //takes 3 times and average
  //take 3 readings for averaging 3 readings for backgroud temp
  for (int r = 0; r < 3; r++) {
    //read all pixels
    amg.readPixels(pixels);

    for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; i++) {
      totalBGtemp += pixels[i];
    }
  }
  // average of 3 readings
  bg_temp_avg_final = totalBGtemp / (AMG88xx_PIXEL_ARRAY_SIZE * 3);

  Serial.print("Average Background temp = ");
  Serial.println(bg_temp_avg_final);

  return true;

}  //end of the set up





//Check the background temp and foreground temp delta
inline bool is_target_detected()

{
  //return this flag at the end
  bool targetDetected = false;


  float tempDelta = 0.0f;
  int pxCount = 0;

  //read tem again
  amg.readPixels(pixels);

  //find the delta
  for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; i++) {
    //get the delta ( not absolute value. Needs negative and positive)
    tempDelta = pixels[i] - bg_temp_avg_final;

    // Serial.println(temp_delta);

    //count pixel above the bg temp
    if (tempDelta >= TEMP_DELTA_THRESHOLD) {

      pxCount++;
    }

  }  //end of the loop


  Serial.println();
  Serial.print("pix count:");
  Serial.println(pxCount);

  // if pixel count  against the threshold is  more than threshold target is present
  //each pixel is about 7cm therefore should fill up 5-7 and up
  bool currentlyDetected = (pxCount > PIXEL_COUNT_THRESHOLD);

  
  if (currentlyDetected != targetDetected) {
    //update the flag
    targetDetected = currentlyDetected;
  }
    Serial.print("Target is ");
    Serial.println(targetDetected ? "DETECTED" : "NOT DETECTED");


  // Serial.print("[");
  // for (int i = 1; i <= AMG88xx_PIXEL_ARRAY_SIZE; i++) {
  //   Serial.print(pixels[i - 1]);
  //   Serial.print(", ");
  //   if (i % 8 == 0) Serial.println();
  // }
  // Serial.println("]");
  // Serial.println();

  // //delay a second
  // delay(1000);

  return targetDetected;

}  //end of function


#endif