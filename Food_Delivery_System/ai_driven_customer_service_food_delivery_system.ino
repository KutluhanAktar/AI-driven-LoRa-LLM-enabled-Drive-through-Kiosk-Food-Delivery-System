         /////////////////////////////////////////////  
        //   AI-driven LoRa & LLM-enabled Kiosk    //
       //         & Food Delivery System          //
      //             ---------------             //
     //          (Arduino Nicla Vision)         //           
    //             by Kutluhan Aktar           // 
   //                                         //
  /////////////////////////////////////////////

//
// A research project on developing a full-fledged drive-through kiosk and food delivery system, utilizing LLMs to generate user-specific menus/deals.
//
// For more information:
// https://www.kutluhanaktar.com/projects/AI_driven_LoRa_LLM_enabled_Drive_through_Kiosk_Food_Delivery_System
//
//
// Connections
// Arduino Nicla Vision :  
//                                Nema 17 (17HS3401) Stepper Motor w/ A4988 Driver Module [Motor 1]
// 3.3V    ------------------------ VDD
// GND     ------------------------ GND
// PE_12   ------------------------ DIR
// PE_13   ------------------------ STEP
//                                Nema 17 (17HS3401) Stepper Motor w/ A4988 Driver Module [Motor 2]
// 3.3V    ------------------------ VDD
// GND     ------------------------ GND
// PE_14   ------------------------ DIR
// PE_11   ------------------------ STEP
//                                Nema 17 (17HS3401) Stepper Motor w/ A4988 Driver Module [Motor 3]
// 3.3V    ------------------------ VDD
// GND     ------------------------ GND
// PG_12   ------------------------ DIR
// PB_9    ------------------------ STEP
//                                Control Button (A)
// PC_4    ------------------------ +
//                                Control Button (B)
// PF_13   ------------------------ +
//                                Control Button (C)
// PF_3    ------------------------ +
//                                Control Button (D)
// PB_8    ------------------------ +
//                                Micro Switch [A]
// PA_9    ------------------------ +
//                                Micro Switch [B]
// PA_10   ------------------------ +


// Include the required libraries:
#include <WiFi.h>
#include "camera.h"
#include "gc2145.h"
#include <ea_malloc.h>
#include "VL53L1X.h"

// Include the Edge Impulse FOMO model converted to an Arduino library and the necessary built-in Edge Impulse image (frame) processing functions.
#include <LoRa_LLM-enabled_Drive-through_Kiosk_AptilTag_Detection_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

// Define the required parameters to run an inference with the provided Edge Impulse FOMO model.
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS    320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS    240
#define EI_CAMERA_RAW_FRAME_BYTE_SIZE      2
static uint8_t *ei_camera_capture_out = NULL;
static uint8_t *ei_camera_frame_mem;
static uint8_t *ei_camera_frame_buffer; // 32-byte aligned
// Define the function required to create aligned pointers.
#define ALIGN_PTR(p,a)   ((p & (a-1)) ?(((uintptr_t)p + a) & ~(uintptr_t)(a-1)) : p)

// Define the individual class names that of the provided Edge Impulse model.
int classes_item_num = 6;
String classes[] = {"station_1", "station_2", "station_3", "station_4", "station_5", "station_6"};

// Include the required server information.
#include "server_secrets.h"

// Initialize the WiFiClient object.
WiFiClient client; /* WiFiSSLClient client; */

// Define the required camera settings for the 2-megapixel CMOS camera (GC2145).
GC2145 galaxyCore;
Camera cam(galaxyCore);

// Define the camera frame buffer.
FrameBuffer fb;

// Define the onboard Time of Flight (Distance - ToF) sensor (VL53L1CBV0FY) object.
VL53L1X proximity;

// Define the required control button and micro switch settings.
#define control_button_A  PC_4
#define control_button_B  PF_13
#define control_button_C  PF_3
#define control_button_D  PB_8
#define micro_switch_A    PA_9
#define micro_switch_B    PA_10

