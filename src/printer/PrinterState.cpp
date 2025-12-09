#include "PrinterState.h"
#include "../config/Settings.h"
#include "../web/WebHandlers.h"

PrinterState printer_state;
SemaphoreHandle_t printerStateMutex = NULL;
volatile bool printer_state_updated = false;

RTC_DATA_ATTR RTCState rtc_state = {false, 0, 0xDEADBEEF};

// Timing variables
extern unsigned long lastMQTTupdate;
extern unsigned long lastMQTTProcessTime;

void updatePrinterState(JsonDocument& doc) {
  if (xSemaphoreTake(printerStateMutex, portMAX_DELAY) == pdTRUE) {
    bool changed = false;
    String previousStatus = printer_state.status;
    
    if (doc.containsKey("print")) {
      JsonObject print = doc["print"];
      
      if (print.containsKey("gcode_state")) {
        String newRawStatus = print["gcode_state"].as<String>();
        
        bool shouldIgnoreStateUpdate = false;
        if (printer_state.state_override_active) {
          if (printer_state.override_reason == "error timeout") {
            if (newRawStatus != "FAILED" && newRawStatus != printer_state.raw_gcode_state) {
              printer_state.state_override_active = false;
              Serial.printf(" Error state override cancelled - MQTT state changed from %s to %s\n", 
                            printer_state.raw_gcode_state.c_str(), newRawStatus.c_str());
            } else {
              shouldIgnoreStateUpdate = true;
            }
          } else if (printer_state.override_reason == "finish timeout") {
            if (newRawStatus != "FINISH" && newRawStatus != printer_state.raw_gcode_state) {
              printer_state.state_override_active = false;
              Serial.printf(" Finish state override cancelled - MQTT state changed from %s to %s\n", 
                            printer_state.raw_gcode_state.c_str(), newRawStatus.c_str());
            } else {
              shouldIgnoreStateUpdate = true;
            }
          }
        }
        
        if (!shouldIgnoreStateUpdate && printer_state.raw_gcode_state != newRawStatus) {
          printer_state.raw_gcode_state = newRawStatus;
          bool was_printing = rtc_state.printing_active;
          rtc_state.printing_active = (newRawStatus == "RUNNING" || newRawStatus == "PAUSE");
          rtc_state.last_update_time = millis();
          if (was_printing != rtc_state.printing_active) {
            Serial.printf(" RTC state updated: printing_active=%s\n", 
                          rtc_state.printing_active ? "true" : "false");
          }
          changed = true;
        }
      }
      
      if (print.containsKey("mc_percent")) {
        int newProgress = print["mc_percent"].as<int>();
        if (printer_state.progress != newProgress) {
          printer_state.progress = newProgress;
          changed = true;
        }
      }
      
      if (print.containsKey("gcode_file_prepare_percent")) {
        String preparePercentStr = print["gcode_file_prepare_percent"].as<String>();
        int downloadProgress = preparePercentStr.toInt();
        if (printer_state.download_progress != downloadProgress) {
          printer_state.download_progress = downloadProgress;
          changed = true;
        }
      }
      
      if (print.containsKey("layer_num") && print.containsKey("total_layer_num")) {
        int currentLayer = print["layer_num"].as<int>();
        int totalLayers = print["total_layer_num"].as<int>();
        if (printer_state.current_layer != currentLayer || printer_state.total_layers != totalLayers) {
          printer_state.current_layer = currentLayer;
          printer_state.total_layers = totalLayers;
          changed = true;
        }
      }
      
      bool hasNewTempData = false;
      
      if (print.containsKey("bed_temper")) {
        int bedTemp = (int)print["bed_temper"].as<float>();
        if (printer_state.bed_temp != bedTemp) {
          if (printer_state.temp_readings_count == 0) printer_state.prev_bed_temp = bedTemp;
          
          if (abs(bedTemp - printer_state.last_significant_bed_temp) > 1) {
            printer_state.last_temp_change_time = millis();
            printer_state.last_significant_bed_temp = bedTemp;
            printer_state.heating_exit_timer_started = false;
            printer_state.cooling_exit_timer_started = false;
          }
          printer_state.bed_temp = bedTemp;
          hasNewTempData = true;
          changed = true;
        }
      }
      
      if (print.containsKey("nozzle_temper")) {
        int nozzleTemp = (int)print["nozzle_temper"].as<float>();
        if (printer_state.nozzle_temp != nozzleTemp) {
          if (printer_state.temp_readings_count == 0) printer_state.prev_nozzle_temp = nozzleTemp;
          
          if (abs(nozzleTemp - printer_state.last_significant_nozzle_temp) > 1) {
            printer_state.last_temp_change_time = millis();
            printer_state.last_significant_nozzle_temp = nozzleTemp;
            printer_state.heating_exit_timer_started = false;
            printer_state.cooling_exit_timer_started = false;
          }
          printer_state.nozzle_temp = nozzleTemp;
          hasNewTempData = true;
          changed = true;
        }
      }
      
      if (hasNewTempData && printer_state.temp_readings_count < 10) {
        printer_state.temp_readings_count++;
      }
      
      if (print.containsKey("bed_target_temper")) {
        int targetBedTemp = (int)print["bed_target_temper"].as<float>();
        if (printer_state.target_bed_temp != targetBedTemp) {
          printer_state.target_bed_temp = targetBedTemp;
          changed = true;
        }
      }
      
      if (print.containsKey("nozzle_target_temper")) {
        int targetNozzleTemp = (int)print["nozzle_target_temper"].as<float>();
        if (printer_state.target_nozzle_temp != targetNozzleTemp) {
          printer_state.target_nozzle_temp = targetNozzleTemp;
          changed = true;
        }
      }
      
      // Temperature state logic
      bool updateThermalState = printer_state.temp_readings_count >= 3;
      bool heating = printer_state.is_heating;
      bool cooling = printer_state.is_cooling;
      
      if (updateThermalState) {
        int nozzle_trend = printer_state.nozzle_temp - printer_state.prev_nozzle_temp;
        int bed_trend = printer_state.bed_temp - printer_state.prev_bed_temp;
        
        bool nozzle_heating_target = (printer_state.target_nozzle_temp > 40 && 
                                      printer_state.nozzle_temp < (printer_state.target_nozzle_temp - 3));
        bool bed_heating_target = (printer_state.target_bed_temp > 40 && 
                                   printer_state.bed_temp < (printer_state.target_bed_temp - 3));
        
        bool active_heating = (nozzle_heating_target && (nozzle_trend > 0 || 
                               printer_state.nozzle_temp < (printer_state.target_nozzle_temp - 8))) || 
                              (bed_heating_target && (bed_trend > 0 || 
                               printer_state.bed_temp < (printer_state.target_bed_temp - 8)));
        
        bool nozzle_cooling = (printer_state.nozzle_temp > 50) && 
                              (printer_state.target_nozzle_temp <= 40 || printer_state.target_nozzle_temp == 0) &&
                              (nozzle_trend <= 0);
                              
        bool bed_cooling = (printer_state.bed_temp > 50) && 
                           (printer_state.target_bed_temp <= 40 || printer_state.target_bed_temp == 0) &&
                           (bed_trend <= 0);
        
        bool active_cooling = nozzle_cooling || bed_cooling;
        bool no_temp_change = (nozzle_trend == 0 && bed_trend == 0);
        
        if (active_heating) {
          if (!heating) {
            printer_state.heating_start_time = millis();
            printer_state.last_temp_change_time = millis();
          }
          heating = true;
          cooling = false;
          printer_state.cooling_exit_timer_started = false;
        } else if (active_cooling && !active_heating) {
          if (!cooling) {
            printer_state.cooling_start_time = millis();
            printer_state.last_temp_change_time = millis();
          }
          heating = false;
          cooling = true;
          printer_state.heating_exit_timer_started = false;
        } else {
          if (heating) {
            if (no_temp_change && !printer_state.heating_exit_timer_started) {
              printer_state.heating_exit_timer = millis();
              printer_state.heating_exit_timer_started = true;
              Serial.println("️ Starting 5-second timer to exit heating state");
            } else if (printer_state.heating_exit_timer_started && 
                       (millis() - printer_state.heating_exit_timer) >= 5000) {
              Serial.println("️ Exiting heating state - 5 seconds of no temperature change");
              heating = false;
              printer_state.heating_exit_timer_started = false;
            }
          }
          
          if (cooling) {
            if (no_temp_change && !printer_state.cooling_exit_timer_started) {
              printer_state.cooling_exit_timer = millis();
              printer_state.cooling_exit_timer_started = true;
              Serial.println("️️ Starting 5-second timer to exit cooling state");
            } else if (printer_state.cooling_exit_timer_started && 
                       (millis() - printer_state.cooling_exit_timer) >= 5000) {
              Serial.println("️️ Exiting cooling state - 5 seconds of no temperature change");
              cooling = false;
              printer_state.cooling_exit_timer_started = false;
            }
          }
        }
        
        printer_state.last_temp_check = millis();
      }
      
      if (heating != printer_state.is_heating || cooling != printer_state.is_cooling) {
        printer_state.is_heating = heating;
        printer_state.is_cooling = cooling;
        Serial.printf(" State changed: Heating=%s, Cooling=%s\n", heating ? "ON" : "OFF", cooling ? "ON" : "OFF");
        changed = true;
      }
      
      if (updateThermalState) {
        printer_state.prev_bed_temp = printer_state.bed_temp;
        printer_state.prev_nozzle_temp = printer_state.nozzle_temp;
      }
      
      if (print.containsKey("mc_remaining_time")) {
        int remainingTime = print["mc_remaining_time"].as<int>();
        if (printer_state.remaining_time != remainingTime) {
          printer_state.remaining_time = remainingTime;
          changed = true;
        }
      }
      
      if (print.containsKey("err")) {
        String errStr = print["err"].as<String>();
        bool hasError = (errStr != "0");
        if (printer_state.has_error != hasError) {
          printer_state.has_error = hasError;
          printer_state.error_message = errStr;
          if (hasError) {
            Serial.printf(" Error detected: err=%s\n", errStr.c_str());
          } else {
            Serial.println(" Error cleared: err=0");
          }
          changed = true;
        }
      }
    }
    
    if (changed) {
      bool statusActuallyChanged = determinePrinterStatus();
      if (statusActuallyChanged) {
        printer_state_updated = true;
        
        static unsigned long lastPrinterStatusPublish = 0;
        if (millis() - lastPrinterStatusPublish > 1000) {
          publishPrinterStatus();
          lastPrinterStatusPublish = millis();
        }
      }
    }
    
    xSemaphoreGive(printerStateMutex);
  }
}

