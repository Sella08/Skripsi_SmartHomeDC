/* ====================================
   Smart Home DC Control System - ESP32 Integration (UPDATED FOR REAL-TIME)
   ==================================== */

/* ====================================
   CONFIGURATION - UPDATED
   ==================================== */
const CONFIG = {
    ESP_ID: 'esp32_smart_home_01',
    UPDATE_INTERVAL: 2000, // 2 detik - real-time updates
    SERVER_URL: window.location.origin,
    ENDPOINTS: {
        DASHBOARD: 'dashboard.php',
        GET_DATA: 'getdata.php',
        UPDATE_DATA: 'updatedata.php',
        REALTIME_API: 'api_realtime.php'  // TAMBAH INI - ENDPOINT BARU
    }
};

/* ====================================
   GLOBAL VARIABLES - Data dari ESP32
   ==================================== */
let currentData = {
    // Device states - dari ESP32
    devices: {
        L1: 'OFF', L2: 'OFF', L3: 'OFF', L4: 'OFF', L5: 'OFF',
        K1: 'OFF', K2: 'OFF'
    },
    
    // System mode - dikirim ke ESP32
    mode: 'MANUAL',
    
    // Sensor data - dari ESP32 (DHT11, LDR, tegangan baterai)
    battery: {
        voltage: 12.6,
        soc: 75.0,
        estimatedRuntime: 8.5
    },
    
    sensors: {
        temperatureKamar: 28.0,
        temperatureRtamu: 28.0,
        humidityKamar: 65.0,
        humidityRtamu: 65.0,
        lightLevel: 250,
        statusReadSensors: 'SUCCEED'
    },
    
    // PIR data - dari ESP32 (PCF8574)
    pir: {
        teras: 0,
        ruangTamu: 0,
        kamar: 0,
        wc: 0,
        dapur: 0
    },
    
    // Power data - dari ESP32 (INA226 sensors)
    power: {
        L1: 0, L2: 0, L3: 0, L4: 0, L5: 0,
        K1: 0, K2: 0, usb: 0, total: 0
    },
    
    // Current data - dari ESP32 (INA226 sensors)
    current: {
        L1: 0, L2: 0, L3: 0, L4: 0, L5: 0,
        K1: 0, K2: 0, usb: 0, total: 0
    },
    
    // Voltage data - dari ESP32 (INA226 sensors)
    voltage: {
        L1: 0, L2: 0, L3: 0, L4: 0, L5: 0,
        K1: 0, K2: 0, usb: 5, system: 12
    },
    
    // Connection status
    lastUpdate: new Date(),
    isConnected: false
};

let updateInterval;
let isUpdating = false;

/* ====================================
   INITIALIZATION
   ==================================== */
document.addEventListener('DOMContentLoaded', function() {
    console.log('Smart Home DC Frontend Starting - ESP32 Integration...');
    
    // Load initial data dari PHP (jika ada)
    if (typeof window.ESP32_DATA !== 'undefined') {
        currentData = { ...currentData, ...window.ESP32_DATA };
        console.log('Initial ESP32 data loaded:', currentData);
    }
    
    initializeSystem();
    setupEventListeners();
    startAutoUpdate();
});

function initializeSystem() {
    // Update display dengan data awal
    updateAllDisplays();
    
    // Mulai fetch data real-time dari ESP32
    setTimeout(() => {
        fetchDataFromServer();
    }, 1000);
}

