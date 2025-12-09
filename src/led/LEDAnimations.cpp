#include "LEDAnimations.h"
#include "../config/Settings.h"
#include "../printer/PrinterState.h"

// Animation timing constants
const unsigned long ANIMATION_INTERVAL = 50;
const unsigned long RAINBOW_INTERVAL = 20;
const unsigned long LIGHTS_ANIMATION_DURATION = 1000;

// LED Animation Global Variables
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Function to reinitialize strip with new pin
void reinitializeStripPin(int pin) {
  strip.updateType(NEO_GRB + NEO_KHZ800);
  strip.setPin(pin);
  strip.begin();
  strip.show();
  Serial.printf(" LED strip reinitialized on GPIO %d\n", pin);
}
unsigned long last_update = 0;
unsigned long last_rainbow = 0;
int rainbow_offset = 0;

// Animation timing variables
unsigned long last_download_progress = 0;
unsigned long last_print_progress = 0;
unsigned long download_head_cycle_start = 0;
unsigned long printing_head_cycle_start = 0;

// Frame buffer for animation continuity
uint32_t* saved_frame_buffer = nullptr;
bool has_saved_frame = false;
String saved_animation_state = "";
int saved_progress = 0;
unsigned long saved_animation_time = 0;

// Rainbow state preservation
bool rainbow_paused = false;
int saved_rainbow_offset = 0;
unsigned long saved_rainbow_time = 0;

// Lights toggle animation
bool lights_turning_on = false;
bool lights_turning_off = false;
unsigned long lights_animation_start = 0;
int lights_animation_progress = 0;

// Get current color for a state (considering custom colors)
uint32_t getStateColor(int stateIndex) {
	if (stateIndex < 0 || stateIndex >= 8) return strip.Color(255, 255, 255);
	
	auto& color = settings.colors[stateIndex];
	uint32_t baseColor = strip.Color(color.r, color.g, color.b);
	
	// Apply brightness multiplier
	uint8_t brightness = settings.night_mode_enabled ? 
						settings.night_mode_brightness : 
						settings.global_brightness;
	
	if (brightness == 255) return baseColor;
	
	// Extract RGB and apply brightness
	uint8_t r = ((baseColor >> 16) & 0xFF) * brightness / 255;
	uint8_t g = ((baseColor >> 8) & 0xFF) * brightness / 255;
	uint8_t b = (baseColor & 0xFF) * brightness / 255;
	
	return strip.Color(r, g, b);
}

void showDownloadProgress() {
	// Check if progress has changed and reset head animation if needed
	if (printer_state.download_progress != last_download_progress) {
		unsigned long currentTime = millis();
		int progressLEDs = map(printer_state.download_progress, 0, 100, 0, settings.led_count);
		
		if (progressLEDs > 0) {
			unsigned long cycleTime = (progressLEDs * 80) + 2500;
			unsigned long timeInCurrentCycle = 0;
			
			if (download_head_cycle_start > 0) {
				timeInCurrentCycle = currentTime - download_head_cycle_start;
			}
			
			if (download_head_cycle_start == 0 || timeInCurrentCycle >= (progressLEDs * 80)) {
				download_head_cycle_start = currentTime;
			}
		} else {
			download_head_cycle_start = 0;
		}
		
		last_download_progress = printer_state.download_progress;
	}
	
	int progressLEDs = map(printer_state.download_progress, 0, 100, 0, settings.led_count);
	uint32_t downloadColor = getStateColor(2);
	int direction = settings.download_direction;
	
	for (int i = 0; i < progressLEDs; i++) {
		int ledIndex = (direction > 0) ? i : (settings.led_count - 1 - i);
		strip.setPixelColor(ledIndex, downloadColor);
	}
	
	if (progressLEDs > 0) {
		int headSpeed = 80 - (progressLEDs * 1);
		if (headSpeed < 30) headSpeed = 30;
		if (headSpeed > 80) headSpeed = 80;
		
		unsigned long cycleTime = (progressLEDs * headSpeed) + 2500;
		unsigned long timeInCycle = 0;
		if (download_head_cycle_start > 0) {
			timeInCycle = (millis() - download_head_cycle_start) % cycleTime;
		}
		
		unsigned long movementTime = progressLEDs * headSpeed;
		
		if (timeInCycle < movementTime) {
			int headPos = (timeInCycle / headSpeed) % progressLEDs;
			int ledIndex;
			if (direction > 0) {
				ledIndex = headPos;
			} else {
				int reversedProgressStart = settings.led_count - progressLEDs;
				ledIndex = reversedProgressStart + (progressLEDs - 1 - headPos);
			}
			
			uint8_t brightness = settings.night_mode_enabled ? settings.night_mode_brightness : settings.global_brightness;
			uint8_t headBrightness = brightness;
			strip.setPixelColor(ledIndex, strip.Color(headBrightness, headBrightness, headBrightness));
		}
	}
	
	if (progressLEDs > 5) {
		uint8_t brightness = settings.night_mode_enabled ? settings.night_mode_brightness : settings.global_brightness;
		int sparkle1 = (millis() / 200) % progressLEDs;
		int sparkle2 = (millis() / 300 + 10) % progressLEDs;
		
		uint8_t sparkle1Brightness = (150 * brightness) / 255;
		uint8_t sparkle2Brightness = (100 * brightness) / 255;
		
		strip.setPixelColor(sparkle1, strip.Color(sparkle1Brightness, sparkle1Brightness, 255 * brightness / 255));
		strip.setPixelColor(sparkle2, strip.Color(sparkle2Brightness, sparkle2Brightness, 255 * brightness / 255));
	}
}

