<?php
// dashboard.php - ESP32 Data Integration (UPDATED)
include 'config.php';

// Handle POST requests from frontend
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $action = $_POST['action'] ?? '';
    
    if ($action === 'control_device') {
        $device = strtoupper($_POST['device'] ?? '');
        $state = $_POST['state'] ?? 'OFF';
        $deviceId = 'esp32_smart_home_01';
        
        // Update device control table untuk ESP32
        $sql = "UPDATE device_control SET $device = '$state' WHERE id = '$deviceId'";
        if ($conn->query($sql)) {
            echo json_encode(['status' => 'success', 'message' => 'Device command sent to ESP32']);
        } else {
            echo json_encode(['status' => 'error', 'message' => 'Failed to send command']);
        }
        exit;
    }
    
    if ($action === 'change_mode') {
        $mode = strtoupper($_POST['mode'] ?? 'MANUAL');
        $deviceId = 'esp32_smart_home_01';
        
        // Update mode untuk ESP32
        $sql = "UPDATE device_control SET mode = '$mode' WHERE id = '$deviceId'";
        if ($conn->query($sql)) {
            echo json_encode(['status' => 'success', 'message' => 'Mode changed on ESP32']);
        } else {
            echo json_encode(['status' => 'error', 'message' => 'Failed to change mode']);
        }
    
    }
}

// Get latest data from ESP32 (current_status table)
$deviceId = 'esp32_smart_home_01';
$sql = "SELECT * FROM current_status WHERE id = '$deviceId'";
$result = $conn->query($sql);
$data = null;

if ($result && $result->num_rows > 0) {
    $data = $result->fetch_assoc();
} else {
    // Default values jika belum ada data dari ESP32
    $data = [
        'mode' => 'MANUAL',
        'temperature_kamar' => 28.0,
        'temperature_rtamu' => 28.0,
        'humidity_kamar' => 65.0,
        'humidity_rtamu' => 65.0,
        'light_level' => 250,
        'battery_voltage' => 12.6,
        'battery_soc' => 75.0,
        'estimated_runtime' => 8.5,
        'status_read_sensors' => 'SUCCEED',
        'pir_teras' => 0,
        'pir_ruang_tamu' => 0,
        'pir_kamar' => 0,
        'pir_wc' => 0,
        'pir_dapur' => 0,
        'l1' => 'OFF', 'l2' => 'OFF', 'l3' => 'OFF', 'l4' => 'OFF', 'l5' => 'OFF',
        'k1' => 'OFF', 'k2' => 'OFF', 'usb' => 'ON',
        'power_l1' => 0, 'power_l2' => 0, 'power_l3' => 0, 'power_l4' => 0, 'power_l5' => 0,
        'power_k1' => 0, 'power_k2' => 0, 'power_usb' => 0, 'total_power' => 0,
        'current_l1' => 0, 'current_l2' => 0, 'current_l3' => 0, 'current_l4' => 0, 'current_l5' => 0,
        'current_k1' => 0, 'current_k2' => 0, 'current_usb' => 0, 'current_total' => 0,
        'voltage_l1' => 0, 'voltage_l2' => 0, 'voltage_l3' => 0, 'voltage_l4' => 0, 'voltage_l5' => 0,
        'voltage_k1' => 0, 'voltage_k2' => 0, 'voltage_usb' => 5, 'voltage_system' => 12
    ];
}
?>
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Home DC Control System</title>
    <link rel="stylesheet" href="smarthomecss.css">
