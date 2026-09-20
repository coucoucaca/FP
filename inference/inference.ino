/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// If your target is limited in memory remove this macro to save 10K RAM
#define EIDSP_QUANTIZE_FILTERBANK 0

/**
 * Define the number of slices per model window. E.g. a model window of 1000 ms
 * with slices per model window set to 4. Results in a slice size of 250 ms.
 * For more info: https://docs.edgeimpulse.com/docs/continuous-audio-sampling
 */
#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 4

/*
 ** NOTE: If you run into TFLite arena allocation issue.
 **
 ** This may be due to may dynamic memory fragmentation.
 ** Try defining "-DEI_CLASSIFIER_ALLOCATION_STATIC" in boards.local.txt (create
 ** if it doesn't exist) and copy this file to
 ** `<ARDUINO_CORE_INSTALL_PATH>/arduino/hardware/<mbed_core>/<core_version>/`.
 **
 ** See
 ** (https://support.arduino.cc/hc/en-us/articles/360012076960-Where-are-the-installed-cores-located-)
 ** to find where Arduino installs cores on your machine.
 **
 ** If the problem persists then there's not enough memory for this model and application.
 */

/* Includes ---------------------------------------------------------------- */
#include <PDM.h>
#include <FP-Multiclass02_inferencing.h>

#include "actuator.h"
#include "imu_sensor.h"
#include "ir_camera.h"

/** Audio buffers, pointers and selectors */
typedef struct {
  signed short *buffers[2];
  unsigned char buf_select;
  unsigned char buf_ready;
  unsigned int buf_count;
  unsigned int n_samples;
} inference_t;

static inference_t inference;
static bool record_ready = false;
static signed short *sampleBuffer;
static bool debug_nn = false;  // Set this to true to see e.g. features generated from the raw signal
static int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);

//global variable used for smoothing method
static ei_classifier_smooth_t smoother;


/**
 * @brief      Arduino setup function
 */
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // comment out the below line to cancel the wait for USB connection (needed for native USB)
  while (!Serial)
    ;
  Serial.println("Edge Impulse Inferencing Demo");


  //IMU setup
  if (!setup_imu()) {
    ei_printf("ERR: Failed to initialize IMU!\n");
  }


  //IR camera setup
  if (!setup_ir_camera()) {
    ei_printf("ERR: Failed to initialize IR Camera!\n");
  }


  //Actuator setup
  setup_actuator();



  // summary of inferencing settings (from model_metadata.h)
  ei_printf("Inferencing settings:\n");
  ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
  ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
  ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
  ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));





  // run the model
  run_classifier_init();

  /*  // smooth function  parameters are
  //  pointer to ei_classifier_smooth_t struct
  // n_readings Number of readings you want to store
  //min_readings_same Minimum readings that need to be the same before concluding (needs to be lower than n_readings)
  //classifier_confidence Minimum confidence in a class (default 0.8 if blank)
  //anomaly_confidence Maximum error for anomalies (default 0.3 if blank)
  */
  //calls and add initial data
  ei_classifier_smooth_init(&smoother, 5, 2, 0.70f, 0.30f);

  //in case of the inference error
  if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
    ei_printf("ERR: Could not allocate audio buffer (size %d), this could be due to the window length of your model\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    return;
  }

}  //end of set up