function setupEventListeners() {
    // Mode switcher event listeners
    const manualModeRadio = document.getElementById('manual-mode');
    const autoModeRadio = document.getElementById('auto-mode');
    
    if (manualModeRadio) {
        manualModeRadio.addEventListener('change', () => {
            if (manualModeRadio.checked) {
                changeMode('MANUAL');
            }
        });
    }
    
    if (autoModeRadio) {
        autoModeRadio.addEventListener('change', () => {
            if (autoModeRadio.checked) {
                changeMode('AUTO');
            }
        });
    }
    
    // Keyboard shortcuts untuk kontrol cepat
    document.addEventListener('keydown', function(e) {
        if (e.ctrlKey) {
            switch(e.key) {
                case '1': toggleDevice('L1'); break;
                case '2': toggleDevice('L2'); break;
                case '3': toggleDevice('L3'); break;
                case '4': toggleDevice('L4'); break;
                case '5': toggleDevice('L5'); break;
                case 'q': toggleDevice('K1'); break;
                case 'w': toggleDevice('K2'); break;
                case 'm': changeMode('MANUAL'); break;
                case 'a': changeMode('AUTO'); break;
            }
            e.preventDefault();
        }
    });
}

/* ====================================
   DEVICE CONTROL FUNCTIONS - Kirim ke ESP32
   ==================================== */
function toggleDevice(deviceId) {
    if (isUpdating) {
        console.log('Update sedang berlangsung, harap tunggu...');
        return;
    }
    
    console.log(`Mengubah status device: ${deviceId}`);
    
    const newState = currentData.devices[deviceId] === 'ON' ? 'OFF' : 'ON';
    
    // Update lokal state untuk UI responsif
    currentData.devices[deviceId] = newState;
    updateDeviceDisplay();
    
    // Kirim command ke ESP32 via server
    sendDeviceCommand(deviceId, newState);
}

function changeMode(newMode) {
    if (isUpdating) {
        console.log('Update sedang berlangsung, harap tunggu...');
        return;
    }
    
    console.log(`Mengubah mode ke: ${newMode}`);
    
    // Update lokal state
    currentData.mode = newMode;
    updateModeDisplay();
    
    // Kirim mode change ke ESP32 via server
    sendModeCommand(newMode);
}

/* ====================================
   SERVER COMMUNICATION - ESP32 Interface (UPDATED)
   ==================================== */
async function sendDeviceCommand(deviceId, state) {
    showLoading(true);
    
    try {
        const formData = new FormData();
        formData.append('action', 'control_device');
        formData.append('device', deviceId.toLowerCase());
        formData.append('state', state);
        
        const response = await fetch(CONFIG.ENDPOINTS.DASHBOARD, {
            method: 'POST',
            body: formData
        });
        
        if (response.ok) {
            const result = await response.json();
            console.log(`Device command berhasil dikirim ke ESP32: ${deviceId} = ${state}`);
            showAlert(`${deviceId} berhasil di${state === 'ON' ? 'aktifkan' : 'matikan'}`, 'success');
            
            // Fetch updated data setelah delay singkat untuk mendapat respons ESP32
            setTimeout(() => {
                fetchDataFromServer();
            }, 500);
        } else {
            throw new Error('Gagal mengirim device command ke ESP32');
        }
    } catch (error) {
        console.error('Error mengirim device command ke ESP32:', error);
        showAlert('Gagal mengirim perintah ke ESP32', 'error');
        
        // Revert lokal state jika error
        currentData.devices[deviceId] = currentData.devices[deviceId] === 'ON' ? 'OFF' : 'ON';
        updateDeviceDisplay();
    } finally {
        showLoading(false);
    }
}

async function sendModeCommand(mode) {
    showLoading(true);
    
    try {
        const formData = new FormData();
        formData.append('action', 'change_mode');
        formData.append('mode', mode);
        
        const response = await fetch(CONFIG.ENDPOINTS.DASHBOARD, {
            method: 'POST',
            body: formData
        });
        
        showAlert('Mengubah mode pada ESP32', 'success');
    } catch (error) {
        console.log(' ====> Error mengubah mode di ESP32:', error);
        showAlert('Gagal mengubah mode pada ESP32', 'error');
        
        // Revert lokal state jika error
        currentData.mode = currentData.mode === 'MANUAL' ? 'AUTO' : 'MANUAL';
        updateModeDisplay();
    } finally {
        showLoading(false);
    }
}