</head>
<body>
    <div class="container">
        <!-- Header dengan status koneksi ESP32 -->
        <header>
            <h1>Smart Home DC Control System</h1>
            <div class="status-indicator">
                <span class="indicator <?= $data ? 'online' : 'offline' ?>" id="connection-indicator"></span>
                <span id="connection-status"><?= $data ? 'ESP32 Connected' : 'ESP32 Disconnected' ?></span>
                <span class="update-time">Last Update: <span id="last-update"><?= date('H:i:s') ?></span></span>
            </div>
        </header>

        <div class="main-dashboard">
            <!-- Mode Control Panel -->
            <div class="mode-control-panel">
                <h2>🎛️ Mode Kontrol Sistem</h2>
                <div class="mode-switcher">
                    <div class="mode-option">
                        <input type="radio" id="manual-mode" name="control-mode" value="MANUAL" <?= ($data['mode'] === 'MANUAL') ? 'checked' : '' ?>>
                        <label for="manual-mode" class="mode-label manual">
                            <span class="mode-icon">👤</span>
                            <span class="mode-text">
                                <strong>Manual</strong>
                                <small>Kontrol penuh oleh pengguna</small>
                            </span>
                        </label>
                    </div>
                    <div class="mode-option">
                        <input type="radio" id="auto-mode" name="control-mode" value="AUTO" <?= ($data['mode'] === 'AUTO') ? 'checked' : '' ?>>
                        <label for="auto-mode" class="mode-label auto">
                            <span class="mode-icon">🤖</span>
                            <span class="mode-text">
                                <strong>Otomatis</strong>
                                <small>ESP32 mengatur beban otomatis</small>
                            </span>
                        </label>
                    </div>
                </div>
                <div class="mode-status">
                    <div class="status-indicator-mode">
                        <span class="indicator-dot <?= ($data['mode'] === 'AUTO') ? 'auto' : '' ?>" id="mode-indicator"></span>
                        <span class="status-text" id="mode-status-text">Mode <?= $data['mode'] ?> Aktif</span>
                    </div>
                    <div class="mode-description" id="mode-description">
                        <?= ($data['mode'] === 'AUTO') ? 'ESP32 mengatur perangkat otomatis berdasarkan sensor dan kondisi baterai.' : 'Kontrol manual aktif. ESP32 menunggu perintah dari pengguna.' ?>
                    </div>
                </div>
            </div>

            <!-- House Layout -->
            <div class="house-layout">
                <h2>🏠 Layout Rumah DC</h2>
                <div class="house-container">
                    <!-- Dapur dan Ruang Makan -->
                    <div class="room dapur-ruang-makan" data-room="dapur">
                        <h3>Dapur & R. Makan</h3>
                        <div class="device lamp <?= ($data['l5'] === 'ON') ? 'active' : '' ?>" data-device="L5" data-power="30">
                            <div class="lamp-icon" onclick="toggleDevice('L5')">💡</div>
                            <span class="device-label">Lampu 5</span>
                            <div class="device-status <?= ($data['l5'] === 'ON') ? 'on' : '' ?>" id="status-L5"><?= $data['l5'] ?></div>
                            <div class="device-metrics">
                                <span id="power-L5-display"><?= number_format($data['power_l5'], 1) ?>W</span>
                                <span id="current-L5-display"><?= number_format($data['current_l5'], 2) ?>A</span>
                                <span id="voltage-L5-display"><?= number_format($data['voltage_l5'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="pir-status">
                            <span class="pir-icon">👤</span>
                            <span id="pir-dapur"><?= $data['pir_dapur'] ? 'Terdeteksi' : 'Tidak Ada' ?></span>
                        </div>
                    </div>

                    <!-- WC -->
                    <div class="room wc" data-room="wc">
                        <h3>WC</h3>
                        <div class="device lamp <?= ($data['l4'] === 'ON') ? 'active' : '' ?>" data-device="L4" data-power="10">
                            <div class="lamp-icon" onclick="toggleDevice('L4')">💡</div>
                            <span class="device-label">Lampu 4</span>
                            <div class="device-status <?= ($data['l4'] === 'ON') ? 'on' : '' ?>" id="status-L4"><?= $data['l4'] ?></div>
                            <div class="device-metrics">
                                <span id="power-L4-display"><?= number_format($data['power_l4'], 1) ?>W</span>
                                <span id="current-L4-display"><?= number_format($data['current_l4'], 2) ?>A</span>
                                <span id="voltage-L4-display"><?= number_format($data['voltage_l4'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="pir-status">
                            <span class="pir-icon">👤</span>
                            <span id="pir-wc"><?= $data['pir_wc'] ? 'Terdeteksi' : 'Tidak Ada' ?></span>
                        </div>
                    </div>

                    <!-- Ruang Tamu -->
                    <div class="room ruang-tamu" data-room="ruang-tamu">
                        <h3>Ruang Tamu</h3>
                        <div class="device lamp <?= ($data['l2'] === 'ON') ? 'active' : '' ?>" data-device="L2" data-power="30">
                            <div class="lamp-icon" onclick="toggleDevice('L2')">💡</div>
                            <span class="device-label">Lampu 2</span>
                            <div class="device-status <?= ($data['l2'] === 'ON') ? 'on' : '' ?>" id="status-L2"><?= $data['l2'] ?></div>
                            <div class="device-metrics">
                                <span id="power-L2-display"><?= number_format($data['power_l2'], 1) ?>W</span>
                                <span id="current-L2-display"><?= number_format($data['current_l2'], 2) ?>A</span>
                                <span id="voltage-L2-display"><?= number_format($data['voltage_l2'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="device fan <?= ($data['k2'] === 'ON') ? 'active' : '' ?>" data-device="K2" data-power="12">
                            <div class="fan-icon-advanced" onclick="toggleDevice('K2')"></div>
                            <span class="device-label">Kipas 2</span>
                            <div class="device-status <?= ($data['k2'] === 'ON') ? 'on' : '' ?>" id="status-K2"><?= $data['k2'] ?></div>
                            <div class="device-metrics">
                                <span id="power-K2-display"><?= number_format($data['power_k2'], 1) ?>W</span>
                                <span id="current-K2-display"><?= number_format($data['current_k2'], 2) ?>A</span>
                                <span id="voltage-K2-display"><?= number_format($data['voltage_k2'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="sensor-info">
                            <div class="pir-status">
                                <span class="pir-icon">👤</span>
                                <span id="pir-ruang-tamu"><?= $data['pir_ruang_tamu'] ? 'Terdeteksi' : 'Tidak Ada' ?></span>
                            </div>
                            <div class="temp-status">
                                <span class="temp-icon">🌡️</span>
                                <span id="temp-rtamu"><?= number_format($data['temperature_rtamu'], 1) ?>°C</span>
                            </div>
                        </div>
                    </div>

                    <!-- Kamar -->
                    <div class="room kamar" data-room="kamar">
                        <h3>Kamar</h3>
                        <div class="device lamp <?= ($data['l3'] === 'ON') ? 'active' : '' ?>" data-device="L3" data-power="20">
                            <div class="lamp-icon" onclick="toggleDevice('L3')">💡</div>
                            <span class="device-label">Lampu 3</span>
                            <div class="device-status <?= ($data['l3'] === 'ON') ? 'on' : '' ?>" id="status-L3"><?= $data['l3'] ?></div>
                            <div class="device-metrics">
                                <span id="power-L3-display"><?= number_format($data['power_l3'], 1) ?>W</span>
                                <span id="current-L3-display"><?= number_format($data['current_l3'], 2) ?>A</span>
                                <span id="voltage-L3-display"><?= number_format($data['voltage_l3'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="device fan <?= ($data['k1'] === 'ON') ? 'active' : '' ?>" data-device="K1" data-power="12">
                            <div class="fan-icon-advanced" onclick="toggleDevice('K1')"></div>
                            <span class="device-label">Kipas 1</span>
                            <div class="device-status <?= ($data['k1'] === 'ON') ? 'on' : '' ?>" id="status-K1"><?= $data['k1'] ?></div>
                            <div class="device-metrics">
                                <span id="power-K1-display"><?= number_format($data['power_k1'], 1) ?>W</span>
                                <span id="current-K1-display"><?= number_format($data['current_k1'], 2) ?>A</span>
                                <span id="voltage-K1-display"><?= number_format($data['voltage_k1'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="sensor-info">
                            <div class="pir-status">
                                <span class="pir-icon">👤</span>
                                <span id="pir-kamar"><?= $data['pir_kamar'] ? 'Terdeteksi' : 'Tidak Ada' ?></span>
                            </div>
                            <div class="temp-status">
                                <span class="temp-icon">🌡️</span>
                                <span id="temp-kamar"><?= number_format($data['temperature_kamar'], 1) ?>°C</span>
                            </div>
                        </div>
                    </div>

                    <!-- Teras -->
                    <div class="room teras" data-room="teras">
                        <h3>Teras</h3>
                        <div class="device lamp <?= ($data['l1'] === 'ON') ? 'active' : '' ?>" data-device="L1" data-power="10">
                            <div class="lamp-icon" onclick="toggleDevice('L1')">💡</div>
                            <span class="device-label">Lampu 1</span>
                            <div class="device-status <?= ($data['l1'] === 'ON') ? 'on' : '' ?>" id="status-L1"><?= $data['l1'] ?></div>
                            <div class="device-metrics">
                                <span id="power-L1-display"><?= number_format($data['power_l1'], 1) ?>W</span>
                                <span id="current-L1-display"><?= number_format($data['current_l1'], 2) ?>A</span>
                                <span id="voltage-L1-display"><?= number_format($data['voltage_l1'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="pir-status">
                            <span class="pir-icon">👤</span>
                            <span id="pir-teras"><?= $data['pir_teras'] ? 'Terdeteksi' : 'Tidak Ada' ?></span>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Control Panel -->
            <div class="control-panel">
                <!-- Battery Status Panel -->
                <div class="panel battery-panel">
                    <h2>🔋 Status Baterai</h2>
                    <div class="battery-visual">
                        <div class="battery-container">
                            <div class="battery-level" id="battery-level" style="width: <?= $data['battery_soc'] ?>%"></div>
                            <div class="battery-percentage" id="battery-percentage"><?= number_format($data['battery_soc'], 1) ?>%</div>
                        </div>
                    </div>
                    <div class="battery-info">
                        <div class="info-item">
                            <span class="label">Tegangan:</span>
                            <span class="value" id="battery-voltage"><?= number_format($data['battery_voltage'], 1) ?>V</span>
                        </div>
                        <div class="info-item">
                            <span class="label">SOC:</span>
                            <span class="value" id="battery-soc"><?= number_format($data['battery_soc'], 1) ?>%</span>
                        </div>
                        <div class="info-item">
                            <span class="label">Est. Runtime:</span>
                            <span class="value" id="estimated-runtime">
                                <?= ($data['estimated_runtime'] > 999 || $data['estimated_runtime'] == 0) ? '∞' : number_format($data['estimated_runtime'], 1) . 'h' ?>
                            </span>
                        </div>
                        <div class="info-item">
                            <span class="label">Status:</span>
                            <span class="value" id="status-read-sensors"><?= $data['status_read_sensors'] ?></span>
                        </div>
                    </div>
                </div>

                <!-- Total Power Summary Panel -->
                <div class="panel total-power-panel">
                    <h2>⚡ Total Konsumsi Sistem</h2>
                    <div class="total-power-grid">
                        <div class="total-metric">
                            <div class="metric-icon">⚡</div>
                            <div class="metric-value" id="total-power"><?= number_format($data['total_power'], 1) ?>W</div>
                            <div class="metric-label">Total Daya</div>
                        </div>
                        <div class="total-metric">
                            <div class="metric-icon">🔌</div>
                            <div class="metric-value" id="total-current"><?= number_format($data['current_total'], 2) ?>A</div>
                            <div class="metric-label">Total Arus</div>
                        </div>
                        <div class="total-metric">
                            <div class="metric-icon">🔋</div>
                            <div class="metric-value" id="system-voltage"><?= number_format($data['voltage_system'], 1) ?>V</div>
                            <div class="metric-label">Tegangan Sistem</div>
                        </div>
                    </div>
                </div>

                <!-- Power Monitoring Panel -->
                <div class="panel power-panel">
                    <h2>📊 Monitor Daya Per Perangkat</h2>
                    
                    <!-- USB Port Information -->
                    <div class="usb-port-info">
                        <h3>🔌 Port USB (Selalu Aktif)</h3>
                        <div class="usb-metrics">
                            <div class="usb-metric">
                                <div class="metric-label">Daya</div>
                                <div class="metric-value" id="power-usb"><?= number_format($data['power_usb'], 1) ?>W</div>
                            </div>
                            <div class="usb-metric">
                                <div class="metric-label">Arus</div>
                                <div class="metric-value" id="current-usb"><?= number_format($data['current_usb'], 2) ?>A</div>
                            </div>
                            <div class="usb-metric">
                                <div class="metric-label">Tegangan</div>
                                <div class="metric-value" id="voltage-usb"><?= number_format($data['voltage_usb'], 1) ?>V</div>
                            </div>
                        </div>
                    </div>
                    
                    <!-- Device List -->
                    <div class="device-list">
                        <div class="device-item <?= ($data['l1'] === 'ON') ? 'active' : '' ?>" data-device="L1">
                            <span class="name">Lampu Teras (L1)</span>
                            <div class="metrics">
                                <span class="power" id="power-L1"><?= number_format($data['power_l1'], 1) ?>W</span>
                                <span class="current" id="current-L1"><?= number_format($data['current_l1'], 2) ?>A</span>
                                <span class="voltage" id="voltage-L1"><?= number_format($data['voltage_l1'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="device-item <?= ($data['l2'] === 'ON') ? 'active' : '' ?>" data-device="L2">
                            <span class="name">Lampu R. Tamu (L2)</span>
                            <div class="metrics">
                                <span class="power" id="power-L2"><?= number_format($data['power_l2'], 1) ?>W</span>
                                <span class="current" id="current-L2"><?= number_format($data['current_l2'], 2) ?>A</span>
                                <span class="voltage" id="voltage-L2"><?= number_format($data['voltage_l2'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="device-item <?= ($data['l3'] === 'ON') ? 'active' : '' ?>" data-device="L3">
                            <span class="name">Lampu Kamar (L3)</span>
                            <div class="metrics">
                                <span class="power" id="power-L3"><?= number_format($data['power_l3'], 1) ?>W</span>
                                <span class="current" id="current-L3"><?= number_format($data['current_l3'], 2) ?>A</span>
                                <span class="voltage" id="voltage-L3"><?= number_format($data['voltage_l3'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="device-item <?= ($data['l4'] === 'ON') ? 'active' : '' ?>" data-device="L4">
                            <span class="name">Lampu WC (L4)</span>
                            <div class="metrics">
                                <span class="power" id="power-L4"><?= number_format($data['power_l4'], 1) ?>W</span>
                                <span class="current" id="current-L4"><?= number_format($data['current_l4'], 2) ?>A</span>
                                <span class="voltage" id="voltage-L4"><?= number_format($data['voltage_l4'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="device-item <?= ($data['l5'] === 'ON') ? 'active' : '' ?>" data-device="L5">
                            <span class="name">Lampu Dapur (L5)</span>
                            <div class="metrics">
                                <span class="power" id="power-L5"><?= number_format($data['power_l5'], 1) ?>W</span>
                                <span class="current" id="current-L5"><?= number_format($data['current_l5'], 2) ?>A</span>
                                <span class="voltage" id="voltage-L5"><?= number_format($data['voltage_l5'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="device-item <?= ($data['k1'] === 'ON') ? 'active' : '' ?>" data-device="K1">
                            <span class="name">Kipas Kamar (K1)</span>
                            <div class="metrics">
                                <span class="power" id="power-K1"><?= number_format($data['power_k1'], 1) ?>W</span>
                                <span class="current" id="current-K1"><?= number_format($data['current_k1'], 2) ?>A</span>
                                <span class="voltage" id="voltage-K1"><?= number_format($data['voltage_k1'], 1) ?>V</span>
                            </div>
                        </div>
                        <div class="device-item <?= ($data['k2'] === 'ON') ? 'active' : '' ?>" data-device="K2">
                            <span class="name">Kipas R. Tamu (K2)</span>
                            <div class="metrics">
                                <span class="power" id="power-K2"><?= number_format($data['power_k2'], 1) ?>W</span>
                                <span class="current" id="current-K2"><?= number_format($data['current_k2'], 2) ?>A</span>
                                <span class="voltage" id="voltage-K2"><?= number_format($data['voltage_k2'], 1) ?>V</span>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Environmental Sensors Panel -->
                <div class="panel sensor-panel">
                    <h2>🌡️ Sensor Lingkungan</h2>
                    <div class="sensor-grid">
                        <div class="sensor-item">
                            <span class="icon">🌡️</span>
                            <span class="label">Suhu Kamar</span>
                            <span class="value" id="temperature-kamar"><?= number_format($data['temperature_kamar'], 1) ?>°C</span>
                        </div>
                        <div class="sensor-item">
                            <span class="icon">🌡️</span>
                            <span class="label">Suhu R.Tamu</span>
                            <span class="value" id="temperature-rtamu"><?= number_format($data['temperature_rtamu'], 1) ?>°C</span>
                        </div>
                        <div class="sensor-item">
                            <span class="icon">💧</span>
                            <span class="label">Kelembaban Kamar</span>
                            <span class="value" id="humidity-kamar"><?= number_format($data['humidity_kamar'], 1) ?>%</span>
                        </div>
                        <div class="sensor-item">
                            <span class="icon">💧</span>
                            <span class="label">Kelembaban R.Tamu</span>
                            <span class="value" id="humidity-rtamu"><?= number_format($data['humidity_rtamu'], 1) ?>%</span>
                        </div>
                        <div class="sensor-item">
                            <span class="icon">💡</span>
                            <span class="label">Cahaya</span>
                            <span class="value" id="light-level"><?= $data['light_level'] ?> lux</span>
                        </div>
                        <div class="sensor-item">
                            <span class="icon">👤</span>
                            <span class="label">Total Gerakan</span>
                            <span class="value" id="total-motion"><?= ($data['pir_teras'] + $data['pir_ruang_tamu'] + $data['pir_kamar'] + $data['pir_wc'] + $data['pir_dapur']) ?>/5</span>
                        </div>
                    </div>
                </div>

                <!-- System Alerts Panel -->
                <?php if ($data['battery_soc'] <= 35 || !$data): ?>
                <div class="panel alert-panel" id="alert-panel">
                    <h2>⚠️ Peringatan Sistem</h2>
                    <div class="alert-message" id="alert-message">
                        <?php if ($data['battery_soc'] <= 20): ?>
                            🚨 EMERGENCY: ESP32 melaporkan kapasitas baterai sangat rendah!
                        <?php elseif ($data['battery_soc'] <= 35): ?>
                            ⚠️ Peringatan: ESP32 melaporkan kapasitas baterai rendah!
                        <?php elseif (!$data): ?>
                            📡 Peringatan: Koneksi ESP32 terputus!
                        <?php endif; ?>
                    </div>
                </div>
                <?php endif; ?>
            </div>
        </div>
    </div>

    <!-- Loading Indicator -->
    <div class="loading-overlay" id="loading-overlay" style="display: none;">
        <div class="loading-spinner"></div>
        <div class="loading-text">Mengirim perintah ke ESP32...</div>
    </div>

    <!-- Hidden data untuk JavaScript -->
    <script>
        // Data dari ESP32 untuk JavaScript
        window.ESP32_DATA = {
            mode: '<?= $data['mode'] ?>',
            devices: {
                L1: '<?= $data['l1'] ?>',
                L2: '<?= $data['l2'] ?>',
                L3: '<?= $data['l3'] ?>',
                L4: '<?= $data['l4'] ?>',
                L5: '<?= $data['l5'] ?>',
                K1: '<?= $data['k1'] ?>',
                K2: '<?= $data['k2'] ?>'
            },
            battery: {
                voltage: <?= $data['battery_voltage'] ?>,
                soc: <?= $data['battery_soc'] ?>,
                estimatedRuntime: <?= $data['estimated_runtime'] ?>
            },
            sensors: {
                temperatureKamar: <?= $data['temperature_kamar'] ?>,
                temperatureRtamu: <?= $data['temperature_rtamu'] ?>,
                humidityKamar: <?= $data['humidity_kamar'] ?>,
                humidityRtamu: <?= $data['humidity_rtamu'] ?>,
                lightLevel: <?= $data['light_level'] ?>,
                statusReadSensors: '<?= $data['status_read_sensors'] ?>'
            },
            pir: {
                teras: <?= $data['pir_teras'] ?>,
                ruangTamu: <?= $data['pir_ruang_tamu'] ?>,
                kamar: <?= $data['pir_kamar'] ?>,
                wc: <?= $data['pir_wc'] ?>,
                dapur: <?= $data['pir_dapur'] ?>
            },
            power: {
                L1: <?= $data['power_l1'] ?>,
                L2: <?= $data['power_l2'] ?>,
                L3: <?= $data['power_l3'] ?>,
                L4: <?= $data['power_l4'] ?>,
                L5: <?= $data['power_l5'] ?>,
                K1: <?= $data['power_k1'] ?>,
                K2: <?= $data['power_k2'] ?>,
                usb: <?= $data['power_usb'] ?>,
                total: <?= $data['total_power'] ?>
            },
            current: {
                L1: <?= $data['current_l1'] ?>,
                L2: <?= $data['current_l2'] ?>,
                L3: <?= $data['current_l3'] ?>,
                L4: <?= $data['current_l4'] ?>,
                L5: <?= $data['current_l5'] ?>,
                K1: <?= $data['current_k1'] ?>,
                K2: <?= $data['current_k2'] ?>,
                usb: <?= $data['current_usb'] ?>,
                total: <?= $data['current_total'] ?>
            },
            voltage: {
                L1: <?= $data['voltage_l1'] ?>,
                L2: <?= $data['voltage_l2'] ?>,
                L3: <?= $data['voltage_l3'] ?>,
                L4: <?= $data['voltage_l4'] ?>,
                L5: <?= $data['voltage_l5'] ?>,
                K1: <?= $data['voltage_k1'] ?>,
                K2: <?= $data['voltage_k2'] ?>,
                usb: <?= $data['voltage_usb'] ?>,
                system: <?= $data['voltage_system'] ?>
            },
            lastUpdate: new Date(),
            isConnected: <?= $data ? 'true' : 'false' ?>
        };
    </script>

    <!-- JavaScript Import -->
    <script src="smarthomejs.js"></script>
</body>
</html>