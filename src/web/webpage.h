#ifndef WEBPAGE_H
#define WEBPAGE_H

// HTML Web Interface
inline const char* webPage = R"delimiter(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MavenLED Control Panel</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #0f0f0f; color: #fff; line-height: 1.6;
        }
        .container { max-width: 1200px; margin: 0 auto; padding: 20px; }
        .header { text-align: center; margin-bottom: 30px; }
        .header h1 { color: #00ff88; font-size: 2.5em; margin-bottom: 10px; }
        .status-card { 
            background: #1a1a1a; border-radius: 15px; padding: 20px; margin-bottom: 20px;
            border: 1px solid #333; box-shadow: 0 4px 15px rgba(0,255,136,0.1);
        }
        .controls-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }
        .control-section { background: #1a1a1a; border-radius: 15px; padding: 20px; border: 1px solid #333; }
        .control-section h3 { color: #00ff88; margin-bottom: 15px; font-size: 1.3em; }
        .color-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; margin-top: 15px; }
        .color-item { 
            display: flex; flex-direction: column; align-items: center; padding: 10px;
            background: #2a2a2a; border-radius: 10px; border: 1px solid #444;
        }
        .color-preview { width: 40px; height: 40px; border-radius: 50%; margin-bottom: 8px; border: 2px solid #555; }
        .color-input { width: 100%; background: #333; border: 1px solid #555; color: #fff; padding: 5px; border-radius: 5px; }
        .slider-container { margin: 15px 0; }
        .slider { width: 100%; height: 8px; border-radius: 5px; background: #333; outline: none; }
        .slider::-webkit-slider-thumb { 
            appearance: none; width: 20px; height: 20px; border-radius: 50%; 
            background: #00ff88; cursor: pointer; border: 2px solid #fff;
        }
        .btn { 
            background: #00ff88; color: #000; border: none; padding: 12px 20px;
            border-radius: 8px; cursor: pointer; font-weight: bold; margin: 5px;
            transition: all 0.3s ease;
        }
        .btn:hover { background: #00cc6a; transform: translateY(-2px); }
        .btn-danger { background: #ff4444; color: #fff; }
        .btn-danger:hover { background: #cc3333; }
        .direction-controls { display: flex; gap: 10px; margin: 10px 0; }
        .wifi-list { max-height: 200px; overflow-y: auto; margin: 10px 0; }
        .wifi-item { 
            padding: 10px; background: #2a2a2a; margin: 5px 0; border-radius: 8px;
            cursor: pointer; border: 1px solid #444; transition: all 0.3s ease;
        }
        .wifi-item:hover { background: #3a3a3a; border-color: #00ff88; }
        .input-group { margin: 10px 0; }
        .input-group label { display: block; margin-bottom: 5px; color: #ccc; }
        .input-group input { 
            width: 100%; padding: 10px; background: #333; border: 1px solid #555;
            border-radius: 5px; color: #fff; font-size: 14px;
        }
        .status-indicator { 
            display: inline-block; width: 12px; height: 12px; border-radius: 50%;
            margin-right: 8px; animation: pulse 2s infinite;
        }
        .status-online { background: #00ff88; }
        .status-offline { background: #ff4444; }
        @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }
        .night-mode-toggle { 
            background: #333; border: 1px solid #555; padding: 15px; border-radius: 10px;
            display: flex; justify-content: space-between; align-items: center; margin: 15px 0;
        }
        .toggle-switch { 
            position: relative; width: 60px; height: 30px; background: #555;
            border-radius: 15px; cursor: pointer; transition: all 0.3s ease;
        }
        .toggle-switch.active { background: #00ff88; }
        .toggle-slider { 
            position: absolute; top: 3px; left: 3px; width: 24px; height: 24px;
            background: #fff; border-radius: 50%; transition: all 0.3s ease;
        }
        .toggle-switch.active .toggle-slider { transform: translateX(30px); }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>MavenLED Control Panel</h1>
            <p>Advanced LED Strip Controller for 3D Printer Monitoring</p>
            <div style="margin-top: 10px;">
                <a href="https://github.com/NASA-0007/MavenLED-Bambu-Lab-Status-LED" target="_blank" style="color: #00ff88; text-decoration: none; font-size: 0.9em; opacity: 0.8;">
                    Created by NASA-0007
                </a>
            </div>
        </div>

        <div class="status-card">
            <h3>Printer Status</h3>
            <p><span id="status-indicator" class="status-indicator status-offline"></span>
               Status: <span id="printer-status">Disconnected</span> | 
               Progress: <span id="printer-progress">0%</span> | 
               WiFi: <span id="wifi-status">Checking...</span></p>
        </div>

        
            <div class="control-section">
                <h3>Custom Colors</h3>
                <p>Customize colors for each printer state using hex values</p>
                <div class="color-grid">
                    <div class="color-item">
                        <div class="color-preview" id="color-idle"></div>
                        <label>Idle</label>
                        <input type="color" class="color-input" id="idle-color">
                    </div>
                    <div class="color-item">
                        <div class="color-preview" id="color-printing"></div>
                        <label>Printing</label>
                        <input type="color" class="color-input" id="printing-color">
                    </div>
                    <div class="color-item">
                        <div class="color-preview" id="color-download"></div>
                        <label>Download</label>
                        <input type="color" class="color-input" id="download-color">
                    </div>
                    <div class="color-item">
                        <div class="color-preview" id="color-paused"></div>
                        <label>Paused</label>
                        <input type="color" class="color-input" id="paused-color">
                    </div>
                    <div class="color-item">
                        <div class="color-preview" id="color-error"></div>
                        <label>Error</label>
                        <input type="color" class="color-input" id="error-color">
                    </div>
                    <div class="color-item">
                        <div class="color-preview" id="color-heating"></div>
                        <label>Heating</label>
                        <input type="color" class="color-input" id="heating-color">
                    </div>
                    <div class="color-item">
                        <div class="color-preview" id="color-cooling"></div>
                        <label>Cooling</label>
                        <input type="color" class="color-input" id="cooling-color">
                    </div>
                    <div class="color-item">
                        <div class="color-preview" id="color-finished"></div>
                        <label>Finished</label>
                        <input type="color" class="color-input" id="finished-color">
                    </div>
                </div>
                <button class="btn" onclick="saveColors()">Save Colors</button>
                <button class="btn btn-danger" onclick="resetColors()">Reset to Default</button>
            </div>

            <div class="control-section">
                <h3>Brightness Control</h3>
                
                <div class="night-mode-toggle" onclick="toggleNightMode()">
                    <span>Night Mode (Ultra Dim)</span>
                    <div class="toggle-switch" id="night-mode-toggle">
                        <div class="toggle-slider"></div>
                    </div>
                </div>

                <div class="night-mode-toggle" onclick="toggleLights()">
                    <span>Lights On/Off</span>
                    <div class="toggle-switch active" id="lights-toggle">
                        <div class="toggle-slider"></div>
                    </div>
                </div>

                <div class="slider-container">
                    <label>Global Brightness: <span id="brightness-value">100%</span></label>
                    <input type="range" min="1" max="255" value="255" class="slider" id="brightness-slider" oninput="updateBrightness(this.value)">
                </div>

                <div class="slider-container">
                    <label>Night Mode Brightness: <span id="night-brightness-value">10%</span></l>
                <input type="range" min="1" max="100" value="25" class="slider" id="night-brightness-slider" oninput="updateNightBrightness(this.value)">
                </div>
            </div>

            <div class="control-section">
                <h3>Animation Directions</h3>
                <p>Control the direction of wave animations</p>
                
                <div class="direction-controls">
                    <label>Rainbow Direction:</label>
                    <button class="btn" id="rainbow-dir-btn" onclick="toggleDirection('rainbow')">Normal</button>
                </div>
                
                <div class="direction-controls">
                    <label>Idle Wave Direction:</label>
                    <button class="btn" id="idle-dir-btn" onclick="toggleDirection('idle')">Normal</button>
                </div>
                
                <div class="direction-controls">
                    <label>Printing Progress Direction:</label>
                    <button class="btn" id="printing-dir-btn" onclick="toggleDirection('printing')">Normal</button>
                </div>
                
                <div class="direction-controls">
                    <label>Download Progress Direction:</label>
                    <button class="btn" id="download-dir-btn" onclick="toggleDirection('download')">Normal</button>
                </div>
                
            </div>

            <div class="controls-grid">
            <div class="control-section">
                <h3>LED Strip Configuration</h3>
                <div class="input-group">
                    <label>Number of LEDs:</label>
                    <input type="number" id="led-count" min="1" max="300" placeholder="60">
                    <small>Current: <span id="current-led-count">60</span> LEDs</small>
                  </div>
                <button class="btn" onclick="updateLEDCount()">Update LED Count</button>
                <div class="input-group" style="margin-top: 20px;">
                    <label>GPIO Pin for LED Data:</label>
                    <input type="number" id="led-pin" min="0" max="33" placeholder="17">
                    <small>Current: GPIO <span id="current-led-pin">17</span> (Avoid pins 6-11)</small>
                </div>
                <button class="btn" onclick="updateLEDPin()">Update GPIO Pin</button>
                <div class="night-mode-toggle" onclick="toggleP1Mode()">
                    <span>P1 Series Mode (Compatibility)</span>
                    <div class="toggle-switch" id="p1-mode-toggle">
                        <div class="toggle-slider"></div>
                    </div>
                </div>

                <div class="night-mode-toggle" onclick="toggleIdleTimeout()">
                    <span>Auto Turn Off When Idle</span>
                    <div class="toggle-switch" id="idle-timeout-toggle">
                        <div class="toggle-slider"></div>
                    </div>
                </div>

                <div class="slider-container" id="idle-timeout-slider-container" style="display: none;">
                    <label>Idle Timeout: <span id="idle-timeout-value">30</span> minutes</label>
                    <input type="range" min="1" max="120" value="30" class="slider" id="idle-timeout-slider" oninput="updateIdleTimeout(this.value)">
                </div>
                <p style="margin-top: 10px; font-size: 0.9em; color: #888;">Device will restart after changing LED count or GPIO pin</p>
            </div>


            <div class="control-section">
                <h3>WiFi Management</h3>
                <button class="btn" onclick="scanWiFi()">Scan Networks</button>
                <div id="wifi-list" class="wifi-list"></div>
                
                <div class="input-group">
                    <label>SSID:</label>
                    <input type="text" id="wifi-ssid" placeholder="Enter WiFi network name">
                </div>
                
                <div class="input-group">
                    <label>Password:</label>
                    <input type="password" id="wifi-password" placeholder="Enter WiFi password">
                </div>
                
                <button class="btn" onclick="connectWiFi()">Connect to WiFi</button>
            </div>

            <div class="control-section">
                <h3>Printer (MQTT) Settings</h3>
                
                <div class="night-mode-toggle" onclick="toggleMQTTMode()">
                    <span>Global Mode (Bambu Cloud)</span>
                    <div class="toggle-switch" id="mqtt-mode-toggle">
                        <div class="toggle-slider"></div>
                    </div>
                </div>
                
                <div id="local-mqtt-settings">
                    <div class="input-group">
                        <label>Printer IP Address:</label>
                        <input type="text" id="mqtt-server" placeholder="192.168.1.25">
                    </div>
                    
                    <div class="input-group">
                        <label>MQTT Password:</label>
                        <input type="password" id="mqtt-password" placeholder="Enter MQTT password">
                    </div>
                </div>
                
                <div id="global-mqtt-settings" style="display: none;">
                    <div class="input-group">
                        <label>Bambu Lab Email:</label>
                        <input type="email" id="global-email" placeholder="your@email.com">
                    </div>
                    
                    <div class="input-group">
                        <label>
                            MakerWorld UID: 
                            <a href="https://makerworld.com/api/v1/design-user-service/my/preference" target="_blank" title="Visit this URL while logged into MakerWorld to find your UID number" style="color: #00ff88; text-decoration: none;">Info</a>
                        </label>
                        <input type="text" id="global-username" placeholder="Enter UID number (e.g., 381738)">
                        <small style="color: #888;">Visit the Info link above to find your UID in the response JSON</small>
                    </div>
                    
                    <div id="auth-section">
                        <button class="btn" onclick="initiateGlobalAuth()">Login to Bambu Lab</button>
                        <div id="verification-section" style="display: none; margin-top: 15px;">
                            <div class="input-group">
                                <label>Verification Code (sent to email):</label>
                                <input type="text" id="verification-code" placeholder="Enter 6-digit code">
                            </div>
                            <button class="btn" onclick="completeGlobalAuth()">Verify Code</button>
                        </div>
                    </div>
                    
                    <div id="token-status" style="margin-top: 15px; display: none;">
                        <p style="color: #00ff88;">Successfully authenticated!</p>
                        <button class="btn" onclick="renewToken()">Renew Access Token</button>
                    </div>
                </div>
                
                <div class="input-group">
                    <label>Device Serial Number:</label>
                    <input type="text" id="device-serial" placeholder="00M09D521340314">
                </div>
                
                <button class="btn" onclick="saveMQTTSettings()">Save MQTT Settings</button>
            </div>

            
            </div>
        </div>
    </div>

    <script>
        let settings = {};
        let nightModeActive = false;

        // Load initial settings
        async function loadSettings() {
            try {
                const response = await fetch('/api/settings');
                settings = await response.json();
                console.log('Loaded settings:', settings); // Debug log
                updateUI();
                loadMQTTSettings(); // Load MQTT settings when page loads
                loadLEDCount(); // Load LED count when page loads
                loadLEDPin(); // Load GPIO pin when page loads
            } catch (error) {
                console.error('Failed to load settings:', error);
                // Set default colors if loading fails
                setDefaultColors();
            }
        }

        // Set default colors for fallback
        function setDefaultColors() {
            const defaultColors = [
                {r: 0, g: 50, b: 0},     // idle
                {r: 75, g: 0, b: 130},   // printing
                {r: 0, g: 0, b: 255},    // download
                {r: 255, g: 255, b: 0},  // paused
                {r: 255, g: 0, b: 0},    // error
                {r: 255, g: 100, b: 0},  // heating
                {r: 0, g: 255, b: 255},  // cooling
                {r: 0, g: 255, b: 0},    // finished
            ];
            
            settings = { colors: defaultColors };
            updateUI();
            console.log('Set default colors'); // Debug log
        }

        // Update UI with current settings
        function updateUI() {
            // Update colors with better error handling
            const states = ['idle', 'printing', 'download', 'paused', 'error', 'heating', 'cooling', 'finished'];
            states.forEach((state, index) => {
                if (settings.colors && settings.colors[index]) {
                    const color = settings.colors[index];
                    const hex = rgbToHex(color.r, color.g, color.b);
                    
                    // Update color input
                    const colorInput = document.getElementById(`${state}-color`);
                    if (colorInput) {
                        colorInput.value = hex;
                    }
                    
                    // Update color preview
                    const colorPreview = document.getElementById(`color-${state}`);
                    if (colorPreview) {
                        colorPreview.style.backgroundColor = hex;
                    }
                    
                    console.log(`Updated ${state} color: ${hex}`); // Debug log
                } else {
                    console.warn(`Missing color data for ${state} at index ${index}`); // Debug log
                }
            });

            // Update brightness
            if (settings.global_brightness) {
                document.getElementById('brightness-slider').value = settings.global_brightness;
                document.getElementById('brightness-value').textContent = Math.round(settings.global_brightness / 255 * 100) + '%';
            }

            if (settings.night_mode_brightness) {
                document.getElementById('night-brightness-slider').value = settings.night_mode_brightness;
                document.getElementById('night-brightness-value').textContent = Math.round(settings.night_mode_brightness / 255 * 100) + '%';
            }

            // Update night mode
            nightModeActive = settings.night_mode_enabled || false;
            const toggle = document.getElementById('night-mode-toggle');
            if (nightModeActive) {
                toggle.classList.add('active');
            } else {
                toggle.classList.remove('active');
            }

            // Update directions
            updateDirectionButtons();
        }

        function rgbToHex(r, g, b) {
            return "#" + [r, g, b].map(x => {
                const hex = x.toString(16);
                return hex.length === 1 ? "0" + hex : hex;
            }).join('');
        }

        function hexToRgb(hex) {
            const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
            return result ? {
                r: parseInt(result[1], 16),
                g: parseInt(result[2], 16),
                b: parseInt(result[3], 16)
            } : null;
        }

        // Color functions
        async function saveColors() {
            const states = ['idle', 'printing', 'download', 'paused', 'error', 'heating', 'cooling', 'finished'];
            const colors = states.map(state => {
                const hex = document.getElementById(`${state}-color`).value;
                return hexToRgb(hex);
            });

            try {
                await fetch('/api/colors', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ colors: colors })
                });
                alert('Colors saved successfully!');
                loadSettings();
            } catch (error) {
                alert('Failed to save colors: ' + error.message);
            }
        }

        async function resetColors() {
            if (confirm('Reset all colors to default values?')) {
                const defaultColors = [
                    {r: 0, g: 50, b: 0},     // idle
                    {r: 75, g: 0, b: 130},   // printing
                    {r: 0, g: 0, b: 255},    // download
                    {r: 255, g: 255, b: 0},  // paused
                    {r: 255, g: 0, b: 0},    // error
                    {r: 255, g: 100, b: 0},  // heating
                    {r: 0, g: 255, b: 255},  // cooling
                    {r: 0, g: 255, b: 0},    // finished
                ];

                try {
                    await fetch('/api/colors', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ colors: defaultColors })
                    });
                    alert('Colors reset to defaults!');
                    loadSettings();
                } catch (error) {
                    alert('Failed to reset colors: ' + error.message);
                }
            }
        }

        // Brightness functions
        async function updateBrightness(value) {
            document.getElementById('brightness-value').textContent = Math.round(value / 255 * 100) + '%';
            
            try {
                await fetch('/api/brightness', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ brightness: parseInt(value) })
                });
            } catch (error) {
                console.error('Failed to update brightness:', error);
            }
        }

        async function updateNightBrightness(value) {
            document.getElementById('night-brightness-value').textContent = Math.round(value / 255 * 100) + '%';
            
            try {
                await fetch('/api/brightness', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ night_brightness: parseInt(value) })
                });
            } catch (error) {
                console.error('Failed to update night brightness:', error);
            }
        }

        async function toggleNightMode() {
            nightModeActive = !nightModeActive;
            const toggle = document.getElementById('night-mode-toggle');
            
            if (nightModeActive) {
                toggle.classList.add('active');
            } else {
                toggle.classList.remove('active');
            }

            try {
                await fetch('/api/nightmode', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ enabled: nightModeActive })
                });
            } catch (error) {
                console.error('Failed to toggle night mode:', error);
            }
        }

        // Lights toggle function
        let lightsEnabled = true; // Default to on
        async function toggleLights() {
            lightsEnabled = !lightsEnabled;
            const toggle = document.getElementById('lights-toggle');
            
            if (lightsEnabled) {
                toggle.classList.add('active');
            } else {
                toggle.classList.remove('active');
            }

            try {
                await fetch('/api/lights/toggle', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ enabled: lightsEnabled })
                });
            } catch (error) {
                console.error('Failed to toggle lights:', error);
            }
        }

        // P1 Mode toggle function
        let p1ModeActive = false;
        async function toggleP1Mode() {
            p1ModeActive = !p1ModeActive;
            const toggle = document.getElementById('p1-mode-toggle');
            
            if (p1ModeActive) {
                toggle.classList.add('active');
            } else {
                toggle.classList.remove('active');
            }

            try {
                await fetch('/api/p1mode', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ p1_series_mode: p1ModeActive })
                });
            } catch (error) {
                console.error('Failed to toggle P1 mode:', error);
            }
        }

        // Idle timeout toggle function
        let idleTimeoutActive = false;
        async function toggleIdleTimeout() {
            idleTimeoutActive = !idleTimeoutActive;
            const toggle = document.getElementById('idle-timeout-toggle');
            const sliderContainer = document.getElementById('idle-timeout-slider-container');
            
            if (idleTimeoutActive) {
                toggle.classList.add('active');
                sliderContainer.style.display = 'block';
            } else {
                toggle.classList.remove('active');
                sliderContainer.style.display = 'none';
            }

            try {
                await fetch('/api/idle/timeout', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ enabled: idleTimeoutActive })
                });
            } catch (error) {
                console.error('Failed to toggle idle timeout:', error);
            }
        }

        async function updateIdleTimeout(value) {
            document.getElementById('idle-timeout-value').textContent = value;
            
            try {
                await fetch('/api/idle/timeout', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ timeout_minutes: parseInt(value) })
                });
            } catch (error) {
                console.error('Failed to update idle timeout:', error);
            }
        }

        // Direction functions
        function updateDirectionButtons() {
            const directions = ['rainbow', 'idle', 'printing', 'download', 'finished'];
            directions.forEach(dir => {
                const btn = document.getElementById(`${dir}-dir-btn`);
                if (btn) {
                    const direction = settings[`${dir}_direction`] || 1;
                    btn.textContent = direction === 1 ? 'Normal' : 'Reversed';
                }
            });
        }

        async function toggleDirection(type) {
            const currentDir = settings[`${type}_direction`] || 1;
            const newDir = currentDir === 1 ? -1 : 1;

            try {
                await fetch('/api/directions', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ [type]: newDir })
                });
                settings[`${type}_direction`] = newDir;
                updateDirectionButtons();
            } catch (error) {
                console.error('Failed to update direction:', error);
            }
        }

        // WiFi functions
        async function scanWiFi() {
            const wifiList = document.getElementById('wifi-list');
            wifiList.innerHTML = '<p>Scanning...</p>';

            try {
                const response = await fetch('/api/wifi/scan');
                const networks = await response.json();
                
                wifiList.innerHTML = '';
                networks.forEach(network => {
                    const item = document.createElement('div');
                    item.className = 'wifi-item';
                    item.innerHTML = `<strong>${network.ssid}</strong><br>Signal: ${network.rssi} dBm | ${network.encryption}`;
                    item.onclick = () => {
                        document.getElementById('wifi-ssid').value = network.ssid;
                    };
                    wifiList.appendChild(item);
                });
            } catch (error) {
                wifiList.innerHTML = '<p>Scan failed</p>';
                console.error('WiFi scan failed:', error);
            }
        }

        async function connectWiFi() {
            const ssid = document.getElementById('wifi-ssid').value;
            const password = document.getElementById('wifi-password').value;

            if (!ssid) {
                alert('Please enter a WiFi network name');
                return;
            }

            try {
                await fetch('/api/wifi/connect', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ ssid: ssid, password: password })
                });
                alert('WiFi connection initiated! Device will restart if successful.');
            } catch (error) {
                alert('Failed to connect: ' + error.message);
            }
        }

        // MQTT Settings Functions
        let globalModeActive = false;

        function toggleMQTTMode() {
            globalModeActive = !globalModeActive;
            const toggle = document.getElementById('mqtt-mode-toggle');
            const localSettings = document.getElementById('local-mqtt-settings');
            const globalSettings = document.getElementById('global-mqtt-settings');
            
            if (globalModeActive) {
                toggle.classList.add('active');
                localSettings.style.display = 'none';
                globalSettings.style.display = 'block';
            } else {
                toggle.classList.remove('active');
                localSettings.style.display = 'block';
                globalSettings.style.display = 'none';
            }
        }

        async function initiateGlobalAuth() {
            const email = document.getElementById('global-email').value;
            const password = prompt('Enter your Bambu Lab password:');
            
            if (!email || !password) {
                alert('Please enter both email and password');
                return;
            }

            try {
                const response = await fetch('/api/auth/login', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ email: email, password: password })
                });
                
                const result = await response.json();
                
                if (result.success) {
                    document.getElementById('verification-section').style.display = 'block';
                    alert('✅ Verification code sent to your email!');
                } else {
                    alert('❌ Login failed: ' + result.error);
                }
            } catch (error) {
                alert('❌ Network error: ' + error.message);
            }
        }

        async function completeGlobalAuth() {
            const email = document.getElementById('global-email').value;
            const code = document.getElementById('verification-code').value;
            
            if (!email || !code) {
                alert('Please enter the verification code');
                return;
            }

            try {
                const response = await fetch('/api/auth/verify', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ email: email, code: code })
                });
                
                const result = await response.json();
                
                if (result.success) {
                    document.getElementById('verification-section').style.display = 'none';
                    document.getElementById('token-status').style.display = 'block';
                    alert('✅ Successfully authenticated with Bambu Lab!');
                } else {
                    alert('❌ Verification failed: ' + result.error);
                }
            } catch (error) {
                alert('❌ Network error: ' + error.message);
            }
        }

        async function renewToken() {
            const email = document.getElementById('global-email').value;
            const password = prompt('Please enter your Bambu Lab account password:');
            
            if (!email) {
                alert('Please enter your email first');
                return;
            }
            
            if (!password) {
                alert('Password is required for token renewal');
                return;
            }

            try {
                const response = await fetch('/api/auth/renew', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ email: email, password: password })
                });
                
                const result = await response.json();
                alert(result.success ? 'Token renewal initiated!' : 'Renewal failed: ' + result.error);
            } catch (error) {
                alert('Network error: ' + error.message);
            }
        }

        async function saveMQTTSettings() {
            const serial = document.getElementById('device-serial').value;

            if (!serial) {
                alert('Please enter the device serial number');
                return;
            }

            let requestData = { serial: serial };

            if (globalModeActive) {
                const email = document.getElementById('global-email').value;
                const username = document.getElementById('global-username').value;
                
                if (!email || !username) {
                    alert('Please fill in email and UID for global mode');
                    return;
                }
                
                requestData.mode = 'global';
                requestData.email = email;
                requestData.username = username;
            } else {
                const server = document.getElementById('mqtt-server').value;
                const password = document.getElementById('mqtt-password').value;
                
                if (!server || !password) {
                    alert('Please fill in server IP and password for local mode');
                    return;
                }
                
                requestData.mode = 'local';
                requestData.server = server;
                requestData.password = password;
            }

            try {
                await fetch('/api/mqtt/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(requestData)
                });
                alert('MQTT settings saved! Device will restart to apply changes.');
            } catch (error) {
                alert('Failed to save MQTT settings: ' + error.message);
            }
        }

        async function loadMQTTSettings() {
            try {
                const response = await fetch('/api/mqtt/config');
                const config = await response.json();
                
                // Set device serial
                document.getElementById('device-serial').value = config.serial || '';
                
                // Check mode and update UI
                if (config.mode === 'global') {
                    globalModeActive = true;
                    document.getElementById('mqtt-mode-toggle').classList.add('active');
                    document.getElementById('local-mqtt-settings').style.display = 'none';
                    document.getElementById('global-mqtt-settings').style.display = 'block';
                    
                    // Load global mode settings
                    document.getElementById('global-email').value = config.email || '';
                    document.getElementById('global-username').value = config.username || '';
                    
                    // Show token status if authenticated
                    if (config.hasToken) {
                        document.getElementById('token-status').style.display = 'block';
                        document.getElementById('auth-section').style.display = 'none';
                    }
                } else {
                    // Local mode
                    globalModeActive = false;
                    document.getElementById('mqtt-mode-toggle').classList.remove('active');
                    document.getElementById('local-mqtt-settings').style.display = 'block';
                    document.getElementById('global-mqtt-settings').style.display = 'none';
                    
                    // Load local mode settings
                    document.getElementById('mqtt-server').value = config.server || '';
                    document.getElementById('mqtt-password').value = config.password || '';
                }
            } catch (error) {
                console.error('Failed to load MQTT settings:', error);
            }
        }

        // LED Count Management
        async function updateLEDCount() {
            const ledCount = parseInt(document.getElementById('led-count').value);
            
            if (!ledCount || ledCount < 1 || ledCount > 300) {
                alert('Please enter a valid LED count between 1 and 300');
                return;
            }

            try {
                await fetch('/api/led/count', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ count: ledCount })
                });
                alert('LED count updated! Device will restart to apply changes.');
            } catch (error) {
                alert('Failed to update LED count: ' + error.message);
            }
        }

        async function loadLEDCount() {
            try {
                const response = await fetch('/api/led/count');
                const config = await response.json();
                
                document.getElementById('current-led-count').textContent = config.count || 60;
                document.getElementById('led-count').value = config.count || 60;
            } catch (error) {
                console.error('Failed to load LED count:', error);
            }
        }

        // GPIO Pin Management
        async function updateLEDPin() {
            const ledPin = parseInt(document.getElementById('led-pin').value);
            
            if (isNaN(ledPin) || ledPin < 0 || ledPin > 33 || 
                [6, 7, 8, 9, 10, 11].includes(ledPin)) {
                alert('Please enter a valid GPIO pin (0-5, 12-33). Avoid pins 6-11.');
                return;
            }

            try {
                await fetch('/api/led/pin', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ pin: ledPin })
                });
                alert('GPIO pin updated! Device will restart to apply changes.');
            } catch (error) {
                alert('Failed to update GPIO pin: ' + error.message);
            }
        }

        async function loadLEDPin() {
            try {
                const response = await fetch('/api/led/pin');
                const config = await response.json();
                
                document.getElementById('current-led-pin').textContent = config.pin || 17;
                document.getElementById('led-pin').value = config.pin || 17;
            } catch (error) {
                console.error('Failed to load GPIO pin:', error);
            }
        }

        // P1 Mode Management
        async function loadP1Mode() {
            try {
                const response = await fetch('/api/p1mode');
                const config = await response.json();
                
                p1ModeActive = config.p1_series_mode || false;
                const toggle = document.getElementById('p1-mode-toggle');
                if (p1ModeActive) {
                    toggle.classList.add('active');
                } else {
                    toggle.classList.remove('active');
                }
            } catch (error) {
                console.error('Failed to load P1 mode:', error);
            }
        }

        // Idle Timeout Management
        async function loadIdleTimeout() {
            try {
                const response = await fetch('/api/idle/timeout');
                const config = await response.json();
                
                idleTimeoutActive = config.enabled || false;
                const toggle = document.getElementById('idle-timeout-toggle');
                const sliderContainer = document.getElementById('idle-timeout-slider-container');
                
                if (idleTimeoutActive) {
                    toggle.classList.add('active');
                    sliderContainer.style.display = 'block';
                } else {
                    toggle.classList.remove('active');
                    sliderContainer.style.display = 'none';
                }
                
                const timeoutMinutes = config.timeout_minutes || 30;
                document.getElementById('idle-timeout-slider').value = timeoutMinutes;
                document.getElementById('idle-timeout-value').textContent = timeoutMinutes;
            } catch (error) {
                console.error('Failed to load idle timeout:', error);
            }
        }

        // Status updates
        async function updateStatus() {
            try {
                const response = await fetch('/api/status');
                const status = await response.json();
                
                document.getElementById('printer-status').textContent = status.printer_status || 'Unknown';
                document.getElementById('printer-progress').textContent = (status.progress || 0) + '%';
                document.getElementById('wifi-status').textContent = status.wifi_connected ? 'Connected' : 'Disconnected';
                
                // Update lights status
                if (status.hasOwnProperty('lights_enabled')) {
                    lightsEnabled = status.lights_enabled;
                    const toggle = document.getElementById('lights-toggle');
                    if (lightsEnabled) {
                        toggle.classList.add('active');
                    } else {
                        toggle.classList.remove('active');
                    }
                }
                
                // Update night mode status
                if (status.hasOwnProperty('night_mode_enabled')) {
                    nightModeActive = status.night_mode_enabled;
                    const nightToggle = document.getElementById('night-mode-toggle');
                    if (nightModeActive) {
                        nightToggle.classList.add('active');
                    } else {
                        nightToggle.classList.remove('active');
                    }
                }
                
                // Update brightness sliders
                if (status.hasOwnProperty('global_brightness')) {
                    document.getElementById('brightness-slider').value = status.global_brightness;
                    document.getElementById('brightness-value').textContent = Math.round(status.global_brightness / 255 * 100) + '%';
                }
                
                if (status.hasOwnProperty('night_mode_brightness')) {
                    document.getElementById('night-brightness-slider').value = status.night_mode_brightness;
                    document.getElementById('night-brightness-value').textContent = Math.round(status.night_mode_brightness / 255 * 100) + '%';
                }
                
                // Update LED count
                if (status.hasOwnProperty('led_count')) {
                    document.getElementById('current-led-count').textContent = status.led_count;
                    document.getElementById('led-count').value = status.led_count;
                }
                
                // Update direction settings in global settings object
                if (status.hasOwnProperty('rainbow_direction')) {
                    settings.rainbow_direction = status.rainbow_direction;
                }
                if (status.hasOwnProperty('idle_direction')) {
                    settings.idle_direction = status.idle_direction;
                }
                if (status.hasOwnProperty('printing_direction')) {
                    settings.printing_direction = status.printing_direction;
                }
                if (status.hasOwnProperty('download_direction')) {
                    settings.download_direction = status.download_direction;
                }
                
                // Update direction buttons
                updateDirectionButtons();
                
                const indicator = document.getElementById('status-indicator');
                if (status.printer_connected) {
                    indicator.className = 'status-indicator status-online';
                } else {
                    indicator.className = 'status-indicator status-offline';
                }
                
                // Handle global mode token status
                if (status.mqtt_mode === 'global') {
                    const tokenStatus = document.getElementById('token-status');
                    if (status.token_expired) {
                        if (tokenStatus) {
                            tokenStatus.innerHTML = '<p style="color: #ff4444;">Access token expired! Please renew.</p><button class="btn" onclick="renewToken()">Renew Access Token</button>';
                            tokenStatus.style.display = 'block';
                        }
                    } else if (status.token_needs_renewal) {
                        if (tokenStatus) {
                            const daysLeft = Math.ceil(status.token_expires_in_seconds / (24 * 60 * 60));
                            tokenStatus.innerHTML = `<p style="color: #ffaa00;">Token expires in ${daysLeft} days. Consider renewing.</p><button class="btn" onclick="renewToken()">Renew Access Token</button>`;
                            tokenStatus.style.display = 'block';
                        }
                    }
                }
            } catch (error) {
                console.error('Failed to update status:', error);
            }
        }

        // Update color previews when inputs change
        document.addEventListener('DOMContentLoaded', function() {
            const states = ['idle', 'printing', 'download', 'paused', 'error', 'heating', 'cooling', 'finished'];
            states.forEach(state => {
                const input = document.getElementById(`${state}-color`);
                const preview = document.getElementById(`color-${state}`);
                
                input.addEventListener('input', function() {
                    preview.style.backgroundColor = this.value;
                });
            });

            // Initial load
            loadSettings();
            updateStatus();
            loadP1Mode();
            loadIdleTimeout();
            
            // Periodic status updates
            setInterval(updateStatus, 5000);
        });

    </script>
</body>
</html>
)delimiter";

#endif