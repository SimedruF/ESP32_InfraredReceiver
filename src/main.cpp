#include <Arduino.h>
#include <IRremote.hpp>
#include <WiFi.h>
#include <WebServer.h>
#include <vector>
#include <Preferences.h>

// ESP32 pin configuration
static const uint8_t IR_RECEIVE_PIN = 14; 
static const uint8_t LED_PIN = 2;

// WiFi Access Point configuration (fallback)
const char* ap_ssid = "ESP32_IR_Receiver";
const char* ap_password = "12345678";

// Variables for WiFi Client (local network)
String wifi_ssid = "";
String wifi_password = "";
bool wifiConfigured = false;
bool wifiConnected = false;

// Preferences for EEPROM storage
Preferences preferences;

// Web server on port 80
WebServer server(80);

// Variables for last received IR data
String lastProtocol = "N/A";
String lastAddress = "N/A";
String lastCommand = "N/A";
String lastRawData = "N/A";
unsigned long lastReceiveTime = 0;
int signalCount = 0;

// Structure for saving commands
struct IRCommand {
  String protocol;
  String address;
  String command;
  String rawData;
  String timestamp;
};

// Vector for saved commands (max 50 commands to avoid filling memory)
std::vector<IRCommand> savedCommands;
const int MAX_SAVED_COMMANDS = 50;

