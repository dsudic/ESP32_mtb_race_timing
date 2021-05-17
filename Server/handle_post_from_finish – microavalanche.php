<?php

$conn = new mysqli("localhost", "pzi_dsudic19", "9cbzCbB6pWlM", "pzidb_dsudic19");
 
// Check connection
if($conn->connect_error){
    die("ERROR: Could not connect. " . $conn->connect_error);
}
 
// Prepare an insert statement
$sql = "UPDATE Timing SET Finish_stamp = CURTIME(3), Finish = ?, Time = ? WHERE Chip_ID = ?";

if($stmt = $conn->prepare($sql)){
    // Bind variables to the prepared statement as parameters
    $stmt->bind_param("iss", $finish, $time, $id);
    
    // Set parameters
    $finish = $_REQUEST['finish'];  
    $id = $_REQUEST['chip_id'];
    
    $sql1 = "SELECT Start FROM Timing WHERE Chip_ID = '04 A1 FF 1A38 66 80'";  // get time from start to calculate final time
    $result = $conn->query($sql1);
    $row = $result->fetch_assoc();

    if($row["Start"] != NULL){
        $time_ms = $finish - $row["Start"];
        
        $ms = $time_ms % 1000;
        $s = (int)($time_ms / 1000 % 60);
        $m = (int)($time_ms / 1000 / 60);
        $time = sprintf("%02d", $m) . ":" . sprintf("%02d", $s) . "." . $ms;
        }
    
        else $time = NULL;

    // Attempt to execute the prepared statement
    if($stmt->execute()){
        echo "Records inserted successfully.";
    } else{
        echo "ERROR: Could not execute query: $sql. " . $conn->connect_error;
    }
} else{
    echo "ERROR: Could not prepare query: $sql. " . $conn->connect_error;
}
$result->free();
 
// Close statement
$stmt->close();
 
// Close connection
$conn->close();

?>