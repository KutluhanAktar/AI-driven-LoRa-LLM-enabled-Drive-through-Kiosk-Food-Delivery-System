CREATE TABLE `lora_user_task_log`(		
    log_id int AUTO_INCREMENT PRIMARY KEY NOT NULL,
    device_id varchar(255),
    application_id varchar(255),
    gateway_id varchar(255),
    received_at varchar(255),
    frm_payload varchar(255),
    decoded_payload varchar(255),
    task_type varchar(255),
    task_details varchar(255),
    task_status varchar(255),
    server_time varchar(255)
);

CREATE TABLE `user_info`(		
    id int AUTO_INCREMENT PRIMARY KEY NOT NULL,
    firstname varchar(255),
    lastname varchar(255),
    email varchar(255),
    username varchar(255),
    password varchar(255),
    menu_preference varchar(255),
    card_holder_name varchar(255),
    card_number varchar(255),
    card_exp_date varchar(255),
    card_cvv varchar(255),
    activated_LLM varchar(255),
    authentication_key varchar(255)
);

CREATE TABLE `menu_info_by_user`(		
    id int AUTO_INCREMENT PRIMARY KEY NOT NULL,
    authentication_key varchar(255),
    deal_1  varchar(1200),
    deal_2  varchar(1200),
    deal_3  varchar(1200),
    deal_4  varchar(1200),
    deal_5  varchar(1200),
    deal_6  varchar(1200),
    deal_7  varchar(1200),
    deal_8  varchar(1200),
    deal_9  varchar(1200),
    deal_10 varchar(1200),
    deal_11 varchar(1200),
    deal_12 varchar(1200)
);

INSERT INTO `menu_info_by_user` (`authentication_key`, `deal_1`, `deal_2`, `deal_3`, `deal_4`, `deal_5`, `deal_6`, `deal_7`, `deal_8`, `deal_9`, `deal_10`, `deal_11`, `deal_12`)
VALUES("generic",
	   '{"order_tag": "f001", "definition": "Friday to Friday: Double Hamburger with French Fries", "item_list": [{"name": "Hamburger", "category": "Burgers", "amount": 2},{"name": "French Fries", "category": "Side Dishes", "amount": 4}], "total_price": 32, "discount_percentage": 5, "discounted_price": 30.4, "prep_station_road_map": "2%0%0%0%4%0"}',
	   '{"order_tag": "f002", "definition": "Friday to Friday: Double Cheeseburger with Onion Rings", "item_list": [{"name": "Cheeseburger", "category": "Burgers", "amount": 2},{"name": "Onion Rings", "category": "Side Dishes", "amount": 6}], "total_price": 50, "discount_percentage": 15, "discounted_price": 42.5, "prep_station_road_map": "2%0%0%0%6%0"}',
	   '{"order_tag": "f003", "definition": "Thursday to Thursday: Vegetarian Burger Special", "item_list": [{"name": "Veggie", "category": "Burgers", "amount": 2},{"name": "French Fries", "category": "Side Dishes", "amount": 4}], "total_price": 26, "discount_percentage": 8, "discounted_price": 23.92, "prep_station_road_map": "2%0%0%0%4%0"}',
	   '{"order_tag": "f004", "definition": "Thursday to Thursday: Taco Special with Brownie", "item_list": [{"name": "Taco", "category": "Mexican", "amount": 2},{"name": "Brownie", "category": "Desserts", "amount": 2}], "total_price": 30, "discount_percentage": 6, "discounted_price": 28.2, "prep_station_road_map": "0%2%0%0%0%2"}',
	   '{"order_tag": "f005", "definition": "Thursday to Thursday: Burrito Special with Cheesecake", "item_list": [{"name": "Burrito", "category": "Mexican", "amount": 2},{"name": "Cheesecake", "category": "Desserts", "amount": 2}], "total_price": 32, "discount_percentage": 8, "discounted_price": 29.44, "prep_station_road_map": "0%2%0%0%0%2"}',
	   '{"order_tag": "f006", "definition": "Thursday to Thursday: Tamales Special with Carrot Cake", "item_list": [{"name": "Tamales", "category": "Mexican", "amount": 3},{"name": "Carrot Cake", "category": "Desserts", "amount": 3}], "total_price": 48, "discount_percentage": 10, "discounted_price": 43.2, "prep_station_road_map": "0%3%0%0%0%3"}',
	   '{"order_tag": "f007", "definition": "Wednesday to Wednesday: Lasagna Triple Special", "item_list": [{"name": "Lasagna", "category": "Italian", "amount": 3}], "total_price": 36, "discount_percentage": 5, "discounted_price": 34.2, "prep_station_road_map": "0%0%3%0%0%0"}',
	   '{"order_tag": "f008", "definition": "Wednesday to Wednesday: Risotto Triple Special", "item_list": [{"name": "Risotto", "category": "Italian", "amount": 3}], "total_price": 45, "discount_percentage": 10, "discounted_price": 40.5, "prep_station_road_map": "0%0%3%0%0%0"}',
	   '{"order_tag": "f009", "definition": "Wednesday to Wednesday: Pizza Triple Special", "item_list": [{"name": "Pizza", "category": "Italian", "amount": 3}], "total_price": 60, "discount_percentage": 12, "discounted_price": 52.8, "prep_station_road_map": "0%0%3%0%0%0"}',
	   '{"order_tag": "f010", "definition": "Sea Weekend Special: Shrimp Tempura", "item_list": [{"name": "Shrimp Tempura", "category": "Seafood", "amount": 10}], "total_price": 80, "discount_percentage": 15, "discounted_price": 68, "prep_station_road_map": "0%0%0%10%0%0"}',
	   '{"order_tag": "f011", "definition": "Sea Weekend Special: Salmon and Chips", "item_list": [{"name": "Salmon", "category": "Seafood", "amount": 5},{"name": "French Fries", "category": "Side Dishes", "amount": 10}], "total_price": 130, "discount_percentage": 20, "discounted_price": 104, "prep_station_road_map": "0%0%0%5%10%0"}',
	   '{"order_tag": "f012", "definition": "Sea Weekend Special: Lobster", "item_list": [{"name": "Lobster", "category": "Seafood", "amount": 10}], "total_price": 150, "discount_percentage": 30, "discounted_price": 105, "prep_station_road_map": "0%0%0%10%0%0"}'
	  );