// Define the required stepper motor (w/ A4988 driver) configurations by creating a struct — stepper_config.
struct stepper_config{
  #define m_num 3
  PinName _pins[m_num][2] = {{PE_12, PE_13}, {PE_14, PE_11}, {PG_12, PB_9}}; // (DIR, STEP)
  // Assign the required revolution and speed variables based on axes — X, Y, and Z (arm).
  int stepsPerRevolution = 200;
  int x_y_speed = 16000;
  int z_speed = 12000;
  // Define stepper motor homing configurations.
  int home_step_number[m_num] = {1, 1, 3};
  // Assign stepper motor tasks based on the associated part.
  int x_core = 0, y_core = 1, z_arm = 2;
  // Assign the required preperation time per requested food item.
  int wait_per_food_item = 3000;
  // Assign the required time for customers to get all of the food items from the tray.
  int customer_wait_time = 10000;
};

// Define the data holders:
struct stepper_config stepper_config;
int current_manual_task = 0;
String detected_class = "";

void setup(){
  Serial.begin(9600);

  // According to the Edge Impulse documentation, it is advised to allocate 288 kB as in the line below.
  malloc_addblock((void*)0x30000000, 288 * 1024);

	// Activate the given stepper motor DIR and STEP pins connected to the A4988 driver module. 
  for(int i = 0; i < m_num; i++){ pinMode(stepper_config._pins[i][0], OUTPUT); pinMode(stepper_config._pins[i][1], OUTPUT); }

  // Register pin configurations.
  pinMode(control_button_A, INPUT_PULLUP); pinMode(control_button_B, INPUT_PULLUP); pinMode(control_button_C, INPUT_PULLUP); pinMode(control_button_D, INPUT_PULLUP);
  pinMode(micro_switch_A, INPUT); pinMode(micro_switch_B, INPUT);
  pinMode(LEDR, OUTPUT); pinMode(LEDG, OUTPUT); pinMode(LEDB, OUTPUT);
  adjustColor(0,0,0);
  
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  WiFi.begin(ssid, pass);
  // Attempt to connect to the Wi-Fi network:
  while(WiFi.status() != WL_CONNECTED){
    // Wait for the connection:
    delay(500);
    Serial.print(".");
  }
  // If connected to the network successfully:
  Serial.println("Connected to the Wi-Fi network successfully!");

  // Define the pixel format and the FPS settings.
  // Then, initialize the GC2145 camera.
  /* I initially collected samples with 320x320 resolution. However, I recommend 320x240 resolution for running inferences to avoid tensor allocation issues. */
  if(!cam.begin(CAMERA_R320x240, CAMERA_RGB565, 30)) { // CAMERA_R320x240 [model], CAMERA_R320x320 [samples]
    Serial.println("GC2145 camera: initialization failed!");
  }else{
    Serial.println("GC2145 camera: initialized successfully!");
  }

  // Define the required settings to initialize the onboard ToF sensor.
  Wire1.begin();
  Wire1.setClock(400000); // use 400 kHz I2C
  proximity.setBus(&Wire1);
  if(!proximity.init()) { Serial.println("ToF sensor: initialization failed!"); while (1); }
  else{ Serial.println("ToF sensor: initialized successfully!"); }
  // Adjust ToF sensor data reading configurations.
  proximity.setDistanceMode(VL53L1X::Long);
  proximity.setMeasurementTimingBudget(50000);
  proximity.startContinuous(50);

}

