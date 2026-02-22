#include <Arduino.h>
#include <IRremote.hpp>
#include <WiFi.h>
#include <WebServer.h>
#include <vector>
#include <Preferences.h>

// Configurare pini ESP32
static const uint8_t IR_RECEIVE_PIN = 14; 
static const uint8_t LED_PIN = 2;

// Configurare WiFi Access Point (fallback)
const char* ap_ssid = "ESP32_IR_Receiver";
const char* ap_password = "12345678";

// Variabile pentru WiFi Client (rețea locală)
String wifi_ssid = "";
String wifi_password = "";
bool wifiConfigured = false;
bool wifiConnected = false;

// Preferences pentru salvare în EEPROM
Preferences preferences;

// Server web pe portul 80
WebServer server(80);

// Variabile pentru ultimele date IR primite
String lastProtocol = "N/A";
String lastAddress = "N/A";
String lastCommand = "N/A";
String lastRawData = "N/A";
unsigned long lastReceiveTime = 0;
int signalCount = 0;

// Structură pentru salvarea comenzilor
struct IRCommand {
  String protocol;
  String address;
  String command;
  String rawData;
  String timestamp;
};

// Vector pentru comenzi salvate (max 50 comenzi pentru a nu umple memoria)
std::vector<IRCommand> savedCommands;
const int MAX_SAVED_COMMANDS = 50;

