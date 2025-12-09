# MavenLED Installation Guide

Complete step-by-step installation instructions for MavenLED firmware.

## Table of Contents
- [Flash Pre-built Firmware](#flash-pre-built-firmware-easiest)
- [Hardware Setup](#hardware-setup)
- [Initial Configuration](#initial-configuration)
- [Troubleshooting](#troubleshooting)

---

## Flash Pre-built Firmware (Easiest)

### What You'll Need
- ESP32 development board
- USB cable (data-capable, not just charging)
- Computer with Chrome, Edge, or Opera browser
- Downloaded firmware file (.bin)

### Step-by-Step Instructions

#### 1. Download Firmware

1. Go to the [Releases page](https://github.com/NASA-0007/MavenLED-Bambu-Lab-Status-LED/releases)
2. Find the latest release
3. Download the `MavenLED_vX.X.X.bin` file
4. Remember where you saved it!

#### 2. Prepare ESP32

1. **Connect ESP32** to your computer using a USB cable
2. **Install USB drivers** if needed:
   - **Windows**: [CP210x Driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) or [CH340 Driver](http://www.wch-ic.com/downloads/CH341SER_EXE.html)
   - **Mac**: Usually no drivers needed
   - **Linux**: Usually no drivers needed

3. **Verify connection**:
   - Windows: Check Device Manager → Ports (COM & LPT) → Should see "Silicon Labs CP210x" or similar
   - Mac: Open Terminal, run `ls /dev/cu.*` → Should see something like `/dev/cu.usbserial-xxx`
   - Linux: Open Terminal, run `ls /dev/ttyUSB*` → Should see `/dev/ttyUSB0` or similar

#### 3. Flash Using ESP Web Flasher

**Method A: ESPTool Web (Recommended)**

1. Open **Chrome**, **Edge**, or **Opera** browser (Firefox doesn't support Web Serial)
2. Go to **[https://espressif.github.io/esptool-js/](https://espressif.github.io/esptool-js/)**
3. Click **"Connect"** button
4. Select your ESP32 from the popup list (e.g., "CP2102 USB to UART Bridge" or similar)
5. Click **"Connect"**
6. Once connected, click **"Choose Files"** under "Program"
7. Select the downloaded `.bin` file
8. **IMPORTANT**: Set **Flash Address** to `0x0`
   - Type `0x0` in the address field
   - This is a combined binary with bootloader and partitions
9. Click **"Program"
10. Wait for completion (progress bar will show status)
11. When finished, you'll see "Finished programming" message
12. Click **"Disconnect"**

**Method B: ESP Flash Download Tool (Windows Only)**

1. Download [Flash Download Tool](https://www.espressif.com/en/support/download/other-tools)
2. Extract and run `flash_download_tool_x.x.x.exe`
3. Select **ESP32** chip type
4. Select **Develop** mode
4. Click the "..." button to select your `.bin` file
6. Enter address: `0x0`
7. Check the checkbox next to the file
8. Select your COM port
9. Click **START**
10. Wait for "FINISH" message

#### 4. Verify Flash

1. Press the **RESET** button on your ESP32 (or unplug and plug back in)
2. Open **Serial Monitor** at 115200 baud (optional but helpful for debugging)
3. You should see startup messages like:
   ```
   ESP32 MQTT Client Starting...
   ✅ SPIFFS initialized
   🔧 AP Mode started!
   📱 Connect to: MavenLED-Setup
   ```

---

## Hardware Setup

### Required Components

| Component | Specification | Notes |
|-----------|--------------|-------|
| ESP32 Dev Board | Any ESP32 variant | Tested on ESP32-WROOM-32 |
| LED Strip | WS2812B | NeoPixel compatible |
| Power Supply | 5V, 2A minimum | Current depends on LED count (60mA per LED max) |
| Jumper Wires | - | For connections |

### Wiring Diagram

```
ESP32 Pin 17 (GPIO17) ────────► LED Strip Data IN
                                 
ESP32 GND ────────────────────► LED Strip GND
                                 
Power Supply 5V ──────────────► LED Strip VCC
Power Supply GND ─────────────► LED Strip GND + ESP32 GND (common ground)
```

**Important Notes:**
- ⚡ **Power**: LED strips draw significant current. For 60 LEDs at full white, you need ~3.6A. Don't power the strip from ESP32!
- 🔌 **Common Ground**: ESP32 ground MUST be connected to LED strip ground
- 📏 **Data Line**: Keep data wire as short as possible (< 1 meter recommended)
- 💡 **First LED**: Connect data line to "DI" or "Data IN" on the first LED

### LED Strip Power Calculation

| LED Count | Max Current (All White) | Recommended PSU |
|-----------|------------------------|-----------------|
| 30 LEDs | 1.8A | 5V 2A |
| 60 LEDs | 3.6A | 5V 4A |
| 120 LEDs | 7.2A | 5V 10A |

Formula: `Current (A) = LED_Count × 0.06A`

---

## Initial Configuration

### Step 1: Connect to Setup WiFi

1. Power on your ESP32
2. On your phone or computer, look for WiFi network: `MavenLED-Setup`
3. Connect using password: `mavenled123`
4. A setup page should open automatically
   - If not, open browser and go to `http://192.168.4.1`
   - Or try `http://mavenled.local`

### Step 2: Configure WiFi

1. On the web interface, go to **WiFi Management** section
2. Click **"Scan Networks"**
3. Select your home WiFi network from the list
4. Enter your WiFi password
5. Click **"Connect to WiFi"**
6. Device will restart and connect to your network

### Step 3: Find Your Device

After restart, find your device IP address:

**Option A: Using mDNS (easiest)**
- Open browser to `http://mavenled.local`

**Option B: Check Router**
- Log into your router's admin panel
- Look for device named "ESP32" or with MAC starting with the ESP32 chipset identifier

**Option C: Serial Monitor**
- Connect ESP32 to computer
- Open Serial Monitor at 115200 baud
- After boot, IP address will be displayed

### Step 4: Configure LED Strip

1. Open the web interface
2. Go to **LED Strip Configuration**
3. Enter your LED count (default is 60)
4. Click **"Update LED Count"**
5. Device will restart

### Step 5: Configure Printer MQTT

Choose your configuration mode:

#### Local Mode (Direct to Printer)
1. Toggle off **"Global Mode (Bambu Cloud)"**
2. Enter your printer's IP address
3. Enter your printer's MQTT access code
4. Enter your printer's serial number
5. Click **"Save MQTT Settings"**

#### Global Mode (via Bambu Cloud)
1. Toggle on **"Global Mode (Bambu Cloud)"**
2. Enter your Bambu Lab email
3. Enter your MakerWorld UID ([How to find](https://makerworld.com/api/v1/design-user-service/my/preference))
4. Click **"Login to Bambu Lab"**
5. Enter verification code sent to your email
6. Enter your printer's serial number
7. Click **"Save MQTT Settings"**

### Step 6: Customize (Optional)

- **Colors**: Customize colors for each printer state
- **Brightness**: Adjust global and night mode brightness
- **Animations**: Set animation directions
- **Night Mode**: Enable automatic dimming

---

## Troubleshooting

### Flashing Issues

**Problem**: "Failed to connect" when clicking Connect
- **Solution**: 
  - Try a different USB cable (must be data-capable)
  - Install USB drivers (see step 2)
  - Try a different USB port
  - Hold BOOT button while clicking Connect

**Problem**: "Failed to write data"
- **Solution**:
  - Make sure flash address is `0x10000`
  - Try erasing flash first using ESP Flash Tool
  - Check USB cable quality

**Problem**: Browser doesn't support Web Serial
- **Solution**: Use Chrome, Edge, or Opera browser (not Firefox or Safari)

### WiFi Issues

**Problem**: Can't see MavenLED-Setup WiFi
- **Solution**:
  - Wait 30 seconds after power-on
  - Check if already connected to your WiFi (won't broadcast AP)
  - Press RESET button on ESP32

**Problem**: Connected to setup WiFi but page won't load
- **Solution**:
  - Manually go to `http://192.168.4.1`
  - Disable mobile data on phone
  - Try different browser

**Problem**: Can't connect to home WiFi
- **Solution**:
  - Verify password is correct
  - Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
  - Check router doesn't block new devices
  - Try temporarily disabling AP isolation

### LED Issues

**Problem**: No LEDs light up
- **Solution**:
  - Check power supply is connected and adequate
  - Verify data line connected to GPIO 17
  - Check common ground between ESP32 and LED strip
  - Set LED count correctly in web interface

**Problem**: LEDs show wrong colors
- **Solution**:
  - Verify LED type is WS2812B
  - Check LED strip isn't damaged
  - Try lowering brightness

**Problem**: Only first few LEDs work
- **Solution**:
  - Insufficient power supply
  - Upgrade to higher current PSU
  - Lower brightness setting

### MQTT Issues

**Problem**: Printer status shows "Disconnected"
- **Solution**:
  - Verify printer IP address is correct
  - Check MQTT access code
  - Ensure printer is on same network
  - Verify printer's MQTT is enabled

**Problem**: Global mode authentication fails
- **Solution**:
  - Check email is correct
  - Verify MakerWorld UID is correct
  - Check internet connection
  - Try re-authenticating

### Serial Monitor Debugging

Open Serial Monitor at **115200 baud** to see detailed logs:

```
✅ = Success
❌ = Error  
⚠️ = Warning
🔧 = Configuration
📡 = Network activity
💾 = Storage operations
```

---

## Advanced Configuration

### OTA Updates (Over-the-Air)

Future firmware updates can be done wirelessly:
1. Download new `.bin` file
2. Go to web interface → Settings → Firmware Update
3. Select `.bin` file and upload
4. Wait for automatic restart

### Factory Reset

To reset all settings to default:
1. Connect to Serial Monitor at 115200 baud
2. Power on ESP32
3. Within 3 seconds, send command: `FACTORY_RESET`
4. Device will erase settings and restart in AP mode

### Backup Configuration

Save your settings:
1. Open web interface
2. Settings will be stored in browser console
3. Use browser dev tools → Console → Type `localStorage`
4. Copy and save values

---

## Getting Help

- **GitHub Issues**: [Report bugs or request features](https://github.com/NASA-0007/MavenLED-Bambu-Lab-Status-LED/issues)
- **Documentation**: Check other `.md` files in repository
- **Serial Logs**: Always include serial monitor output when reporting issues

---

**Need more help?** Open an issue with:
- Firmware version
- ESP32 board type
- LED strip type and count
- Serial monitor logs
- Steps to reproduce the problem