void showPrintingProgress() {
	if (printer_state.progress != last_print_progress) {
		unsigned long currentTime = millis();
		float exactProgress = (float)printer_state.progress / 100.0 * settings.led_count;
		int fullLEDs = (int)exactProgress;
		float partialBrightness = exactProgress - fullLEDs;
		int totalLitArea = fullLEDs + (partialBrightness > 0.5 ? 1 : 0);
		
		if (totalLitArea > 0) {
			unsigned long cycleTime = (totalLitArea * 80) + 2500;
			unsigned long timeInCurrentCycle = 0;
			
			if (printing_head_cycle_start > 0) {
				timeInCurrentCycle = currentTime - printing_head_cycle_start;
			}
			
			if (printing_head_cycle_start == 0 || timeInCurrentCycle >= (totalLitArea * 80)) {
				printing_head_cycle_start = currentTime;
			}
		} else {
			printing_head_cycle_start = 0;
		}
		
		last_print_progress = printer_state.progress;
	}
	
	float exactProgress = (float)printer_state.progress / 100.0 * settings.led_count;
	int fullLEDs = (int)exactProgress;
	float partialBrightness = exactProgress - fullLEDs;
	int direction = settings.printing_direction;
	
	uint32_t printingColor = getStateColor(1);
	
	for (int i = 0; i < settings.led_count; i++) {
		int ledIndex = (direction > 0) ? i : (settings.led_count - 1 - i);
		
		if (i < fullLEDs) {
			strip.setPixelColor(ledIndex, printingColor);
		} else if (i == fullLEDs && partialBrightness > 0) {
			uint8_t r = ((printingColor >> 16) & 0xFF) * partialBrightness;
			uint8_t g = ((printingColor >> 8) & 0xFF) * partialBrightness;
			uint8_t b = (printingColor & 0xFF) * partialBrightness;
			strip.setPixelColor(ledIndex, strip.Color(r, g, b));
		} else {
			strip.setPixelColor(ledIndex, strip.Color(0, 0, 0));
		}
	}
	
	if (fullLEDs > 0) {
		int totalLitArea = fullLEDs + (partialBrightness > 0.5 ? 1 : 0);
		
		int headSpeed = 80 - (totalLitArea * 1);
		if (headSpeed < 30) headSpeed = 30;
		if (headSpeed > 80) headSpeed = 80;
		
		unsigned long cycleTime = (totalLitArea * headSpeed) + 2500;
		unsigned long timeInCycle = 0;
		if (printing_head_cycle_start > 0) {
			timeInCycle = (millis() - printing_head_cycle_start) % cycleTime;
		}
		
		unsigned long movementTime = totalLitArea * headSpeed;
		
		if (timeInCycle < movementTime) {
			int headPos = (timeInCycle / headSpeed) % totalLitArea;
			int ledIndex;
			if (direction > 0) {
				ledIndex = headPos;
			} else {
				int reversedProgressStart = settings.led_count - totalLitArea;
				ledIndex = reversedProgressStart + (totalLitArea - 1 - headPos);
			}
			
			uint8_t brightness = settings.night_mode_enabled ? settings.night_mode_brightness : settings.global_brightness;
			strip.setPixelColor(ledIndex, strip.Color(brightness, brightness, brightness));
		}
	}
}

