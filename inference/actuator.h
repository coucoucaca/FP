/*
  @brief Turn on the built in LED based on the classification results

    The circuit:
  - Arduino Nano 33 BLE Sense Rev2 

  Concept of the code:
  1. Each class maps to a specific color: red for mix, green for speech, yellow for music, cyan for ambient, and magenta for noise.

  2. The trigger_actuator() function outputs the light based on the model's prediction. This function is called once process_actuator() in classification_state.h file verifies the confidence score.
  
  3. Hardware macros (LEDR, LEDG, LEDB) used instead of the numeric pin values 

  4. The trigger_actuator() function takes the label as a pointer. The strcmp() function then compares the string stored at that address with a hardcoded string

  5. digitalWrite() instead of PWM because it requires less overhead, helping to minimize latency.

*/

#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>


//pin assignment
const int LEDR_PIN = LEDR;
const int LEDG_PIN = LEDG;
const int LEDB_PIN = LEDB;


// function signiature
void setup_actuators();
void trigger_actuator(const char* label);
/* 
@brief actuator setup
*/
// Pin assignments variables

inline void setup_actuator() {
  //pin mode Arduino Nano 33 BLE is inverted
  pinMode(LEDR_PIN, OUTPUT);
  pinMode(LEDG_PIN, OUTPUT);
  pinMode(LEDB_PIN, OUTPUT);
  Serial.println("LED Pins initiated");
}


/* 
@brief logic for which color for which class
//label is a pointer. (const char* active_label= result.classification[active_category_index].label;)
//compares the content stored at the address to the string specifiled 
//checks if there's a difference between the content at the address to the string specified if it's match there's 0 diffrence s
*/

inline void trigger_actuator(const char* label) {

  Serial.print("Actuator triggered label: ");
  Serial.println(label);
  Serial.println(" \n ");

  //checks if there's a difference between the content at the address to the string specified
  // if it's match there's 0 diffrence so  == 0
  if (strcmp(label, "mixed") == 0) {
    //red
    digitalWrite(LEDR_PIN, LOW);
    digitalWrite(LEDG_PIN, HIGH);
    digitalWrite(LEDB_PIN, HIGH);
  }

  else if (strcmp(label, "speech") == 0) {
    //green
    digitalWrite(LEDR_PIN, HIGH);
    digitalWrite(LEDG_PIN, LOW);
    digitalWrite(LEDB_PIN, HIGH);
  }

  else if (strcmp(label, "music") == 0) {
    //yellow
    digitalWrite(LEDR_PIN, LOW);
    digitalWrite(LEDG_PIN, LOW);
    digitalWrite(LEDB_PIN, HIGH);
  }

  else if (strcmp(label, "ambient") == 0) {
    //cyan
    digitalWrite(LEDR_PIN, HIGH);
    digitalWrite(LEDG_PIN, LOW);
    digitalWrite(LEDB_PIN, LOW);
  }

  else if (strcmp(label, "noise") == 0) {
    //magenta
    digitalWrite(LEDR_PIN, LOW);
    digitalWrite(LEDG_PIN, HIGH);
    digitalWrite(LEDB_PIN, LOW);
  }

  else {
    Serial.print("Error \n");
  }

}  //end of the LED logic


/* @brief turn off actuator */

inline void stop_actuator() {

  
  //Turn off LED
  digitalWrite(LEDR_PIN, HIGH);
  digitalWrite(LEDG_PIN, HIGH);
  digitalWrite(LEDB_PIN, HIGH);

  //Serial.println("Stop Actuator called \n");
}

#endif