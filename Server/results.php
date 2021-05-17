<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale = 0.55">
  <title>Results</title>

  <link rel="stylesheet" href="./styles/style.css">
</head>

<body>

  <h1>Live results</h1>

  <?php

  $servername = "localhost";
  $dbname = "pzidb_dsudic19";
  $username = "pzi_dsudic19";
  $password = "9cbzCbB6pWlM";

  $counter = 1;  // counter for Rank column

  $conn = new mysqli($servername, $username, $password, $dbname);
  // Check connection
  if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
  }

  // $sql = "SELECT Start_No, Chip_ID, Name, Club, Start_stamp, Start, Finish_stamp, Finish, Time  FROM Timing ORDER BY Start, Name ASC";  // get all columns from table Timing in database and sort it by column Time in ascending order
  $sql = "SELECT Start_No, Name, Club, Time  FROM Timing WHERE Name != '' AND Chip_ID != 'Fastest' ORDER BY Time is NULL, Time, Start_No ASC";

  $sql1 = "SELECT Name, Time FROM Timing WHERE Chip_ID = 'Fastest'";
  $res = $conn->query($sql1);
  $fastest = $res->fetch_assoc();

  echo '
<table cellspacing="5" cellpadding="5">
<caption id = "fastest"> <b>Fastest run: </b>' . $fastest["Name"] . ' - <strong><i>' . $fastest["Time"] . '</i></strong> </caption>
      <tr> 
        <th>Rank</th> 
        <th>Start_No</th>
        <!-- <th>Chip_ID</th> -->
        <th>Name</th>
        <th>Club</th>
        <!-- <th>Start_stamp</th> -->
        <!-- <th>Start</th> -->
        <!-- <th>Finish_stamp</th> -->
        <!-- <th>Finish</th> -->
        <th>Time</th> 
      </tr>';

  if ($result = $conn->query($sql)) {
    while ($row = $result->fetch_assoc()) {
      //$rank = $row["Rank"];
      $start_no = $row["Start_No"];
      // $chip_ID = $row["Chip_ID"];
      $name = $row["Name"];
      $club = $row["Club"];
      // $start_stamp = $row["Start_stamp"];
      // $start = $row["Start"];
      // $finish_stamp = $row["Finish_stamp"];
      // $finish = $row["Finish"];
      $time = $row["Time"];


      echo '<tr> 
                <td>' . $counter++ . '</td>  
                <td>' . $start_no . '</td>
                <!-- <td>' . $chip_ID . '</td> -->
                <td>' . $name . '</td> 
                <td>' . $club . '</td> 
                <!-- <td>' . $start_stamp . '</td> -->
                <!-- <td>' . $start . '</td> -->
                <!-- <td>' . $finish_stamp . '</td> -->
                <!-- <td>' . $finish . '</td> -->
                <td>' . $time . '</td>  
              </tr>';
    }
    $result->free();
    $res->free();
  }

  $conn->close();
  ?>
  </table>
</body>

</html>