void loop(){
  // Enable the real-time communication channel with the web application through the provided webhook.
  String web_data_packet = web_app_conn_channel("listen", "");
  // Follow the requested prep station road map according to the received data packet if the delivery system is not idle.
  if(web_data_packet != "failed" || web_data_packet != "idle"){
    // Derive the requested task type from the fetched data packet.
    String task_type = web_data_packet.substring(0, web_data_packet.indexOf("&"));
    // Check the task type validity.
    if(task_type == "order_generic" || task_type == "order_specific"){
      // Obtain the requested prep station road map (task objectives) from the fetched data packet.
      String task_objectives = web_data_packet.substring(web_data_packet.indexOf("&") + 1, web_data_packet.indexOf("&", web_data_packet.indexOf("&") + 1));
      Serial.println("Requested Task Type: " + task_type + "\t\t" + "Road Map (Objectives): " + task_objectives + "\n");
      // Using the given delimiters (%), decode the task objectives string to obtain the amount of food items required from each prep station.
      int del_1 = task_objectives.indexOf("%"), del_2 = task_objectives.indexOf("%", del_1 + 1), del_3 = task_objectives.indexOf("%", del_2 + 1), del_4 = task_objectives.indexOf("%", del_3 + 1), del_5 = task_objectives.indexOf("%", del_4 + 1);
      int road_map[classes_item_num] = {task_objectives.substring(0, del_1).toInt(), task_objectives.substring(del_1+1, del_2).toInt(), task_objectives.substring(del_2+1, del_3).toInt(), task_objectives.substring(del_3+1, del_4).toInt(), task_objectives.substring(del_4+1, del_5).toInt(), task_objectives.substring(del_5+1).toInt(), 1};
      
      // After obtaining the necessary food item list, home the food delivery platform X-axis and Y-axis.
      h_bot_mechanism_home(true, true);
      // Then, position the food delivery platform to make it ready to collect the requested food items from the associated prep stations.
      h_bot_mechanism_move(220, 10, "Y", "up");
      // Depending on the retrieved station road map, move the food delivery platform to the menu-associated prep stations by utilizing their unique AprilTags.
      int passed_station = 0;
      for(int i=0; i<classes_item_num; i++){
        /*
          The retrieved prep station road map shows the amount of food items required from a station to complete the ordered generic or user-specific menu / deal.
          Find prep stations by identifying assigned unique AprilTags by running the Edge Impulse FOMO object detection model.
          Then, depending on the amount of food items required from a prep station, wait before navigating the food delivery platform to the subsequent prep station.
        */
        // Display the required food item numbers by prep station for debugging.  
        Serial.print("Prep Station ["); Serial.print(i+1); Serial.print("] ==> "); Serial.print("Has "); Serial.print(road_map[i]); Serial.println(" Food Items!");
        // According to the prep station road map, navigate only to the prep stations which has food items from the ordered menu.
        if(road_map[i] > 0){
          bool home_x_again = (passed_station == 0) ? false : true;
          move_the_delivery_platform_to_station(i, road_map[i], home_x_again);
          passed_station++;
        }
      }
      delay(1000);

      // After successfully retrieving all of the food items of the purchased menu / deal from prep stations, move the food delivery platform to the drive-through kiosk's vehicle platform
      // and swivel the tray-carrying arm to the customer waiting on the vehicle platform.
      h_bot_mechanism_home(true, true);
      h_bot_mechanism_move(450, 10, "X", "right"); delay(500);
      z_arm_move(stepper_config.stepsPerRevolution/2, 5, "CW"); delay(500);
      h_bot_mechanism_move(140, 10, "X", "right"); delay(500);
      // Wait the customer to get all of the food items presented by the tray.
      delay(stepper_config.customer_wait_time);

      // Then, inform the web application of the order completion to make the food delivery system idle.
      web_app_conn_channel("update", "completed");
      delay(2000);
      // Finally, return the food delivery platform and the tray-carrying arm to their default positions.
      h_bot_mechanism_move(150, 10, "Y", "up"); delay(500);
      h_bot_mechanism_move(450, 10, "X", "left"); delay(500);
      z_arm_move(stepper_config.stepsPerRevolution/2, 10, "CW"); delay(500);
    }
  }
  
  /*
    Manual tasks assigned to the control buttons on the food delivery platform — A, B, C, and D.
    1) Manual H-Bot mechanism movement debugging.
    2) Manual tray-carrying arm movement debugging.
    3) Configurations for the prep station AprilTag sample image collection.
    4) Manual H-Bot mechanism homing — X-axis and Y-axis.
    ***The current manual task option can be changed via the control button D and is distinguished by the RGB LED color.***

    M) Manual Edge Impulse model inference running (debugging).
    ***An inference can be manually initiated via the control button C.***
    
  */
  if(!digitalRead(control_button_C)){
    run_inference();
    // Clear the detected label to avoid errors afterward.
    detected_class = "";
  }
  if(!digitalRead(control_button_D)){
    current_manual_task++;
    if(current_manual_task > 4) current_manual_task = 0;
    delay(1000);
  }
  switch(current_manual_task){
    // Home X-axis and Y-axis (manually)
    case 0:
      adjustColor(0,1,1);
      if(!digitalRead(control_button_A)) h_bot_mechanism_home(true, false);
      if(!digitalRead(control_button_B)) h_bot_mechanism_home(false, true);
    break;
    // Move X-axis (manually)
    case 1:
      adjustColor(1,0,0);
      if(!digitalRead(control_button_A)) h_bot_mechanism_move(stepper_config.stepsPerRevolution/2, 10, "X", "right");
      if(!digitalRead(control_button_B)) h_bot_mechanism_move(stepper_config.stepsPerRevolution/2, 10, "X", "left");
      /* Debug micro switch pin responses. */
      if(!digitalRead(micro_switch_A)) Serial.println("Limit Switch [A] => Working!\n");
      if(!digitalRead(micro_switch_B)) Serial.println("Limit Switch [B] => Working!\n");
    break;
    // Move Y-axis (manually)
    case 2:
      adjustColor(0,1,0);
      if(!digitalRead(control_button_A)) h_bot_mechanism_move(stepper_config.stepsPerRevolution/2, 10, "Y", "up");
      if(!digitalRead(control_button_B)) h_bot_mechanism_move(stepper_config.stepsPerRevolution/2, 10, "Y", "down");
    break;
    // Move the arm (manually)
    case 3:
      adjustColor(0,0,1);
      if(!digitalRead(control_button_A)) z_arm_move(stepper_config.stepsPerRevolution/2, 10, "CW");
      if(!digitalRead(control_button_B)) z_arm_move(stepper_config.stepsPerRevolution/2, 10, "CCW");
    break;
    // Capture prep station AprilTag (unique for each station) sample images and transfer the captured AprilTag pictures to the given web application.
    case 4:
      adjustColor(1,0,1);
      if(!digitalRead(control_button_A)){
        take_picture();
        web_app_conn_channel("send_img", "station_tag_img_");
      }
    break;              
    default:
      // Do nothing.
    break;
  }

}

void run_inference(){
  // Summarize the Edge Impulse FOMO model inference settings (from model_metadata.h).
  ei_printf("\nInference settings:\n");
  ei_printf("\tImage resolution: %dx%d\n", EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
  ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
  ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

  // Capture a new image (frame) via the built-in GC2145 camera.
  if(cam.grabFrame(fb, 100) == 0){
    // Create and align the camera frame buffer.
    ei_camera_frame_mem = (uint8_t *) ei_malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_RAW_FRAME_BYTE_SIZE + 32 /*alignment*/);
    if(ei_camera_frame_mem == NULL) ei_printf("\nFailed to create ei_camera_frame_mem\n");
    ei_camera_frame_buffer = (uint8_t *)ALIGN_PTR((uintptr_t)ei_camera_frame_mem, 32);
    // Save the captured raw image buffer to the aligned frame buffer.
    fb.setBuffer(ei_camera_frame_buffer);
    // Convert the captured raw RGB565 image buffer to a RGB888 buffer required by the Edge Impulse functions.
    ei_camera_capture_out = (uint8_t*)ea_malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * 3 + 32);
    ei_camera_capture_out = (uint8_t *)ALIGN_PTR((uintptr_t)ei_camera_capture_out, 32);
    RBG565ToRGB888(ei_camera_frame_buffer, ei_camera_capture_out, cam.frameSize());

    // Depending on the given model's image (frame) resolution, resize the converted RGB888 buffer by utilizing built-in Edge Impulse functions.
    if(EI_CAMERA_RAW_FRAME_BUFFER_COLS != EI_CLASSIFIER_INPUT_WIDTH && EI_CAMERA_RAW_FRAME_BUFFER_ROWS != EI_CLASSIFIER_INPUT_HEIGHT){
      ei::image::processing::crop_and_interpolate_rgb888(
        ei_camera_capture_out, // Output image buffer, can be same as the input image buffer.
        EI_CAMERA_RAW_FRAME_BUFFER_COLS,
        EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
        ei_camera_capture_out,
        EI_CLASSIFIER_INPUT_WIDTH,
        EI_CLASSIFIER_INPUT_HEIGHT);
    }

    // Run an inference to make predictions based on the trained classes.
    ei::signal_t signal;
    // Create a signal object from the converted and resized RGB888 image buffer.
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_cutout_get_data;
    // Run the provided classifier.
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR _err = run_classifier(&signal, &result, false);
    if(_err != EI_IMPULSE_OK){
      ei_printf("ERR: Failed to run classifier (%d)\n", _err);
      return;
    }

    // Print the inference timings on the serial monitor.
    ei_printf("\nPredictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);

    // Obtain the object detection results and bounding boxes for the detected labels (classes). 
    bool bb_found = result.bounding_boxes[0].value > 0;
    for(size_t ix = 0; ix < EI_CLASSIFIER_OBJECT_DETECTION_COUNT; ix++){
      auto bb = result.bounding_boxes[ix];
      if(bb.value == 0) continue;
      // Print the calculated bounding box measurements on the serial monitor.
      ei_printf("    %s (", bb.label);
      ei_printf_float(bb.value);
      ei_printf(") [ x: %u, y: %u, width: %u, height: %u ]\n", bb.x, bb.y, bb.width, bb.height);
      // Fetch the predicted label (class) and the detected object's bounding box measurements (if necessary).
      for(int i=0; i<classes_item_num; i++){
        if(bb.label == classes[i]){
          detected_class = bb.label;
          ei_printf("\nPredicted Class [Label]: %s\n", detected_class); 
        }
      }
    }
    // Notify the user the model cannot detect any labels.
    if(!bb_found){
      ei_printf("\nPredicted Class [Label]: No objects found!\n");
      detected_class = "none";
    }
    // Notify the user of the model anomalies if occured any while running the inference.
    #if EI_CLASSIFIER_HAS_ANOMALY == 1
      ei_printf("Anomaly: ");
      ei_printf_float(result.anomaly);
      ei_printf("\n");
    #endif 
    // Release the image buffers.
    ei_free(ei_camera_frame_mem);
    ea_free(ei_camera_capture_out); 
  }
}

bool RBG565ToRGB888(uint8_t *src_buf, uint8_t *dst_buf, uint32_t src_len){
    uint8_t hb, lb;
    uint32_t pix_count = src_len / 2;
    // Convert the passed raw RGB565 image buffer to an RGB888 buffer.
    for(uint32_t i = 0; i < pix_count; i ++) {
       // Next source byte.
        hb = *src_buf++;
        lb = *src_buf++;
        // Conversion [RGB565 -> RGB888].
        *dst_buf++ = hb & 0xF8;
        *dst_buf++ = (hb & 0x07) << 5 | (lb & 0xE0) >> 3;
        *dst_buf++ = (lb & 0x1F) << 3;
    }
    return true;
}

static int ei_camera_cutout_get_data(size_t offset, size_t length, float *out_ptr){
  // Convert the given image data (buffer) to the out_ptr format required by the Edge Impulse FOMO model.
  size_t pixel_ix = offset * 3;
  size_t pixels_left = length;
  size_t out_ptr_ix = 0;
  // Since the image data is already converted to an RGB888 buffer, directly recalculate offset into pixel index.
  while(pixels_left != 0){  
    out_ptr[out_ptr_ix] = (ei_camera_capture_out[pixel_ix] << 16) + (ei_camera_capture_out[pixel_ix + 1] << 8) + ei_camera_capture_out[pixel_ix + 2];
    // Move to the next pixel.
    out_ptr_ix++;
    pixel_ix+=3;
    pixels_left--;
  }
  return 0;
}