// Pagina HTML cu AJAX
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
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
        <p class="subtitle">Monitorizare și configurare</p>
        
        <!-- Tab-uri -->
        <div class="tabs">
            <button class="tab-button active" onclick="switchTab('monitor')">📊 Monitorizare IR</button>
            <button class="tab-button" onclick="switchTab('wifi')">📡 Configurare WiFi</button>
        </div>
        
        <!-- Tab Monitorizare IR -->
        <div id="monitor-tab" class="tab-content active">
            <div class="status">
                <span class="status-dot"></span>
                <strong>Conexiune activă</strong> | Actualizare la fiecare 500ms
            </div>

            <div class="info-grid">
                <div class="info-box">
                    <div class="info-label">Protocol</div>
                    <div class="info-value" id="protocol">-</div>
                </div>
                <div class="info-box">
                    <div class="info-label">Adresă</div>
                    <div class="info-value" id="address">-</div>
                </div>
                <div class="info-box">
                    <div class="info-label">Comandă</div>
                    <div class="info-value" id="command">-</div>
                </div>
                <div class="info-box">
                    <div class="info-label">Semnale primite</div>
                    <div class="info-value" id="count">0</div>
                </div>
            </div>

            <div class="data-card">
                <div class="data-label">Ultimul semnal primit</div>
                <div class="data-value" id="lastTime">-</div>
            </div>

            <div style="margin-bottom: 10px;">
                <strong>Date RAW (Timings):</strong>
            </div>
            <div class="raw-data" id="rawData">Așteptare date...</div>

            <div style="margin-top: 15px; padding: 15px; background: #f0f9ff; border-radius: 8px; border-left: 4px solid #3b82f6;">
                <label style="display: flex; align-items: center; cursor: pointer; font-weight: 500; color: #1e40af;">
                    <input type="checkbox" id="autoSave" style="width: 20px; height: 20px; cursor: pointer; margin-right: 10px;">
                    <span>🔄 Salvare automată (salvează fiecare comandă nouă primită)</span>
                </label>
            </div>

            <div style="margin-top: 20px; display: flex; gap: 10px; justify-content: center;">
                <button onclick="saveCommand()" class="btn btn-success">
                    💾 Salvează comandă
                </button>
                <button onclick="downloadCommands()" class="btn btn-primary">
                    📥 Descarcă toate (<span id="savedCount">0</span>)
                </button>
                <button onclick="clearCommands()" class="btn btn-danger">
                    🗑️ Șterge toate
                </button>
            </div>

            <div id="message" style="margin-top: 15px; padding: 10px; border-radius: 8px; text-align: center; display: none;"></div>
        </div>
        
        <!-- Tab Configurare WiFi -->
        <div id="wifi-tab" class="tab-content">
            <div id="wifiStatus" class="wifi-status wifi-disconnected">
                Mode: Access Point | IP: <span id="currentIP">-</span>
            </div>
            
            <h3 style="margin-bottom: 15px; color: #1e293b;">Conectare la rețea WiFi locală</h3>
            <p style="margin-bottom: 20px; color: #64748b;">Configurează ESP32 să se conecteze la rețeaua ta WiFi. Datele vor fi salvate în memoria dispozitivului.</p>
            
            <div class="form-group">
                <label class="form-label">SSID (Numele rețelei WiFi)</label>
                <input type="text" id="wifiSSID" class="form-input" placeholder="Numele rețelei WiFi">
            </div>
            
            <div class="form-group">
                <label class="form-label">Parolă WiFi</label>
                <input type="password" id="wifiPassword" class="form-input" placeholder="Parola rețelei WiFi">
            </div>
            
            <div style="display: flex; gap: 10px; margin-top: 20px;">
                <button onclick="saveWiFiConfig()" class="btn btn-primary" style="flex: 1;">
                    💾 Salvează și Conectează
                </button>
                <button onclick="clearWiFiConfig()" class="btn btn-danger">
                    🗑️ Șterge configurație
                </button>
            </div>
            
            <div id="wifiMessage" style="margin-top: 15px; padding: 10px; border-radius: 8px; text-align: center; display: none;"></div>
            
            <div style="margin-top: 30px; padding: 15px; background: #f8fafc; border-radius: 8px; border-left: 4px solid #667eea;">
                <h4 style="margin-bottom: 10px; color: #1e293b;">ℹ️ Informații</h4>
                <ul style="margin-left: 20px; color: #64748b; line-height: 1.8;">
                    <li>Configurația se salvează în EEPROM</li>
                    <li>ESP32 va încerca să se conecteze la WiFi la următoarea pornire</li>
                    <li>Dacă conexiunea eșuează, se va activa Access Point-ul</li>
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
                    
                    // Salvare automată dacă checkbox-ul este bifat
                    const autoSave = document.getElementById('autoSave').checked;
                    if (autoSave && data.command !== 'N/A') {
                        const currentCommand = data.protocol + '_' + data.address + '_' + data.command;
                        // Salvează doar dacă e o comandă nouă (diferită de ultima salvată)
                        if (currentCommand !== lastSavedCommand) {
                            lastSavedCommand = currentCommand;
                            saveCommandAuto();
                        }
                    }
                })
                .catch(error => {
                    console.error('Eroare la actualizare:', error);
                });
        }

        // Actualizare la fiecare 500ms
        setInterval(updateData, 500);
        
        // Prima actualizare imediată
        updateData();
        
        // Actualizare contor comenzi salvate
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
                    showMessage('Eroare la salvare!', 'error');
                });
        }
        
        function saveCommandAuto() {
            fetch('/save')
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        updateSavedCount();
                        // Mesaj discret pentru salvare automată
                        console.log('✓ Comandă salvată automat');
                    }
                })
                .catch(error => {
                    console.error('Eroare la salvare automată:', error);
                });
        }
        
        function downloadCommands() {
            window.location.href = '/download';
        }
        
        function clearCommands() {
            if (confirm('Sigur vrei să ștergi toate comenzile salvate?')) {
                fetch('/clear')
                    .then(response => response.json())
                    .then(data => {
                        showMessage(data.message, 'success');
                        updateSavedCount();
                    })
                    .catch(error => {
                        showMessage('Eroare la ștergere!', 'error');
                    });
            }
        }
        
        function updateSavedCount() {
            fetch('/count')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('savedCount').textContent = data.count;
                })
                .catch(error => console.error('Eroare:', error));
        }
        
        function showMessage(text, type) {
            const msg = document.getElementById('message');
            msg.textContent = text;
            msg.style.display = 'block';
            msg.style.background = type === 'success' ? '#d1fae5' : '#fee2e2';
            msg.style.color = type === 'success' ? '#065f46' : '#991b1b';
            setTimeout(() => { msg.style.display = 'none'; }, 3000);
        }
        
        // Funcții pentru tab-uri
        function switchTab(tabName) {
            // Ascunde toate tab-urile
            document.querySelectorAll('.tab-content').forEach(tab => {
                tab.classList.remove('active');
            });
            document.querySelectorAll('.tab-button').forEach(btn => {
                btn.classList.remove('active');
            });
            
            // Arată tab-ul selectat
            document.getElementById(tabName + '-tab').classList.add('active');
            event.target.classList.add('active');
            
            // Actualizează statusul WiFi când se deschide tab-ul WiFi
            if (tabName === 'wifi') {
                updateWiFiStatus();
            }
        }
        
        // Funcții pentru WiFi
        function updateWiFiStatus() {
            fetch('/wifi_status')
                .then(response => response.json())
                .then(data => {
                    const statusDiv = document.getElementById('wifiStatus');
                    document.getElementById('currentIP').textContent = data.ip;
                    
                    if (data.connected) {
                        statusDiv.className = 'wifi-status wifi-connected';
                        statusDiv.innerHTML = '✅ Conectat la: <strong>' + data.ssid + '</strong> | IP: ' + data.ip;
                    } else {
                        statusDiv.className = 'wifi-status wifi-disconnected';
                        statusDiv.innerHTML = '📡 Mode: Access Point | IP: ' + data.ip;
                    }
                    
                    if (data.saved_ssid) {
                        document.getElementById('wifiSSID').value = data.saved_ssid;
                    }
                })
                .catch(error => console.error('Eroare:', error));
        }
        
        function saveWiFiConfig() {
            const ssid = document.getElementById('wifiSSID').value;
            const password = document.getElementById('wifiPassword').value;
            
            if (!ssid) {
                showWiFiMessage('Te rog introdu SSID-ul!', 'error');
                return;
            }
            
            showWiFiMessage('Se salvează configurația...', 'info');
            
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
                        showWiFiMessage('ESP32 se va restarta în 3 secunde...', 'info');
                    }, 2000);
                }
            })
            .catch(error => {
                showWiFiMessage('Eroare la salvare!', 'error');
            });
        }
        
        function clearWiFiConfig() {
            if (confirm('Sigur vrei să ștergi configurația WiFi?')) {
                fetch('/wifi_clear')
                    .then(response => response.json())
                    .then(data => {
                        showWiFiMessage(data.message, 'success');
                        document.getElementById('wifiSSID').value = '';
                        document.getElementById('wifiPassword').value = '';
                        setTimeout(() => updateWiFiStatus(), 1000);
                    })
                    .catch(error => {
                        showWiFiMessage('Eroare la ștergere!', 'error');
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

// Handler pentru pagina principală
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Handler pentru date JSON (AJAX endpoint)
void handleData() {
  // Escape newlines pentru JSON valid
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
    json += "\"lastTime\":\"Acum " + String(timeAgo) + " secunde\"";
  } else {
    json += "\"lastTime\":\"Niciun semnal încă\"";
  }
  
  json += "}";
  
  server.send(200, "application/json", json);
}

// Handler pentru salvarea comenzii curente
void handleSave() {
  if (lastProtocol == "N/A" || lastCommand == "N/A") {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Nu există date de salvat!\"}");
    return;
  }
  
  if (savedCommands.size() >= MAX_SAVED_COMMANDS) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Limită atinsă! (max 50 comenzi)\"}");
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

// Handler pentru descărcarea fișierului cu comenzi
void handleDownload() {
  if (savedCommands.empty()) {
    server.send(200, "text/plain", "Nu există comenzi salvate!");
    return;
  }
  
  String content = "========================================\n";
  content += "ESP32 IR RECEIVER - COMENZI SALVATE\n";
  content += "========================================\n";
  content += "Total comenzi: " + String(savedCommands.size()) + "\n";
  content += "Data export: " + String(millis() / 1000) + " secunde de la pornire\n";
  content += "========================================\n\n";
  
  for (size_t i = 0; i < savedCommands.size(); i++) {
    content += "--- Comanda #" + String(i + 1) + " ---\n";
    content += "Timestamp: " + savedCommands[i].timestamp + "\n";
    content += "Protocol: " + savedCommands[i].protocol + "\n";
    content += "Address: " + savedCommands[i].address + "\n";
    content += "Command: " + savedCommands[i].command + "\n";
    content += "Details:\n" + savedCommands[i].rawData + "\n";
    content += "\n";
  }
  
  content += "========================================\n";
  content += "Generat de ESP32 WROOM cu IR Receiver KY-022\n";
  content += "========================================\n";
  
  server.sendHeader("Content-Disposition", "attachment; filename=ir_commands.txt");
  server.send(200, "text/plain", content);
}

// Handler pentru ștergerea comenzilor
void handleClear() {
  savedCommands.clear();
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Toate comenzile au fost șterse!\"}");
}

// Handler pentru numărul de comenzi salvate
void handleCount() {
  String json = "{\"count\":" + String(savedCommands.size()) + "}";
  server.send(200, "application/json", json);
}

// Funcții pentru gestionarea WiFi config în Preferences
void saveWiFiCredentials(String ssid, String password) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putBool("configured", true);
  preferences.end();
  Serial.println("WiFi credentials salvate în EEPROM");
}

