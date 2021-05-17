<?php
$servername = "localhost";
$username = "pzi_dsudic19";
$password = "9cbzCbB6pWlM";
$dbname = "pzidb_dsudic19";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$sql = "SELECT Name, Start_stamp, Start FROM Timing";
$result = $conn->query($sql);

if ($result->num_rows > 0) {
    echo "<table><tr><th>Name</th>><th>Start_stamp</th>><th>Start</th></tr>";
    // output data of each row
    while($row = $result->fetch_assoc()) {
        echo "<tr><td>".$row["Name"]."</td><td>".$row["Start_stamp"]."</td><td>".$row["Start"]."</td></tr>";
    }
    echo "</table>";
} else {
    echo "0 results";
}
$conn->close();
?>