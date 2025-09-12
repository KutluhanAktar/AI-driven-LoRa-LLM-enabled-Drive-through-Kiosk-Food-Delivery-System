<?php

// Include the database settings.
include_once "database_secrets.php";

// Define the db_kiosk class and its functions.
class db_kiosk{
	protected $db_conn;
	protected $lora_task_table = "lora_user_task_log", $user_info_table = "user_info", $menu_info_table = "menu_info_by_user", $current_user_table = "current_kiosk_user", $food_items_table = "food_items_by_prep_station";
	
	private $default_LLM = "llama3.2:3b";
	
	public function __init__($_db_conn){
		// Init the MySQL object with the passed database settings.
		$this->db_conn = $_db_conn;
	}
		
	// Database -> Obtain the registered user (account) information by the account authentication key.
	public function obtain_user_information($auth_key){
		$sql = "SELECT * FROM `$this->user_info_table` WHERE `authentication_key` = '$auth_key'";
		$result = mysqli_query($this->db_conn, $sql);
		$check = mysqli_num_rows($result);
		if($check > 0){
			// If found successfully, return the registered user information.
			if($row = mysqli_fetch_assoc($result)){
				return $row;
			}else{
				return false;
			} 
		}else{
			return false;
		}		
	}
	
	// Database -> Configurations for the current dashboard/kiosk user information.
	public function current_kiosk_user_config($db_task, $user_config){
		// Get the current date & time (server).
		$date = date("Y_m_d_h_i_s");
		// Change the current user (customer) information in the given MariaDB database table.
		if($db_task == "change"){
			$sql = "UPDATE `$this->current_user_table`
					SET `authentication_key` = '".$user_config["auth_key"]."',
						`last_change` = '$date',
						`account_status` = '".$user_config["account_status"]."',
						`kiosk_auth_status` = '".$user_config["kiosk_auth_status"]."'
					WHERE `id` = 1;";
			// Show the query result.
			return (mysqli_query($this->db_conn, $sql)) ? true : false;
		}
		// Update the kiosk authentication status (vehicle recognition) status.
		else if($db_task == "update_status"){
			$sql = "UPDATE `$this->current_user_table`
					SET `last_change` = '$date',
						`kiosk_auth_status` = '".$user_config["kiosk_auth_status"]."'
					WHERE `id` = 1;";
			// Show the query result.
			return (mysqli_query($this->db_conn, $sql)) ? true : false;			
		}
		// Return the current user information.
		else if($db_task == "return"){
			$sql = "SELECT * FROM `$this->current_user_table` WHERE `id` = 1;";
			$result = mysqli_query($this->db_conn, $sql);
			$check = mysqli_num_rows($result);
			if($check > 0){
				// If found successfully, return the latest task information.
				if($row = mysqli_fetch_assoc($result)){
					return $row;
				}else{
					return false;
				} 
			}else{
				return false;
			}		
		}		
	}
	
	// Database -> Open the requested customer (user) account by the registered username and password.
	public function login_to_user_account($login_info){
		$sql = "SELECT * FROM `$this->user_info_table` WHERE `username` = '".strip_tags(mysqli_real_escape_string($this->db_conn, $login_info['username']))."' and `password` = '".strip_tags(mysqli_real_escape_string($this->db_conn, $login_info['_password']))."'";
		$result = mysqli_query($this->db_conn, $sql);
		$check = mysqli_num_rows($result);
		if($check > 0){
			// If found successfully, return the required user information.
			if($row = mysqli_fetch_assoc($result)){
				// After successfully finding the passed user account, change the current user (customer) information for further tasks.
				if($this->current_kiosk_user_config("change", array("auth_key" => $row["authentication_key"], "account_status" => "signed", "kiosk_auth_status" => "pending"))){
					// Return the required info.
					return array("res" => "Given account found successfully!", "auth_key" => $row["authentication_key"]);
				}else{
					return array("res" => "Database error! Status change!", "auth_key" => "");
				}				
			}else{
				return array("res" => "Database error! Information retrieval!", "auth_key" => "");
			} 
		}else{
			return array("res" => "Database error! No user found with the passed credentials!", "auth_key" => "");
		}		
	}
	
