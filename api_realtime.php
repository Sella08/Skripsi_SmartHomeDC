<?php
// api_realtime.php - OPTIMIZED VERSION
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Cache-Control: no-cache, must-revalidate');
header('Expires: Mon, 26 Jul 1997 05:00:00 GMT');

include 'config.php';

$device_id = 'esp32_smart_home_01';

try {
    // Query dengan LEFT JOIN untuk memastikan device_control data juga diambil
    $sql = "SELECT 
                cs.*,
                dc.mode as control_mode,
                dc.updated_at as last_command_time
            FROM current_status cs 
            LEFT JOIN device_control dc ON cs.id = dc.id 
            WHERE cs.id = ? 
            ORDER BY cs.timestamp DESC 
            LIMIT 1";
    
    $stmt = $conn->prepare($sql);
    $stmt->bind_param("s", $device_id);
    $stmt->execute();
    $result = $stmt->get_result();

    if ($result->num_rows > 0) {
        $data = $result->fetch_assoc();
        
        // Cek koneksi ESP32 berdasarkan timestamp terakhir
        $last_update_time = strtotime($data['timestamp']);
        $current_time = time();
        $time_diff = $current_time - $last_update_time;
        $is_connected = $time_diff <= 15; // ESP32 dianggap terputus jika > 15 detik
        
        // Format response sesuai dengan struktur JavaScript yang diperbaiki
        $response = [
            'status' => 'success',
            'timestamp' => $data['timestamp'],
            'isConnected' => $is_connected,
            'connectionDelay' => $time_diff,
            'mode' => $data['control_mode'] ?? $data['mode'], // Prioritas dari device_control
            
            'devices' => [
                'L1' => $data['l1'],
                'L2' => $data['l2'],
                'L3' => $data['l3'],
                'L4' => $data['l4'],
                'L5' => $data['l5'],
                'K1' => $data['k1'],
                'K2' => $data['k2']
            ],
            
            'battery' => [
                'voltage' => floatval($data['battery_voltage']),
                'soc' => floatval($data['battery_soc']),
                'estimatedRuntime' => floatval($data['estimated_runtime'])
            ],
            
            'sensors' => [
                'temperatureKamar' => floatval($data['temperature_kamar']),
                'temperatureRtamu' => floatval($data['temperature_rtamu']),
                'humidityKamar' => floatval($data['humidity_kamar']),
                'humidityRtamu' => floatval($data['humidity_rtamu']),
                'lightLevel' => intval($data['light_level']),
                'statusReadSensors' => $data['status_read_sensors']
            ],
            
            'pir' => [
                'teras' => intval($data['pir_teras']),
                'ruangTamu' => intval($data['pir_ruang_tamu']),
                'kamar' => intval($data['pir_kamar']),
                'wc' => intval($data['pir_wc']),
                'dapur' => intval($data['pir_dapur'])
            ],
            
            'power' => [
                'L1' => floatval($data['power_l1']),
                'L2' => floatval($data['power_l2']),
                'L3' => floatval($data['power_l3']),
                'L4' => floatval($data['power_l4']),
                'L5' => floatval($data['power_l5']),
                'K1' => floatval($data['power_k1']),
                'K2' => floatval($data['power_k2']),
                'usb' => floatval($data['power_usb']),
                'total' => floatval($data['total_power'])
            ],
            
            'current' => [
                'L1' => floatval($data['current_l1']),
                'L2' => floatval($data['current_l2']),
                'L3' => floatval($data['current_l3']),
                'L4' => floatval($data['current_l4']),
                'L5' => floatval($data['current_l5']),
                'K1' => floatval($data['current_k1']),
                'K2' => floatval($data['current_k2']),
                'usb' => floatval($data['current_usb']),
                'total' => floatval($data['current_total'])
            ],
            
            'voltage' => [
                'L1' => floatval($data['voltage_l1']),
                'L2' => floatval($data['voltage_l2']),
                'L3' => floatval($data['voltage_l3']),
                'L4' => floatval($data['voltage_l4']),
                'L5' => floatval($data['voltage_l5']),
                'K1' => floatval($data['voltage_k1']),
                'K2' => floatval($data['voltage_k2']),
                'usb' => floatval($data['voltage_usb']),
                'system' => floatval($data['voltage_system'])
            ],
            
            // TAMBAHAN: Auto mode calculation untuk frontend
            'autoModeData' => [
                'recommendedDevices' => calculateRecommendedDevices($data['battery_soc']),
                'batteryStatus' => getBatteryStatus($data['battery_soc']),
                'energyEfficiencyScore' => calculateEfficiencyScore($data),
                'lastCommandTime' => $data['last_command_time']
            ],
            
            'debug' => [
                'query_time' => microtime(true),
                'device_id' => $device_id,
                'last_update' => $data['timestamp'],
                'data_age_seconds' => $time_diff,
                'sql_executed' => 'current_status + device_control JOIN'
            ]
        ];
        
        // Log successful API call dengan detail koneksi
        error_log("API Real-time: Data berhasil dikirim untuk device $device_id, koneksi=" . ($is_connected ? 'ONLINE' : 'OFFLINE') . ", delay={$time_diff}s");
        
        echo json_encode($response);
        
    } else {
        // Tidak ada data di database
        $error_response = [
            'status' => 'error',
            'message' => 'No data found in current_status table',
            'isConnected' => false,
            'timestamp' => date('Y-m-d H:i:s'),
            'debug' => [
                'device_id' => $device_id,
                'table_checked' => 'current_status + device_control',
                'suggestion' => 'Check if ESP32 has sent data to updatedata.php'
            ]
        ];
        
        error_log("API Real-time ERROR: No data found for device $device_id");
        echo json_encode($error_response);
    }

} catch (Exception $e) {
    // Database error
    $error_response = [
        'status' => 'error',
        'message' => 'Database error: ' . $e->getMessage(),
        'isConnected' => false,
        'timestamp' => date('Y-m-d H:i:s'),
        'debug' => [
            'error_type' => 'database_exception',
            'device_id' => $device_id,
            'sql_error' => $e->getMessage()
        ]
    ];
    
    error_log("API Real-time EXCEPTION: " . $e->getMessage());
    echo json_encode($error_response);
}