// HTML page with AJAX
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 IR Receiver</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #2e2e2e 0%, #1e033a 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            padding: 30px;
        }
        h1 {
            color: #55d445;
            text-align: center;
            margin-bottom: 10px;
            font-size: 2em;
        }
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 30px;
            font-size: 0.9em;
        }
        .status {
            background: #f0f4f8;
            padding: 15px;
            border-radius: 10px;
            margin-bottom: 20px;
            text-align: center;
        }
        .status-dot {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            background: #10b981;
            animation: pulse 2s infinite;
            margin-right: 8px;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        .data-card {
            background: linear-gradient(135deg, #413b5c 0%, #51277a 100%);
            color: white;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 15px;
        }
        .data-label {
            font-size: 0.85em;
            opacity: 0.9;
            margin-bottom: 5px;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .data-value {
            font-size: 1.5em;
            font-weight: bold;
            font-family: 'Courier New', monospace;
            word-break: break-all;
        }
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-bottom: 20px;
        }
        .info-box {
            background: #f8fafc;
            padding: 15px;
            border-radius: 8px;
            border-left: 4px solid #3252e2;
        }
        .info-label {
            font-size: 0.8em;
            color: #64748b;
            margin-bottom: 5px;
        }
        .info-value {
            font-size: 1.2em;
            font-weight: bold;
            color: #1e293b;
        }
        .raw-data {
            background: #1e293b;
            color: #10b981;
            padding: 15px;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            font-size: 0.85em;
            max-height: 200px;
            overflow-y: auto;
            white-space: pre-wrap;
            word-break: break-all;
        }
        .footer {
            text-align: center;
            margin-top: 20px;
            color: #64748b;
            font-size: 0.85em;
        }
        .tabs {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
            border-bottom: 2px solid #e5e7eb;
        }
        .tab-button {
            padding: 12px 24px;
            background: none;
            border: none;
            border-bottom: 3px solid transparent;
            cursor: pointer;
            font-weight: 600;
            color: #64748b;
            transition: all 0.3s;
        }
        .tab-button.active {
            color: #667eea;
            border-bottom-color: #667eea;
        }
        .tab-button:hover {
            color: #667eea;
        }
        .tab-content {
            display: none;
        }
        .tab-content.active {
            display: block;
        }
        .form-group {
            margin-bottom: 15px;
        }
        .form-label {
            display: block;
            margin-bottom: 5px;
            font-weight: 600;
            color: #1e293b;
        }
        .form-input {
            width: 100%;
            padding: 10px;
            border: 2px solid #e5e7eb;
            border-radius: 8px;
            font-size: 1em;
        }
        .form-input:focus {
            outline: none;
            border-color: #667eea;
        }
        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 8px;
            font-weight: bold;
            cursor: pointer;
            font-size: 1em;
        }
        .btn-primary {
            background: #667eea;
            color: white;
        }
        .btn-success {
            background: #10b981;
            color: white;
        }
        .btn-danger {
            background: #ef4444;
            color: white;
        }
        .wifi-status {
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            font-weight: 600;
        }
        .wifi-connected {
            background: #d1fae5;
            color: #065f46;
        }
        .wifi-disconnected {
            background: #fee2e2;
            color: #991b1b;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎯 ESP32 IR Receiver</h1>
        <p class="subtitle">Real-time monitoring and configuration</p>
        
        <!-- Tabs -->
        <div class="tabs">
            <button class="tab-button active" onclick="switchTab('monitor')">📊 IR Monitoring</button>
            <button class="tab-button" onclick="switchTab('wifi')">📡 WiFi Configuration</button>
        </div>
        
        <!-- IR Monitoring Tab -->
        <div id="monitor-tab" class="tab-content active">
            <div class="status">
                <span class="status-dot"></span>
                <strong>Active connection</strong> | Updating every 500ms
            </div>

            <div class="info-grid">
                <div class="info-box">
                    <div class="info-label">Protocol</div>
                    <div class="info-value" id="protocol">-</div>
                </div>
                <div class="info-box">
                    <div class="info-label">Address</div>
                    <div class="info-value" id="address">-</div>
                </div>
                <div class="info-box">
                    <div class="info-label">Command</div>
                    <div class="info-value" id="command">-</div>
                </div>
                <div class="info-box">
                    <div class="info-label">Signals received</div>
                    <div class="info-value" id="count">0</div>
                </div>
            </div>

            <div class="data-card">
                <div class="data-label">Last signal received</div>
                <div class="data-value" id="lastTime">-</div>
            </div>

            <div style="margin-bottom: 10px;">
                <strong>RAW Data (Timings):</strong>
            </div>
            <div class="raw-data" id="rawData">Waiting for data...</div>

            <div style="margin-top: 15px; padding: 15px; background: #f0f9ff; border-radius: 8px; border-left: 4px solid #3b82f6;">
                <label style="display: flex; align-items: center; cursor: pointer; font-weight: 500; color: #1e40af;">
                    <input type="checkbox" id="autoSave" style="width: 20px; height: 20px; cursor: pointer; margin-right: 10px;">
                    <span>🔄 Auto-save (saves each new command received)</span>
                </label>
            </div>

            <div style="margin-top: 20px; display: flex; gap: 10px; justify-content: center;">
                <button onclick="saveCommand()" class="btn btn-success">
                    💾 Save command
                </button>
                <button onclick="downloadCommands()" class="btn btn-primary">
                    📥 Download all (<span id="savedCount">0</span>)
                </button>
                <button onclick="clearCommands()" class="btn btn-danger">
                    🗑️ Delete all
                </button>
            </div>

            <div id="message" style="margin-top: 15px; padding: 10px; border-radius: 8px; text-align: center; display: none;"></div>
        </div>
        
        <!-- WiFi Configuration Tab -->
        <div id="wifi-tab" class="tab-content">
            <div id="wifiStatus" class="wifi-status wifi-disconnected">
                Mode: Access Point | IP: <span id="currentIP">-</span>
            </div>
            
            <h3 style="margin-bottom: 15px; color: #1e293b;">Connect to local WiFi network</h3>
            <p style="margin-bottom: 20px; color: #64748b;">Configure ESP32 to connect to your WiFi network. Data will be saved in device memory.</p>
            
            <div class="form-group">
                <label class="form-label">SSID (WiFi network name)</label>
                <input type="text" id="wifiSSID" class="form-input" placeholder="WiFi network name">
            </div>
            
            <div class="form-group">
                <label class="form-label">WiFi Password</label>
                <input type="password" id="wifiPassword" class="form-input" placeholder="WiFi network password">
            </div>
            
            <div style="display: flex; gap: 10px; margin-top: 20px;">
                <button onclick="saveWiFiConfig()" class="btn btn-primary" style="flex: 1;">
                    💾 Save and Connect
                </button>
                <button onclick="clearWiFiConfig()" class="btn btn-danger">
                    🗑️ Delete configuration
                </button>
            </div>
            
            <div id="wifiMessage" style="margin-top: 15px; padding: 10px; border-radius: 8px; text-align: center; display: none;"></div>
            
            <div style="margin-top: 30px; padding: 15px; background: #f8fafc; border-radius: 8px; border-left: 4px solid #667eea;">
                <h4 style="margin-bottom: 10px; color: #1e293b;">ℹ️ Information</h4>
                <ul style="margin-left: 20px; color: #64748b; line-height: 1.8;">
                    <li>Configuration is saved in EEPROM</li>
                    <li>ESP32 will try to connect to WiFi on next boot</li>
                    <li>If connection fails, Access Point will be activated</li>
                    <li>Access Point: <strong>ESP32_IR_Receiver</strong> / <strong>12345678</strong></li>
                </ul>
            </div>
        </div>

        <div class="footer">
            Made with ❤️ using ESP32 WROOM
        </div>
    </div>

    <script>
        let lastSavedCommand = '';
        
        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('protocol').textContent = data.protocol;
                    document.getElementById('address').textContent = data.address;
                    document.getElementById('command').textContent = data.command;
                    document.getElementById('count').textContent = data.count;
                    document.getElementById('lastTime').textContent = data.lastTime;
                    document.getElementById('rawData').textContent = data.rawData;
                    
                    // Auto-save if checkbox is checked
                    const autoSave = document.getElementById('autoSave').checked;
                    if (autoSave && data.command !== 'N/A') {
                        const currentCommand = data.protocol + '_' + data.address + '_' + data.command;
                        // Save only if it's a new command (different from last saved)
                        if (currentCommand !== lastSavedCommand) {
                            lastSavedCommand = currentCommand;
                            saveCommandAuto();
                        }
                    }
                })
                .catch(error => {
                    console.error('Update error:', error);
                });
        }

        // Update every 500ms
        setInterval(updateData, 500);
        
        // First update immediately
        updateData();
        
        // Update saved commands counter
        updateSavedCount();
        
        function saveCommand() {
            fetch('/save')
                .then(response => response.json())
                .then(data => {
                    showMessage(data.message, data.success ? 'success' : 'error');
                    if (data.success) {
                        updateSavedCount();
                    }
                })
                .catch(error => {
                    showMessage('Save error!', 'error');
                });
        }
        
        function saveCommandAuto() {
            fetch('/save')
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        updateSavedCount();
                        // Discreet message for auto-save
                        console.log('✓ Command saved automatically');
                    }
                })
                .catch(error => {
                    console.error('Auto-save error:', error);
                });
        }
        
        function downloadCommands() {
            window.location.href = '/download';
        }
        
        function clearCommands() {
            if (confirm('Are you sure you want to delete all saved commands?')) {
                fetch('/clear')
                    .then(response => response.json())
                    .then(data => {
                        showMessage(data.message, 'success');
                        updateSavedCount();
                    })
                    .catch(error => {
                        showMessage('Delete error!', 'error');
                    });
            }
        }
        
        function updateSavedCount() {
            fetch('/count')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('savedCount').textContent = data.count;
                })
                .catch(error => console.error('Error:', error));
        }
        
        function showMessage(text, type) {
            const msg = document.getElementById('message');
            msg.textContent = text;
            msg.style.display = 'block';
            msg.style.background = type === 'success' ? '#d1fae5' : '#fee2e2';
            msg.style.color = type === 'success' ? '#065f46' : '#991b1b';
            setTimeout(() => { msg.style.display = 'none'; }, 3000);
        }
        
        // Functions for tabs
        function switchTab(tabName) {
            // Hide all tabs
            document.querySelectorAll('.tab-content').forEach(tab => {
                tab.classList.remove('active');
            });
            document.querySelectorAll('.tab-button').forEach(btn => {
                btn.classList.remove('active');
            });
            
            // Show selected tab
            document.getElementById(tabName + '-tab').classList.add('active');
            event.target.classList.add('active');
            
            // Update WiFi status when WiFi tab is opened
            if (tabName === 'wifi') {
                updateWiFiStatus();
            }
        }
        
        // Functions for WiFi
        function updateWiFiStatus() {
            fetch('/wifi_status')
                .then(response => response.json())
                .then(data => {
                    const statusDiv = document.getElementById('wifiStatus');
                    document.getElementById('currentIP').textContent = data.ip;
                    
                    if (data.connected) {
                        statusDiv.className = 'wifi-status wifi-connected';
                        statusDiv.innerHTML = '✅ Connected to: <strong>' + data.ssid + '</strong> | IP: ' + data.ip;
                    } else {
                        statusDiv.className = 'wifi-status wifi-disconnected';
                        statusDiv.innerHTML = '📡 Mode: Access Point | IP: ' + data.ip;
                    }
                    
                    if (data.saved_ssid) {
                        document.getElementById('wifiSSID').value = data.saved_ssid;
                    }
                })
                .catch(error => console.error('Error:', error));
        }
        
        function saveWiFiConfig() {
            const ssid = document.getElementById('wifiSSID').value;
            const password = document.getElementById('wifiPassword').value;
            
            if (!ssid) {
                showWiFiMessage('Please enter SSID!', 'error');
                return;
            }
            
            showWiFiMessage('Saving configuration...', 'info');
            
            fetch('/wifi_config', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password)
            })
            .then(response => response.json())
            .then(data => {
                showWiFiMessage(data.message, data.success ? 'success' : 'error');
                if (data.success) {
                    setTimeout(() => {
                        showWiFiMessage('ESP32 will restart in 3 seconds...', 'info');
                    }, 2000);
                }
            })
            .catch(error => {
                showWiFiMessage('Save error!', 'error');
            });
        }
        
        function clearWiFiConfig() {
            if (confirm('Are you sure you want to delete WiFi configuration?')) {
                fetch('/wifi_clear')
                    .then(response => response.json())
                    .then(data => {
                        showWiFiMessage(data.message, 'success');
                        document.getElementById('wifiSSID').value = '';
                        document.getElementById('wifiPassword').value = '';
                        setTimeout(() => updateWiFiStatus(), 1000);
                    })
                    .catch(error => {
                        showWiFiMessage('Delete error!', 'error');
                    });
            }
        }
        
        function showWiFiMessage(text, type) {
            const msg = document.getElementById('wifiMessage');
            msg.textContent = text;
            msg.style.display = 'block';
            
            if (type === 'success') {
                msg.style.background = '#d1fae5';
                msg.style.color = '#065f46';
            } else if (type === 'error') {
                msg.style.background = '#fee2e2';
                msg.style.color = '#991b1b';
            } else {
                msg.style.background = '#dbeafe';
                msg.style.color = '#1e40af';
            }
            
            if (type !== 'info') {
                setTimeout(() => { msg.style.display = 'none'; }, 5000);
            }
        }
    </script>
