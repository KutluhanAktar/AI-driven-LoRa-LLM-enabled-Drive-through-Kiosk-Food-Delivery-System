<?php

session_start();

// Include the required kiosk class functions.
require "class.php";

// Define the db_kiosk class object.
$db_kiosk_obj = new db_kiosk(); 
$db_kiosk_obj->__init__($_db_conn);


// If the information required to create a new customer (user) account is received successfully:
if(isset($_POST["user_info"])){
	$response = $db_kiosk_obj->create_new_user_account($_POST["user_info"]);
	echo $response["res"];
	// Change the dashboard status by utilizing a session variable.
	if(str_contains($response["res"], "successfully created")){
		$_SESSION["account"] = "signed";
		$_SESSION["acc_auth_key"] = $response["auth_key"];
	}
}

// If the information required to open a customer (user) account is received successfully:
if(isset($_POST["login_info"])){
	$response = $db_kiosk_obj->login_to_user_account($_POST["login_info"]);
	echo $response["res"];
	// Change the dashboard status by utilizing a session variable.
	if(str_contains($response["res"], "found successfully")){
		$_SESSION["account"] = "signed";
		$_SESSION["acc_auth_key"] = $response["auth_key"];
	}
}

?>