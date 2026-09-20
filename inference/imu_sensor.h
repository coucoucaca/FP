/*
  @brief Checking is the device is still with Arduino BMI270  Accelerometer

  The concept of the code: 
  1. Using average of window of magnitude.
  2. Difference of Average & individual magnitude 
  3. Compared to the thershold to determines still or not.
  4. if still return true

  Using circular buffer to save memory
  IMU's variables should be more cafully handled static vs dynamic.
*/


#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

#include <Arduino.h>
#include "Arduino_BMI270_BMM150.h"

// configuration
const int WINDOW_SIZE = 10;  // how many imu readings to store for smoothing
const float IMU_THRESHOLD = 0.05f; 

// array to store needs access to both functions
static float imu_readings[WINDOW_SIZE];  

//total val of  imu readings needs access to both functions
static float imu_total = 0.0f;


// function signiature
bool setup_imu();
bool is_device_still();



/* @brief set up IMU */
inline bool setup_imu() {

  Serial.begin(115200);

  bool status = IMU.begin();

  if (!status) {
    Serial.println("Failed to initialize IMU!");
    return false;
  }


  //IMU capability
  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.println();
  // Serial.println("Acceleration in G's");
  // Serial.println("X\tY\tZ");


  // initialize the array with 1G because when sitting still 1G
  for (int i = 0; i < WINDOW_SIZE; i++) {
    imu_readings[i] = 1.0f;
  }
  imu_total = WINDOW_SIZE * 1.0f;
  return true;

}  //end of function





/*  @brief calculate magnitude and deviation  then comppared to the threshold to see 
if the device is still and return flag  */


inline bool is_device_still() {

  //return this flag
  bool isStill = false;

  // tracking the readings in the array
  int imuReadIndex = 0;

  float x, y, z;

  if (IMU.accelerationAvailable()) {


    IMU.readAcceleration(x, y, z);


    //culcurate the raw magnitude with pythagorean theorem
    float currentMag = sqrt(x * x + y * y + z * z);

    // substract the previous value

    imu_total = imu_total - imu_readings[imuReadIndex];
    //update the value in the array with the current raw mag
    imu_readings[imuReadIndex] = currentMag;

    //add current mag to total
    imu_total = imu_total + imu_readings[imuReadIndex];


    //circular buffer index number update
    //it  will return to 0 when reach the window size
    imuReadIndex = (imuReadIndex + 1) % WINDOW_SIZE;

    //after totaling then average
    float averageMag = imu_total / WINDOW_SIZE;

    //calculate absolute mean of deviations
    //diffrence of prev average and the curent reading
    //total deviation
    float totalIMUdev = 0;

    for (int i = 0; i < WINDOW_SIZE; i++) {
      //calculate difference from the current reading to the average and totals them
      totalIMUdev += abs(imu_readings[i] - averageMag);
    }

    //average of the Deviation
    float avgIMUdev = totalIMUdev / WINDOW_SIZE;

    // compare to the threshold
    bool currentlyStill = (avgIMUdev < IMU_THRESHOLD);


    if (currentlyStill != isStill) {
      //update flag
      isStill = currentlyStill;
    }

    Serial.print("Device is ");
    Serial.println(isStill ? "STILL" : "MOVING");

    return isStill;


  }  // if IMU available

  return isStill;


}  //end of function

#endif