</body>
</html>
)rawliteral";

// Handler for main page
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Handler for JSON data (AJAX endpoint)
void handleData() {
  // Escape newlines for valid JSON
  String escapedRawData = lastRawData;
  escapedRawData.replace("\n", "\\n");
  escapedRawData.replace("\"", "\\\"");
  
  String json = "{";
  json += "\"protocol\":\"" + lastProtocol + "\",";
  json += "\"address\":\"" + lastAddress + "\",";
  json += "\"command\":\"" + lastCommand + "\",";
  json += "\"rawData\":\"" + escapedRawData + "\",";
  json += "\"count\":" + String(signalCount) + ",";
  
  if (lastReceiveTime > 0) {
    unsigned long timeAgo = (millis() - lastReceiveTime) / 1000;
    json += "\"lastTime\":\"" + String(timeAgo) + " seconds ago\"";
  } else {
    json += "\"lastTime\":\"No signal yet\"";
  }
  
  json += "}";
  
  server.send(200, "application/json", json);
}

// Handler for saving current command
void handleSave() {
  if (lastProtocol == "N/A" || lastCommand == "N/A") {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"No data to save!\"}");
    return;
  }
  
  if (savedCommands.size() >= MAX_SAVED_COMMANDS) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Limit reached! (max 50 commands)\"}");
    return;
  }
  
  IRCommand cmd;
  cmd.protocol = lastProtocol;
  cmd.address = lastAddress;
  cmd.command = lastCommand;
  cmd.rawData = lastRawData;
  cmd.timestamp = String(millis() / 1000) + "s";
  
  savedCommands.push_back(cmd);
  
  String response = "{\"success\":true,\"message\":\"Comandă salvată! Total: " + String(savedCommands.size()) + "\"}";
  server.send(200, "application/json", response);
}