// FUNGSI BARU: Fetch data real-time dari database via API
async function fetchDataFromServer() {
    try {
        console.log('🔄 Fetching real-time data from database...');
        console.log('📡 API URL:', CONFIG.ENDPOINTS.REALTIME_API);
        console.log('⏰ Last successful update:', currentData.lastUpdate);
        console.log('🔗 Connection status:', currentData.isConnected);
        
        // GANTI: Gunakan API endpoint khusus, bukan parsing HTML
        const response = await fetch(CONFIG.ENDPOINTS.REALTIME_API + '?t=' + Date.now());
        
        if (response.ok) {
            const jsonData = await response.json();
            
            if (jsonData.status === 'success') {
                // Simpan data lama untuk comparison
                const oldData = JSON.parse(JSON.stringify(currentData));
                
                // Update currentData dengan data real-time dari database
                updateCurrentDataFromAPI(jsonData);
                currentData.isConnected = true;
                currentData.lastUpdate = new Date();
                
                // Log changes
                logDataChanges(currentData, oldData);
                
                console.log('✅ Real-time data berhasil diterima dari database:', jsonData);
            } else {
                throw new Error('API returned error: ' + jsonData.message);
            }
        } else {
            throw new Error('HTTP error: ' + response.status);
        }
    } catch (error) {
        console.error('❌ Error mengambil data real-time dari database:', error);
        currentData.isConnected = false;
    }
    
    updateConnectionStatus();
    updateAllDisplays();
}

// FUNGSI BARU: Update data dari API response
function updateCurrentDataFromAPI(apiData) {
    // Update semua data dari API response
    currentData.mode = apiData.mode;
    currentData.devices = apiData.devices;
    currentData.battery = apiData.battery;
    currentData.sensors = apiData.sensors;
    currentData.pir = apiData.pir;
    currentData.power = apiData.power;
    currentData.current = apiData.current;
    currentData.voltage = apiData.voltage;
}

// FUNGSI BARU: Log perubahan data untuk debugging
function logDataChanges(newData, oldData) {
    const changes = [];
    
    // Check device changes
    for (let device in newData.devices) {
        if (newData.devices[device] !== oldData.devices[device]) {
            changes.push(`Device ${device}: ${oldData.devices[device]} → ${newData.devices[device]}`);
        }
    }
    
    // Check battery changes
    if (Math.abs(newData.battery.soc - oldData.battery.soc) > 0.1) {
        changes.push(`Battery SOC: ${oldData.battery.soc}% → ${newData.battery.soc}%`);
    }
    
    // Check power changes
    if (Math.abs(newData.power.total - oldData.power.total) > 0.1) {
        changes.push(`Total Power: ${oldData.power.total}W → ${newData.power.total}W`);
    }
    
    // Check temperature changes
    if (Math.abs(newData.sensors.temperatureKamar - oldData.sensors.temperatureKamar) > 0.1) {
        changes.push(`Temp Kamar: ${oldData.sensors.temperatureKamar}°C → ${newData.sensors.temperatureKamar}°C`);
    }
    
    if (changes.length > 0) {
        console.log('📊 Data berubah:', changes);
    }
}

/* ====================================
   DISPLAY UPDATE FUNCTIONS - Frontend Display
   ==================================== */
function updateAllDisplays() {
    updateDeviceDisplay();
    updateModeDisplay();
    updateBatteryDisplay();
    updateSensorDisplay();
    updatePowerDisplay();
    updateConnectionStatus();
    updateLastUpdateTime();
}

