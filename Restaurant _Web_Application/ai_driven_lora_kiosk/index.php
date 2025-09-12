<?php

// Include the required kiosk class functions.
require "assets/class.php";

// Define the db_kiosk_llm class object.
$db_kiosk_llm_obj = new db_kiosk_llm(); 
$db_kiosk_llm_obj->__init__($_db_conn);

// Start the session.
session_start();

// Logout and destroy the current session.
if(isset($_GET["logout"])){
	session_unset();
	session_destroy();
	// Remove the current dashboard/kiosk user information.
	$db_kiosk_llm_obj->discard_current_kiosk_user();
	// Reload the page without the passed GET parameters to avoid errors.
	header("Location: /ai_driven_lora_kiosk");
	exit();
}

// Track the dashboard status via session variables.
$dash_status = (isset($_SESSION["account"]) && $_SESSION["account"] == "signed") ? true : false;

// If signed, get the registered user information.
$user_info = ($dash_status) ? $db_kiosk_llm_obj->obtain_user_information($_SESSION["acc_auth_key"]) : "none";

// Obtain the available menu categories by prep station.
$menu_categories = $db_kiosk_llm_obj->obtain_available_food_item_info("category");

?>

<!DOCTYPE html>
<html>
<head>
    <!-- Page info. -->
	<title>AI-driven LoRa/LLM Kiosk Dashboard</title>
    <!-- Site settings. -->
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">

	<!-- link to site icon -->
	<link rel="icon" type="image/png"  href="assets/img/site_icon.png">

	<!-- Add jQuery -->
	<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.7.1/jquery.min.js"></script>

	<!-- Add dashboard style file -->
	<link rel="stylesheet" type="text/css" href="assets/style/dashboard_style.css"></link>
	
	<!-- Add Google fonts. -->
	<link rel="preconnect" href="https://fonts.googleapis.com">
	<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
	<link href="https://fonts.googleapis.com/css2?family=Codystar:wght@300;400&display=swap" rel="stylesheet">
	<link href="https://fonts.googleapis.com/css2?family=Comic+Relief:wght@400;700&display=swap" rel="stylesheet">
	<link href="https://fonts.googleapis.com/css2?family=Anton&display=swap" rel="stylesheet">
	<link href="https://fonts.googleapis.com/css2?family=Pacifico&display=swap" rel="stylesheet">
</head>

<body> 

<div class="main">

<!-- Show LLM-generated text real-time via the ollama-php/client library. -->
<div class="LLM_interface"> <iframe src=""></iframe> </div>

<!-- Start menus / deals. -->
<div def="menu_show" <?php $cat_num = 0; foreach($menu_categories as $cat){ $cat_num++; echo 'cat_'.$cat_num.'="'.$cat.'" '; } ?> >

<section class="header"><h2>Generic Menus / Deals</h2></section>
<section class="menu_container" id="generic_menus">
<?php
// Obtain generic menu / deal information from the database.
$generic_menu_information = $db_kiosk_llm_obj->obtain_menu_deal_information("generic", "all");
if($generic_menu_information != false){
	$menu_num = 0;
	foreach($generic_menu_information as $menu){
		$menu_num++;
		// Get the food item list for the given menu / deal.
		$food_item_list = '';
		foreach($menu["item_list"] as $item){
			$food_item_list .= '<button>'.$item["name"].' <span> x '.$item["amount"].'</span></button>';
		}
		// Show each generic menu / deal.
		echo '
				<div>
				<img src="assets/img/generic_menu_'.$menu_num.'.png" />
				<article>
				<h2 def="order_code">'.$menu["order_tag"].'</h2>
				<p def="menu_definition">'.$menu["definition"].'</p>
				'.$food_item_list.'
				<p def="menu_discount">Discount: <span def="d_percentage">'.$menu["discount_percentage"].'%</span></p>
				<p def="menu_price">Price: <span def="t_price">$'.$menu["total_price"].'</span> <span def="d_price">$'.$menu["discounted_price"].'</span></p>
				</article>
				</div>		
	         ';
	}
}else{
	echo "<h2>Database error!</h2>";
}
?>
</section>

<section class="header" def="user_specific"><h2>User-specific Menus / Deals</h2></section>
<section class="menu_container" id="user_specific_menus">
<?php
if($dash_status == false){
	echo '
			<div>
			<img src="assets/img/specific_menu_not_signed.png" />
			<article>
			<p def="menu_definition">🚫 Please sign in to enable the customer dashboard to generate your user-specific menus / deals!</p>
			</article>
			</div>		
	     ';
}else{
	// Obtain user-specific menu / deal information from the database, that of the current user account if signed in successfully.
	$user_specific_menu_information = $db_kiosk_llm_obj->obtain_menu_deal_information($_SESSION["acc_auth_key"], "all");
	if($user_specific_menu_information != false){
		if($user_specific_menu_information[0] == NULL){
			echo '
					<div>
					<img src="assets/img/specific_menu_pending.png" />
					<article>
					<p def="menu_definition">⚠️ Please utilize the customer dashboard to select an available LLM and generate your user-specific menus / deals!</p>
					</article>
					</div>		
				 ';			
		}else{
			// Show each user-specific menu / deal if the user employed an available LLM to produce menus / deals based on the selected deal preference.
			$menu_num = 0;
			foreach($user_specific_menu_information as $menu){
				$menu_num++;
				// Get the food item list for the given menu / deal.
				$food_item_list = '';
				foreach($menu["item_list"] as $item){
					$food_item_list .= '<button>'.$item["name"].' <span> x '.$item["amount"].'</span></button>';
				}
				// Show.
				echo '
						<div>
						<img src="assets/img/specific_menu_'.$menu_num.'.png" />
						<article>
						<h2 def="order_code">'.$menu["order_tag"].'</h2>
						<p def="menu_definition">'.$menu["definition"].'</p>
						'.$food_item_list.'
						<p def="menu_discount">Discount: <span def="d_percentage">'.$menu["discount_percentage"].'%</span></p>
						<p def="menu_price">Price: <span def="t_price">$'.$menu["total_price"].'</span> <span def="d_price">$'.$menu["discounted_price"].'</span></p>
						</article>
						</div>		
					 ';			
				}
		}
	}else{
		echo "<h2>Database error!</h2>";
	}
}
?>
</section>
</div>
<!-- End menus / deals. -->