// Handler for downloading commands file
void handleDownload() {
  if (savedCommands.empty()) {
    server.send(200, "text/plain", "No saved commands!");
    return;
  }
  
  String content = "========================================\n";
  content += "ESP32 IR RECEIVER - SAVED COMMANDS\n";
  content += "========================================\n";
  content += "Total commands: " + String(savedCommands.size()) + "\n";
  content += "Export date: " + String(millis() / 1000) + " seconds since boot\n";
  content += "========================================\n\n";
  
  for (size_t i = 0; i < savedCommands.size(); i++) {
    content += "--- Command #" + String(i + 1) + " ---\n";
    content += "Timestamp: " + savedCommands[i].timestamp + "\n";
    content += "Protocol: " + savedCommands[i].protocol + "\n";
    content += "Address: " + savedCommands[i].address + "\n";
    content += "Command: " + savedCommands[i].command + "\n";
    content += "Details:\n" + savedCommands[i].rawData + "\n";
    content += "\n";
  }
  
  content += "========================================\n";
  content += "Generated by ESP32 WROOM with IR Receiver KY-022\n";
  content += "========================================\n";
  
  server.sendHeader("Content-Disposition", "attachment; filename=ir_commands.txt");
  server.send(200, "text/plain", content);
}

// Handler for deleting commands
void handleClear() {
  savedCommands.clear();
  server.send(200, "application/json", "{\"success\":true,\"message\":\"All commands deleted!\"}");
}