	// Database -> Discard the current dashboard/kiosk user information.
	public function discard_current_kiosk_user(){
		$this->current_kiosk_user_config("change", array("auth_key" => "none", "account_status" => "not_signed", "kiosk_auth_status" => "none"));
	}
		
	// Database -> Create a new customer (user) account with the required database configurations.
	public function create_new_user_account($user_info){
		// Generate the authentic account authentication key.
		$auth_key = $this->generate_auth_key(4);
		// Insert the provided user information into the given MariaDB database table.
		$sql = "INSERT INTO `$this->user_info_table` (`firstname`, `lastname`, `email`, `username`,  `password`,  `menu_preference`, `card_holder_name`, `card_number`, `card_exp_date`, `card_cvv`, `activated_LLM`, `authentication_key`)
		        VALUES ('".strip_tags(mysqli_real_escape_string($this->db_conn, $user_info['firstname']))."', '"
		                  .strip_tags(mysqli_real_escape_string($this->db_conn, $user_info['lastname']))."', '"
		                  .strip_tags(mysqli_real_escape_string($this->db_conn, $user_info['email']))."', '"
		                  .strip_tags(mysqli_real_escape_string($this->db_conn, $user_info['username']))."', '"
		                  .strip_tags(mysqli_real_escape_string($this->db_conn, $user_info['_password']))."', '"
		                  .strip_tags(mysqli_real_escape_string($this->db_conn, $user_info['menu_preference']))."', '"
		                  .strip_tags(mysqli_real_escape_string($this->db_conn, $user_info['card_holder_name']))."', '"
		                  .strip_tags(mysqli_real_escape_string($this->db_conn, $user_info['card_number']))."', '"
		                  .strip_tags(mysqli_real_escape_string($this->db_conn, $user_info['card_exp_date']))."', '"
		                  .strip_tags(mysqli_real_escape_string($this->db_conn, $user_info['card_cvv']))."', '"
		                  .$this->default_LLM."', '"
						  .$auth_key."');";
		// Exit and notify if the server throws a database error.
		if(!mysqli_query($this->db_conn, $sql)){ return array("res" => "Database error!", "auth_key" => ""); }
				 
        // Insert the placeholder user-specific menu and deal information into the given MariaDB database table.
		$menu_information = array("deal_1" => "pending", "deal_2" => "pending", "deal_3" => "pending", "deal_4" => "pending", "deal_5" => "pending", "deal_6" => "pending", "deal_7" => "pending", "deal_8" => "pending", "deal_9" => "pending", "deal_10" => "pending", "deal_11" => "pending", "deal_12" => "pending");
		$sql_menu_insert = "INSERT INTO `$this->menu_info_table` (`authentication_key`, `deal_1`, `deal_2`, `deal_3`, `deal_4`, `deal_5`, `deal_6`, `deal_7`, `deal_8`, `deal_9`, `deal_10`, `deal_11`, `deal_12`)
							VALUES ('".$auth_key."', '"
									  .$menu_information["deal_1"]."', '"
									  .$menu_information["deal_2"]."', '"
									  .$menu_information["deal_3"]."', '"
									  .$menu_information["deal_4"]."', '"
									  .$menu_information["deal_5"]."', '"
									  .$menu_information["deal_6"]."', '"
									  .$menu_information["deal_7"]."', '"
									  .$menu_information["deal_8"]."', '"
									  .$menu_information["deal_9"]."', '"
									  .$menu_information["deal_10"]."', '"
									  .$menu_information["deal_11"]."', '"
									  .$menu_information["deal_12"]."');";
		// Exit and notify if the server throws a database error.
		if(!mysqli_query($this->db_conn, $sql_menu_insert)){
			return array("res" => "Database error!", "auth_key" => "");
		}else{
			// After successfully creating a new user account with all necessities, change the current user (customer) information for further tasks.
			if($this->current_kiosk_user_config("change", array("auth_key" => $auth_key, "account_status" => "signed", "kiosk_auth_status" => "pending"))){
				return array("res" => "Thanks 😊\nYour account is successfully created, ".$user_info['firstname']."!", "auth_key" => $auth_key);
			}else{
				return array("res" => "Application Error!", "auth_key" => "");
			}			
		}
	}
	
	// Generate a unique account authentication key as a new account is created.
	private function generate_auth_key($len){
		// All authentication keys should start with the 'a' character.
		$auth_key = "a";
		// Define the applicable characters.
		$auth_chars = array("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "b", "c", "d", "e", "f");
		// Get and assign a random character from the predefined character array.
		for($i=0; $i<$len-1; $i++){
			$auth_key .= $auth_chars[rand(0, count($auth_chars)-1)];
		}
		// Return the generated authentication key.
		return $auth_key;
	}

}	

class db_kiosk_llm extends db_kiosk{
	// After the selected LLM generates user-specific deals as JSON objects based on the provided customer configurations and deal preference,
	// modify the fetched LLM response according to the syntax of the given LLM.
	// Based on my experiments, I noticed JSON syntax is slightly different for different models.
	public function process_LLM_generated_user_specific_deals($llm_response){
		// Modify the LLM-generated text to obtain JSON objects (menus) in the required format.
		if(str_contains($llm_response, '```json [') || str_contains($llm_response, '```json[')){
			$llm_response = explode('```json', $llm_response)[1];
			$llm_response = explode('```', $llm_response)[0];
		}else if(str_contains($llm_response, '```json')){
			if(substr_count($llm_response, '```json') == 1){
				$llm_response = str_replace('```json', '```json [', $llm_response);
				$llm_response = str_replace(array('} {', '}{'), '}, {', $llm_response);
				$llm_response = str_replace(array('}```', '} ```'), '} ] ```', $llm_response);
			}else{
				$llm_response_mod = "```json [ ";
				foreach(explode('```json', $llm_response) as $object){
					if(str_contains($object, 'order_tag')){
						$object = explode('```', $object)[0];
						$llm_response_mod .= $object.", ";
					}
				}
				$llm_response_mod .= "] ```";
				$llm_response_mod = str_replace(', ] ```', '] ```', $llm_response_mod);
				$llm_response = $llm_response_mod;
			}
			$llm_response = explode('```json', $llm_response)[1];
			$llm_response = explode('```', $llm_response)[0];
		}else if(str_contains($llm_response, '**')){
			$llm_response = str_replace('**Menu e001**', '```json [', $llm_response);
			for($i=1;$i<13;$i++){
				$m_code = "";
				if($i < 10){ $m_code = "e00".$i; }
				else{ $m_code = "e0".$i; }
				$llm_response = str_replace('**Menu '.$m_code.'**', ',', $llm_response);
			}
			$llm_response .= ' ]';
			$llm_response = explode('```json', $llm_response)[1];
			$llm_response = explode('```', $llm_response)[0];			
		}
		
		// Obtain the available food (menu) item categories by prep station.
		$menu_categories = $this->obtain_available_food_item_info("category");
		
		// Replace repeating characters (if any) causing errors while decoding JSON objects. 
		$llm_response = str_replace(array("::", ": :"), ":", $llm_response);
		
		// Decode the retrieved JSON objects to process menu / deal information accurately.
		$llm_menu_data = json_decode($llm_response, true); // As array.
		// First, produce the prep station road map depending on the requested food item amount per prep station. For instance, 1%0%0%1%2%5 [items from (station_1, station_2, station_2, station_4, station_5, station_6)].
		for($menu_n=0;$menu_n<12;$menu_n++){
			$station_1 = 0; $station_2 = 0; $station_3 = 0; $station_4 = 0; $station_5 = 0; $station_6 = 0;
			foreach($llm_menu_data[$menu_n]["item_list"] as $food_item){
				switch($food_item["category"]){
					case $menu_categories[0]:
						$station_1 += $food_item["amount"];
					break;
					case $menu_categories[1]:
						$station_2 += $food_item["amount"];
					break;	
					case $menu_categories[2]:
						$station_3 += $food_item["amount"];
					break;
					case $menu_categories[3]:
						$station_4 += $food_item["amount"];
					break;
					case $menu_categories[4]:
						$station_5 += $food_item["amount"];
					break;
					case $menu_categories[5]:
						$station_6 += $food_item["amount"];
					break;
					default:
                        // Do nothing.
                    break;						
				
				}
			}
			// Append the produced prep station road map into the passed menu array.
			$prep_station_road_map = $station_1.'%'.$station_2.'%'.$station_3.'%'.$station_4.'%'.$station_5.'%'.$station_6;
			$llm_menu_data[$menu_n]["prep_station_road_map"] = $prep_station_road_map;
		}
		
		// Obtain the current dashboard/kiosk user (customer) information.
		$current_user_info = $this->current_kiosk_user_config("return", array());
		// If the current account has been authorized successfully:
		if($current_user_info["kiosk_auth_status"] == "authorized"){
			// After successfully processing the LLM-generated menu / deal information,
			// convert each menu array to a JSON object in order to update the user-specific menu /deal information that of the current dashboard/kiosk user.
			$column = 0;
			foreach($llm_menu_data as $user_specific_deal){
				// Next column.
				$column++;
				// After encoding the menu array, replace single quotation mark with its HTML entity to avoid errors.
				$user_specific_deal_json = json_encode($user_specific_deal);
				$user_specific_deal_json = str_replace("'", "&apos;",$user_specific_deal_json);
				// Update the user-specific menu /deal information.
				$sql = "UPDATE `$this->menu_info_table`
						SET `deal_".$column."` = '$user_specific_deal_json'
						WHERE `authentication_key` = '".$current_user_info["authentication_key"]."';";
				// Show the query result.
				if(mysqli_query($this->db_conn, $sql)){
					echo "<p>".$user_specific_deal["order_tag"].": registered successfully!</p><br>";
				}else{
					echo "<p>".$user_specific_deal["order_tag"].": database registration error!</p><br>";
				}		
			}			
		}else{
			echo "<p>Account not authorized!</p>";
		}		
	}
	
	// Database -> Obtain the available food item information by prep station.
    public function obtain_available_food_item_info($type){
		$sql = "SELECT * FROM `$this->food_items_table`";
		$result = mysqli_query($this->db_conn, $sql);
		$check = mysqli_num_rows($result);
		if($check > 0){
			if($row = mysqli_fetch_assoc($result)){
				// Remove the id column to avoid errors.
				unset($row["id"]);
				// If requested, return the retrieved food item information without modifying (as JSON objects) to feed the selected LLM as data points.
				if($type == "json"){
					$food_item_info_json = "";
					foreach($row as $prep_table){
						$food_item_info_json .= $prep_table.",";
					}
					return $food_item_info_json;					
				}
				// Otherwise, decode the retrieved JSON objects for each prep station and return only the requested data type in the output array.
				else{
					$output_food_item_info = [];
					foreach($row as $prep_table){
						$prep_table_arr = json_decode($prep_table, true); // As array.
						array_push($output_food_item_info, $prep_table_arr[$type]);
					}
					return $output_food_item_info;					
				}
			}else{
				return false;
			} 
		}else{
			return false;
		}		
	}

	// Database -> Obtain the menu / deal information — user-specific or generic — by the assigned account authentication code.
    public function obtain_menu_deal_information($auth_key, $type){
		$sql = "SELECT * FROM `$this->menu_info_table` WHERE `authentication_key` = '$auth_key'";
		$result = mysqli_query($this->db_conn, $sql);
		$check = mysqli_num_rows($result);
		if($check > 0){
			if($row = mysqli_fetch_assoc($result)){
				// Remove the id and authentication_key columns to avoid errors.
				unset($row["id"], $row["authentication_key"]);
				// Decode the retrieved JSON objects for each menu / deal.
				$deals_arr = [];
				foreach($row as $deal){
					$deal_arr = json_decode($deal, true); // As array.
					array_push($deals_arr, $deal_arr);
				}
				// Depending on the passed data type, return the whole deal information or only the necessary information by the order tag.
				if($type == "all"){
					return $deals_arr;
				}else{
					$necessary_info_arr = [];
					for($i=0;$i<count($deals_arr);$i++){
						$necessary_info_arr[$deals_arr[$i]["order_tag"]] = $deals_arr[$i][$type];
					}
					return $necessary_info_arr;
				}
			}else{
				return false;
			}
		}else{
			return false;
		}
		
	}	
}
	
class db_kiosk_lora extends db_kiosk_llm{
	protected $food_delivery_table = "food_delivery_system_log";

	// Database -> Obtain the latest LoRa task log information.
	public function obtain_latest_lora_log($task_type){
		$lora_log_info = false;
		// Obtain the latest LoRa task log information according to the requested task type.
		$query = ($task_type != "all") ? "WHERE `task_type` = '$task_type'" : "";
		$sql = "SELECT * FROM `$this->lora_task_table` ".$query." ORDER BY `log_id` DESC";
		$result = mysqli_query($this->db_conn, $sql);
		$check = mysqli_num_rows($result);
		if($check > 0){
			// Generate the required LoRa task log array.
			if($row = mysqli_fetch_assoc($result)){
				$lora_log_info = array( 
				                        "device_id" => $row["device_id"],
				                        "gateway_id" => $row["gateway_id"],
										"lora_code" => $row["decoded_payload"],
										"task_type" => $row["task_type"],
										"task_details" => $row["task_details"],
										"task_status" => $row["task_status"],
										"up_date" => $row["server_time"]
									  );						
			}
		}
		// Return the retrieved LoRa log information.
		return $lora_log_info;		
	}
	
    // Database -> Append new LoRa message log with the given TTN MQTT server credentials.	
	public function append_lora_message_log($lora_message_log){
		// Insert the recent LoRa message information into the given MariaDB database table.
		$sql = "INSERT INTO `$this->lora_task_table` (`device_id`, `application_id`, `gateway_id`, `received_at`,  `frm_payload`,  `decoded_payload`, `task_type`, `task_details`, `task_status`, `server_time`)
		        VALUES ('".$lora_message_log['device_id']."', '"
				          .$lora_message_log['application_id']."', '"
						  .$lora_message_log['gateway_id']."', '"
				          .$lora_message_log['received_at']."', '"
						  .$lora_message_log['frm_payload']."', '"
						  .$lora_message_log['decoded_payload']."', '"
						  .$lora_message_log['task_type']."', '"
						  .$lora_message_log['task_details']."', '"
						  .$lora_message_log['task_status']."', '"
						  .$lora_message_log['server_time']."');";
		// Show the query result.
	    return (mysqli_query($this->db_conn, $sql)) ? true : false;
	}

	// Decode and log the latest LoRa message obtained through the TTN MQTT server.
	public function decode_lora_message($lora_message){
		// Get the current date & time (server).
		$date = date("Y_m_d_h_i_s");
		// Decode the passed JSON object (LoRa message).
		$lora_message = json_decode($lora_message);
		// Convert the retrieved frm_payload (Base64 encoded) to hexadecimal string (Base64 -> ASCII -> Hex) so as to extrapolate the transferred uplink message value.
		$payload = bin2hex(base64_decode($lora_message->uplink_message->frm_payload));
		/*
			Then, obtain the requested task depending on the first character of the retrieved hexadecimal string — 4-digit.
				a___ [User Account Authentication Code]
				e___ [User-specific Menus and Deals — LLM-generated]
				f___ [Generic Menus and Deals]				
		*/
		$task_type = ""; $task_details = ""; $task_status = "";
		switch($payload[0]){
			case "a":
				$task_type = "auth_account";
				$task_details = "Kiosk sent the account authentication code.";
				// When the user sends the account authentication code, compare it with the registered dashboard/kiosk user information.
				$current_user_info = $this->current_kiosk_user_config("return", array());
				if($current_user_info != false){
					// If the authentication code is correct and the current account has not already been authorized,
					// update the dashboard/kiosk user information accordingly to enable the current user account to place orders.
					if($payload == $current_user_info["authentication_key"] && $current_user_info["kiosk_auth_status"] != "authorized"){	
						$this->current_kiosk_user_config("update_status", array("kiosk_auth_status" => "authorized"));
						$task_status = "Account has been authorized successfully";
					}else{
						$task_status = "Given auth code is wrong or the account has already been authorized!";
					}
				}else{
					$task_status = "Database error [compare current user]!";
				}				  
			break;
			case "e":
				$task_type = "order_specific";
				$task_details = "Customer purchased a user-specific menu / deal generated by the selected LLM based on the passed deal preference.";
				/* When the user sends a menu tag to purchase a user-specific menu / deal (generated by the selected LLM), update the latest task information accordingly. */
				// First, obtain the current dashboard/kiosk user (customer) information.
				$current_user_info = $this->current_kiosk_user_config("return", array());
				if($current_user_info != false){
					// If the current user account is successfully authorized, obtain the prep station road map that of the requested menu.
					if($current_user_info["kiosk_auth_status"] == "authorized"){
						$prep_station_road_maps = $this->obtain_menu_deal_information($current_user_info["authentication_key"], "prep_station_road_map");
						// Check whether the retrieved menu information is correct or not.
						if($prep_station_road_maps != false && array_key_exists($payload, $prep_station_road_maps)){
							// If the menu / deal information is fetched successfully, transfer the assigned prep station road map to the food delivery system.
							$menu_prep_road_map = $prep_station_road_maps[$payload];
							if($this->food_delivery_system_log("insert", array("task_type" => $task_type, "task_objectives" => $menu_prep_road_map, "task_status" => "initiated", "authentication_key" => $current_user_info["authentication_key"], "order_tag" => $payload))){
								$task_status = "Requested user-specific menu / deal [LLM-generated] order information successfully transferred to the food delivery system!"; /* 2%2%0%0%6%2 [station_1, station_2, station_3, station_4, station_5, station_6] */
							}else{
								$task_status = "Cannot transfer information to the food delivery system!";
							}							
						}else{
							$task_status = "There is no menu / deal with the passed menu tag.";
						}
					}else{
						$task_status = "User account has not been authorized yet!";
					}
				}else{
					$task_status = "Cannot access the current user information!";
				}
			break;			
			case "f":
				$task_type = "order_generic";
				$task_details = "Customer purchased a generic menu / deal.";
				/* When the user sends a menu tag to purchase a generic menu / deal, update the latest task information accordingly. */
				// First, obtain the prep station road map that of the requested menu.
				$prep_station_road_maps = $this->obtain_menu_deal_information("generic", "prep_station_road_map");
				// Check whether the retrieved menu information is correct or not.
				if($prep_station_road_maps != false && array_key_exists($payload, $prep_station_road_maps)){
					// If the menu / deal information is fetched successfully, transfer the assigned prep station road map to the food delivery system.
					$menu_prep_road_map = $prep_station_road_maps[$payload];
					if($this->food_delivery_system_log("insert", array("task_type" => $task_type, "task_objectives" => $menu_prep_road_map, "task_status" => "initiated", "authentication_key" => "generic", "order_tag" => $payload))){
						$task_status = "Requested generic menu / deal order information successfully transferred to the food delivery system!"; /* 3%0%0%0%6%0 [station_1, station_2, station_3, station_4, station_5, station_6] */
					}else{
						$task_status = "Cannot transfer information to the food delivery system!";
					}					
				}else{
					$task_status = "There is no menu / deal with the passed menu tag.";
				}
			break;
            default:
				$task_type = "not_recognized"; $task_details = "LoRa message sent by the kiosk is not an assigned task."; $task_status = "idle";
		}

        // Generate the LoRa uplink message log	array with the extrapolated task information.
		$lora_message_log = array(
								   "device_id" => $lora_message->end_device_ids->device_id,
								   "application_id" => $lora_message->end_device_ids->application_ids->application_id,
								   "gateway_id" => $lora_message->uplink_message->rx_metadata[0]->gateway_ids->gateway_id,
								   "received_at" => $lora_message->uplink_message->rx_metadata[0]->received_at,
								   "frm_payload" => $lora_message->uplink_message->frm_payload,
								   "decoded_payload" => $payload,
								   "task_type" => $task_type,
								   "task_details" => $task_details,
								   "task_status" => $task_status,
								   "server_time" => $date
								 );		
		// Return the generated LoRa message log.
		return $lora_message_log;
	}

    // Database -> Operation log settings for the food delivery system (based on Nicla Vision) with the given task information.
	public function food_delivery_system_log($db_command, $task_info){
		// Get the current date & time (server).
		$date = date("Y_m_d_h_i_s");
		// Insert the provided task information into the given MariaDB database table.		
		if($db_command == "insert"){
			$sql = "INSERT INTO `$this->food_delivery_table` (`task_type`, `task_objectives`, `task_status`, `authentication_key`, `order_tag`, `server_time`)
					VALUES ('".$task_info["task_type"]."', '"
							  .$task_info["task_objectives"]."', '"
							  .$task_info["task_status"]."', '"
							  .$task_info["authentication_key"]."', '"
							  .$task_info["order_tag"]."', '"
							  .$date."');";
			// Show the query result.
			return (mysqli_query($this->db_conn, $sql)) ? true : false;	
		}
		// Update the ongoing task information.	
		else if($db_command == "update"){
			$sql = "UPDATE `$this->food_delivery_table`
					SET `task_status` = '".$task_info["task_status"]."',
						`server_time` = '$date'
					ORDER BY `id` DESC LIMIT 1;";
			// Show the query result.
			return (mysqli_query($this->db_conn, $sql)) ? true : false;			
		}
		// Return the ongoing task information.
		else if($db_command == "get"){
			$sql = "SELECT * FROM `$this->food_delivery_table` ORDER BY `id` DESC";
			$result = mysqli_query($this->db_conn, $sql);
			$check = mysqli_num_rows($result);
			if($check > 0){
				// If found successfully, return the latest task information.
				if($row = mysqli_fetch_assoc($result)){
					return $row;
				}else{
					return false;		
				} 
			}else{
				return false;
				
			}			
		}
	}
	
}	

class db_kiosk_status_upt extends db_kiosk_lora{
	// Database -> Generate the necessary dashboard status updates.
	public function generate_dashboard_updates(){
		$kiosk_status = "not_found";
		$enabled_LLM = "not_found";
		$latest_order_log_info = "not_found";
		$latest_lora_log_info = "not_found";
		// Get the current dashboard/kiosk user information.
		$current_user_info = $this->current_kiosk_user_config("return", []);
		// Continue if the database returns the current dashboard/kiosk user information successfully.
		if($current_user_info != false){
			// Kiosk status.
			$kiosk_status = $current_user_info["kiosk_auth_status"];
			// Get the current user (account) information.
			$user_info = $this->obtain_user_information($current_user_info["authentication_key"]);
			// Continue if the database returns the current user (account) information successfully.
			if($user_info != false){
				// Activated LLM.
				$enabled_LLM = $user_info["activated_LLM"];
				// Get the latest LoRa log information from the registered LoRa entries, including error messages.
				$lora_log = $this->obtain_latest_lora_log("all");
				// Continue if the database returns the latest LoRa task log successfully.
				if($lora_log != false){
					// Latest LoRa task log.
					$latest_lora_log_info = $lora_log;
					// Get the latest ordered menu / deal information — generic or user-specific.
					$food_log_info = $this->food_delivery_system_log("get", []);	
					// Continue if the database returns the latest food delivery system log successfully.
					if($food_log_info != false){
						// Latest order log.
						$latest_order_log_info = array("order_type" => $food_log_info["task_type"], "order_tag" => $food_log_info["order_tag"], "order_status" => $food_log_info["task_status"], "order_upt_time" => $food_log_info["server_time"]);
					}					
				}				
			}
		}
		// Return the collected necessary dashboard updates as a JSON object.
		return json_encode(
		                   array("kiosk_status" => $kiosk_status,
						         "enabled_LLM" => $enabled_LLM,
								 "latest_order_log_info" => $latest_order_log_info,
								 "latest_lora_log_info" => $latest_lora_log_info
								 )
						  );
	}
	
	// Database -> Change the selected LLM.
	public function update_selected_LLM($selected_LLM){
		// Get the current dashboard/kiosk user information.
		$current_user_info = $this->current_kiosk_user_config("return", []);
		// Continue if the database returns the current dashboard/kiosk user information successfully.
		if($current_user_info != false){
			// Update the selected LLM of the fetched user.
			$sql = "UPDATE `$this->user_info_table`
					SET `activated_LLM` = '$selected_LLM'
					WHERE `authentication_key` = '".$current_user_info["authentication_key"]."';";
			// Show the query result.
			return (mysqli_query($this->db_conn, $sql)) ? true : false;
		}		
	}
	
	// Database -> Change the deal preference.
	public function update_deal_preference($given_preference){
		// Get the current dashboard/kiosk user information.
		$current_user_info = $this->current_kiosk_user_config("return", []);
		// Continue if the database returns the current dashboard/kiosk user information successfully.
		if($current_user_info != false){
			// Update the deal preference of the fetched user.
			$sql = "UPDATE `$this->user_info_table`
					SET `menu_preference` = '$given_preference'
					WHERE `authentication_key` = '".$current_user_info["authentication_key"]."';";
			// Show the query result.
			return (mysqli_query($this->db_conn, $sql)) ? true : false;
		}		
	}
	
}

class db_kiosk_status_order extends db_kiosk_lora{
	// Database -> Generate the necessary order status updates.
	public function generate_order_schedule_updates(){
		$order_status_html_content = '';
		// First, obtain all of the food delivery system logs.
		$sql = "SELECT * FROM `$this->food_delivery_table` ORDER BY `id` DESC";
		$result = mysqli_query($this->db_conn, $sql);
		$check = mysqli_num_rows($result);
		if($check > 0){
			while($row = mysqli_fetch_assoc($result)){
				$html_content = '';
				// If there are food delivery system logs, fetch the menu / deal list of the given customer account via its authentication key.
				$auth_key = $row["authentication_key"];
				$menu_tag = $row["order_tag"];
				// Define the initial HTML table columns.
				$html_content .= '<tr class="'.$row["task_status"].'"><td>'.$auth_key.'</td><td>'.$menu_tag.'</td><td>'.$row["task_type"].'</td>';
				$menu_information = $this->obtain_menu_deal_information($auth_key, "all");
				// Then, obtain the available menu categories by prep station.
				$menu_categories = $this->obtain_available_food_item_info("category");
				// If the menu / deal list and the available menu categories are retrieved successfully, obtain the requested menu / deal information (generic or user-specific) via its provided menu tag.
				if($menu_information != false && $menu_categories != false){
					// According to the retrieved information, generate the required HTML table content.
					// List items per prep station.
					$station_prep_item_lists = ['', '', '', '', '', ''];
					foreach($menu_information as $menu){
						if($menu["order_tag"] == $menu_tag){
							foreach($menu["item_list"] as $item){
								for($i=0; $i<count($menu_categories); $i++){
									if($item["category"] == $menu_categories[$i]){
										$station_prep_item_lists[$i] .= $item["amount"].' x '.$item["name"].'<br>';
									}
								}
							}
						}
					}
					// Define HTML table columns of prep station items.
					foreach($station_prep_item_lists as $station_item_list){
						if($station_item_list != ''){
							$html_content .= '<td>'.$station_item_list.'</td>';
						}else{
							$html_content .= '<td>No items required!</td>';
						}
					}
					// Define the HTML table columns for the remaining parameters.
					$html_content .= '<td>'.$row["task_status"].'</td><td>'.$row["server_time"].'</td></tr>';
					// Finally, add the produced HTML table row to the HTML table content.
                    $order_status_html_content .= $html_content;			
				}else{
					return false;
				}
			} 
			// After generating all HTML table rows successfully, return the HTML table content.
			return $order_status_html_content;
		}else{
			return false;		
		}
	}
}

?>
