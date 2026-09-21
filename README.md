# 1: Inference folder
## inferece.ino
- Listen to live audio via Arduino Nano 33 BLE Sense Rev2 built-in mic
- Extract audio features using SDK
- Run classifier from SDK


## imu_sensor.h

- Detect is the device is still using built-in IMU (BMU270_BMM150) 


## ir_camera.h

- Detect target speaker presence with Adafruit GridEYE(AMG 8833 IR Thermal Camera Breakout)

  
## Actuator

- Turn on the built in RGB LED based on the classification results from the inference 

# 2: Zip file
Edge impulse SDK files including quantized model 
Include the zip in the Arduino IDE

# 3: Edge Impluse Project(Machine Learning porrtion of the project)
Data set, DSP and and models
https://studio.edgeimpulse.com/public/1056404/live