// Handler for saved commands count
void handleCount() {
  String json = "{\"count\":" + String(savedCommands.size()) + "}";
  server.send(200, "application/json", json);
}

// Functions for managing WiFi config in Preferences
void saveWiFiCredentials(String ssid, String password) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putBool("configured", true);
  preferences.end();
  Serial.println("WiFi credentials saved to EEPROM");
}

void loadWiFiCredentials() {
  preferences.begin("wifi", true);
  wifiConfigured = preferences.getBool("configured", false);
  if (wifiConfigured) {
    wifi_ssid = preferences.getString("ssid", "");
    wifi_password = preferences.getString("password", "");
    Serial.println("WiFi credentials loaded from EEPROM");
    Serial.println("SSID: " + wifi_ssid);
  }
  preferences.end();
}

void clearWiFiCredentials() {
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
  wifiConfigured = false;
  wifiConnected = false;
  wifi_ssid = "";
  wifi_password = "";
  Serial.println("WiFi credentials erased from EEPROM");
}

// Handler for WiFi status
void handleWiFiStatus() {
  String json = "{";
  json += "\"connected\":" + String(wifiConnected ? "true" : "false") + ",";
  json += "\"ssid\":\"" + (wifiConnected ? WiFi.SSID() : String(ap_ssid)) + "\",";
  json += "\"ip\":\"" + (wifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) + "\",";
  json += "\"saved_ssid\":\"" + wifi_ssid + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// Handler for saving WiFi configuration
void handleWiFiConfig() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  
  if (ssid.length() == 0) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Invalid SSID!\"}");
    return;
  }
  
  // Save credentials
  saveWiFiCredentials(ssid, password);
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Configuration saved! ESP32 will restart...\"}");
  
  // Restart ESP32 after 3 seconds
  delay(3000);
  ESP.restart();
}

