<?php
// Database info.
$server = array(
	"server" => "localhost",
	"username" => "root",
	"password" => "",
	"database_name" => "ai_lora_kiosk_user_data"
);

// Database connection credentials.
$_db_conn = mysqli_connect($server["server"], $server["username"], $server["password"], $server["database_name"]);

?>
