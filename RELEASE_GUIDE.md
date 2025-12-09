# Creating GitHub Releases for MavenLED

This guide shows how to create releases on GitHub for distributing pre-built firmware.

## Prerequisites

- GitHub account with access to the repository
- Compiled `.bin` firmware file
- Updated version number in code
- Tested firmware on hardware

---

## Step 1: Prepare the Firmware Binary

### Using Arduino IDE

1. Open `MavenLED.ino` in Arduino IDE
2. Select **Sketch → Export Compiled Binary**
3. Wait for compilation to complete
4. The `.bin` file will be in the sketch folder
5. Look for: `MavenLED.ino.esp32.bin` or similar
6. Rename to: `MavenLED_v1.0.0.bin` (use your version number)

### Using PlatformIO

1. Open terminal in project directory
2. Run: `pio run`
3. Binary will be in `.pio/build/esp32dev/firmware.bin`
4. Copy and rename to: `MavenLED_v1.0.0.bin`

---

## Step 2: Update Version Information

Before creating a release, update version in your code:

### In Main File (MavenLED.ino)

Add near the top:
```cpp
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_DATE "2025-01-15"
```

### In README.md

Update the version badge and changelog:
```markdown
## Changelog

### v1.0.0 (2025-01-15)
- Initial release
- Full MQTT control
- Web interface
- Printer status monitoring
```

Commit and push these changes:
```bash
git add .
git commit -m "Bump version to v1.0.0"
git push origin main
```

---

## Step 3: Create a Git Tag

Tags mark specific points in repository history.

### Create and Push Tag

```bash
# Create annotated tag
git tag -a v1.0.0 -m "Release version 1.0.0"

# Push tag to GitHub
git push origin v1.0.0
```

**Tag Naming Convention:**
- `v1.0.0` - Major release
- `v1.1.0` - Minor update (new features)
- `v1.0.1` - Patch (bug fixes)

---

## Step 4: Create GitHub Release

### Using GitHub Web Interface

1. **Navigate to Repository**
   - Go to your GitHub repository
   - Example: `https://github.com/YOUR_USERNAME/MavenLED`

2. **Go to Releases**
   - Click on **"Releases"** in the right sidebar
   - Or go directly to: `https://github.com/YOUR_USERNAME/MavenLED/releases`

3. **Draft New Release**
   - Click **"Draft a new release"** button

4. **Configure Release**

   **Tag:** 
   - Select existing tag `v1.0.0` from dropdown
   - Or create new tag by typing `v1.0.0`

   **Release Title:**
   ```
   MavenLED v1.0.0 - Initial Release
   ```

   **Description:** (Use markdown formatting)
   ```markdown
   ## 🎉 MavenLED v1.0.0 - Initial Release
   
   First stable release of MavenLED - ESP32 LED controller for 3D printer monitoring!
   
   ### ✨ Features
   - 🎨 Full RGB LED strip control with 8 customizable states
   - 🖨️ Real-time 3D printer status monitoring via MQTT
   - 🌐 Web-based configuration interface
   - 📱 Remote control via MQTT commands
   - 💾 Persistent settings storage
   - 🌙 Night mode with adjustable brightness
   - 🔄 Configurable animation directions
   
   ### 📦 Installation
   
   **Quick Start (No Coding Required):**
   1. Download `MavenLED_v1.0.0.bin` below
   2. Visit [ESP Web Flasher](https://espressif.github.io/esptool-js/)
   3. Flash to ESP32 at address `0x0`
   4. Connect to `MavenLED-Setup` WiFi
   5. Configure via web interface
   
   **Detailed Instructions:** See [INSTALLATION.md](https://github.com/YOUR_USERNAME/MavenLED/blob/main/INSTALLATION.md)
   
   ### 🔧 Hardware Requirements
   - ESP32 development board
   - WS2812B LED strip (NeoPixel compatible)
   - 5V power supply (capacity depends on LED count)
   
   ### 📚 Documentation
   - [User Guide](https://github.com/YOUR_USERNAME/MavenLED/blob/main/README.md)
   - [Installation Guide](https://github.com/YOUR_USERNAME/MavenLED/blob/main/INSTALLATION.md)
   - [MQTT Commands](https://github.com/YOUR_USERNAME/MavenLED/blob/main/MQTT_COMMANDS.md)
   - [Library Requirements](https://github.com/YOUR_USERNAME/MavenLED/blob/main/LIBRARIES.md)
   
   ### ⚙️ Default Configuration
   - LED Count: 60
   - LED Pin: GPIO 17
   - Setup WiFi: MavenLED-Setup / mavenled123
   - Web Interface: http://mavenled.local
   
   ### 🐛 Known Issues
   - None currently
   
   ### 📝 Changelog
   - Initial stable release
   - Tested with Bambu Lab P1P/P1S/X1C printers
   - Web interface optimized for mobile and desktop
   
   ### 🙏 Credits
   - Built for Bambu Lab 3D printer community
   - Uses Adafruit NeoPixel library
   - Created by [NASA-0007](https://github.com/NASA-0007)
   
   ---
   
   **Full Changelog**: https://github.com/YOUR_USERNAME/MavenLED/commits/v1.0.0
   ```

5. **Attach Binary Files**
   - Scroll to **"Attach binaries"** section
   - Drag and drop `MavenLED_v1.0.0.bin`
   - Or click to browse and select file
   - File will upload and appear in list

6. **Additional Files (Optional)**
   - Add `INSTALLATION.md` as a separate download
   - Add any hardware schematics or diagrams
   - Add configuration examples

7. **Set Release Options**
   - ✅ Check **"Set as the latest release"** (for stable releases)
   - ⚠️ Check **"Set as a pre-release"** (for beta/alpha versions)
   - 📝 Uncheck if this is a draft (not ready to publish)

