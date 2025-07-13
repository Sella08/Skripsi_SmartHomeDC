<?php
// getdata.php - FIXED VERSION untuk ESP32 Mode Control
include 'config.php';

// Set error reporting untuk debugging
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

// Set content type untuk JSON response
header('Content-Type: application/json');

// Add CORS headers if needed
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, GET, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if (isset($_POST['id']) && !empty($_POST['id'])) {
    $id = mysqli_real_escape_string($conn, $_POST['id']);
    
    // Log request untuk debugging
    error_log("ESP32 getdata request: ID=$id, Time=" . date('Y-m-d H:i:s'));
    
    try {
        // Query device control commands untuk ESP32
        $sql = "SELECT * FROM device_control WHERE id = ?";
        $stmt = $conn->prepare($sql);
        
        if (!$stmt) {
            throw new Exception("Prepare failed: " . $conn->error);
        }
        
        $stmt->bind_param("s", $id);
        
        if (!$stmt->execute()) {
            throw new Exception("Execute failed: " . $stmt->error);
        }
        
        $result = $stmt->get_result();
        
        if ($result && $result->num_rows > 0) {
            $row = $result->fetch_assoc();
            
            // PERBAIKAN: Response format yang konsisten untuk ESP32
            $response = [
                "status" => "success",
                "device_id" => $row["id"],
                "MODE" => strtoupper($row["mode"]),
                "L1" => strtoupper($row["l1"]),
                "L2" => strtoupper($row["l2"]),
                "L3" => strtoupper($row["l3"]),
                "L4" => strtoupper($row["l4"]),
                "L5" => strtoupper($row["l5"]),
                "K1" => strtoupper($row["k1"]),
                "K2" => strtoupper($row["k2"]),
                "timestamp" => $row["updated_at"] ?? date("Y-m-d H:i:s"),
                "debug_info" => [
                    "query_time" => date('Y-m-d H:i:s'),
                    "mode_from_db" => $row["mode"],
                    "all_devices" => [
                        "l1" => $row["l1"],
                        "l2" => $row["l2"],
                        "l3" => $row["l3"],
                        "l4" => $row["l4"],
                        "l5" => $row["l5"],
                        "k1" => $row["k1"],
                        "k2" => $row["k2"]
                    ]
                ]
            ];
            
            // Log untuk debugging ESP32 communication
            error_log("ESP32 Command Response: ID=$id, Mode=" . $row["mode"] . ", Commands sent successfully");
            
            echo json_encode($response, JSON_PRETTY_PRINT);
            
        } else {
            // PERBAIKAN: Jika device belum ada, buat default entry
            $default_sql = "INSERT IGNORE INTO device_control (id, mode, l1, l2, l3, l4, l5, k1, k2) 
                           VALUES (?, 'MANUAL', 'OFF', 'OFF', 'OFF', 'OFF', 'OFF', 'OFF', 'OFF')";
            
            $stmt_insert = $conn->prepare($default_sql);
            
            if ($stmt_insert) {
                $stmt_insert->bind_param("s", $id);
                
                if ($stmt_insert->execute()) {
                    $response = [
                        "status" => "success",
                        "message" => "Device baru dibuat dengan default settings",
                        "device_id" => $id,
                        "MODE" => "MANUAL",
                        "L1" => "OFF",
                        "L2" => "OFF", 
                        "L3" => "OFF",
                        "L4" => "OFF",
                        "L5" => "OFF",
                        "K1" => "OFF",
                        "K2" => "OFF",
                        "timestamp" => date("Y-m-d H:i:s"),
                        "debug_info" => [
                            "action" => "device_created",
                            "default_mode" => "MANUAL"
                        ]
                    ];
                    
                    error_log("ESP32 New Device Created: ID=$id with default settings");
                } else {
                    throw new Exception("Failed to create default device entry: " . $stmt_insert->error);
                }
                
                $stmt_insert->close();
            } else {
                throw new Exception("Failed to prepare insert statement: " . $conn->error);
            }
            
            echo json_encode($response, JSON_PRETTY_PRINT);
        }
        
        $stmt->close();
        
    } catch (Exception $e) {
        // Error response dengan detail untuk debugging
        $error_response = [
            "status" => "error",
            "message" => "Database error: " . $e->getMessage(),
            "device_id" => $id,
            "timestamp" => date("Y-m-d H:i:s"),
            "debug_info" => [
                "error_type" => "database_exception",
                "mysql_error" => $conn->error,
                "file" => __FILE__,
                "line" => __LINE__
            ]
        ];
        
        echo json_encode($error_response, JSON_PRETTY_PRINT);
        error_log("ESP32 getdata Error: " . $e->getMessage());
    }
    
} else {
    // PERBAIKAN: Error response jika tidak ada ID dengan informasi lengkap
    $error_response = [
        "status" => "error",
        "message" => "ID parameter diperlukan untuk ESP32",
        "required_format" => "POST dengan parameter 'id'",
        "example_usage" => [
            "method" => "POST",
            "content_type" => "application/x-www-form-urlencoded",
            "body" => "id=esp32_smart_home_01"
        ],
        "received_data" => [
            "post_keys" => array_keys($_POST),
            "post_count" => count($_POST),
            "get_keys" => array_keys($_GET),
            "request_method" => $_SERVER['REQUEST_METHOD'] ?? 'unknown'
        ],
        "timestamp" => date("Y-m-d H:i:s"),
        "debug_info" => [
            "error_type" => "missing_id_parameter",
            "suggestion" => "Ensure ESP32 sends POST data with 'id' parameter"
        ]
    ];
    
    echo json_encode($error_response, JSON_PRETTY_PRINT);
    error_log("ESP32 getdata Error: No ID parameter. POST data: " . print_r($_POST, true));
}

$conn->close();

// TAMBAHAN: Function untuk test manual (jika diakses via browser)
if ($_SERVER['REQUEST_METHOD'] === 'GET' && !isset($_POST['id'])) {
    // Tambahkan informasi untuk debugging manual
    echo "\n\n<!-- DEBUG INFO FOR MANUAL TESTING -->\n";
    echo "<!-- File: " . __FILE__ . " -->\n";
    echo "<!-- Time: " . date('Y-m-d H:i:s') . " -->\n";
    echo "<!-- Method: " . $_SERVER['REQUEST_METHOD'] . " -->\n";
    echo "<!-- Expected: POST with id parameter -->\n";
    echo "<!-- Example: curl -X POST -d 'id=esp32_smart_home_01' " . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'] . " -->\n";
}
?>