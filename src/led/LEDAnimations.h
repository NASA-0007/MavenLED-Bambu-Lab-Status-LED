#ifndef LED_ANIMATIONS_H
#define LED_ANIMATIONS_H

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

// Hardware definitions
#define LED_PIN 17
#define LED_COUNT 60

// Animation timing
extern const unsigned long ANIMATION_INTERVAL;
extern const unsigned long RAINBOW_INTERVAL;
extern const unsigned long LIGHTS_ANIMATION_DURATION;

// LED strip object
extern Adafruit_NeoPixel strip;

// Animation state
extern unsigned long last_update;
extern unsigned long last_rainbow;
extern int rainbow_offset;
extern unsigned long download_head_cycle_start;
extern unsigned long printing_head_cycle_start;
extern unsigned long last_download_progress;
extern unsigned long last_print_progress;

// Lights animation state
extern bool lights_turning_on;
extern bool lights_turning_off;
extern unsigned long lights_animation_start;
extern int lights_animation_progress;

// Frame buffer for lights animation
extern uint32_t* saved_frame_buffer;
extern String saved_animation_state;
extern int saved_progress;
extern unsigned long saved_animation_time;
extern bool has_saved_frame;
extern int saved_rainbow_offset;
extern unsigned long saved_rainbow_time;
extern bool rainbow_paused;

// LED functions
void reinitializeLEDStrip();
void reinitializeStripPin(int pin);
uint32_t getStateColor(int stateIndex);
void startupAnimation();
void updateLEDDisplay();
void showDownloadProgress();
void showPrintingProgress();
void showPausedState();
void showErrorState();
void showRecoverableErrorState();
void showHeatingState();
void showCoolingState();
void showFinishedState();
void showIdleState();
void showAutoOffState();
void rainbowAnimation();
uint32_t wheel(byte wheelPos);
void captureCurrentFrame();
void restoreSavedFrame();
bool shouldResumeAnimation();
void generateRainbowFrame(uint32_t* frameBuffer, int offset);
void generateCurrentAnimationFrame(uint32_t* frameBuffer);
void showLightsAnimation();

#endif