void showPausedState() {
	uint32_t pausedColor = getStateColor(3);
	uint8_t brightness = (sin(millis() / 500.0) + 1) * 127;
	
	uint8_t r = ((pausedColor >> 16) & 0xFF) * brightness / 255;
	uint8_t g = ((pausedColor >> 8) & 0xFF) * brightness / 255;
	uint8_t b = (pausedColor & 0xFF) * brightness / 255;
	uint32_t color = strip.Color(r, g, b);
	
	for (int i = 0; i < settings.led_count; i++) {
		strip.setPixelColor(i, color);
	}
}

void showErrorState() {
	bool on = (millis() / 250) % 2;
	uint32_t color = on ? getStateColor(4) : strip.Color(0, 0, 0);
	
	for (int i = 0; i < settings.led_count; i++) {
		strip.setPixelColor(i, color);
	}
}

void showRecoverableErrorState() {
	unsigned long cycleTime = 1000;
	unsigned long timeInCycle = millis() % cycleTime;
	
	uint32_t color;
	if (timeInCycle < 500) {
		color = getStateColor(3);
	} else {
		color = getStateColor(4);
	}
	
	for (int i = 0; i < settings.led_count; i++) {
		strip.setPixelColor(i, color);
	}
}

void showHeatingState() {
	uint32_t heatingBaseColor = getStateColor(5);
	
	uint8_t baseR = (heatingBaseColor >> 16) & 0xFF;
	uint8_t baseG = (heatingBaseColor >> 8) & 0xFF;
	uint8_t baseB = heatingBaseColor & 0xFF;
	
	float timeOffset = millis() / 1000.0;
	float moveOffset = timeOffset * 2.0;
	
	for (int i = 0; i < settings.led_count; i++) {
		float wave1 = sin((i * 0.35 - moveOffset * 3.0)) * 0.5 + 0.5;
		float wave2 = sin((i * 0.5 - moveOffset * 4.5)) * 0.3 + 0.4;
		float heatBuildup = sin((i * 0.2 - moveOffset * 2.0)) * 0.2 + 0.8;
		
		float combined = (wave1 * 0.5 + wave2 * 0.4 + heatBuildup * 0.1);
		uint8_t brightness = combined * 255;
		
		float positionFactor = (float)i / settings.led_count;
		uint8_t red = (baseR * brightness / 255);
		uint8_t green = (baseG * brightness / 255) * (1.0 - 0.3 * positionFactor);
		uint8_t blue = (baseB * brightness / 255);
		
		strip.setPixelColor(i, strip.Color(red, green, blue));
	}
}

void showCoolingState() {
	int direction = -1;
	uint32_t coolingBaseColor = getStateColor(6);
	
	uint8_t baseR = (coolingBaseColor >> 16) & 0xFF;
	uint8_t baseG = (coolingBaseColor >> 8) & 0xFF;
	uint8_t baseB = coolingBaseColor & 0xFF;
	
	for (int i = 0; i < settings.led_count; i++) {
		int effectivePos = (direction > 0) ? i : (settings.led_count - 1 - i);
		float timeOffset = millis() / 1000.0;
		
		float wave1 = sin((effectivePos * 0.25 + timeOffset * direction * 2.5)) * 0.5 + 0.5;
		float wave2 = sin((effectivePos * 0.4 + timeOffset * direction * 3.5)) * 0.3 + 0.3;
		float tempDrop = sin((effectivePos * 0.15 + timeOffset * direction * 1.5)) * 0.2 + 0.8;
		
		float combined = (wave1 * 0.6 + wave2 * 0.3 + tempDrop * 0.1);
		uint8_t brightness = combined * 255;
		
		float positionFactor = (float)i / settings.led_count;
		uint8_t red = (baseR * brightness / 255);
		uint8_t green = (baseG * brightness / 255) * (0.7 + 0.3 * positionFactor);
		uint8_t blue = (baseB * brightness / 255);
		
		strip.setPixelColor(i, strip.Color(red, green, blue));
	}
}

