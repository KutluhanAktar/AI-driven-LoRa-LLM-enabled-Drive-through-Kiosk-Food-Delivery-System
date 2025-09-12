<?php

/* Show all errors for debugging. */
error_reporting(E_ALL);
ini_set('display_errors', '1');

// Include the ollama-php/client library.
require "assets/lib/llm/vendor/autoload.php";

// Add the required classes from the ollama-php/client library.
use ArdaGnsrn\Ollama\Ollama;
use ArdaGnsrn\Ollama\Responses\Chat\ChatMessageResponse;
use ArdaGnsrn\Ollama\Responses\Chat\ChatResponse;
use ArdaGnsrn\Ollama\Responses\StreamResponse;

// Include the required kiosk class functions.
require "assets/class.php";

// Define the db_kiosk_llm class object.
$db_kiosk_llm_obj = new db_kiosk_llm(); 
$db_kiosk_llm_obj->__init__($_db_conn);

// Define the required parameters to generate user-specific menus / deals with the selected LLM.
$customer_selected_LLM = false;
$customer_deal_preference = false;
$available_food_items_and_categories = false;

// Obtain the current customer account information.
$current_user_info = $db_kiosk_llm_obj->current_kiosk_user_config("return", array());
if($current_user_info != false){
	$user_info = $db_kiosk_llm_obj->obtain_user_information($current_user_info["authentication_key"]);
	if($user_info != false){
		$customer_selected_LLM = $user_info["activated_LLM"];
		$customer_deal_preference = $user_info["menu_preference"];		
	}
}

// Obtain the available food item information and categories by prep station as JSON objects.
$available_food_items_and_categories = $db_kiosk_llm_obj->obtain_available_food_item_info("json");

// Define the required JSON object format for each LLM-generated menu / deal.
$generated_menu_json_format = '{
									"order_tag": "",
									"definition": "",
									"item_list": [
										{"name": "", "category": "", "amount": },

									],
									"total_price": ,
									"discount_percentage": ,
									"discounted_price": 
								} ';

?>

<!DOCTYPE html>
<html>
<head>
    <!-- Page info. -->
	<title>LLM Interface</title> 
    <!-- Site settings. -->
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">

	<!-- link to site icon -->
	<link rel="icon" type="image/png"  href="assets/img/site_icon.png">

	<!-- Add jQuery -->
	<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.7.1/jquery.min.js"></script>
	
	<!-- Style the LLM interface output. -->
	<style>
	    body{background-color:transparent;}
		p{color:rgb(235,235,240);font-size:1.5em;font-family: Consolas, Menlo, Monaco, Lucida Console, Liberation Mono, DejaVu Sans Mono, Bitstream Vera Sans Mono, Courier New, monospace, serif;letter-spacing:.1em;}
		
		/* Assign custom cursor selection colors. */ 
		::selection {color: #E2F1F6; background-color: #2A4D70;}
		/* Assign custom scrollbar configurations. */ 
		::-webkit-scrollbar {width:.5em;height:.5em;}
		::-webkit-scrollbar-track {background-color: rgb(174,174,178);border-radius:100vh;margin-block:.5em;}
		::-webkit-scrollbar-thumb {background-color: rgb(0,133,117);border-radius:100vh;}
		::-webkit-scrollbar-thumb:hover {background-color: rgb(0,218,195);}
		::-webkit-scrollbar-corner{background-color:rgb(142,142,147);}
		/* Settings for browsers not supporting -webkit-scrollbar, such as Firefox */
		@supports not selector(::-webkit-scrollbar){
			* {scrollbar-color:rgb(0,133,117) rgb(174,174,178); scrollbar-width:thin;}
		}
	</style>
	
</head>

<body> 

<?php

// If all the necessary information is fetched successfully, run the selected LLM.
if($customer_selected_LLM != false && $customer_deal_preference != false && $available_food_items_and_categories != false){
	// Define the system requirements for the selected LLM.
	$llm_system_requirements = 'Answer as a drive-through restaurant kiosk computer.';
	// Define the primary task and necessary objectives for the selected LLM based on the provided customer configurations and deal preference.
	$llm_task_and_objectives = '
								Generate 12 different menus using food items from these JSON objects:
								
								'.$available_food_items_and_categories.'
								
								Each menu has to include at least one item from the category '.$customer_deal_preference.'.
								You must add additional items from the category Side Dishes and Desserts to each menu.
								Each menu has to include up to 5 different items.
								You must assign an amount for each item from 2 to 6 for the category Side Dishes and Desserts and from 2 to 4 for the category '.$customer_deal_preference.'.
								You must calculate the total menu price by using the provided food item prices and the assigned amounts.
								You must assign a discount percentage from 2 percent to 8 percent for each menu.
								You must calculate the discounted menu price based on the estimated total menu price and the assigned discount percentage.
								You must generate a brief menu description based on the added food items for each menu. The description must be at least 50 words.
								You must assign an order tag for each of the 12 different menus, from e001 to e012.
								
								You must generate and return 12 different menus and return them as JSON objects in this format:				
								
								'.$generated_menu_json_format;

    echo '<p>Generated LLM objectives:</p><br><p>'.$llm_task_and_objectives.'</p>';
	

	// Create a new Ollama PHP client based on the built-in Ollama chat API.
	$client = \ArdaGnsrn\Ollama\Ollama::client();

	// Create a new chat session with the passed LLM to generate user-specific menus / deals with the provided customer configurations and deal preference.
    /*
		According to my experiments concerning this use case, thinking models tend to generate inconsistent results or incompatible JSON objects since they gaslight themselves to tangential outcomes for simple or straightforward tasks such as restaurant menu generation.
		Thus, I disabled the thinking feature via the built-in Ollama chat API.
    */

	$response = $client->chat()->create([
		'model' => $customer_selected_LLM,
		'messages' => [
			['role' => 'system', 'content' => $llm_system_requirements],
			['role' => 'user', 'content' => $llm_task_and_objectives]
		],
		'think' => false // Disable thinking to get more accurate results with the given use case.
	]);

	echo "<br><p>Selected (Utilized) LLM: ".$customer_selected_LLM."</p><br>";

	echo "<p>Thinking: Disabled!</p><br>";
	
	echo "<p>Deal Preference: ".$customer_deal_preference."</p><br>";
       
	// Remove the escaped character sequences from the LLM-generated text to avoid errors while parsing the text as JSON objects.
	$remove_escaped_LLM_response = str_replace(array("\n", "\r", "\t"), " ", $response->message->content);
	// Print the response with all hidden characters for debugging.
	echo "<p>LLM Response (text/plain): ".json_encode($remove_escaped_LLM_response)."</p><br>";
	
	// Process the modified LLM-generated response, consisting of all 12 menus / deals as JSON objects, to register each menu / deal to the associated database table.
	$db_kiosk_llm_obj->process_LLM_generated_user_specific_deals($remove_escaped_LLM_response);
    
}

?>

</body>
</html>
