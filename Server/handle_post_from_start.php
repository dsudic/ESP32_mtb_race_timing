<?php

$conn = new mysqli("localhost", "pzi_dsudic19", "9cbzCbB6pWlM", "pzidb_dsudic19");
 
// Check connection
if($conn->connect_error){
    die("ERROR: Could not connect. " . $conn->connect_error);
}
 
// Prepare an insert statement
$sql = "UPDATE Timing SET Start_stamp = CURTIME(3), Start = ? WHERE Chip_ID = ?";
 
if($stmt = $conn->prepare($sql)){
    // Bind variables to the prepared statement as parameters
    $stmt->bind_param("is", $start, $id);
    
    // Set parameters
    $start = $_REQUEST['start'];  
    $id = $_REQUEST['chip_id'];  
    
    // Attempt to execute the prepared statement
    if($stmt->execute()){
        echo "Records inserted successfully.";
    } else{
        echo "ERROR: Could not execute query: $sql. " . $conn->connect_error;
    }
} else{
    echo "ERROR: Could not prepare query: $sql. " . $conn->connect_error;
}
 
// Close statement
$stmt->close();
 
// Close connection
$conn->close();

?>