CREATE TABLE `current_kiosk_user`(
	id int AUTO_INCREMENT PRIMARY KEY NOT NULL,
    authentication_key varchar(255),
    last_change varchar(255),
    account_status varchar(255),
    kiosk_auth_status varchar(255)
);

INSERT INTO `current_kiosk_user` (authentication_key, last_change, account_status, kiosk_auth_status)
VALUES ("none", "none", "none", "none");

CREATE TABLE `food_delivery_system_log`(
	id int AUTO_INCREMENT PRIMARY KEY NOT NULL,
    task_type varchar(255),
    task_objectives varchar(255),
	task_status varchar(255),
	authentication_key varchar(255),
	order_tag varchar(255),
	server_time varchar(255)
);

CREATE TABLE `food_items_by_prep_station`(
	id int AUTO_INCREMENT PRIMARY KEY NOT NULL,
    station_1 varchar(255),
    station_2 varchar(255),
    station_3 varchar(255),
    station_4 varchar(255),
    station_5 varchar(255),
    station_6 varchar(255)
);

INSERT INTO `food_items_by_prep_station` (station_1, station_2, station_3, station_4, station_5, station_6)
VALUES ('{"category":"Burgers", "items":[{"name": "Hamburger", "price": "12"},{"name": "Cheeseburger", "price": "10"},{"name": "Veggie", "price": "9"}]}',
        '{"category":"Mexican", "items":[{"name": "Taco", "price": "12"},{"name": "Burrito", "price": "12"},{"name": "Tamales", "price": "12"}]}',
		'{"category":"Italian", "items":[{"name": "Lasagna", "price": "12"},{"name": "Risotto", "price": "15"},{"name": "Pizza", "price": "20"}]}',
		'{"category":"Seafood", "items":[{"name": "Shrimp Tempura", "price": "8"},{"name": "Salmon", "price": "22"},{"name": "Lobster", "price": "15"}]}',
		'{"category":"Side Dishes", "items":[{"name": "French Fries", "price": "2"},{"name": "Onion Rings", "price": "5"},{"name": "Chicken Nuggets", "price": "6"}]}',
		'{"category":"Desserts", "items":[{"name": "Brownie", "price": "3"},{"name": "Cheesecake", "price": "4"},{"name": "Carrot Cake", "price": "4"}]}'
	   );