bool determinePrinterStatus() {
  String currentProcessedStatus = printer_state.status;
  String rawStatus = printer_state.raw_gcode_state;
  String newStatus = currentProcessedStatus;
  
  // Check idle timeout
  if (settings.idle_timeout_enabled && !printer_state.auto_off_active) {
    if (currentProcessedStatus == "idle" && printer_state.idle_state_start > 0) {
      unsigned long idleDuration = millis() - printer_state.idle_state_start;
      unsigned long timeoutMillis = settings.idle_timeout_minutes * 60000UL;
      if (idleDuration >= timeoutMillis) {
        Serial.printf(" Idle timeout reached (%d minutes) - entering auto-off state\n", settings.idle_timeout_minutes);
        printer_state.auto_off_active = true;
        newStatus = "auto_off";
      }
    }
  }
  
  // Reset auto-off state when printer becomes active
  if (printer_state.auto_off_active) {
    if (rawStatus == "RUNNING" || rawStatus == "PREPARE" || rawStatus == "PAUSE" || 
        rawStatus == "FINISH" || rawStatus == "FAILED" || 
        printer_state.is_heating || printer_state.is_cooling) {
      Serial.println(" Printer active - exiting auto-off state");
      printer_state.auto_off_active = false;
      printer_state.idle_state_start = 0;
    } else {
      // Stay in auto_off state
      if (printer_state.status != "auto_off") {
        newStatus = "auto_off";
      }
    }
  }
  
  if (settings.state_timeout_reached && (rawStatus == "FINISH" || rawStatus == "FAILED")) {
    Serial.printf(" Persistent timeout detected - forcing idle mode\n");
    newStatus = "idle";
  } else if (!settings.state_timeout_reached) {
    if (printer_state.finish_animation_active) {
      unsigned long finishDuration = millis() - printer_state.finish_animation_start;
      if (finishDuration >= 120000) {
        printer_state.finish_animation_active = false;
        printer_state.state_override_active = true;
        printer_state.state_override_start = millis();
        printer_state.override_reason = "finish timeout";
        settings.state_timeout_reached = true;
        saveSettings();
        Serial.println("️ Finish animation stopped - forcing idle state");
        newStatus = "idle";
      } else if (rawStatus != "FINISH") {
        printer_state.finish_animation_active = false;
        Serial.printf("️ Finish animation stopped - status changed\n");
      } else {
        newStatus = "finished";
      }
    }
    
    if (printer_state.error_recovery_active) {
      unsigned long errorDuration = millis() - printer_state.error_recovery_start;
      if (errorDuration >= 120000) {
        printer_state.error_recovery_active = false;
        printer_state.state_override_active = true;
        printer_state.state_override_start = millis();
        printer_state.override_reason = "error timeout";
        settings.state_timeout_reached = true;
        saveSettings();
        Serial.println("️ Error recovery timeout - forcing idle state");
        newStatus = "idle";
      } else if (rawStatus != "FAILED" && (!printer_state.has_error || rawStatus == "PAUSE")) {
        printer_state.error_recovery_active = false;
        Serial.printf("️ Error recovery stopped\n");
      } else {
        newStatus = "error";
      }
    }
    
    if (printer_state.is_heating && printer_state.temp_readings_count >= 3) {
        newStatus = "heating";
    } else if (printer_state.is_cooling && printer_state.temp_readings_count >= 3) {
      // P1 Series Mode: Skip cooling state during active printing
      if (settings.p1_series_mode && printer_state.progress > 0 && printer_state.progress < 100) {
        newStatus = "printing";
      } else {
        newStatus = "cooling";
      }
    } else if (printer_state.state_override_active && 
               !printer_state.is_heating && !printer_state.is_cooling) {
      newStatus = "idle";
    } else if (!printer_state.finish_animation_active && !printer_state.error_recovery_active && !printer_state.state_override_active) {
      if (rawStatus == "PREPARE") {
        newStatus = "downloading";
      } else if (rawStatus == "RUNNING") {
        newStatus = "printing";
      } else if (rawStatus == "PAUSE") {
        if (printer_state.has_error) {
          newStatus = "recoverable_error";
        } else {
          newStatus = "paused";
        }
      } else if (rawStatus == "FINISH") {
        if (!printer_state.finish_animation_active) {
          printer_state.finish_animation_active = true;
          printer_state.finish_animation_start = millis();
          Serial.println(" Print finished - starting 2-minute celebration animation");
        }
        newStatus = "finished";
      } else if (rawStatus == "FAILED") {
        if (!printer_state.error_recovery_active) {
          printer_state.error_recovery_active = true;
          printer_state.error_recovery_start = millis();
          Serial.println(" Error detected - starting 2-minute error recovery");
        }
        newStatus = "error";
      } else if (printer_state.has_error && rawStatus != "PAUSE") {
        if (!printer_state.error_recovery_active) {
          printer_state.error_recovery_active = true;
          printer_state.error_recovery_start = millis();
          Serial.println(" Non-pause error detected - starting 2-minute error recovery");
        }
        newStatus = "error";
      } else if (rawStatus == "IDLE") {
        newStatus = "idle";
      } else {
        if (printer_state.progress > 0) {
          newStatus = "printing";
        } else if (currentProcessedStatus == "unknown") {
          newStatus = "idle";
          Serial.printf(" Unknown raw status '%s' - defaulting to idle\n", rawStatus.c_str());
        }
      }
    }
  }
  
  if (printer_state.status != newStatus) {
    Serial.printf(" Status changed from '%s' to '%s'\n", printer_state.status.c_str(), newStatus.c_str());
    
    // Track idle state start time for timeout
    if (newStatus == "idle" && printer_state.status != "idle") {
      printer_state.idle_state_start = millis();
      Serial.printf("️ Idle state started - timeout in %d minutes\n", settings.idle_timeout_minutes);
    } else if (newStatus != "idle" && newStatus != "auto_off") {
      printer_state.idle_state_start = 0;
    }
    
    if (settings.state_timeout_reached && 
        newStatus != "finished" && newStatus != "error" && 
        rawStatus != "FINISH" && rawStatus != "FAILED") {
      settings.state_timeout_reached = false;
      saveSettings();
      Serial.println(" Persistent timeout flag reset");
    }
    
    printer_state.last_stable_status = printer_state.status;
    printer_state.status = newStatus;
    printer_state.last_status_change = millis();
    return true;
  }
  return false;
}

void checkConnectionTimeout() {
  if (printer_state.is_connected && (millis() - lastMQTTupdate > 45000)) {
    Serial.println("️ MQTT connection timeout - marking printer as disconnected");
    printer_state.is_connected = false;
    printer_state.status = "unknown";
    printer_state.raw_gcode_state = "unknown";
    printer_state.temp_readings_count = 0;
    printer_state.is_heating = false;
    printer_state.is_cooling = false;
    printer_state.last_temp_change_time = millis();
    printer_state.error_recovery_active = false;
  }
}