String web_app_conn_channel(String conn_mode, String _info){
  String app_res = "";
  // Connect to the given web application. Change '80' with '443' if you are using SSL connection.
  if(client.connect(server, 80)){
    // If the connection is established successfully, perform the passed connection mode.
    Serial.println("\nConnected to the web application successfully!\n");  
    if(conn_mode == "listen"){
      // Create the query string accordingly.
      String query = application + "?get_latest_food_system_log";  
      // Make an HTTP GET request to the provided webhook to optain latest commands.
      client.println("GET " + query + " HTTP/1.1");
      client.print("Host: "); client.println(server);
      client.println("Connection: close");
      client.println();
      // After performing the requested mode successfully, obtain the application response.
      delay(1000);
      while (client.available()) app_res = client.readString();
      // Then, modify the string to derive the data packet from the header when the delivery system is not idle.
      if(app_res.indexOf("%") >= 0){
        app_res = app_res.substring(app_res.indexOf("!") + 1, app_res.indexOf("!", app_res.indexOf("!") + 1));
      }else{
        app_res = "idle";
        Serial.println("\nFood delivery system is idle!\n");
      }           
    }else if(conn_mode == "update"){
      // Create the query string accordingly.
      String query = application + "?update_latest_food_system_log_status=" + _info;  
      // Make an HTTP GET request to the provided webhook to update the food system log status.
      client.println("GET " + query + " HTTP/1.1");
      client.print("Host: "); client.println(server);
      client.println("Connection: close");
      client.println();
      delay(3000);
      // After updating, stop the client connection to prevent errors.
      client.stop();
      Serial.println("\nOrder status is updated successfully! \n");      
    }else if(conn_mode == "send_img"){
      // Create the query string accordingly.
      String query = application + "?save_slot_image_sample=" + _info;     
      // Make an HTTP POST request to transfer the captured image buffer.
      String head = "--SlotSample\r\nContent-Disposition: form-data; name=\"captured_image\"; filename=\"slot_sample.txt\"\r\nContent-Type: text/plain\r\n\r\n";
      String tail = "\r\n--SlotSample--\r\n";
      // Estimate the total message length.
      uint32_t totalLen = head.length() + cam.frameSize() + tail.length();
      // Initiate the POST request.
      client.println("POST " + query + " HTTP/1.1");
      client.print("Host: "); client.println(server);
      client.println("Content-Length: " + String(totalLen));
      client.println("Content-Type: multipart/form-data; boundary=SlotSample");
      client.println();
      client.print(head);
      client.write(fb.getBuffer(), cam.frameSize());
      client.print(tail);
      client.println("Connection: close");
      client.println();
      // Wait until transferring the passed image buffer. Then, stop the client connection to prevent errors.
      Serial.println("\nSlot sample image is transferred!");
      delay(3000);
      client.stop();
    }
  // If the connection is failed:
  }else{
    app_res = "failed";
    Serial.println("\nConnection failed to the web application!\n");
    delay(1000);
  }
  // Finally, return the generated application response for further analysis.
  return app_res;
}

void move_the_delivery_platform_to_station(int associated_station, int food_item_amount, bool home_x_again){
  // Following the first associated prep station of the requested order, rehome the X-axis.
  if(home_x_again) h_bot_mechanism_home(true, false);
  // Move the food delivery system until detecting the assigned AprilTag that of the associated prep station.
  while(detected_class != classes[associated_station]){
    // Run an inference with the Edge Impulse model in order to obtain the label (class) of the recognized AprilTags.
    // NOTE: I employed the onboard Time of Flight (Distance - ToF) sensor (VL53L1CBV0FY) to run inferences while only passing by prep stations to optimize the menu preparation process.
    if(get_tof_sensor_data() < 25) run_inference();
    // Move the food delivery system slowly while attempting to detect the assigned AprilTag.
    h_bot_mechanism_move(10, 10, "X", "right");
  }
  // After recognizing the assigned AprilTag, wait before proceeding to the next prep station depending on the amount of food items required from the current station.
  delay(food_item_amount * stepper_config.wait_per_food_item);
  // Then, clear the detected label.
  detected_class = "";
}