void loadWiFiCredentials() {
  preferences.begin("wifi", true);
  wifiConfigured = preferences.getBool("configured", false);
  if (wifiConfigured) {
    wifi_ssid = preferences.getString("ssid", "");
    wifi_password = preferences.getString("password", "");
    Serial.println("WiFi credentials încărcate din EEPROM");
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
  Serial.println("WiFi credentials șterse din EEPROM");
}

// Handler pentru statusul WiFi
void handleWiFiStatus() {
  String json = "{";
  json += "\"connected\":" + String(wifiConnected ? "true" : "false") + ",";
  json += "\"ssid\":\"" + (wifiConnected ? WiFi.SSID() : String(ap_ssid)) + "\",";
  json += "\"ip\":\"" + (wifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) + "\",";
  json += "\"saved_ssid\":\"" + wifi_ssid + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// Handler pentru salvarea configurației WiFi
void handleWiFiConfig() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  
  if (ssid.length() == 0) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"SSID invalid!\"}");
    return;
  }
  
  // Salvează credențialele
  saveWiFiCredentials(ssid, password);
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Configurație salvată! ESP32 se va restarta...\"}");
  
  // Restart ESP32 după 3 secunde
  delay(3000);
  ESP.restart();
}

// Handler pentru ștergerea configurației WiFi
void handleWiFiClear() {
  clearWiFiCredentials();
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Configurație WiFi ștearsă!\"}");
}