/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */
void loop() {

  //IMU movement check
  bool ready_to_classify_IMU = is_device_still();


  // if the device is still then skip this code
  if (!ready_to_classify_IMU) {
    //turn off LED if device moved
    stop_actuator();
    return;
  }

 //IR camera target Check
  bool ready_to_classify_IR = is_target_detected();

  // if not detected then skip this code
  if (!ready_to_classify_IR) {
    //turn off LED if no target
    stop_actuator();
    return;
  }



  //Check if the audio buffer is filled
  bool m = microphone_inference_record();
  if (!m) {
    ei_printf("ERR: Failed to record audio...\n");
    return;
  }

  //Continuous recording on audio
  signal_t signal;
  signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
  signal.get_data = &microphone_audio_signal_get_data;

  //results array
  ei_impulse_result_t result = { 0 };

  //run classifier
  EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);

  if (r != EI_IMPULSE_OK) {
    ei_printf("ERR: Failed to run classifier (%d)\n", r);
    return;
  }

  //smoothed results (takes the raw result and returns label)
  const char *smoothed_result = ei_classifier_smooth_update(&smoother, &result);

  //Printing smoothed

  //when not uncertained
  if (smoothed_result != NULL && strcmp(smoothed_result, "uncertain") && strcmp(smoothed_result, "anomaly") != 0) {
    // "uncertain" label is added by ei_classifier_smooth_updat
    // print the predictions
    ei_printf("Smoothed Predictions: ");
    ei_printf(smoothed_result);
    ei_printf("\n");

    //Trigger actuator function in actuator.h
    trigger_actuator(smoothed_result);

  } else {

    stop_actuator();
  }

  // if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
  //   // print the predictions
  //   ei_printf("Predictions ");
  //   ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
  //             result.timing.dsp, result.timing.classification, result.timing.anomaly);
  //   ei_printf(": \n");
  // }

  /* Print full prediction values original example code  

  if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
              result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");

    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
      ei_printf("    %s: %.5f\n", result.classification[ix].label,
                result.classification[ix].value);
    }
*/




#if EI_CLASSIFIER_HAS_ANOMALY == 1
  ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif

  print_results = 0;



  //classification state
  // process_actuator(result);

}  // loop end



/**
 * @brief      PDM buffer full callback
 *             Get data and call audio thread callback
 */
static void pdm_data_ready_inference_callback(void) {
  int bytesAvailable = PDM.available();

  // read into the sample buffer
  int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

  if (record_ready == true) {
    for (int i = 0; i < bytesRead >> 1; i++) {
      inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

      if (inference.buf_count >= inference.n_samples) {
        inference.buf_select ^= 1;
        inference.buf_count = 0;
        inference.buf_ready = 1;
      }
    }
  }
}





/**
 * @brief      Init inferencing struct and setup/start PDM
 *
 * @param[in]  n_samples  The n samples
 *
 * @return     { description_of_the_return_value }
 */
static bool microphone_inference_start(uint32_t n_samples) {
  inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));

  if (inference.buffers[0] == NULL) {
    return false;
  }

  inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

  if (inference.buffers[1] == NULL) {
    free(inference.buffers[0]);
    return false;
  }

  sampleBuffer = (signed short *)malloc((n_samples >> 1) * sizeof(signed short));

  if (sampleBuffer == NULL) {
    free(inference.buffers[0]);
    free(inference.buffers[1]);
    return false;
  }

  inference.buf_select = 0;
  inference.buf_count = 0;
  inference.n_samples = n_samples;
  inference.buf_ready = 0;

  // configure the data receive callback
  PDM.onReceive(&pdm_data_ready_inference_callback);

  PDM.setBufferSize((n_samples >> 1) * sizeof(int16_t));

  // initialize PDM with:
  // - one channel (mono mode)
  // - a 16 kHz sample rate
  if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
    ei_printf("Failed to start PDM!");
  }

  // set the gain, defaults to 20
  PDM.setGain(127);

  record_ready = true;

  return true;
}

/**
 * @brief      Wait on new data
 *
 * @return     True when finished
 */
static bool microphone_inference_record(void) {
  bool ret = true;

  if (inference.buf_ready == 1) {
    ei_printf(
      "Error sample buffer overrun. Decrease the number of slices per model window "
      "(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)\n");
    ret = false;
  }

  while (inference.buf_ready == 0) {
    delay(1);
  }

  inference.buf_ready = 0;

  return ret;
}

/**
 * Get raw audio signal data
 */
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
  numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);

  return 0;
}

/**
 * @brief      Stop PDM and release buffers
 */
static void microphone_inference_end(void) {
  PDM.end();
  free(inference.buffers[0]);
  free(inference.buffers[1]);
  free(sampleBuffer);
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif
