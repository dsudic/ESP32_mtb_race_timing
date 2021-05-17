<?php

$conn = new mysqli("localhost", "pzi_dsudic19", "9cbzCbB6pWlM", "pzidb_dsudic19");
 
// Check connection
if($conn->connect_error){
    die("ERROR: Could not connect. " . $conn->connect_error);
}

// delete times for every rider
if(isset($_REQUEST['delete_times'])){
    $sql1 = "UPDATE Timing SET Start_stamp = NULL, Start = NULL, Finish_stamp = NULL, Finish = NULL, Time = NULL WHERE Chip_ID != 'Fastest'";
    if ($conn->query($sql1) == TRUE)
    echo "Times deleted";
    else
    echo "ERROR: Could not execute query: $sql1." . $conn->connect_error;
}
// delete all riders (except Chip_ID)
else if(isset($_REQUEST['delete_riders'])){
    $sql2 = "UPDATE Timing SET Name = '', Club = '', Start_stamp = NULL, Start = NULL, Finish_stamp = NULL, Finish = NULL, Time = NULL WHERE Chip_ID != 'Fastest'";
    if ($conn->query($sql2) == TRUE)
    echo "Riders deleted";
    else
    echo "ERROR: Could not execute query: $sql2." . $conn->connect_error;
}

// delete fastest rider's data
else if(isset($_REQUEST['delete_fastest'])){
    $sql3 = "UPDATE Timing SET Name = '', Time = NULL WHERE Chip_ID = 'Fastest'";
    if ($conn->query($sql3) == TRUE)
    echo "Fastest deleted";
    else
    echo "ERROR: Could not execute query: $sql3." . $conn->connect_error;
}

else{
// Prepare an insert statement
//$sql = "INSERT INTO Timing (Start_No, Chip_ID, Name, Club) VALUES (?, ?, ?, ?)";
$sql = "UPDATE Timing SET Name = ?, Club = ? WHERE Start_No = ?";
 
if($stmt = $conn->prepare($sql)){
    // Bind variables to the prepared statement as parameters
    //$stmt->bind_param("isss", $numb, $id, $name, $club);
    $stmt->bind_param("ssi", $name, $club, $numb);
    
    // Set parameters
    $numb = $_REQUEST['start_no']; 
    // $id = $_REQUEST['p1']. " " . $_REQUEST['p2']. " ". $_REQUEST['p3']. " ". $_REQUEST['p4'].  $_REQUEST['p5']. " ".  $_REQUEST['p6']. " ".  $_REQUEST['p7'] ; 
    $name = $_REQUEST['name'];
    $club = $_REQUEST['club'];
    
    $sql1 = "SELECT * FROM Timing WHERE Chip_ID = '$id'";
    $result = $conn->query($sql1);
    
    if($result->num_rows > 0){
        //$row = $result->fetch_assoc();
        $sql2 = "UPDATE Timing SET Start_No = '$numb' WHERE Chip_ID = '$id'";
        $conn->query($sql2);
        echo "Start number updated!";
        $result->free();
    }
    else{
        // Attempt to execute the prepared statement
    if($stmt->execute()){
        //echo  '<script type = "text/javascript"> alert("Records inserted successfully."); </script>';
        echo "Records inserted successfully.";
        
    } else{
       // echo  '<script type = "text/javascript"> alert("ERROR: Could not execute query: '$sql.' '$conn->connect_error'"); </script>' ;
       echo "ERROR: Could not execute query: $sql." . $conn->connect_error;
    }
    }

} else{
    echo "ERROR: Could not prepare query: $sql. " . $conn->connect_error;
}
}
// Close statement
$stmt->close();

// Close connection
$conn->close();

?>