void showFinishedState() {
	uint32_t finishedColor = getStateColor(7);
	uint8_t brightness = (sin(millis() / 1000.0) + 1) * 127;
	
	uint8_t r = ((finishedColor >> 16) & 0xFF) * brightness / 255;
	uint8_t g = ((finishedColor >> 8) & 0xFF) * brightness / 255;
	uint8_t b = (finishedColor & 0xFF) * brightness / 255;
	uint32_t color = strip.Color(r, g, b);
	
	for (int i = 0; i < settings.led_count; i++) {
		strip.setPixelColor(i, color);
	}
	
	if ((millis() / 100) % 10 == 0) {
		int sparklePos = random(settings.led_count);
		uint8_t sparkleBrightness = settings.night_mode_enabled ? settings.night_mode_brightness : settings.global_brightness;
		strip.setPixelColor(sparklePos, strip.Color(sparkleBrightness, sparkleBrightness, sparkleBrightness));
	}
}

void showIdleState() {
	if (!printer_state.is_heating && !printer_state.is_cooling) {
		uint32_t idleColor = getStateColor(0);
		unsigned long currentTime = millis();
		int direction = settings.idle_direction;
		
		uint8_t baseR = (idleColor >> 16) & 0xFF;
		uint8_t baseG = (idleColor >> 8) & 0xFF;
		uint8_t baseB = idleColor & 0xFF;
		
		static float wavePhase = 0;
		static float breathPhase = 0;
		static float sparklePhase = 0;
		
		wavePhase += 0.08 * direction;
		breathPhase += 0.02;
		sparklePhase += 0.05;
		
		float globalBreath = sin(breathPhase) * 0.15 + 0.85;
		
		for (int i = 0; i < settings.led_count; i++) {
			strip.setPixelColor(i, strip.Color(0, 0, 0));
		}
		
		for (int i = 0; i < settings.led_count; i++) {
			float ledPosition = (float)i / settings.led_count;
			
			float wave1 = sin((ledPosition * 6.28) + wavePhase) * 0.5 + 0.5;
			float wave2 = sin((ledPosition * 12.56) + (wavePhase * 1.3)) * 0.3 + 0.3;
			float wave3 = sin((ledPosition * 18.84) + (sparklePhase * 2)) * 0.2 + 0.2;
			
			float combinedIntensity = (wave1 * 0.6) + (wave2 * 0.3) + (wave3 * 0.1);
			combinedIntensity *= globalBreath;
			
			float positionGradient = sin(ledPosition * 3.14159) * 0.2 + 0.8;
			
			uint8_t r = (baseR * combinedIntensity * positionGradient);
			uint8_t g = (baseG * combinedIntensity * positionGradient);
			uint8_t b = (baseB * combinedIntensity * positionGradient);
			
			if (combinedIntensity > 0.8) {
				float highlight = (combinedIntensity - 0.8) * 5;
				r = min(255, r + (int)(highlight * 40));
				g = min(255, g + (int)(highlight * 40));
				b = min(255, b + (int)(highlight * 40));
			}
			
			strip.setPixelColor(i, strip.Color(r, g, b));
		}
		
		static unsigned long lastSparkleTime = 0;
		static float sparklePosition = 0;
		static bool sparkleActive = false;
		static unsigned long sparkleStartTime = 0;
		
		if (currentTime - lastSparkleTime > random(3000, 5000)) {
			lastSparkleTime = currentTime;
			sparkleActive = true;
			sparkleStartTime = currentTime;
			sparklePosition = (direction > 0) ? 0 : settings.led_count - 1;
		}
		
		if (sparkleActive) {
			unsigned long sparkleAge = currentTime - sparkleStartTime;
			if (sparkleAge < 2000) {
				float progress = (float)sparkleAge / 2000.0;
				sparklePosition = (direction > 0) ? 
					progress * settings.led_count : 
					(1.0 - progress) * settings.led_count;
				
				int centerPos = (int)sparklePosition;
				float sparkleIntensity = sin(progress * 3.14159) * 0.8 + 0.2;
				
				uint8_t brightnessMultiplier = settings.night_mode_enabled ? 
											  settings.night_mode_brightness : 
											  settings.global_brightness;
				float brightnessFactor = (float)brightnessMultiplier / 255.0;
				
				if (centerPos >= 0 && centerPos < settings.led_count) {
					uint8_t sparkleBoost = (int)(sparkleIntensity * 100 * brightnessFactor);
					uint8_t sparkleR = min(255, baseR + sparkleBoost);
					uint8_t sparkleG = min(255, baseG + sparkleBoost);
					uint8_t sparkleB = min(255, baseB + sparkleBoost);
					strip.setPixelColor(centerPos, strip.Color(sparkleR, sparkleG, sparkleB));
				}
				
				for (int trail = 1; trail <= 3; trail++) {
					int trailPos = centerPos - (trail * direction);
					if (trailPos >= 0 && trailPos < settings.led_count) {
						float trailIntensity = sparkleIntensity / (trail + 1);
						uint8_t currentR, currentG, currentB;
						uint32_t currentColor = strip.getPixelColor(trailPos);
						currentR = (currentColor >> 16) & 0xFF;
						currentG = (currentColor >> 8) & 0xFF;
						currentB = currentColor & 0xFF;
						
						uint8_t trailBoost = (int)(trailIntensity * 50 * brightnessFactor);
						uint8_t trailR = min(255, currentR + trailBoost);
						uint8_t trailG = min(255, currentG + trailBoost);
						uint8_t trailB = min(255, currentB + trailBoost);
						strip.setPixelColor(trailPos, strip.Color(trailR, trailG, trailB));
					}
				}
			} else {
				sparkleActive = false;
			}
		}
	}
	else return;
}

