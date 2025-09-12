<?php

// Include the required kiosk class functions.
require "class.php";

// Define the db_kiosk_status_order class object.
$db_kiosk_status_order_obj = new db_kiosk_status_order(); 
$db_kiosk_status_order_obj->__init__($_db_conn);


// Generate the required HTML table content to inform the prep stations with latest order schedule updates.
$obtained_schedule_updates = $db_kiosk_status_order_obj->generate_order_schedule_updates();

$schedule_updates = '<tr><th>Account Authentication Key</th><th>Menu Tag</th><th>Deal Type</th><th>Station 1</th><th>Station 2</th><th>Station 3</th><th>Station 4</th><th>Station 5</th><th>Station 6</th><th>Order Status</th><th>Updated</th></tr>';

if($obtained_schedule_updates == false){
	$schedule_updates .= '<tr class="none"><td>none</td><td>none</td><td>none</td><td>none</td><td>none</td><td>none</td><td>none</td><td>none</td><td>none</td><td>none</td><td>none</td></tr>';
}else{
	$schedule_updates .= $obtained_schedule_updates;
}

// Print the produced HTML table content.
echo $schedule_updates;

?>