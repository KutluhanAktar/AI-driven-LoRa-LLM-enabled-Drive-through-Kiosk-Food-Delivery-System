<?php

// Include the required kiosk class functions.
require "class.php";

// Define the db_kiosk_lora class object.
$db_kiosk_lora_obj = new db_kiosk_lora(); 
$db_kiosk_lora_obj->__init__($_db_conn);

// If requested, return the latest food delivery system log information.
if(isset($_GET["get_latest_food_system_log"])){
	$log = $db_kiosk_lora_obj->food_delivery_system_log("get", array());
	if($log != false){
		if($log["task_status"] == "initiated") echo "!".$log["task_type"]."&".$log["task_objectives"]."&".$log["task_status"]."!";
	}else{
		echo "Database error [Retrieve]!";
	}
}

// If requested, update the task status of the latest food delivery system log.
if(isset($_GET["update_latest_food_system_log_status"])){
	if($db_kiosk_lora_obj->food_delivery_system_log("update", array("task_status" => $_GET["update_latest_food_system_log_status"]))){
		echo "Food delivery system task status is successfully updated!";
	}else{
		echo "Database error [Update]!";
	}
}

// If Nicla Vision transfers a slot sample image, save the received raw image buffer (RGB565) as a TXT file to the slot_samples folder.
if(!empty($_FILES["captured_image"]['name'])){
	// If exists, obtain the transferred image name.
	$sent_name = (isset($_GET["save_slot_image_sample"])) ? $_GET["save_slot_image_sample"] : "debug_sample_";
	// Get the current date and time.
	$date = date("Y_m_d_H_i_s");
	// Create the sample image file name.
	$img_file = $sent_name.$date;
	
	// Sort the received image file information.
	$captured_image_properties = array(
	    "name" => $_FILES["captured_image"]["name"],
	    "tmp_name" => $_FILES["captured_image"]["tmp_name"],
		"size" => $_FILES["captured_image"]["size"],
		"extension" => pathinfo($_FILES["captured_image"]["name"], PATHINFO_EXTENSION)
	);

    // Check whether the uploaded file extension is in the allowed file formats.
	$allowed_formats = array('jpg', 'png', 'bmp', 'txt');
	if(!in_array($captured_image_properties["extension"], $allowed_formats)){
		echo 'FILE => File Format Not Allowed!';
	}else{
		// Check whether the uploaded file size exceeds the 5 MB data limit.
		if($captured_image_properties["size"] > 5000000){
			echo "FILE => File size cannot exceed 5MB!";
		}else{
			// Save the uploaded file (raw image buffer).
			move_uploaded_file($captured_image_properties["tmp_name"], "./prep_stations/apriltag_samples/".$img_file.".".$captured_image_properties["extension"]);
			echo "FILE => Saved Successfully!";
		}
	}
}

?>