void showAutoOffState() {
	// Turn off all LEDs
	strip.clear();
}

void rainbowAnimation() {
	if (millis() - last_rainbow < RAINBOW_INTERVAL)
		return;
	last_rainbow = millis();
	
	int direction = settings.rainbow_direction;
	
	for (int i = 0; i < settings.led_count; i++) {
		int pos = (i * direction + rainbow_offset * direction) & 255;
		if (pos < 0) pos += 256;
		
		uint32_t wheelColor = wheel(pos);
		
		uint8_t brightness = settings.night_mode_enabled ? settings.night_mode_brightness : settings.global_brightness;
		if (brightness != 255) {
			uint8_t r = ((wheelColor >> 16) & 0xFF) * brightness / 255;
			uint8_t g = ((wheelColor >> 8) & 0xFF) * brightness / 255;
			uint8_t b = (wheelColor & 0xFF) * brightness / 255;
			wheelColor = strip.Color(r, g, b);
		}
		
		strip.setPixelColor(i, wheelColor);
	}
	strip.show();
	
	rainbow_offset += direction;
	if (rainbow_offset >= 256 || rainbow_offset < 0) {
		rainbow_offset = direction > 0 ? 0 : 255;
	}
}

uint32_t wheel(byte wheelPos) {
	wheelPos = 255 - wheelPos;
	if (wheelPos < 85) {
		return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3);
	}
	if (wheelPos < 170) {
		wheelPos -= 85;
		return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3);
	}
	wheelPos -= 170;
	return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

void startupAnimation() {
	for (int i = 0; i < settings.led_count; i++) {
		strip.setPixelColor(i, strip.Color(0, 0, 255));
		strip.show();
		delay(30);
	}
	
	delay(500);
	
	for (int i = 0; i < settings.led_count; i++) {
		strip.setPixelColor(i, 0);
		strip.show();
		delay(20);
	}
}