// TAMBAHAN: Helper functions sesuai proposal research
function calculateRecommendedDevices($battery_soc) {
    if ($battery_soc >= 90) {
        return ['L1', 'L2', 'L3', 'L4', 'L5', 'K1', 'K2'];
    } elseif ($battery_soc >= 80) {
        return ['L1', 'L2', 'L3', 'L4', 'L5', 'K1'];
    } elseif ($battery_soc >= 70) {
        return ['L1', 'L2', 'L3', 'L4', 'L5'];
    } elseif ($battery_soc >= 60) {
        return ['L1', 'L2', 'L3', 'L4'];
    } elseif ($battery_soc >= 50) {
        return ['L1', 'L2', 'L3'];
    } elseif ($battery_soc > 35) {
        return ['L1', 'L2'];
    } else {
        return []; // Semua beban mati
    }
}

function getBatteryStatus($battery_soc) {
    if ($battery_soc <= 20) {
        return 'EMERGENCY';
    } elseif ($battery_soc <= 35) {
        return 'CRITICAL';
    } elseif ($battery_soc <= 50) {
        return 'LOW';
    } elseif ($battery_soc <= 70) {
        return 'MEDIUM';
    } elseif ($battery_soc <= 90) {
        return 'GOOD';
    } else {
        return 'EXCELLENT';
    }
}

function calculateEfficiencyScore($data) {
    $total_power = floatval($data['total_power']);
    $battery_soc = floatval($data['battery_soc']);
    
    // Skor efisiensi berdasarkan rasio daya vs kapasitas baterai
    if ($total_power == 0) return 100;
    
    $efficiency = ($battery_soc / 100) * (100 / max(1, $total_power / 10));
    return min(100, max(0, $efficiency));
}

$conn->close();
?>