void h_bot_mechanism_home(bool x_axis, bool y_axis){
  /* Home the requested axis via the assigned limit (micro) switches. */
  if(x_axis){
    while(digitalRead(micro_switch_B)){
      h_bot_mechanism_move(5, 2, "X", "left");
    }delay(500);
    h_bot_mechanism_move(30, 10, "X", "right");
    delay(500);
  }
  if(y_axis){
    while(digitalRead(micro_switch_A)){
      h_bot_mechanism_move(5, 2, "Y", "down");
    }delay(500);
    h_bot_mechanism_move(30, 10, "Y", "up");
    delay(500);
  }  
}

void h_bot_mechanism_move(int step_number, int acc, String axis, String move_to){
  /*
    Move the H-Bot mechanism by driving the associated stepper motors according to these calculations:
    Move along X-axis: rotate stepper motors in the same direction at the same velocity.
    Move along Y-axis: rotate stepper motors in opposite directions at the same velocity.
    Move diagonally: rotate only one of the stepper motors. 
  */
  if(axis == "X"){
    if(move_to == "left"){
      digitalWrite(stepper_config._pins[stepper_config.x_core][0], HIGH);
      digitalWrite(stepper_config._pins[stepper_config.y_core][0], HIGH);      
    }else if(move_to == "right"){
      digitalWrite(stepper_config._pins[stepper_config.x_core][0], LOW);
      digitalWrite(stepper_config._pins[stepper_config.y_core][0], LOW);
    }
  }
  if(axis == "Y"){
    if(move_to == "up"){
      digitalWrite(stepper_config._pins[stepper_config.x_core][0], LOW);
      digitalWrite(stepper_config._pins[stepper_config.y_core][0], HIGH); 
    }else if(move_to == "down"){
      digitalWrite(stepper_config._pins[stepper_config.x_core][0], HIGH);
      digitalWrite(stepper_config._pins[stepper_config.y_core][0], LOW);       
    }
  }

  for(int i = 0; i < step_number; i++){
    digitalWrite(stepper_config._pins[stepper_config.x_core][1], HIGH);
    digitalWrite(stepper_config._pins[stepper_config.y_core][1], HIGH);
    delayMicroseconds(stepper_config.x_y_speed/acc);
    digitalWrite(stepper_config._pins[stepper_config.x_core][1], LOW);
    digitalWrite(stepper_config._pins[stepper_config.y_core][1], LOW);
    delayMicroseconds(stepper_config.x_y_speed/acc);
  }  

}

void z_arm_move(int step_number, int acc, String _dir){
  /*
    Move the arm carrying the tray.
    CW:  Clockwise
    CCW: Counter-clockwise
  */
  if(_dir == "CW"){ digitalWrite(stepper_config._pins[stepper_config.z_arm][0], HIGH); }
  else if(_dir == "CCW"){ digitalWrite(stepper_config._pins[stepper_config.z_arm][0], LOW); }
	
	for(int i = 0; i < step_number; i++){
	  digitalWrite(stepper_config._pins[stepper_config.z_arm][1], HIGH);
		delayMicroseconds(stepper_config.z_speed/acc);
		digitalWrite(stepper_config._pins[stepper_config.z_arm][1], LOW);
		delayMicroseconds(stepper_config.z_speed/acc);
	}
}

void take_picture(){
  // Capture a picture with the GC2145 camera.
  if(cam.grabFrame(fb, 3000) == 0){
   Serial.println("\nGC2145 camera: image captured successfully!");
  }else{
    Serial.println("\nGC2145 camera: image capture failed!");
  }
  delay(2000);
}

int get_tof_sensor_data(){
  // Obtain the most recent ToF sensor reading.
  return proximity.read();
}

void adjustColor(int r, int g, int b){
  // Built-in RGB LED.
  digitalWrite(LEDR, 1-r);
  digitalWrite(LEDG, 1-g);
  digitalWrite(LEDB, 1-b);
}