void captureCurrentFrame() {
	if (settings.led_count <= 0 || settings.led_count > 1000) {
		Serial.println(" Invalid LED count for frame capture");
		return;
	}
	
	if (saved_frame_buffer == nullptr) {
		saved_frame_buffer = (uint32_t*)malloc(settings.led_count * sizeof(uint32_t));
		if (saved_frame_buffer == nullptr) {
			Serial.println(" Failed to allocate frame buffer");
			return;
		}
	}
	
	if (saved_frame_buffer != nullptr) {
		for (int i = 0; i < settings.led_count; i++) {
			if (i >= 0 && i < settings.led_count) {
				saved_frame_buffer[i] = strip.getPixelColor(i);
			}
		}
		
		saved_animation_state = printer_state.status;
		saved_progress = (printer_state.status == "printing") ? printer_state.progress : 
						 (printer_state.status == "downloading") ? printer_state.download_progress : 0;
		saved_animation_time = millis();
		
		if (!printer_state.is_connected || printer_state.status == "unknown") {
			saved_rainbow_offset = rainbow_offset;
			saved_rainbow_time = last_rainbow;
			rainbow_paused = true;
			Serial.printf(" Rainbow paused at offset: %d\n", saved_rainbow_offset);
		} else {
			rainbow_paused = false;
		}
		
		has_saved_frame = true;
		
		Serial.printf(" Frame captured: State=%s, Progress=%d%%, LEDs=%d, Rainbow=%s\n", 
					  saved_animation_state.c_str(), saved_progress, settings.led_count,
					  rainbow_paused ? "PAUSED" : "N/A");
	}
}

void restoreSavedFrame() {
	if (saved_frame_buffer != nullptr && has_saved_frame && settings.led_count > 0) {
		for (int i = 0; i < settings.led_count; i++) {
			if (i >= 0 && i < settings.led_count) {
				strip.setPixelColor(i, saved_frame_buffer[i]);
			}
		}
		Serial.printf(" Frame restored: State=%s, Progress=%d%%\n", 
					  saved_animation_state.c_str(), saved_progress);
	} else {
		Serial.println("️ Cannot restore frame - invalid buffer or LED count");
	}
}

bool shouldResumeAnimation() {
	if (!has_saved_frame) return false;
	
	if (rainbow_paused && (!printer_state.is_connected || printer_state.status == "unknown")) {
		Serial.printf(" Resuming rainbow from offset: %d\n", saved_rainbow_offset);
		return true;
	}
	
	bool sameState = (saved_animation_state == printer_state.status);
	
	if (sameState && (saved_animation_state == "printing" || saved_animation_state == "downloading")) {
		int currentProgress = (saved_animation_state == "printing") ? 
							  printer_state.progress : printer_state.download_progress;
		int progressDiff = abs(currentProgress - saved_progress);
		
		if (progressDiff <= 5) {
			Serial.printf(" Resuming animation: %s from %d%% to %d%%\n", 
						  saved_animation_state.c_str(), saved_progress, currentProgress);
			return true;
		}
	}
	
	if (sameState && saved_animation_state != "printing" && saved_animation_state != "downloading") {
		Serial.printf(" Resuming animation: %s (continuous)\n", saved_animation_state.c_str());
		return true;
	}
	
	Serial.printf(" State changed: %s -> %s, starting fresh\n", 
				  saved_animation_state.c_str(), printer_state.status.c_str());
	return false;
}

void generateRainbowFrame(uint32_t* frameBuffer, int offset) {
	int direction = settings.rainbow_direction;
	
	for (int i = 0; i < settings.led_count; i++) {
		int pos = (i * direction + offset * direction) & 255;
		if (pos < 0) pos += 256;
		
		uint32_t wheelColor = wheel(pos);
		
		uint8_t brightness = settings.night_mode_enabled ? settings.night_mode_brightness : settings.global_brightness;
		if (brightness != 255) {
			uint8_t r = ((wheelColor >> 16) & 0xFF) * brightness / 255;
			uint8_t g = ((wheelColor >> 8) & 0xFF) * brightness / 255;
			uint8_t b = (wheelColor & 0xFF) * brightness / 255;
			wheelColor = strip.Color(r, g, b);
		}
		
		frameBuffer[i] = wheelColor;
	}
}

