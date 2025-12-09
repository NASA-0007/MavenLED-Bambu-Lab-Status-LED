#ifndef PRINTER_STATE_H
#define PRINTER_STATE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Printer state variables
struct PrinterState {
  String status = "unknown";
  String raw_gcode_state = "unknown";
  int progress = 0;
  int download_progress = 0;
  String stage = "";
  int bed_temp = 0;
  int nozzle_temp = 0;
  int target_bed_temp = 0;
  int target_nozzle_temp = 0;
  String filename = "";
  int remaining_time = 0;
  bool has_error = false;
  String error_message = "";
  bool is_connected = false;
  String layer_info = "";
  int current_layer = 0;
  int total_layers = 0;
  bool finish_animation_active = false;
  unsigned long finish_animation_start = 0;
  
  bool error_recovery_active = false;
  unsigned long error_recovery_start = 0;
  
  bool state_override_active = false;
  unsigned long state_override_start = 0;
  String override_reason = "";
  
  int prev_bed_temp = 0;
  int prev_nozzle_temp = 0;
  unsigned long last_temp_check = 0;
  bool is_heating = false;
  bool is_cooling = false;
  
  String last_stable_status = "unknown";
  unsigned long last_status_change = 0;
  int temp_readings_count = 0;

  unsigned long heating_start_time = 0;
  unsigned long cooling_start_time = 0;
  unsigned long last_temp_change_time = 0;
  int last_significant_nozzle_temp = 0;
  int last_significant_bed_temp = 0;
  
  unsigned long heating_exit_timer = 0;
  unsigned long cooling_exit_timer = 0;
  bool heating_exit_timer_started = false;
  bool cooling_exit_timer_started = false;
  
  // Idle timeout tracking
  unsigned long idle_state_start = 0;
  bool auto_off_active = false;
};

extern PrinterState printer_state;
extern SemaphoreHandle_t printerStateMutex;
extern volatile bool printer_state_updated;

// RTC memory for persistent state
typedef struct {
  bool printing_active;
  unsigned long last_update_time;
  uint32_t magic_number;
} RTCState;

extern RTC_DATA_ATTR RTCState rtc_state;

// Printer state functions
void updatePrinterState(JsonDocument& doc);
bool determinePrinterStatus();
void checkConnectionTimeout();

#endif
