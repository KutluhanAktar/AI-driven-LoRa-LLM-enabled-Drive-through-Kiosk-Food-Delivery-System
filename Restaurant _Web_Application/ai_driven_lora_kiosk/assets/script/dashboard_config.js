// Define required global variables.
let user_log_info;

// Obtain the available menu items by slot.
const available_menu_categories = {
	cat_1: $('div[def="menu_show"]').attr("cat_1"),
	cat_2: $('div[def="menu_show"]').attr("cat_2"),
	cat_3: $('div[def="menu_show"]').attr("cat_3"),
	cat_4: $('div[def="menu_show"]').attr("cat_4"),
	cat_5: $('div[def="menu_show"]').attr("cat_5"),
	cat_6: $('div[def="menu_show"]').attr("cat_6")
};


// Obtain the LLM version requested by the current customer to update the database.
$("#llm_models").on("change", function(event){
	let selected_LLM = $(this).val();
	// Update the user (account) information according to the passed LLM.
	$.ajax({
		url: "/ai_driven_lora_kiosk/assets/dashboard_status_update.php?update_llm=" + selected_LLM,
		type: "GET",
		success: (response) => {
			// Do nothing.
		}
	});	
});

// Obtain the deal preference requested by the current customer to update the database.
$("#deal_preference_change").on("change", function(event){
	let selected_deal_preference = $(this).val();
	// Update the user (account) information according to the passed deal preference.
	$.ajax({
		url: "/ai_driven_lora_kiosk/assets/dashboard_status_update.php?update_preference=" + selected_deal_preference,
		type: "GET",
		success: (response) => {
			// Do nothing.
		}
	});		
});

// According to the requested option, generate the sign-in or sign-up interface.
function generate_sign_interface(_opt){
	let interface_content = "";
	switch(_opt){
		case "sign_up":
			interface_content = '   <section><label>Firstname: </label><input id="firstname" placeholder="Kutluhan"></input></section> \
									<section><label>Lastname: </label><input id="lastname" placeholder="Aktar"></input></section> \
									<section><label>Email: </label><input id="email" placeholder="contact@kutluhanaktar.com"></input></section> \
									<section><label>Username: </label><input id="username" placeholder="kutluhan123"></input></section> \
									<section><label>Password: </label><input id="password" type="password" placeholder="password1234"></input></section> \
									<section class="radio"> \
									<label>Deal preference: </label> \
									<div> \
									<label> <input type="radio" name="preference" value="' + available_menu_categories["cat_1"] + '" checked> ' + available_menu_categories["cat_1"] + ' </label> \
									<label> <input type="radio" name="preference" value="' + available_menu_categories["cat_2"] + '"> ' + available_menu_categories["cat_2"] + ' </label> \
									<label> <input type="radio" name="preference" value="' + available_menu_categories["cat_3"] + '"> ' + available_menu_categories["cat_3"] + ' </label> \
									<label> <input type="radio" name="preference" value="' + available_menu_categories["cat_4"] + '"> ' + available_menu_categories["cat_4"] + ' </label> \
									<label> <input type="radio" name="preference" value="' + available_menu_categories["cat_5"] + '"> ' + available_menu_categories["cat_5"] + ' </label> \
									<label> <input type="radio" name="preference" value="' + available_menu_categories["cat_6"] + '"> ' + available_menu_categories["cat_6"] + ' </label> \
									</div> \
									</section> \
									<h2>Add new credit / debit card</h2> \
									<section><label>Card holder name: </label><input id="card_holder_name" placeholder="Kutluhan Aktar"></input></section> \
									<section><label>Card number: </label><input id="card_number" placeholder="378282246310005"></input></section> \
									<section><label>Card expiration date: </label><input type="date" id="card_exp_date"></input></section> \
									<section><label>Card CVV/CVC: </label><input id="card_cvv" placeholder="111"></input></section> \
									<button id="submit_but_sign_up">Submit</button>	';
		break;
		case "sign_in":
			interface_content = '   <section><label>Username: </label><input id="username" placeholder="kutluhan123"></input></section>\
									<section><label>Password: </label><input id="password" type="password" placeholder="password1234"></input></section>\
									<button id="submit_but_sign_in">Login</button> ';
		break;
        default:
			// Do nothing.
        break;		
	}
	return interface_content;
}

