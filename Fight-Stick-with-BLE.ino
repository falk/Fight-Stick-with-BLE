/**
 * Project to reanimate a dead Xbox360 Controller
 * by using an ESP32 to read the inputs and connecting to Windows via BLE
 */

#include "nvs_flash.h"
#include "esp_log.h" // for time measurements
#include "BleGamepad.h"
#include "LedModule.h"
#include "Profiles.h"
#include "GamepadPins.h"
#include "Log.h"

#define N_MEASUREMENTS  200
#define FREQUENCY 30
const int DELAY_MS = 1000 / FREQUENCY;
const int DELAY_NOT_CONNECTED = 750;
uint8_t ledOn = 0;

BleGamepad bleGamepad("Fight Stick BLE", "falk", 100);
Profiles *p;
inputs_t inputs2send;
SemaphoreHandle_t coordinator;
TaskHandle_t TaskScanInputsHandle;

void TaskScanInputs(void*);

void sleep_until_multiple_of(uint16_t n_ms) { delay(n_ms - (esp_log_timestamp() % n_ms)); }

void setup() {
  nvs_flash_init();
  p = new Profiles();
  SERIAL_INIT();
  setupPins();
  PRINTF("Boot complete\n");
  bleGamepad.begin();

  coordinator = xSemaphoreCreateMutex();
  
  xTaskCreatePinnedToCore(TaskScanInputs,        // Task function
                          "ScanInputs",          // name
                          10000,                 // stack size
                          NULL,                  // parameters
                          1,                     // priority
                          &TaskScanInputsHandle, // Task handle to keep track of created task
                          1);                    // pin to core 1 (second core)

  led_greeting();
}

int n, t0, t1;

void loop() {
  if (bleGamepad.isConnected()) {
    xSemaphoreTake(p->inputsSemaphore, portMAX_DELAY);
    xSemaphoreTake(p->inputsMutex, portMAX_DELAY);
    inputs2send = p->inputs;
    xSemaphoreGive(p->inputsMutex);

    // set values to bleGamepad
    bleGamepad.release(~inputs2send.buttons);
    bleGamepad.press(inputs2send.buttons);
   // bleGamepad.setAxes(0, 0, 0, 0, 0, 0, inputs2send.dpad);
    bleGamepad.setHat1(inputs2send.dpad);

    // output
    PRINT_BINARY(inputs2send.buttons);
    PRINTF(" | %c |\n", dpadToChar(inputs2send.dpad));
  } else {
    led_circle();
    led_set(0x1 << p->getCurrentProfile());
  }
}

void TaskScanInputs(void *params) {
  while(1) {
    STOPWATCH_START();
    for (n = 1; n <= N_MEASUREMENTS; n++) {
      int consecutiveSwitchPresses = p->scanInputs();
      if (consecutiveSwitchPresses >= FREQUENCY) {
        PRINTF("RESTART\n");
        throw -1;
      }
      xSemaphoreTake(*bleGamepad.semaphor(), portMAX_DELAY);
      sleep_until_multiple_of(DELAY_MS);
      xSemaphoreGive(*bleGamepad.semaphor());
    }
    STOPWATCH_STOP();
  }
}