function updateDeviceDisplay() {
    for (let deviceId in currentData.devices) {
        const isActive = currentData.devices[deviceId] === 'ON';
        const deviceElement = document.querySelector(`[data-device="${deviceId}"]`);
        const statusElement = document.getElementById(`status-${deviceId}`);
        const deviceItemElement = document.querySelector(`.device-item[data-device="${deviceId}"]`);
        
        if (deviceElement) {
            if (isActive) {
                deviceElement.classList.add('active');
            } else {
                deviceElement.classList.remove('active');
            }
        }
        
        if (statusElement) {
            statusElement.textContent = isActive ? 'ON' : 'OFF';
            statusElement.className = `device-status ${isActive ? 'on' : ''}`;
        }
        
        if (deviceItemElement) {
            if (isActive) {
                deviceItemElement.classList.add('active');
            } else {
                deviceItemElement.classList.remove('active');
            }
        }
        
        // Update power, current, voltage display untuk individual devices
        const powerElement = document.getElementById(`power-${deviceId}`);
        if (powerElement) {
            powerElement.textContent = `${currentData.power[deviceId].toFixed(1)}W`;
        }
        
        const currentElement = document.getElementById(`current-${deviceId}`);
        if (currentElement) {
            currentElement.textContent = `${currentData.current[deviceId].toFixed(2)}A`;
        }
        
        const voltageElement = document.getElementById(`voltage-${deviceId}`);
        if (voltageElement) {
            voltageElement.textContent = `${currentData.voltage[deviceId].toFixed(1)}V`;
        }
        
        // Update display metrics di room layout
        const powerDisplayElement = document.getElementById(`power-${deviceId}-display`);
        if (powerDisplayElement) {
            powerDisplayElement.textContent = `${currentData.power[deviceId].toFixed(1)}W`;
        }
        
        const currentDisplayElement = document.getElementById(`current-${deviceId}-display`);
        if (currentDisplayElement) {
            currentDisplayElement.textContent = `${currentData.current[deviceId].toFixed(2)}A`;
        }
        
        const voltageDisplayElement = document.getElementById(`voltage-${deviceId}-display`);
        if (voltageDisplayElement) {
            voltageDisplayElement.textContent = `${currentData.voltage[deviceId].toFixed(1)}V`;
        }
    }
}

function updateModeDisplay() {
    const modeIndicator = document.getElementById('mode-indicator');
    const modeStatusText = document.getElementById('mode-status-text');
    const modeDescription = document.getElementById('mode-description');
    const manualRadio = document.getElementById('manual-mode');
    const autoRadio = document.getElementById('auto-mode');
    
    if (currentData.mode === 'MANUAL') {
        if (modeIndicator) modeIndicator.className = 'indicator-dot';
        if (modeStatusText) modeStatusText.textContent = 'Mode Manual Aktif';
        if (modeDescription) {
            modeDescription.textContent = 'Kontrol manual aktif. ESP32 menunggu perintah dari pengguna.';
        }
        if (manualRadio) manualRadio.checked = true;
    } else {
        if (modeIndicator) modeIndicator.className = 'indicator-dot auto';
        if (modeStatusText) modeStatusText.textContent = 'Mode Otomatis Aktif';
        if (modeDescription) {
            modeDescription.textContent = 'ESP32 mengatur perangkat otomatis berdasarkan sensor dan kondisi baterai.';
        }
        if (autoRadio) autoRadio.checked = true;
    }
}

function updateBatteryDisplay() {
    const { soc, voltage, estimatedRuntime } = currentData.battery;
    
    // Update battery level visual
    const batteryLevel = document.getElementById('battery-level');
    if (batteryLevel) {
        batteryLevel.style.width = `${soc}%`;
        
        // Update warna berdasarkan SOC
        if (soc <= 20) {
            batteryLevel.style.background = 'linear-gradient(90deg, #e74c3c, #c0392b)';
        } else if (soc <= 35) {
            batteryLevel.style.background = 'linear-gradient(90deg, #f39c12, #e67e22)';
        } else {
            batteryLevel.style.background = 'linear-gradient(90deg, #e74c3c, #f39c12, #27ae60)';
        }
    }
    
    // Update battery percentage
    const batteryPercentage = document.getElementById('battery-percentage');
    if (batteryPercentage) {
        batteryPercentage.textContent = `${soc.toFixed(1)}%`;
    }
    
    // Update battery info
    const batteryVoltage = document.getElementById('battery-voltage');
    if (batteryVoltage) {
        batteryVoltage.textContent = `${voltage.toFixed(1)}V`;
    }
    
    const batterySoc = document.getElementById('battery-soc');
    if (batterySoc) {
        batterySoc.textContent = `${soc.toFixed(1)}%`;
    }
    
    const runtimeElement = document.getElementById('estimated-runtime');
    if (runtimeElement) {
        if (estimatedRuntime === 0 || estimatedRuntime > 999) {
            runtimeElement.textContent = '∞';
        } else {
            runtimeElement.textContent = `${estimatedRuntime.toFixed(1)}h`;
        }
    }
    
    const statusElement = document.getElementById('status-read-sensors');
    if (statusElement) {
        statusElement.textContent = currentData.sensors.statusReadSensors;
    }
    
    // Update alerts berdasarkan kalkulasi baterai ESP32
    updateBatteryAlerts(soc);
}

