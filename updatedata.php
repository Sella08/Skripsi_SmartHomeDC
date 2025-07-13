<?php
// updatedata.php - FIXED VERSION dengan error handling lengkap
include 'config.php';

// Set error reporting untuk debugging
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

// Set content type
header('Content-Type: application/json');

if (isset($_POST['id']) && !empty($_POST['id'])) {
    $id = mysqli_real_escape_string($conn, $_POST['id']);
    $mode = mysqli_real_escape_string($conn, $_POST['mode'] ?? 'MANUAL');
    
    // Sensor data dengan validasi
    $temperature_kamar = floatval($_POST['temperature_kamar'] ?? 0);
    $temperature_rtamu = floatval($_POST['temperature_rtamu'] ?? 0);
    $humidity_kamar = floatval($_POST['humidity_kamar'] ?? 0);
    $humidity_rtamu = floatval($_POST['humidity_rtamu'] ?? 0);
    $light_level = intval($_POST['light_level'] ?? 0);
    $battery_voltage = floatval($_POST['battery_voltage'] ?? 12.6);
    $battery_soc = floatval($_POST['battery_soc'] ?? 75);
    $estimated_runtime = floatval($_POST['estimated_runtime'] ?? 0);
    $status_read_sensors = mysqli_real_escape_string($conn, $_POST['status_read_sensors'] ?? 'UNKNOWN');
    
    // PIR sensors
    $pir_teras = intval($_POST['pir_teras'] ?? 0);
    $pir_ruang_tamu = intval($_POST['pir_ruang_tamu'] ?? 0);
    $pir_kamar = intval($_POST['pir_kamar'] ?? 0);
    $pir_wc = intval($_POST['pir_wc'] ?? 0);
    $pir_dapur = intval($_POST['pir_dapur'] ?? 0);
    
    // Device states dengan validasi
    $l1 = in_array($_POST['l1'] ?? 'OFF', ['ON', 'OFF']) ? $_POST['l1'] : 'OFF';
    $l2 = in_array($_POST['l2'] ?? 'OFF', ['ON', 'OFF']) ? $_POST['l2'] : 'OFF';
    $l3 = in_array($_POST['l3'] ?? 'OFF', ['ON', 'OFF']) ? $_POST['l3'] : 'OFF';
    $l4 = in_array($_POST['l4'] ?? 'OFF', ['ON', 'OFF']) ? $_POST['l4'] : 'OFF';
    $l5 = in_array($_POST['l5'] ?? 'OFF', ['ON', 'OFF']) ? $_POST['l5'] : 'OFF';
    $k1 = in_array($_POST['k1'] ?? 'OFF', ['ON', 'OFF']) ? $_POST['k1'] : 'OFF';
    $k2 = in_array($_POST['k2'] ?? 'OFF', ['ON', 'OFF']) ? $_POST['k2'] : 'OFF';
    $usb = in_array($_POST['usb'] ?? 'ON', ['ON', 'OFF']) ? $_POST['usb'] : 'ON';
    
    // Power data
    $power_l1 = floatval($_POST['power_l1'] ?? 0);
    $power_l2 = floatval($_POST['power_l2'] ?? 0);
    $power_l3 = floatval($_POST['power_l3'] ?? 0);
    $power_l4 = floatval($_POST['power_l4'] ?? 0);
    $power_l5 = floatval($_POST['power_l5'] ?? 0);
    $power_k1 = floatval($_POST['power_k1'] ?? 0);
    $power_k2 = floatval($_POST['power_k2'] ?? 0);
    $power_usb = floatval($_POST['power_usb'] ?? 0);
    $total_power = floatval($_POST['total_power'] ?? 0);
    
    // Current data
    $current_l1 = floatval($_POST['current_l1'] ?? 0);
    $current_l2 = floatval($_POST['current_l2'] ?? 0);
    $current_l3 = floatval($_POST['current_l3'] ?? 0);
    $current_l4 = floatval($_POST['current_l4'] ?? 0);
    $current_l5 = floatval($_POST['current_l5'] ?? 0);
    $current_k1 = floatval($_POST['current_k1'] ?? 0);
    $current_k2 = floatval($_POST['current_k2'] ?? 0);
    $current_usb = floatval($_POST['current_usb'] ?? 0);
    $current_total = floatval($_POST['current_total'] ?? 0);
    
    // Voltage data
    $voltage_l1 = floatval($_POST['voltage_l1'] ?? 0);
    $voltage_l2 = floatval($_POST['voltage_l2'] ?? 0);
    $voltage_l3 = floatval($_POST['voltage_l3'] ?? 0);
    $voltage_l4 = floatval($_POST['voltage_l4'] ?? 0);
    $voltage_l5 = floatval($_POST['voltage_l5'] ?? 0);
    $voltage_k1 = floatval($_POST['voltage_k1'] ?? 0);
    $voltage_k2 = floatval($_POST['voltage_k2'] ?? 0);
    $voltage_usb = floatval($_POST['voltage_usb'] ?? 5);
    $voltage_system = floatval($_POST['voltage_system'] ?? 12);
    
    $timestamp = date("Y-m-d H:i:s");
    
    // Validasi range data
    if ($temperature_kamar < -50 || $temperature_kamar > 100) $temperature_kamar = 0;
    if ($temperature_rtamu < -50 || $temperature_rtamu > 100) $temperature_rtamu = 0;
    if ($humidity_kamar < 0 || $humidity_kamar > 100) $humidity_kamar = 0;
    if ($humidity_rtamu < 0 || $humidity_rtamu > 100) $humidity_rtamu = 0;
    if ($light_level < 0) $light_level = 0;
    if ($battery_voltage < 0 || $battery_voltage > 15) $battery_voltage = 12.6;
    if ($battery_soc < 0 || $battery_soc > 100) $battery_soc = 75;
    
    try {
        // Start transaction
        $conn->autocommit(FALSE);
        
        // Insert ke sensor_data table (history)
        $sql_sensor = "INSERT INTO sensor_data (
            id, mode,
            temperature_kamar, temperature_rtamu, humidity_kamar, humidity_rtamu, 
            light_level, battery_voltage, battery_soc, estimated_runtime, status_read_sensors,
            pir_teras, pir_ruang_tamu, pir_kamar, pir_wc, pir_dapur,
            l1, l2, l3, l4, l5, k1, k2, usb,
            power_l1, power_l2, power_l3, power_l4, power_l5, power_k1, power_k2, power_usb, total_power,
            current_l1, current_l2, current_l3, current_l4, current_l5, current_k1, current_k2, current_usb, current_total,
            voltage_l1, voltage_l2, voltage_l3, voltage_l4, voltage_l5, voltage_k1, voltage_k2, voltage_usb, voltage_system,
            timestamp
        ) VALUES (
            '$id', '$mode',
            $temperature_kamar, $temperature_rtamu, $humidity_kamar, $humidity_rtamu,
            $light_level, $battery_voltage, $battery_soc, $estimated_runtime, '$status_read_sensors',
            $pir_teras, $pir_ruang_tamu, $pir_kamar, $pir_wc, $pir_dapur,
            '$l1', '$l2', '$l3', '$l4', '$l5', '$k1', '$k2', '$usb',
            $power_l1, $power_l2, $power_l3, $power_l4, $power_l5, $power_k1, $power_k2, $power_usb, $total_power,
            $current_l1, $current_l2, $current_l3, $current_l4, $current_l5, $current_k1, $current_k2, $current_usb, $current_total,
            $voltage_l1, $voltage_l2, $voltage_l3, $voltage_l4, $voltage_l5, $voltage_k1, $voltage_k2, $voltage_usb, $voltage_system,
            '$timestamp'
        )";
        
        $result1 = $conn->query($sql_sensor);
        
        if (!$result1) {
            throw new Exception("Error inserting sensor data: " . $conn->error);
        }
        
        // Update current_status table (current state)
        $sql_status = "REPLACE INTO current_status (
            id, mode,
            temperature_kamar, temperature_rtamu, humidity_kamar, humidity_rtamu, 
            light_level, battery_voltage, battery_soc, estimated_runtime, status_read_sensors,
            pir_teras, pir_ruang_tamu, pir_kamar, pir_wc, pir_dapur,
            l1, l2, l3, l4, l5, k1, k2, usb,
            power_l1, power_l2, power_l3, power_l4, power_l5, power_k1, power_k2, power_usb, total_power,
            current_l1, current_l2, current_l3, current_l4, current_l5, current_k1, current_k2, current_usb, current_total,
            voltage_l1, voltage_l2, voltage_l3, voltage_l4, voltage_l5, voltage_k1, voltage_k2, voltage_usb, voltage_system,
            timestamp
        ) VALUES (
            '$id', '$mode',
            $temperature_kamar, $temperature_rtamu, $humidity_kamar, $humidity_rtamu,
            $light_level, $battery_voltage, $battery_soc, $estimated_runtime, '$status_read_sensors',
            $pir_teras, $pir_ruang_tamu, $pir_kamar, $pir_wc, $pir_dapur,
            '$l1', '$l2', '$l3', '$l4', '$l5', '$k1', '$k2', '$usb',
            $power_l1, $power_l2, $power_l3, $power_l4, $power_l5, $power_k1, $power_k2, $power_usb, $total_power,
            $current_l1, $current_l2, $current_l3, $current_l4, $current_l5, $current_k1, $current_k2, $current_usb, $current_total,
            $voltage_l1, $voltage_l2, $voltage_l3, $voltage_l4, $voltage_l5, $voltage_k1, $voltage_k2, $voltage_usb, $voltage_system,
            '$timestamp'
        )";
        
        $result2 = $conn->query($sql_status);
        
        if (!$result2) {
            throw new Exception("Error updating current status: " . $conn->error);
        }
        
        // Commit transaction
        $conn->commit();
        
        // Success response
        $response = [
            'status' => 'success',
            'message' => 'Data berhasil disimpan dari ESP32',
            'timestamp' => $timestamp,
            'records_affected' => [
                'sensor_data' => $conn->affected_rows,
                'current_status' => 'updated'
            ],
            'data_received' => [
                'battery_soc' => $battery_soc,
                'total_power' => $total_power,
                'mode' => $mode
            ]
        ];
        
        echo json_encode($response);
        
        // Log untuk debugging
        error_log("ESP32 Data Success: ID=$id, Mode=$mode, Battery=$battery_soc%, Power=${total_power}W");
        
    } catch (Exception $e) {
        // Rollback transaction
        $conn->rollback();
        
        $error_response = [
            'status' => 'error',
            'message' => 'Database error: ' . $e->getMessage(),
            'timestamp' => $timestamp,
            'sql_error' => $conn->error,
            'debug_info' => [
                'id' => $id,
                'mode' => $mode,
                'battery_soc' => $battery_soc
            ]
        ];
        
        echo json_encode($error_response);
        
        // Log error
        error_log("ESP32 Database Error: " . $e->getMessage());
    }
    
} else {
    // Error jika tidak ada ID
    $error_response = [
        'status' => 'error',
        'message' => 'ID parameter tidak ditemukan atau kosong',
        'required_fields' => ['id'],
        'received_post' => array_keys($_POST),
        'timestamp' => date("Y-m-d H:i:s")
    ];
    
    echo json_encode($error_response);
    error_log("ESP32 Update Error: No ID provided. POST data: " . print_r($_POST, true));
}

$conn->close();
?>