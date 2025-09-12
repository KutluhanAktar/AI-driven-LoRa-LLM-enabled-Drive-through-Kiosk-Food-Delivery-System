// Every 2 seconds, update the order schedule interface to inform the drive-through restaurant workers of the latest placed order information and status.
setInterval(() => {
	// Obtain the required updates from the database.
	$.ajax({
		url: "/ai_driven_lora_kiosk/assets/order_schedule_update.php",
		type: "GET",
		success: (response) => {
				// After getting the necessary order schedule updates, append them into the given HTML table.
                $('.order_schedule > table').html(response);			
			}
		});	
}, 2000);