function updateSensorDisplay() {
    const { temperatureKamar, temperatureRtamu, humidityKamar, humidityRtamu, lightLevel } = currentData.sensors;
    
    // Update panel sensor utama (pembacaan sensor ESP32)
    const tempKamarElement = document.getElementById('temperature-kamar');
    if (tempKamarElement) tempKamarElement.textContent = `${temperatureKamar.toFixed(1)}°C`;
    
    const tempRtamuElement = document.getElementById('temperature-rtamu');
    if (tempRtamuElement) tempRtamuElement.textContent = `${temperatureRtamu.toFixed(1)}°C`;
    
    const humidityKamarElement = document.getElementById('humidity-kamar');
    if (humidityKamarElement) humidityKamarElement.textContent = `${humidityKamar.toFixed(1)}%`;
    
    const humidityRtamuElement = document.getElementById('humidity-rtamu');
    if (humidityRtamuElement) humidityRtamuElement.textContent = `${humidityRtamu.toFixed(1)}%`;
    
    const lightElement = document.getElementById('light-level');
    if (lightElement) lightElement.textContent = `${lightLevel} lux`;
    
    // Update display suhu ruangan
    const tempKamarRoom = document.getElementById('temp-kamar');
    if (tempKamarRoom) tempKamarRoom.textContent = `${temperatureKamar.toFixed(1)}°C`;
    
    const tempRtamuRoom = document.getElementById('temp-rtamu');
    if (tempRtamuRoom) tempRtamuRoom.textContent = `${temperatureRtamu.toFixed(1)}°C`;
    
    // Update PIR sensor displays (pembacaan PIR ESP32)
    updatePIRDisplay();
    
    // Update total motion count
    const totalMotion = Object.values(currentData.pir).reduce((sum, val) => sum + val, 0);
    const totalMotionElement = document.getElementById('total-motion');
    if (totalMotionElement) {
        totalMotionElement.textContent = `${totalMotion}/5`;
    }
}

function updatePIRDisplay() {
    const pirElements = {
        'pir-teras': currentData.pir.teras,
        'pir-ruang-tamu': currentData.pir.ruangTamu,
        'pir-kamar': currentData.pir.kamar,
        'pir-wc': currentData.pir.wc,
        'pir-dapur': currentData.pir.dapur
    };
    
    for (let elementId in pirElements) {
        const element = document.getElementById(elementId);
        if (element) {
            element.textContent = pirElements[elementId] ? 'Terdeteksi' : 'Tidak Ada';
        }
    }
}

