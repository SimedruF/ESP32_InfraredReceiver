# ESP32 IR Receiver - Web Monitor

Real-time infrared signal monitoring and recording system using ESP32 with web interface and WiFi configuration.

## 📋 Description

This project transforms an ESP32 microcontroller into a powerful infrared signal analyzer with a web-based interface. It captures IR signals from remote controls, displays them in real-time via AJAX, saves commands, and allows downloading them for later use. Features dual WiFi mode (Access Point + Station) with persistent configuration storage.

## 🔧 Hardware Requirements

- **ESP32 DevKit board** (ESP32 WROOM)
- **KY-022 IR Receiver Module** connected to GPIO 14
- **USB cable** (for programming and power)
- **Serial port**: CP2102 USB to UART Bridge (typically `/dev/ttyUSB0` on Linux)

### Wiring

| KY-022 Pin | ESP32 Pin |
|------------|-----------|
| S (Signal) | GPIO 14   |
| VCC (+)    | 3.3V      |
| GND (-)    | GND       |

**Built-in LED** on GPIO 2 provides visual feedback when IR signals are detected.

## 💻 Software Requirements

- **PlatformIO** (VS Code extension or CLI)
- **Arduino Framework** for ESP32
- **Libraries** (auto-installed via PlatformIO):
  - IRremote v4.6.0 (z3t0/IRremote)
  - WiFi (built-in)
  - WebServer (built-in)
  - Preferences (built-in)

## ✨ Features

### IR Signal Detection
- Real-time infrared signal decoding
- Support for multiple protocols (NEC, Sony, RC5, RC6, etc.)
- Visual LED feedback on signal reception
- Automatic protocol recognition

### Web Interface
- **Real-time monitoring** with AJAX updates (500ms intervals)
- Modern responsive design with tabs
- Display of Protocol, Address, Command, and Raw data
- Signal counter and timestamp tracking
- Auto-save mode with checkbox toggle

### Command Storage
- Save up to 50 IR commands in memory
- Download all saved commands as `.txt` file
- Clear command history
- Automatic saving option

### WiFi Management
- **Dual WiFi Mode**:
  - **Station Mode**: Connect to existing WiFi network
  - **Access Point Mode**: Create own WiFi hotspot (fallback)
- Web-based WiFi configuration interface
- **Persistent storage** in EEPROM (survives reboots)
- Easy WiFi credential management (save/clear)

## 🚀 Installation

### 1. Clone the Repository
```bash
git clone https://github.com/SimedruF/ESP32_InfraredReceiver.git
cd ESP32_InfraredReceiver
```

### 2. Open in PlatformIO
- Open the project folder in VS Code with PlatformIO extension
- PlatformIO will automatically install dependencies

### 3. Configure Serial Port
Edit `platformio.ini` if your serial port differs:
```ini
upload_port = /dev/ttyUSB0
monitor_port = /dev/ttyUSB0
```

### 4. Build and Upload
```bash
platformio run --target upload
```

### 5. Monitor Serial Output
```bash
platformio device monitor
```

## 📡 Usage

### First Boot (Access Point Mode)

On first boot or when no WiFi is configured, the ESP32 creates an Access Point:

1. **Connect to WiFi**:
   - SSID: `ESP32_IR_Receiver`
   - Password: `12345678`

2. **Open browser** and navigate to:
   - `http://192.168.4.1`

3. **Configure WiFi** (optional):
   - Go to "WiFi Configuration" tab
   - Enter your local WiFi credentials
   - Click "Save Configuration"
   - ESP32 will restart and connect to your network

### Station Mode (Connected to Local WiFi)

When connected to your WiFi network:

1. Find the IP address in Serial Monitor output:
   ```
   ✅ Mode: WiFi Client
   Open in browser: http://192.168.1.XXX
   ```

2. Access the web interface using that IP address

### Using the Web Interface

#### IR Monitoring Tab
- **View real-time signals**: Point any IR remote at the KY-022 sensor
- **Auto-save mode**: Check "Auto-save commands" to automatically store signals
- **Manual save**: Click "Save Command" button after receiving a signal
- **Download**: Export all saved commands as text file
- **Clear**: Delete all saved commands from memory

#### WiFi Configuration Tab
- **Current status**: Shows connection mode and IP address
- **Configure WiFi**: Enter SSID and password for local network
- **Clear settings**: Removes saved credentials (reverts to AP mode)

## 🔍 Technical Details

### Pin Configuration
- `IR_RECEIVE_PIN`: GPIO 14
- `LED_PIN`: GPIO 2

### WiFi Credentials
- **Default AP SSID**: `ESP32_IR_Receiver`
- **Default AP Password**: `12345678`
- **Station credentials**: Stored in EEPROM via Preferences library

### HTTP Endpoints
- `GET /` - Main web interface
- `GET /data` - Get latest IR signal data (JSON)
- `POST /save` - Save current command to memory
- `GET /download` - Download saved commands as text file
- `POST /clear` - Clear all saved commands
- `GET /count` - Get number of saved commands
- `GET /wifi_status` - Get WiFi connection status
- `POST /wifi_config` - Save WiFi credentials
- `POST /wifi_clear` - Clear WiFi credentials

### Data Structure
```cpp
struct IRCommand {
  String timestamp;
  String protocol;
  String address;
  String command;
  String rawData;
};
```

## 📊 Serial Monitor Output

The Serial Monitor displays detailed information:
```
=== WIFI CONFIGURATION ===
WiFi credentials loaded from EEPROM
SSID: MyWiFi

✅ Mode: WiFi Client
IP Address: 192.168.1.100

✅ Web server started!
Functions: IR monitoring, save commands, WiFi configuration

=== IR SIGNAL RECEIVED ===
Protocol: NEC
Address: 0x00
Command: 0x45
```

## 🛠️ Troubleshooting

### IR Receiver Not Working
- Verify KY-022 wiring (Signal on GPIO 14)
- Check power connections (3.3V and GND)
- Test with different remote controls
- Verify IR receiver orientation (sensor faces remote)

### Cannot Connect to ESP32
- Check if connected to correct WiFi network
- Verify IP address in Serial Monitor
- Try Access Point mode if Station mode fails
- Reset WiFi credentials and reconfigure

### LED Not Blinking
- LED blinks only when IR signal is detected
- Point remote directly at KY-022 sensor
- LED is on GPIO 2 (built-in LED on most boards)

### Upload Failed
- Check USB cable connection
- Verify serial port in `platformio.ini`
- List available ports: `platformio device list`
- Hold BOOT button during upload if necessary

## 📝 License

This project is open source and available for educational and personal use.

## 👤 Author

**SimedruF**
- GitHub: [@SimedruF](https://github.com/SimedruF)

## 🙏 Acknowledgments

- IRremote library by z3t0
- ESP32 Arduino Core
- PlatformIO development platform

---

**Note**: This project was developed using PlatformIO with the Arduino framework for ESP32. The web interface uses vanilla JavaScript with AJAX for real-time updates without requiring external frameworks.
