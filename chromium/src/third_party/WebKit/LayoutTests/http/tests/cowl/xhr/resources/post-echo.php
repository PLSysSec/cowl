<?php
if (isset($_POST['type']))
    header("Content-Type: {$_POST['type']}");
else
    header("Content-Type: application/labeled-json");

$str = @file_get_contents('php://input');
header("Content-Length: " . strlen($str));
echo $str;
?>
