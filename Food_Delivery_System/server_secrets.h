char ssid[] = "<SSID>";          // your network SSID (name)
char pass[] = "<PASSWORD>";      // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key Index number (needed only for WEP)

// Define the required webhook information, hosted by LattePanda Mu (N305).
char server[] = "192.168.1.21";
String application = "/ai_driven_lora_kiosk/assets/food_delivery_system_conn.php";