# MavenLED 🎨💡

<div align="center">

![License](https://img.shields.io/badge/license-GPL--3.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)
![Arduino](https://img.shields.io/badge/Arduino-compatible-blue.svg)

**A comprehensive ESP32-based LED strip controller for 3D printer status monitoring with full MQTT remote control**

[Features](#features) • [Installation](#installation) • [Documentation](#documentation) • [Contributing](#contributing) • [License](#license)

</div>

---

## Features

### LED Control
- ✨ Dynamic LED animations for different printer states
- 🎨 Customizable colors for each state
- 🌙 Night mode with adjustable brightness
- 🔄 Configurable animation directions
- 📱 Web-based control interface
- 💾 Settings persistence in EEPROM

### MQTT Integration
- 🌐 Remote control via MQTT
- 🔄 Real-time status updates
- 📡 Command acknowledgments
- 🔧 Full LED control
- 📊 Device status monitoring
- 🖨️ **NEW!** Dedicated printer status topic with real-time updates
- 🎨 **NEW!** Simplified individual color setting by state name
- 🔄 **NEW!** Boolean direction values (true/false) for easier control

### 3D Printer Integration
- 🖨️ Bambu Lab printer MQTT support
- 📊 Real-time print status monitoring
- 🔥 Temperature monitoring (heating/cooling)
- 📈 Print progress visualization
- ⏱️ Remaining time display

## Hardware Requirements

- ESP32 development board
- WS2812B LED strip (default 60 LEDs)
- Power supply for LED strip

## Pin Configuration

| Component | GPIO Pin | Description |
|-----------|----------|-------------|
| LED Strip | 17 | Data line for WS2812B |

## 🚀 Quick Start

### Option 1: Flash Pre-built Firmware (Recommended for Users)

**No Arduino IDE or coding required!** Just flash the firmware directly to your ESP32.

#### Step 1: Download Firmware
1. Go to [Releases](https://github.com/NASA-0007/MavenLED-Bambu-Lab-Status-LED/releases)
2. Download the latest `MavenLED_vX.X.X.bin` file

#### Step 2: Flash to ESP32
1. Visit **[ESP Tool Web Flasher](https://espressif.github.io/esptool-js/)** in Chrome or Edge browser
2. Connect your ESP32 to your computer via USB
3. Click **"Connect"** and select your ESP32's COM port
4. Click **"Choose File"** and select the downloaded `.bin` file
5. Set **Flash Address** to `0x10000`
6. Click **"Program"**
7. Wait for the flashing process to complete (usually 30-60 seconds)
8. Click **"Disconnect"** and reset your ESP32

> **Note**: For detailed troubleshooting, see [INSTALLATION.md](docs/INSTALLATION.md)

#### Step 3: Initial Setup
1. Power on your ESP32
2. Connect to the WiFi network `MavenLED-Setup` (password: `mavenled123`)
3. Your browser should open automatically to the setup page (or go to `http://192.168.4.1`)
4. Configure your WiFi and printer MQTT settings
5. Device will restart and connect to your network

---

### Option 2: Build from Source (For Developers)
1. Connect WS2812B LED strip data line to **GPIO 17**
2. Connect appropriate power supply to LED strip (5V, capacity depends on LED count)
3. Flash the ESP32 with the MavenLED firmware

### Software Installation

#### Prerequisites
- [Arduino IDE](https://www.arduino.cc/en/software) (1.8.x or later) or [PlatformIO](https://platformio.org/)
- ESP32 board support installed

#### Step 1: Clone Repository
```bash
git clone https://github.com/NASA-0007/MavenLED-Bambu-Lab-Status-LED.git
## 📚 Documentation

- **[INSTALLATION.md](docs/INSTALLATION.md)** - Complete installation and setup guide

### Core Dependencies

| Library | Purpose | Installation |
|---------|---------|--------------|
| Adafruit_NeoPixel | LED strip control | Arduino Library Manager |
| ArduinoJson | JSON parsing | Arduino Library Manager |
| PubSubClient | MQTT communication | Arduino Library Manager |
3. Configure WiFi credentials (optional - can use web interface)
4. Upload to ESP32

#### Step 4: Initial Configuration
1. Power on ESP32
2. Connect to WiFi or use AP mode (`MavenLED-Setup`)
3. Access web interface at `http://mavenled.local` or `http://192.168.4.1` (AP mode)
4. Configure MQTT settings for your printer

## Required Libraries

```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
```

## Web Interface

Access the web interface at:
- `http://mavenled.local` (mDNS)
- `http://[ESP32_IP_ADDRESS]`

### Features:
- 🎨 Color customization for all LED states
- 💡 Brightness control and night mode
- 🔄 Animation direction settings
- 🌐 MQTT configuration
- 📊 Real-time status display

## MQTT Remote Control

### Quick Start
1. Configure MQTT broker in web interface
2. Subscribe to command topic: `mavenled/{device_id}/cmd`
3. Send JSON commands for control

### Basic Commands

#### Set LED Brightness
```json
{
  "command": "set_brightness",
  "id": "cmd_002",
  "value": 150
}
```

## LED States

| State | Default Color | Animation | Description |
|-------|---------------|-----------|-------------|
| Idle | Green | Breathing wave | Printer ready |
| Printing | Indigo | Progress bar | Print in progress |
| Download | Blue | Moving head | File downloading |
| Paused | Yellow | Static | Print paused |
| Error | Red | Blinking | Error occurred |
| Heating | Orange | Moving dots | Heating up |
| Cooling | Cyan | Reverse dots | Cooling down |
| Finished | Bright Green | Celebration | Print complete |

## Configuration

### WiFi Settings
- Default SSID: `NAVI@AirFiber`
- Configurable via web interface
- WiFiManager fallback for easy setup

### MQTT Settings (Default)
- **Remote Control MQTT**: `broker.hivemq.com:1883`

### Hardware Settings
- **LED Count**: 60 (configurable)
- **Global Brightness**: 255 (configurable)
- **Night Mode Brightness**: 25 (configurable)

## API Endpoints

### LED Control
- `GET /api/status` - Device status
- `POST /api/settings` - Update settings
- `POST /api/colors` - Set custom colors
## 📄 License

This project is licensed under the **GNU General Public License v3.0** - see the [LICENSE](LICENSE) file for details.

### What this means:
- ✅ You can use this software commercially
- ✅ You can modify and distribute the software
- ✅ You can use this software privately
- ⚠️ You must disclose source code when distributing
- ⚠️ You must include the original license and copyright
- ⚠️ Changes must be documented

## 🤝 Contributing

Contributions are welcome and appreciated! Here's how you can help:

### How to Contribute

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/AmazingFeature`)
3. **Commit** your changes (`git commit -m 'Add some AmazingFeature'`)
4. **Push** to the branch (`git push origin feature/AmazingFeature`)
5. **Open** a Pull Request

### Reporting Issues

Found a bug? Have a feature request?
- Check if the issue already exists
- Provide clear steps to reproduce
- Include relevant logs and screenshots
- Specify your hardware and software versions

### Code Style

- Follow existing code conventions
- Comment complex logic
- Test thoroughly before submitting
- Update documentation as needed

## Troubleshooting

### WiFi Connection Issues
- Check SSID and password in settings
- Use WiFiManager for initial setup
- Verify 2.4GHz network compatibility

### MQTT Connection Issues
- Verify broker address and port
- Check firewall settings
- Ensure device ID is unique

### LED Issues
- Verify power supply capacity
- Check data line connection to GPIO 17
- Confirm LED count setting matches strip

## 💬 Support & Community

- **Issues**: [GitHub Issues](https://github.com/NASA-0007/MavenLED-Bambu-Lab-Status-LED/issues)
- **Discussions**: [GitHub Discussions](https://github.com/NASA-0007/MavenLED-Bambu-Lab-Status-LED/discussions)
- **Documentation**: See [docs](docs/) folder for detailed guides

## 🙏 Acknowledgments

- Built for Bambu Lab 3D printers
- Uses Adafruit NeoPixel library
- MQTT integration via PubSubClient
- Created by [NASA-0007](https://github.com/NASA-0007)

## ⭐ Star History

If you find this project useful, please consider giving it a star! It helps others discover the project.

---

<div align="center">

**Made with ❤️ for the 3D printing community**

</div>
This project is open source. Feel free to modify and distribute according to your needs.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Changelog

### Latest Changes (v2.0) 🎉
- ✅ **NEW!** Dedicated printer status MQTT topic (`MavenLED/<chip_id>/printer`)
- ✅ **NEW!** Real-time printer status updates (every 10s + on change)
- ✅ **NEW!** Simplified color setting with `set_color` command (set individual colors by state name)
- ✅ **NEW!** Boolean direction values (true/false instead of 1/-1)
- ✅ **NEW!** Individual direction setting with `set_direction` command
- ✅ Enhanced buffer size for remote MQTT (2KB for reliable JSON handling)
- ✅ Added keepalive setting for stable MQTT connections
- ✅ Comprehensive debugging for MQTT command reception

### Previous Updates
- ✅ Added comprehensive MQTT remote control commands
- ✅ Updated documentation with MQTT integration
- ✅ Improved command acknowledgment system

## Support

For support and questions, please open an issue in the repository or refer to the comprehensive documentation provided.