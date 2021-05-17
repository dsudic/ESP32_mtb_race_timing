<?php

$conn = new mysqli("localhost", "pzi_dsudic19", "9cbzCbB6pWlM", "pzidb_dsudic19");
 
// Check connection
if($conn->connect_error){
    die("ERROR: Could not connect. " . $conn->connect_error);
}
 
// Prepare an insert statement
$sql = "INSERT INTO Timing (Start_No, Chip_ID, Name, Club) VALUES (?, ?, ?, ?)";
 
if($stmt = $conn->prepare($sql)){
    // Bind variables to the prepared statement as parameters
    $stmt->bind_param("isss", $numb, $id, $name, $club);
    
    // Set parameters
    $numb = $_REQUEST['start_no']; 
    $id = $_REQUEST['p1']. " " . $_REQUEST['p2']. " ". $_REQUEST['p3']. " ". $_REQUEST['p4']; 
    $name = $_REQUEST['name'];
    $club = $_REQUEST['club'];
    
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