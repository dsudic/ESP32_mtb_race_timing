<?php

$conn = new mysqli("localhost", "pzi_dsudic19", "9cbzCbB6pWlM", "pzidb_dsudic19");

// Check connection
if ($conn->connect_error) {
    die("ERROR: Could not connect. " . $conn->connect_error);
}

// Prepare an insert statement
$sql = "UPDATE Timing SET Finish_stamp = CURTIME(3), Finish = ?, Time = ? WHERE Chip_ID = ?";

if ($stmt = $conn->prepare($sql)) {
    // Bind variables to the prepared statement as parameters
    $stmt->bind_param("iss", $finish, $time, $id);

    // Set parameters
    $finish = $_REQUEST['finish'];
    $id = $_REQUEST['chip_id'];

    $sql1 = "SELECT Start, Name FROM Timing WHERE Chip_ID = '$id'";  // get time from start to calculate final time
    $result = $conn->query($sql1);
    $row = $result->fetch_assoc();

    if ($row["Start"] != NULL && $row["Start"] < $finish) { // dont calculate result time if there is no Start time or Start time is greater tham finish time (would result in negative time)
        $time_ms = $finish - $row["Start"];

        $ms = $time_ms % 1000;
        $s = (int)($time_ms / 1000 % 60);
        $m = (int)($time_ms / 1000 / 60);
        $time = sprintf("%02d", $m) . ":" . sprintf("%02d", $s) . "." . $ms;
    } else $time = NULL;

    // update fastest time
    $sql2 = "UPDATE Timing SET Time = ?, Name = ? WHERE Chip_ID = 'Fastest'";
    $stmt1 = $conn->prepare($sql2);
    $stmt1->bind_param("ss", $f_time, $f_name);

    // get current fastest time
    $sql3 = "SELECT Time, Name FROM Timing Where Chip_ID = 'Fastest'";
    $res = $conn->query($sql3);
    $fastest_row = $res->fetch_assoc();

    if ($fastest_row["Time"] == NULL || $time < $fastest_row["Time"] && $time) { // if there is no any fastest time already written or new received time is faster than previous Fastest time, put that new time and riders name in DB (Fastest row) only if $time is calculated, that is, it is not NULL
        $f_time = $time;
        $f_name = $row["Name"];
     
    } else {                                 // else, keep everything as is
        $f_time = $fastest_row["Time"];
        $f_name = $fastest_row["Name"];
    }
    $stmt1->execute();

    // Attempt to execute the prepared statement
    if ($stmt->execute()) {
        echo "Records inserted successfully.";
    } else {
        echo "ERROR: Could not execute query: $sql. " . $conn->connect_error;
    }
} else {
    echo "ERROR: Could not prepare query: $sql. " . $conn->connect_error;
}
$result->free();
$res->free();

// Close statement
$stmt->close();
$stmt1->close();

// Close connection
$conn->close();