void generateCurrentAnimationFrame(uint32_t* frameBuffer) {
	for (int i = 0; i < settings.led_count; i++) {
		frameBuffer[i] = strip.Color(0, 0, 0);
	}
	
	if (!printer_state.is_connected || printer_state.status == "unknown") {
		generateRainbowFrame(frameBuffer, rainbow_offset);
		return;
	}
	
	if (printer_state.status == "printing") {
		int progressLEDs = map(printer_state.progress, 0, 100, 0, settings.led_count);
		uint32_t printColor = getStateColor(1);
		
		for (int i = 0; i < progressLEDs && i < settings.led_count; i++) {
			int ledIndex = (settings.printing_direction > 0) ? i : (settings.led_count - 1 - i);
			frameBuffer[ledIndex] = printColor;
		}
	}
	else if (printer_state.status == "downloading") {
		int progressLEDs = map(printer_state.download_progress, 0, 100, 0, settings.led_count);
		uint32_t downloadColor = getStateColor(2);
		
		for (int i = 0; i < progressLEDs && i < settings.led_count; i++) {
			int ledIndex = (settings.download_direction > 0) ? i : (settings.led_count - 1 - i);
			frameBuffer[ledIndex] = downloadColor;
		}
	}
	else if (printer_state.status == "heating") {
		uint32_t heatColor = getStateColor(5);
		for (int i = 0; i < settings.led_count; i++) {
			frameBuffer[i] = heatColor;
		}
	}
	else if (printer_state.status == "cooling") {
		uint32_t coolColor = getStateColor(6);
		for (int i = 0; i < settings.led_count; i++) {
			frameBuffer[i] = coolColor;
		}
	}
	else {
		uint32_t stateColor = getStateColor(0);
		if (printer_state.status == "paused") stateColor = getStateColor(3);
		else if (printer_state.status == "error") stateColor = getStateColor(4);
		else if (printer_state.status == "finished") stateColor = getStateColor(7);
		else if (printer_state.status == "idle") stateColor = getStateColor(0);
		
		for (int i = 0; i < settings.led_count; i++) {
			frameBuffer[i] = stateColor;
		}
	}
}

