<?php
session_start();
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Login</title>
</head>
<body>

<form action="" method="post">
    <input type="text" name="username" placeholder="Your name" required>
    <input type="password" name="password" placeholder="Enter password" required>
    <input type="submit" value="Submit">
</form>

    <?php

    if(!empty($_POST) && $_POST['username'] == "Dino" && $_POST['password'] == "timingXyZ"){
    $_SESSION['user'] = $_POST['username'];
    header("Location: ./index.php");
    }

    else if(!empty($_POST) && ($_POST['username'] != "Dino" || $_POST['password'] != "timingXyZ"))
    echo "Access denied!"
    ?>

  
</body>
</html>