8. **Publish Release**
   - Click **"Publish release"** button
   - Release is now live!

---

## Step 5: Verify Release

After publishing:

1. **Check Release Page**
   - Visit: `https://github.com/YOUR_USERNAME/MavenLED/releases`
   - Verify version appears correctly
   - Test download link works

2. **Update README Badges**
   - Add release badge to README.md:
   ```markdown
   ![Release](https://img.shields.io/github/v/release/YOUR_USERNAME/MavenLED)
   ![Downloads](https://img.shields.io/github/downloads/YOUR_USERNAME/MavenLED/total)
   ```

3. **Test Installation**
   - Download the `.bin` file
   - Follow installation instructions
   - Verify firmware works correctly

---

## Release Checklist Template

Before creating each release, verify:

### Code Quality
- [ ] All features tested on hardware
- [ ] No compilation warnings
- [ ] Version number updated in code
- [ ] README.md updated with changes
- [ ] CHANGELOG updated
- [ ] Documentation is current

### Build
- [ ] Firmware compiles without errors
- [ ] `.bin` file generated successfully
- [ ] File size is reasonable (~1-2 MB)
- [ ] Binary tested on ESP32 hardware

### Git
- [ ] All changes committed
- [ ] Code pushed to main/master branch
- [ ] Tag created with correct version
- [ ] Tag pushed to GitHub

### Release
- [ ] Release notes written
- [ ] Binary file attached
- [ ] Installation instructions clear
- [ ] Links in description work
- [ ] Version matches tag

### Post-Release
- [ ] Download link tested
- [ ] Installation guide works
- [ ] Device functions correctly
- [ ] No critical bugs reported

---

## Version Numbering Guide

Use [Semantic Versioning](https://semver.org/):

**Format:** `MAJOR.MINOR.PATCH`

- **MAJOR** (1.x.x): Breaking changes, incompatible API changes
  - Example: Complete rewrite, different hardware requirements
  
- **MINOR** (x.1.x): New features, backward compatible
  - Example: New MQTT commands, additional LED patterns
  
- **PATCH** (x.x.1): Bug fixes, backward compatible
  - Example: Fix memory leak, correct color display

**Pre-release versions:**
- `v1.0.0-alpha.1` - Early testing, unstable
- `v1.0.0-beta.1` - Feature complete, testing
- `v1.0.0-rc.1` - Release candidate, final testing

---

## Example Release Schedule

### Initial Development
- `v0.1.0-alpha.1` - First working prototype
- `v0.2.0-alpha.1` - Basic LED control
- `v0.5.0-beta.1` - MQTT integration added
- `v0.9.0-rc.1` - Feature complete, testing

### Stable Releases
- `v1.0.0` - Initial public release
- `v1.0.1` - Bug fix: WiFi connection stability
- `v1.1.0` - New feature: Custom animations
- `v1.1.1` - Bug fix: MQTT reconnection
- `v2.0.0` - Major update: New hardware support

---

## Automated Releases (Advanced)

### Using GitHub Actions

Create `.github/workflows/release.yml`:

```yaml
name: Create Release

on:
  push:
    tags:
      - 'v*'

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Setup Arduino CLI
        uses: arduino/setup-arduino-cli@v1
        
      - name: Install ESP32 Platform
        run: |
          arduino-cli core update-index
          arduino-cli core install esp32:esp32
          
      - name: Install Libraries
        run: |
          arduino-cli lib install "Adafruit NeoPixel"
          arduino-cli lib install "ArduinoJson"
          arduino-cli lib install "PubSubClient"
          
      - name: Compile
        run: |
          arduino-cli compile --fqbn esp32:esp32:esp32 MavenLED.ino
          
      - name: Create Release
        uses: actions/create-release@v1
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
        with:
          tag_name: ${{ github.ref }}
          release_name: Release ${{ github.ref }}
          draft: false
          prerelease: false
          
      - name: Upload Binary
        uses: actions/upload-release-asset@v1
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
        with:
          upload_url: ${{ steps.create_release.outputs.upload_url }}
          asset_path: ./build/MavenLED.ino.bin
          asset_name: MavenLED_${{ github.ref }}.bin
          asset_content_type: application/octet-stream
```

This automatically:
- Compiles firmware when you push a tag
- Creates a GitHub release
- Uploads the binary file

---

## Tips for Good Releases

1. **Write Clear Release Notes**
   - List all new features
   - Mention bug fixes
   - Include breaking changes warnings
   - Add upgrade instructions

2. **Include Screenshots**
   - Show new web interface features
   - Demonstrate LED patterns
   - Include configuration examples

3. **Provide Migration Guides**
   - How to upgrade from previous version
   - Configuration changes needed
   - Breaking changes and fixes

4. **Test Before Release**
   - Always test on real hardware
   - Try fresh installation
   - Verify all documented features work

5. **Communicate with Users**
   - Announce release on project page
   - Update documentation
   - Respond to feedback quickly

---

## Troubleshooting Releases

**Problem**: Can't create tag
```bash
# Delete local tag
git tag -d v1.0.0

# Delete remote tag
git push origin :refs/tags/v1.0.0

# Recreate tag
git tag -a v1.0.0 -m "Version 1.0.0"
git push origin v1.0.0
```

**Problem**: Binary file too large
- Remove debug symbols
- Optimize compilation settings
- Use compression

**Problem**: Release not appearing
- Check tag was pushed: `git ls-remote --tags origin`
- Verify release is published (not draft)
- Clear browser cache

---

## Next Steps After Release

1. Monitor for issues
2. Respond to user feedback
3. Plan next version features
4. Update documentation
5. Start development branch for next version

---

Happy releasing! 🚀