void showLightsAnimation() {
	unsigned long elapsed = millis() - lights_animation_start;
	float progress = (float)elapsed / LIGHTS_ANIMATION_DURATION;
	
	if (progress >= 1.0f) {
		progress = 1.0f;
		
		if (lights_turning_off) {
			settings.lights_off_override = true;
			saveSettings();
			Serial.println(" Lights OFF animation complete - saved to EEPROM");
		} else if (lights_turning_on) {
			settings.lights_off_override = false;
			saveSettings();
			Serial.println(" Lights ON animation complete - saved to EEPROM");
		}
		
		lights_turning_on = false;
		lights_turning_off = false;
		return;
	}
	
	float easedProgress = 1 - pow(1 - progress, 3);
	
	int totalLEDs = settings.led_count;
	int middle = totalLEDs / 2;
	int maskRadius = (int)(easedProgress * (totalLEDs / 2.0f));
	
	if (lights_turning_off) {
		if (saved_frame_buffer != nullptr && has_saved_frame) {
			restoreSavedFrame();
		} else {
			strip.clear();
			if (printer_state.is_connected && printer_state.status != "unknown") {
				uint32_t stateColor = strip.Color(255, 255, 255);
				if (printer_state.status == "idle") stateColor = getStateColor(0);
				else if (printer_state.status == "printing") stateColor = getStateColor(1);
				else if (printer_state.status == "downloading") stateColor = getStateColor(2);
				
				for (int i = 0; i < totalLEDs; i++) {
					strip.setPixelColor(i, stateColor);
				}
			}
		}
		
		for (int i = 0; i < maskRadius; i++) {
			int leftLED = i;
			int rightLED = totalLEDs - 1 - i;
			
			if (leftLED < middle) {
				strip.setPixelColor(leftLED, strip.Color(0, 0, 0));
			}
			if (rightLED >= middle) {
				strip.setPixelColor(rightLED, strip.Color(0, 0, 0));
			}
		}
	} 
	else if (lights_turning_on) {
		strip.clear();
		
		if (shouldResumeAnimation() && saved_frame_buffer != nullptr) {
			if (rainbow_paused && (!printer_state.is_connected || printer_state.status == "unknown")) {
				rainbow_offset = saved_rainbow_offset;
				last_rainbow = saved_rainbow_time;
				rainbow_paused = false;
				Serial.printf(" Rainbow state restored: offset=%d\n", rainbow_offset);
			}
			
			uint32_t* currentFrame = (uint32_t*)malloc(settings.led_count * sizeof(uint32_t));
			if (currentFrame == nullptr) {
				Serial.println(" Failed to allocate current frame buffer");
				return;
			}
			
			strip.clear();
			generateCurrentAnimationFrame(currentFrame);
			
			for (int i = 0; i < totalLEDs && i < settings.led_count; i++) {
				uint32_t savedColor = saved_frame_buffer[i];
				uint32_t currentColor = currentFrame[i];
				
				uint8_t savedR = (savedColor >> 16) & 0xFF;
				uint8_t savedG = (savedColor >> 8) & 0xFF;
				uint8_t savedB = savedColor & 0xFF;
				
				uint8_t currentR = (currentColor >> 16) & 0xFF;
				uint8_t currentG = (currentColor >> 8) & 0xFF;
				uint8_t currentB = currentColor & 0xFF;
				
				uint8_t blendedR = savedR + (currentR - savedR) * easedProgress;
				uint8_t blendedG = savedG + (currentG - savedG) * easedProgress;
				uint8_t blendedB = savedB + (currentB - savedB) * easedProgress;
				
				strip.setPixelColor(i, strip.Color(blendedR, blendedG, blendedB));
			}
			
			free(currentFrame);
			currentFrame = nullptr;
			
			for (int i = maskRadius; i < totalLEDs / 2; i++) {
				int leftLED = middle - i - 1;
				int rightLED = middle + i;
				
				if (leftLED >= 0) {
					strip.setPixelColor(leftLED, strip.Color(0, 0, 0));
				}
				if (rightLED < totalLEDs) {
					strip.setPixelColor(rightLED, strip.Color(0, 0, 0));
				}
			}
		} 
		else {
			uint32_t animationColor = strip.Color(255, 255, 255);
			if (printer_state.is_connected && printer_state.status != "unknown") {
				if (printer_state.status == "idle") animationColor = getStateColor(0);
				else if (printer_state.status == "printing") animationColor = getStateColor(1);
				else if (printer_state.status == "downloading") animationColor = getStateColor(2);
				else if (printer_state.status == "paused") animationColor = getStateColor(3);
				else if (printer_state.status == "error") animationColor = getStateColor(4);
				else if (printer_state.status == "heating") animationColor = getStateColor(5);
				else if (printer_state.status == "cooling") animationColor = getStateColor(6);
				else if (printer_state.status == "finished") animationColor = getStateColor(7);
			}
			
			for (int i = 0; i < maskRadius; i++) {
				int leftLED = middle - i - 1;
				int rightLED = middle + i;
				
				if (leftLED >= 0) {
					strip.setPixelColor(leftLED, animationColor);
				}
				if (rightLED < totalLEDs) {
					strip.setPixelColor(rightLED, animationColor);
				}
			}
		}
	}
	
	strip.show();
}

void updateLEDDisplay() {
	if (lights_turning_on || lights_turning_off) {
		showLightsAnimation();
		return;
	}
	
	if (settings.lights_off_override) {
		strip.clear();
		strip.show();
		return;
	}
	
	if (!printer_state.is_connected) {
		rainbowAnimation();
		return;
	}
	
	if (printer_state.status == "unknown" || printer_state.status.length() == 0) {
		rainbowAnimation();
		return;
	}
	
	if (millis() - last_update < ANIMATION_INTERVAL)
		return;
	last_update = millis();
	
	strip.clear();
	
	if (printer_state.status == "auto_off") {
		showAutoOffState();
	} else if (printer_state.status == "downloading") {
		showDownloadProgress();
	} else if (printer_state.status == "printing" || printer_state.status == "RUNNING") {
		showPrintingProgress();
	} else if (printer_state.status == "paused" || printer_state.status == "PAUSE") {
		showPausedState();
	} else if (printer_state.status == "recoverable_error") {
		showRecoverableErrorState();
	} else if (printer_state.status == "error" || printer_state.status == "FAILED") {
		showErrorState();
	} else if (printer_state.status == "heating") {
		showHeatingState();
	} else if (printer_state.status == "cooling") {
		showCoolingState();
	} else if (printer_state.status == "finished" || printer_state.status == "FINISH") {
		showFinishedState();
	} else if (printer_state.status == "idle" || printer_state.status == "IDLE") {
		showIdleState();
	} else {
		rainbowAnimation();
		return;
	}
	
	strip.show();
}