// Obtain the requested interface option — sign-in and sign-up.
$(".sign_nav > button").on("click", function(event){
	// Check whether the clicked button has already been enabled or not.
	if(!$(this).hasClass("enabled")){
		let selected_interface = $(this).attr("def");
		// Change the account interface HTML content accordingly.
		$(".sign_content").html(generate_sign_interface(selected_interface));
		// Notify the user.
		$(".sign_nav > button").toggleClass("enabled");
	}
});

// After getting the required information, open the given customer (user) account.
$(".sign_content").on("click", "#submit_but_sign_in", function(event){
	// Check the provided information.
	let login_info = {
		username: $("#username").val(),
		_password: $("#password").val()
	};	
	
	if(login_info["username"] == "" || login_info["_password"] == ""){
		alert("⚠️ Please provide all required fields to login!");
	}else{
		// If the user provided all the required information to login:
		$.ajax({
			url: "/ai_driven_lora_kiosk/assets/account_config.php",
			type: "POST",
			data: {"login_info" : login_info},
			success: (response) => {
				if(response.includes("found successfully")){
					window.location.reload();
				}else{
					alert(response);
				}
			}
		});		
	}
});

// After getting the required information, create a new customer (user) account.
$(".sign_content").on("click", "#submit_but_sign_up", function(event){
	// Check the provided information.
	let user_info = {
		firstname: $("#firstname").val(),
		lastname: $("#lastname").val(),
		email: $("#email").val(),
		username: $("#username").val(),
		_password: $("#password").val(),
		menu_preference: $('input[name="preference"]:checked').val(),
		card_holder_name: $("#card_holder_name").val(),
		card_number: $("#card_number").val(),
		card_exp_date: $("#card_exp_date").val(),
		card_cvv: $("#card_cvv").val()
	};
	
	let entered_info_num = Object.keys(user_info).length;
	
	$.each(user_info, (info, val) => {
		if(val == "") entered_info_num--;
	});
	
	if(entered_info_num != 10){
		alert("⚠️ Please provide all required fields to create an account!");
	}else{
		// If the user provided all the required information to create a new account:
		$.ajax({
			url: "/ai_driven_lora_kiosk/assets/account_config.php",
			type: "POST",
			data: {"user_info" : user_info},
			success: (response) => {
				if(response.includes("successfully created")){
					if(confirm(response)){ window.location.reload(); } else{ window.location.reload(); }
				}else{
					alert(response);
				}
			}
		});		
	}
});

// If the user requested, logout from the current account and terminate authentication sessions.
$('div[def="settings_show"] > section:nth-child(2)').on("click", '.user_info > button[def="logout"]', () => {
	if(!window.location.href.includes("?")) window.location.href += "?logout";
});

// Open the LLM interface to trigger the generate_LLM_based_user_specific_deals.php file in order to generate user-specific menus / deals based on the selected LLM and the provided customer configurations.
$('div[def="settings_show"] > section:nth-child(2)').on("click", '.user_info > button[def="generate_new_deals"]', () => {
	let llm_interface = $(".LLM_interface");
	llm_interface.children("iframe").attr("src", "generate_LLM_based_user_specific_deals.php");
	if(!llm_interface.hasClass("enabled")) llm_interface.addClass("enabled");
});

// Close the LLM interface if the user requested.
$(".LLM_interface").on("click", function(event){
	if($(this).hasClass("enabled")) $(this).removeClass("enabled");
	$(this).children("iframe").attr("src", "");
	window.location.reload();
});
