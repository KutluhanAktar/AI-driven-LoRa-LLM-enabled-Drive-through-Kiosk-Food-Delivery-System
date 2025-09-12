<?php
// Include the required kiosk class functions.
require "assets/class.php";

// Include the php-mqtt/client library.
require "assets/lib/vendor/autoload.php";

// Add the required classes from the php-mqtt/client library.
use PhpMqtt\Client\ConnectionSettings;
use PhpMqtt\Client\Exceptions\MqttClientException;
use PhpMqtt\Client\MqttClient;

// Define the TTN MQTT server (host) settings and the MQTT link (uplink topic), including the unique end device username, password, and ID.
$ttn_server = array(
				"host" => "au1.cloud.thethings.network",
				"port" => 1883,
				"username" => "kiosk-customer-end1@ttn",
				"password" => "<TTN_PASSWORD>", // e.g., NNSXS.TCN__________________________.JMB_________________________________
				"device_id" => "kiosk-customer-end-device1",
				"version" => MqttClient::MQTT_3_1_1,
				"client_id" => "kiosk_web_app"
			  );
$ttn_topic_up = "v3/".$ttn_server["username"]."/devices/".$ttn_server["device_id"]."/up";

try{	
	// Enable a new instance of the MqttClient class to establish a connection with the given TTN MQTT server.
	$ttn_client = new MqttClient($ttn_server["host"], $ttn_server["port"], $ttn_server["client_id"], $ttn_server["version"]);
	
	// Configure the connection settings required by the TTN MQTT server depending on the registered TTN application and end device.
	$ttn_connection_setttings = (new ConnectionSettings)
		->setUsername($ttn_server["username"])
		->setPassword($ttn_server["password"]);

	// Attempt to establish a connection with the TTN MQTT server. Disable clean session.
	$ttn_client->connect($ttn_connection_setttings, false);

	// Subscribe to the TTN uplink message topic using QoS 0.
	$ttn_client->subscribe($ttn_topic_up, function(string $topic, string $message, bool $retained){
	    // Include the database settings separately to avoid variable scope errors.
	    include "assets/database_secrets.php";
	        
		// Define the db_kiosk_lora class object in this function to avoid variable errors due to the ongoing MQTT loop.
		$db_kiosk_lora_obj = new db_kiosk_lora(); 
		$db_kiosk_lora_obj->__init__($_db_conn);
		
		// After getting the transferred uplink message from the connected TTN MQTT server, decode the retrieved information to generate the LoRa message log.
		$lora_message_log = $db_kiosk_lora_obj->decode_lora_message($message);
		printf("\n\nTopic => [%s] => Uplink message is received from the connected TTN MQTT server!\n\nLoRa message log is successfully generated!", $topic);
		// Then, save the generated LoRa message log to the given database table with the extrapolated task information.
		if($db_kiosk_lora_obj->append_lora_message_log($lora_message_log)){
			printf("\nLoRa message log is successfully registered to the database!"); 
		}else{
			printf("\nDatabase Error: LoRa message log cannot be saved to the database!");
		}		
	}, MqttClient::QOS_AT_MOST_ONCE);
	
	// Activate the client loop to wait and receive the published messages immediately.
	$ttn_client->loop(true);

	// Terminate the server connection.
	$ttn_client->disconnect();

// Catch and log MQTT server connection errors for debugging.	
}catch(MqttClientException $e){
	printf("MQTT Connection Error => [%s]", $e);
}

?>