function updatePowerDisplay() {
    // Total system power, current, voltage (dari kalkulasi ESP32)
    const totalPowerElement = document.getElementById('total-power');
    if (totalPowerElement) {
        totalPowerElement.textContent = `${currentData.power.total.toFixed(1)}W`;
    }
    
    const totalCurrentElement = document.getElementById('total-current');
    if (totalCurrentElement) {
        totalCurrentElement.textContent = `${currentData.current.total.toFixed(2)}A`;
    }
    
    const systemVoltageElement = document.getElementById('system-voltage');
    if (systemVoltageElement) {
        systemVoltageElement.textContent = `${currentData.voltage.system.toFixed(1)}V`;
    }
    
    // USB power data (dari ESP32 INA226)
    const usbPowerElement = document.getElementById('power-usb');
    if (usbPowerElement) {
        usbPowerElement.textContent = `${currentData.power.usb.toFixed(1)}W`;
    }
    
    const usbCurrentElement = document.getElementById('current-usb');
    if (usbCurrentElement) {
        usbCurrentElement.textContent = `${currentData.current.usb.toFixed(2)}A`;
    }
    
    const usbVoltageElement = document.getElementById('voltage-usb');
    if (usbVoltageElement) {
        usbVoltageElement.textContent = `${currentData.voltage.usb.toFixed(1)}V`;
    }
}

function updateConnectionStatus() {
    const indicator = document.getElementById('connection-indicator');
    const statusText = document.getElementById('connection-status');
    
    if (indicator && statusText) {
        if (currentData.isConnected) {
            indicator.className = 'indicator online';
            statusText.textContent = 'ESP32 Connected';
        } else {
            indicator.className = 'indicator offline';
            statusText.textContent = 'ESP32 Disconnected';
        }
    }
}

function updateLastUpdateTime() {
    const lastUpdateElement = document.getElementById('last-update');
    if (lastUpdateElement) {
        const timeString = currentData.lastUpdate.toLocaleTimeString('id-ID');
        lastUpdateElement.textContent = timeString;
    }
}

function updateBatteryAlerts(soc) {
    const alertPanel = document.getElementById('alert-panel');
    const alertMessage = document.getElementById('alert-message');
    
    let alertText = '';
    let showAlert = false;
    
    if (soc <= 20) {
        alertText = '🚨 EMERGENCY: ESP32 melaporkan kapasitas baterai sangat rendah!';
        showAlert = true;
    } else if (soc <= 35) {
        alertText = '⚠️ Peringatan: ESP32 melaporkan kapasitas baterai rendah!';
        showAlert = true;
    } else if (!currentData.isConnected) {
        alertText = '📡 Peringatan: Koneksi ESP32 terputus!';
        showAlert = true;
    }
    
    if (alertPanel && alertMessage) {
        if (showAlert) {
            alertMessage.textContent = alertText;
            alertPanel.style.display = 'block';
        } else {
            alertPanel.style.display = 'none';
        }
    }
}

/* ====================================
   UI HELPER FUNCTIONS
   ==================================== */
function showLoading(show) {
    const loadingOverlay = document.getElementById('loading-overlay');
    if (loadingOverlay) {
        loadingOverlay.style.display = show ? 'flex' : 'none';
    }
    isUpdating = show;
}

