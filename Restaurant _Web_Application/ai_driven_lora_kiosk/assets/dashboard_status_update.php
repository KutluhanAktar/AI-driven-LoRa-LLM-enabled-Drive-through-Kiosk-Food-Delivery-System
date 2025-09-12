<?php

// Include the required kiosk class functions.
require "class.php";

// Define the db_kiosk_status_upt class object.
$db_kiosk_status_upt_obj = new db_kiosk_status_upt(); 
$db_kiosk_status_upt_obj->__init__($_db_conn);

// Obtain the necessary status updates from the related database tables.
if(isset($_GET["get_updates"])){
	echo $db_kiosk_status_upt_obj->generate_dashboard_updates();
}

// If requested, update the selected LLM that of the current user account.
if(isset($_GET["update_llm"])){
	$db_kiosk_status_upt_obj->update_selected_LLM($_GET["update_llm"]);
}

// If requested, update the deal preference that of the current customer account.
if(isset($_GET["update_preference"])){
	$db_kiosk_status_upt_obj->update_deal_preference($_GET["update_preference"]);
}

?>