// Handler for clearing WiFi configuration
void handleWiFiClear() {
  clearWiFiCredentials();
  server.send(200, "application/json", "{\"success\":true,\"message\":\"WiFi configuration cleared!\"}");
}

// Function for connecting to WiFi
bool connectToWiFi() {
  if (!wifiConfigured || wifi_ssid.length() == 0) {
    return false;
  }
  
  Serial.println("\nAttempting to connect to WiFi: " + wifi_ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n✅ Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\n❌ Could not connect to WiFi");
    return false;
  }
}

// Function for starting Access Point
void startAccessPoint() {
  Serial.println("\nStarting Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point created! SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Password: ");
  Serial.println(ap_password);
  Serial.print("IP Address: ");
  Serial.println(IP);
}

void setup() { 
  Serial.begin(115200); 
  delay(200); 
  
  // LED configuration
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // LED test
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  
  // IR receiver configuration
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); 
  Serial.println("KY-022 + ESP32: IR receiver ready."); 
  
  // Load WiFi credentials from EEPROM
  Serial.println("\n=== WIFI CONFIGURATION ===");
  loadWiFiCredentials();
  
  // Try WiFi connection or start Access Point
  if (wifiConfigured && connectToWiFi()) {
    Serial.println("\n✅ Mode: WiFi Client");
    Serial.println("Open in browser: http://" + WiFi.localIP().toString());
  } else {
    startAccessPoint();
    Serial.println("\n📡 Mode: Access Point");
    Serial.println("Open in browser: http://" + WiFi.softAPIP().toString());
  }
  
  // Web server configuration
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/save", handleSave);
  server.on("/download", handleDownload);
  server.on("/clear", handleClear);
  server.on("/count", handleCount);
  server.on("/wifi_status", handleWiFiStatus);
  server.on("/wifi_config", handleWiFiConfig);
  server.on("/wifi_clear", handleWiFiClear);
  server.begin();
  
  Serial.println("\n✅ Web server started!");
  Serial.println("Functions: IR monitoring, save commands, WiFi configuration");
  Serial.println("========================\n");
} 

void loop() 
{ 
  // Process HTTP requests
  server.handleClient();
  
  // Check IR signals
  if (IrReceiver.decode()) 
  { 
    digitalWrite(LED_PIN, HIGH);
    signalCount++;
    
    Serial.println("\n=== IR SIGNAL RECEIVED ===");
    
    // Extract data
    lastProtocol = String(getProtocolString(IrReceiver.decodedIRData.protocol));
    lastAddress = "0x" + String(IrReceiver.decodedIRData.address, HEX);
    lastCommand = "0x" + String(IrReceiver.decodedIRData.command, HEX);
    lastReceiveTime = millis();
    
    // Extract RAW data - simplified format
    lastRawData = "Protocol: " + lastProtocol + "\n";
    lastRawData += "Address: " + lastAddress + "\n";
    lastRawData += "Command: " + lastCommand + "\n";
    lastRawData += "Flags: 0x" + String(IrReceiver.decodedIRData.flags, HEX) + "\n";
    lastRawData += "Raw Code: 0x" + String(IrReceiver.decodedIRData.decodedRawData, HEX) + "\n";
    lastRawData += "Bits: " + String(IrReceiver.decodedIRData.numberOfBits);
    
    // Display in Serial
    Serial.print("Protocol: "); Serial.println(lastProtocol);
    Serial.print("Address: "); Serial.println(lastAddress);
    Serial.print("Command: "); Serial.println(lastCommand);
    Serial.println("Raw: " + lastRawData);
    
    IrReceiver.resume(); 
    
    delay(200);
    digitalWrite(LED_PIN, LOW);
  }
  
  delay(10);
}