<!-- Start customer configurations. -->
<div def="settings_show">
<section>
<h2>Active LLM</h2>
<div>
<label>Change: </label>
<?php
// Print the available models and the selected LLM that of the current user account if signed in successfully.
$available_LLMs = ["deepseek-r1:8b", "deepseek-r1:7b", "deepseek-r1:1.5b", "gemma3:4b", "gemma3:1b", "llama3.2:3b", "qwen3:4b", "phi4-mini"];
$llm_change_disable = ($dash_status) ? "" : "disabled";
$llm_option_html_content = '<select name="llm_models" id="llm_models" '.$llm_change_disable.'>';
foreach($available_LLMs as $llm){
	if($dash_status && $llm == $user_info["activated_LLM"]){
		$llm_option_html_content .= '<option value="'.$llm.'" selected>'.$llm.'</option>';
	}else{
		$llm_option_html_content .= '<option value="'.$llm.'">'.$llm.'</option>';
	}
}
$llm_option_html_content .= '</select>';
echo $llm_option_html_content;
?>
</div>
</section>

<section>
<?php
// According to the dashboard status, change the dashboard account interface.
if($dash_status){
	// Generate the deal preference options based on the given menu categories.
	$deal_preference_select = '<select name="deal_preference_change" id="deal_preference_change">';
	foreach($menu_categories as $cat){
		if($cat == $user_info["menu_preference"]){
			$deal_preference_select .= '<option value="'.$cat.'" selected>'.$cat.'</option>';
		}else{
			$deal_preference_select .= '<option value="'.$cat.'">'.$cat.'</option>';
		}
	}
	$deal_preference_select .= "</select>";
	// Show the necessary account and ongoing task information.
	$dashboard_acc_content = '<div class="user_info">
							  <h2>Invoice</h2>
							  <section><p>Customer Name:</p><span>'.$user_info["firstname"].' '.$user_info["lastname"].'</span></section>
							  <section><p>Customer Email:</p><span>'.$user_info["email"].'</span></section>
							  <section><p>Registered Card:</p><span>'.$user_info["card_number"].'</span></section>
							  <section><p>Card Holder Name:</p><span>'.$user_info["card_holder_name"].'</span></section>
							  <section><p>Card Exp. Date:</p><span>'.$user_info["card_exp_date"].'</span></section>
							  <h2>User-specific Deals</h2>
							  <section><p>Enabled LLM:</p><span id="current_activated_llm">'.$user_info["activated_LLM"].'</span></section>
							  <section><p>Deal Preference:</p>'.$deal_preference_select.'</section>
							  <button def="generate_new_deals">Generate New <span>LLM</span> Deals</button>							  
							  <h2>Track Order</h2>
							  <section><p>Kiosk Status:</p><span id="current_kiosk_status">🔄</span></section>
							  <section><p>Order Type:</p><span id="current_order_type">🔄</span></section>
							  <section><p>Order Tag:</p><span id="current_order_tag">🔄</span></section>
							  <section><p>Status:</p><span id="current_order_status">🔄</span></section>
							  <section><p>Updated:</p><span id="current_order_upt_time">🔄</span></section>
                              <h2>Review LoRa Task</h2>
							  <section><p>Code:</p><span id="current_lora_code">🔄</span></section>							  
							  <section><p>Type:</p><span id="current_lora_type">🔄</span></section>							  
							  <section><p>Details:</p><textarea id="current_lora_details">🔄</textarea></section>							  
							  <section><p>Status:</p><textarea id="current_lora_status">🔄</textarea></section>							  
							  <section><p>Device ID:</p><span id="current_lora_device_id">🔄</span></section>
							  <section><p>Gateway ID:</p><span id="current_lora_gateway_id">🔄</span></section>
							  <section><p>Requested:</p><span id="current_lora_upt_time">🔄</span></section>  
							  <h2>Account</h2>
							  <section><p>Username:</p><span>'.$user_info["username"].'</span></section>
							  <section><p>Authentication Code:</p><span>'.$user_info["authentication_key"].'</span></section>
							  <button def="logout">Logout</button>
                              </div>';
}else{
	// Show the sign in form.
	$dashboard_acc_content = '<div class="sign_nav">
							  <button class="enabled" def="sign_in">Sign In</button>
							  <button def="sign_up">Sign Up</button>
							  </div>
							  <div class="sign_content">
							  <section><label>Username: </label><input id="username" placeholder="kutluhan123"></input></section>
							  <section><label>Password: </label><input id="password" type="password" placeholder="password1234"></input></section>
							  <button id="submit_but_sign_in">Login</button>
							  </div>';
}
echo $dashboard_acc_content;
?>
</section>
</div>
</div>
<!-- End customer configurations. -->

<!-- Add the required scripts. -->
<script src="assets/script/dashboard_config.js"></script>
<?php
// If the customer signs in successfully, obtain and display necessary status updates from the database automatically.
echo '<script src="assets/script/dashboard_status_update.js"></script>';
?>

</body>
<html>