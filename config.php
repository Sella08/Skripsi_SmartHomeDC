<?php
// config.php - Database configuration (FIXED untuk ESP32)
$servername = "127.0.0.1";
$username = "root";
$password = "";
$dbname = "smart_home_dc";

// Create connection dengan error handling
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

// Set charset untuk konsistensi data
$conn->set_charset("utf8");

// Set timezone sesuai Indonesia (optional)
$conn->query("SET time_zone = '+07:00'");
?>