function showAlert(message, type = 'info') {
    const alertDiv = document.createElement('div');
    alertDiv.id = 'alert';
    alertDiv.className = `alert-toast alert-${type}`;
    alertDiv.textContent = message;
    
    alertDiv.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        padding: 15px 20px;
        border-radius: 12px;
        color: white;
        font-weight: 700;
        z-index: 10000;
        max-width: 350px;
        box-shadow: 0 8px 25px rgba(0,0,0,0.3);
        animation: slideIn 0.4s ease-out;
        backdrop-filter: blur(10px);
    `;
    
    switch(type) {
        case 'success':
            alertDiv.style.background = 'linear-gradient(135deg, #27ae60, #2ecc71)';
            break;
        case 'error':
            alertDiv.style.background = 'linear-gradient(135deg, #e74c3c, #c0392b)';
            break;
        default:
            alertDiv.style.background = 'linear-gradient(135deg, #667eea, #764ba2)';
    }
    
    const style = document.createElement('style');
    style.textContent = `
        @keyframes slideIn {
            from { transform: translateX(100%); opacity: 0; }
            to { transform: translateX(0); opacity: 1; }
        }
        @keyframes slideOut {
            from { transform: translateX(0); opacity: 1; }
            to { transform: translateX(100%); opacity: 0; }
        }
    `;
    document.head.appendChild(style);
    
    document.body.appendChild(alertDiv);
    
    setTimeout(() => {
        alertDiv.style.animation = 'slideOut 0.4s ease-in';
        setTimeout(() => {
            if (alertDiv.parentNode) {
                alertDiv.parentNode.removeChild(alertDiv);
            }
        }, 400);
    }, 3000);
}

/* ====================================
   AUTO UPDATE SYSTEM - Real-time ESP32 Data
   ==================================== */
function startAutoUpdate() {
    if (updateInterval) {
        clearInterval(updateInterval);
    }
    
    updateInterval = setInterval(() => {
        if (!isUpdating) {
            fetchDataFromServer(); // Ambil data real-time ESP32
        }
    }, CONFIG.UPDATE_INTERVAL);
    
    console.log(`Auto-update dimulai - mengambil data ESP32 real-time setiap ${CONFIG.UPDATE_INTERVAL}ms`);
}

function stopAutoUpdate() {
    if (updateInterval) {
        clearInterval(updateInterval);
        updateInterval = null;
        console.log('Auto-update dihentikan');
    }
}

/* ====================================
   PAGE VISIBILITY MANAGEMENT
   ==================================== */
document.addEventListener('visibilitychange', function() {
    if (document.hidden) {
        console.log('Halaman tersembunyi - mengurangi frekuensi polling data ESP32');
        stopAutoUpdate();
    } else {
        console.log('Halaman terlihat - melanjutkan polling data ESP32');
        fetchDataFromServer();
        startAutoUpdate();
    }
});

window.addEventListener('focus', function() {
    fetchDataFromServer(); // Immediate ESP32 data fetch
});

window.addEventListener('beforeunload', function() {
    stopAutoUpdate();
});

/* ====================================
   ERROR HANDLING
   ==================================== */
window.addEventListener('error', function(e) {
    console.error('Frontend error:', e.error);
    showAlert('Terjadi kesalahan pada tampilan', 'error');
});

window.addEventListener('unhandledrejection', function(e) {
    console.error('ESP32 communication error:', e.reason);
    showAlert('Koneksi ke ESP32 bermasalah', 'error');
});

/* ====================================
   DEBUG INTERFACE - ESP32 Data Access
   ==================================== */
window.smartHome = {
    // Data access (read-only - semua dari ESP32)
    getCurrentData: () => currentData,
    
    // Control functions (kirim ke ESP32)
    toggleDevice,
    changeMode,
    
    // Update functions
    fetchDataFromServer,
    updateAllDisplays,
    
    // System functions
    startAutoUpdate,
    stopAutoUpdate,
    showAlert,
    
    // Debug helpers
    setConnectionStatus: (status) => {
        currentData.isConnected = status;
        updateConnectionStatus();
    },
    
    // ESP32 specific debugging
    simulateESP32Data: (data) => {
        currentData = { ...currentData, ...data };
        updateAllDisplays();
        console.log('ESP32 data disimulasikan:', data);
    }
};

// ESP32 Connection Monitor
function monitorESP32Connection() {
    const connectionCheckInterval = setInterval(() => {
        const timeSinceLastUpdate = Date.now() - currentData.lastUpdate.getTime();
        
        // Jika tidak ada update selama 10 detik, anggap koneksi terputus
        if (timeSinceLastUpdate > 10000) {
            currentData.isConnected = false;
            updateConnectionStatus();
            
            // Coba reconnect
            fetchDataFromServer();
        }
    }, 5000);
    
    return connectionCheckInterval;
}

// Start connection monitoring
const connectionMonitor = monitorESP32Connection();

console.log('Smart Home DC Frontend loaded - Layer display untuk data real-time ESP32');
console.log('Tidak ada kalkulasi yang dilakukan - semua data dari sensor dan kalkulasi ESP32');
console.log('Fungsi debug tersedia:', Object.keys(window.smartHome));