// Funcție pentru conectare la WiFi
bool connectToWiFi() {
  if (!wifiConfigured || wifi_ssid.length() == 0) {
    return false;
  }
  
  Serial.println("\nÎncerc conectare la WiFi: " + wifi_ssid);
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
    Serial.println("\n✅ Conectat la WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\n❌ Nu s-a putut conecta la WiFi");
    return false;
  }
}

// Funcție pentru pornirea Access Point
void startAccessPoint() {
  Serial.println("\nPornire Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point creat! SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Parola: ");
  Serial.println(ap_password);
  Serial.print("IP Address: ");
  Serial.println(IP);
}

void setup() { 
  Serial.begin(115200); 
  delay(200); 
  
  // Configurare LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Test LED
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  
  // Configurare IR receiver
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); 
  Serial.println("KY-022 + ESP32: IR receiver ready."); 
  
  // Încarcă credențialele WiFi din EEPROM
  Serial.println("\n=== CONFIGURARE WIFI ===");
  loadWiFiCredentials();
  
  // Încearcă conectare la WiFi sau pornește Access Point
  if (wifiConfigured && connectToWiFi()) {
    Serial.println("\n✅ Mod: WiFi Client");
    Serial.println("Deschide în browser: http://" + WiFi.localIP().toString());
  } else {
    startAccessPoint();
    Serial.println("\n📡 Mod: Access Point");
    Serial.println("Deschide în browser: http://" + WiFi.softAPIP().toString());
  }
  
  // Configurare server web
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
  
  Serial.println("\n✅ Server web pornit!");
  Serial.println("Funcții: monitorizare IR, salvare comenzi, configurare WiFi");
  Serial.println("========================\n");
} 

void loop() 
{ 
  // Procesează cererile HTTP
  server.handleClient();
  
  // Verifică semnale IR
  if (IrReceiver.decode()) 
  { 
    digitalWrite(LED_PIN, HIGH);
    signalCount++;
    
    Serial.println("\n=== SEMNAL IR PRIMIT ===");
    
    // Extrage datele
    lastProtocol = String(getProtocolString(IrReceiver.decodedIRData.protocol));
    lastAddress = "0x" + String(IrReceiver.decodedIRData.address, HEX);
    lastCommand = "0x" + String(IrReceiver.decodedIRData.command, HEX);
    lastReceiveTime = millis();
    
    // Extrage RAW data - formatare simplificată
    lastRawData = "Protocol: " + lastProtocol + "\n";
    lastRawData += "Address: " + lastAddress + "\n";
    lastRawData += "Command: " + lastCommand + "\n";
    lastRawData += "Flags: 0x" + String(IrReceiver.decodedIRData.flags, HEX) + "\n";
    lastRawData += "Raw Code: 0x" + String(IrReceiver.decodedIRData.decodedRawData, HEX) + "\n";
    lastRawData += "Bits: " + String(IrReceiver.decodedIRData.numberOfBits);
    
    // Afișare în Serial
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