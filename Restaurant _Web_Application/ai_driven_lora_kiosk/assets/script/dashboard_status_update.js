// Every 2 seconds, update the user dashboard (interface) with necessary task and status information to inform the customer of ongoing operations.
setInterval(() => {
	// Obtain the required updates from the database.
	$.ajax({
		url: "/ai_driven_lora_kiosk/assets/dashboard_status_update.php?get_updates",
		type: "GET",
		success: (response) => {
				// After getting the necessary dashboard updates, validate retrieved information to show the updates accurately.
				let upt_obj = JSON.parse(response);
				// Kiosk status.
				if(upt_obj["kiosk_status"] != "not_found"){
					let kiosk_status = (upt_obj["kiosk_status"] == "authorized") ? "Authorized by Customer" : "Not Authorized";
					$("#current_kiosk_status").text(kiosk_status);
				}else{
					$("#current_kiosk_status").text("⛔");
				}
				// Activated LLM.
				if(upt_obj["enabled_LLM"] != "not_found"){
					$("#current_activated_llm").text(upt_obj["enabled_LLM"]);
				}else{
					$("#current_activated_llm").text("⛔");
				}
				// Latest order log.
				if(upt_obj["latest_order_log_info"] != "not_found"){
					$("#current_order_type").text((upt_obj["latest_order_log_info"]["order_type"] == "order_generic") ? "Generic Deal" : "User-specific Deal");
					$("#current_order_tag").text(upt_obj["latest_order_log_info"]["order_tag"]);
					$("#current_order_status").text(upt_obj["latest_order_log_info"]["order_status"]);
					$("#current_order_upt_time").text(upt_obj["latest_order_log_info"]["order_upt_time"]);
				}else{
					$("#current_order_type").text("⛔");
					$("#current_order_tag").text("⛔");
					$("#current_order_status").text("⛔");
					$("#current_order_upt_time").text("⛔");
				}
				// Latest LoRa task log.
				if(upt_obj["latest_lora_log_info"] != "not_found"){
					$("#current_lora_code").text(upt_obj["latest_lora_log_info"]["lora_code"]);
					$("#current_lora_type").text(upt_obj["latest_lora_log_info"]["task_type"]);
					$("#current_lora_details").text(upt_obj["latest_lora_log_info"]["task_details"]);
					$("#current_lora_status").text(upt_obj["latest_lora_log_info"]["task_status"]);
					$("#current_lora_device_id").text(upt_obj["latest_lora_log_info"]["device_id"]);
					$("#current_lora_gateway_id").text(upt_obj["latest_lora_log_info"]["gateway_id"]);
					$("#current_lora_upt_time").text(upt_obj["latest_lora_log_info"]["up_date"]);
				}else{
					$("#current_lora_code").text("⛔");
					$("#current_lora_type").text("⛔");
					$("#current_lora_details").text("⛔");
					$("#current_lora_status").text("⛔");
					$("#current_lora_device_id").text("⛔");
					$("#current_lora_gateway_id").text("⛔");
					$("#current_lora_upt_time").text("⛔");
				}			
			}
		});	
}, 2000);